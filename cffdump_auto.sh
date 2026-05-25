#!/usr/bin/env bash

if [ -z "${BASH_VERSION:-}" ]; then
  exec bash "$0" "$@"
fi

set -u

BUILD_DIR=${BUILD_DIR:-build}
RESULTS_DIR=${RESULTS_DIR:-results}
RD_ROOT=${RD_ROOT:-.}
RUN_DRM_SHIM_PROBE=${RUN_DRM_SHIM_PROBE:-0}
VALID_FD_GPU_ID=${VALID_FD_GPU_ID:-307}
INVALID_FD_GPU_ID=${INVALID_FD_GPU_ID:-999999}
REPLAY_SAMPLE_COUNT=${REPLAY_SAMPLE_COUNT:-3}

log() {
  echo "[$(date -u +'%Y-%m-%dT%H:%M:%SZ')] $*"
}

setup_build() {
  log "Configuring Meson build in ${BUILD_DIR}"
  meson setup "${BUILD_DIR}" \
    -Dtools=freedreno,drm-shim \
    -Dgallium-drivers= \
    -Dvulkan-drivers=freedreno \
    -Dopengl=false \
    -Dgbm=disabled \
    -Degl=disabled \
    -Dgles1=disabled \
    -Dgles2=disabled \
    -Dplatforms= \
    -Dbuildtype=release \
    --reconfigure 2>/dev/null || \
  meson setup "${BUILD_DIR}" \
    -Dtools=freedreno,drm-shim \
    -Dgallium-drivers= \
    -Dvulkan-drivers=freedreno \
    -Dopengl=false \
    -Dgbm=disabled \
    -Degl=disabled \
    -Dgles1=disabled \
    -Dgles2=disabled \
    -Dplatforms= \
    -Dbuildtype=release
}

build_targets() {
  log "Compiling cffdump"
  ninja -C "${BUILD_DIR}" src/freedreno/decode/cffdump

  if [ "${RUN_DRM_SHIM_PROBE}" = "1" ]; then
    log "Compiling Turnip ICD, freedreno drm-shim and replay tool"
    ninja -C "${BUILD_DIR}" src/freedreno/vulkan/libvulkan_freedreno.so src/freedreno/drm-shim/libfreedreno_noop_drm_shim.so
    ninja -C "${BUILD_DIR}" src/freedreno/decode/replay src/freedreno/decode/replay-msm || true
  fi
}

run_decode() {
  mkdir -p "${RESULTS_DIR}"
  log "Decoding .rd files from ${RD_ROOT}"

  local count=0
  while IFS= read -r f; do
    filename=$(basename "$f")
    out="${RESULTS_DIR}/${filename}.txt"

    log "Processing ${f}"
    "./${BUILD_DIR}/src/freedreno/decode/cffdump" --no-color "$f" >"$out" 2>&1
    if [ $? -ne 0 ]; then
      log "Warning: cffdump returned non-zero for ${filename} (see ${out})"
    fi
    count=$((count + 1))
  done < <(find "${RD_ROOT}" -type f -name "*.rd" -not -path "*/${BUILD_DIR}/*" | sort)

  log "Decoded ${count} dump file(s)"
}


run_replay_with_drm_shim() {
  mkdir -p "${RESULTS_DIR}"

  local shim="${PWD}/${BUILD_DIR}/src/freedreno/drm-shim/libfreedreno_noop_drm_shim.so"
  local replay_bin=""

  if [ -x "${PWD}/${BUILD_DIR}/src/freedreno/decode/replay" ]; then
    replay_bin="${PWD}/${BUILD_DIR}/src/freedreno/decode/replay"
  elif [ -x "${PWD}/${BUILD_DIR}/src/freedreno/decode/replay-msm" ]; then
    replay_bin="${PWD}/${BUILD_DIR}/src/freedreno/decode/replay-msm"
  fi

  if [ ! -f "$shim" ] || [ -z "$replay_bin" ]; then
    log "Skipping drm-shim replay: missing shim or replay binary"
    return
  fi

  log "Running replay under drm-shim for up to ${REPLAY_SAMPLE_COUNT} dump(s)"
  local i=0
  while IFS= read -r f; do
    i=$((i + 1))
    local base
    base=$(basename "$f")

    env MESA_LOADER_DRIVER_OVERRIDE=msm \
      LD_PRELOAD="$shim" \
      FD_GPU_ID="${VALID_FD_GPU_ID}" \
      "$replay_bin" "$f" >"${RESULTS_DIR}/replay-${base}.log" 2>&1 || true

    if [ "$i" -ge "${REPLAY_SAMPLE_COUNT}" ]; then
      break
    fi
  done < <(find "${RD_ROOT}" -type f -name "*.rd" -not -path "*/${BUILD_DIR}/*" | sort)

  log "drm-shim replay finished for ${i} dump(s)"
}

run_drm_shim_probe() {
  mkdir -p "${RESULTS_DIR}"

  local shim="${PWD}/${BUILD_DIR}/src/freedreno/drm-shim/libfreedreno_noop_drm_shim.so"
  local icd="${PWD}/${BUILD_DIR}/src/freedreno/vulkan/tu_icd.${HOSTTYPE:-x86_64}.json"

  if [ ! -f "$icd" ]; then
    icd=$(find "${PWD}/${BUILD_DIR}" -type f -name "tu_icd*.json" | head -n1)
  fi

  if [ ! -f "$shim" ] || [ -z "${icd}" ] || [ ! -f "$icd" ]; then
    log "Skipping drm-shim probe: missing shim or ICD json"
    return
  fi

  log "Running vulkaninfo smoke checks with drm-shim"
  local common_env=(
    MESA_LOADER_DRIVER_OVERRIDE=msm
    LD_PRELOAD="$shim"
    VK_ICD_FILENAMES="$icd"
    DRM_SHIM_DEBUG=1
  )

  env "${common_env[@]}" FD_GPU_ID="${VALID_FD_GPU_ID}" vulkaninfo --summary \
    >"${RESULTS_DIR}/vulkaninfo-valid-${VALID_FD_GPU_ID}.log" 2>&1 || true

  env "${common_env[@]}" FD_GPU_ID="${INVALID_FD_GPU_ID}" vulkaninfo --summary \
    >"${RESULTS_DIR}/vulkaninfo-invalid-${INVALID_FD_GPU_ID}.log" 2>&1 || true

  log "drm-shim probe finished. Compare valid/invalid vulkaninfo logs in ${RESULTS_DIR}"
}

log "--- Starting build/decode pipeline ---"
setup_build
build_targets
run_decode

if [ "${RUN_DRM_SHIM_PROBE}" = "1" ]; then
  run_replay_with_drm_shim
  run_drm_shim_probe
fi

log "--- All done. Artifacts are in ${RESULTS_DIR} ---"
