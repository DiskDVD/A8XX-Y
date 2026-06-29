/*
 * Copyright © 2026 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include "vulkan/vulkan_core.h"

enum rti_driver {
   rti_driver_radv,
};

struct rti_header {
   uint32_t version;
   enum rti_driver driver;
   uint64_t chunk_count;
};

enum rti_chunk_type {
   rti_chunk_type_acceleration_structure,
   rti_chunk_type_driver_start = 0x80000000,

   rti_chunk_type_trace_info_radv = rti_chunk_type_driver_start,
   rti_chunk_type_ray_history_radv,
};

struct rti_chunk_header {
   enum rti_chunk_type type;
   uint64_t size;
};

struct rti_acceleration_structure_header {
   uint64_t address;
   uint64_t allocated_size;
   uint64_t compacted_size;
   VkAccelerationStructureTypeKHR type;
   VkGeometryTypeKHR geometry_type;
   uint32_t geometry_count;
   uint32_t name_size;
};
