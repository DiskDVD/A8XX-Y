#!/bin/bash
# Не останавливаемся при ошибках, чтобы вытянуть инфу из всех файлов
set +e

echo "--- Starting Build ---"

# Хак для обхода зависимости от libdrm в коде Mesa
if [ -f "meson.build" ]; then
    sed -i "s/dependency('libdrm'/dependency('libdrm_disabled'/g" meson.build
fi

# Настройка сборки: только инструменты, архитектура x86_64
meson setup build \
    -Dtools=freedreno \
    -Dgallium-drivers= \
    -Dvulkan-drivers= \
    -Dopengl=false \
    -Dgbm=disabled \
    -Degl=disabled \
    -Dgles1=disabled \
    -Dgles2=disabled \
    -Dplatforms= \
    -Dbuildtype=release \
    --reconfigure || meson setup build -Dtools=freedreno -Dgallium-drivers= -Dvulkan-drivers= -Dopengl=false -Dgbm=disabled -Degl=disabled -Dgles1=disabled -Dgles2=disabled -Dplatforms= -Dbuildtype=release

# Компиляция cffdump
echo "--- Compiling cffdump ---"
ninja -C build src/freedreno/decode/cffdump

echo "--- Build Finished ---"

mkdir -p results

# Находим cffdump динамически
CFFDUMP_PATH=$(find build -name "cffdump" -type f 2>/dev/null | head -1)

if [ -z "$CFFDUMP_PATH" ]; then
    echo "ERROR: cffdump binary not found!"
    exit 1
fi

echo "Using cffdump at: $CFFDUMP_PATH"

echo "--- Searching and Decoding .rd files ---"

# Ищем все файлы .rd, где бы они ни лежали (кроме папки build)
find . -type f -name "*.rd" -not -path "*/build/*" | while read -r f; do
    filename=$(basename "$f")
    echo "Processing $f..."
    
    # Декодируем. Даже если файл битый, ошибки запишутся в текстовик для анализа
    $CFFDUMP_PATH --no-color "$f" > "results/${filename}.txt" 2>&1
    
    if [ $? -ne 0 ]; then
        echo "Warning: Error decoding $filename"
    fi
done

echo "--- All done! Check Artifacts. ---"
