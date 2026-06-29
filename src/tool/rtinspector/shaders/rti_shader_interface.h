/*
 * Copyright © 2026 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RTI_SHADER_INTERFACE_H
#define RTI_SHADER_INTERFACE_H

#ifdef VULKAN
#else
#include "rti_util.h"
#define mat4 rti_mat4
#define vec3 rti_vec3
#endif

struct rti_render_params {
   mat4 transform;
   vec3 color;
};

struct rti_push_constants {
   mat4 transform;
   uint32_t first_param;
   uint32_t flags;
};

#define RTI_RENDERER_COLOR_PUSH_CONSTANT   0
#define RTI_RENDERER_COLOR_PRIMITIVE_INDEX 1
#define RTI_RENDERER_COLOR_GEOMETRY_INDEX  2
#define RTI_RENDERER_COLOR_BIT_COUNT       2

#define RTI_RENDERER_FLAG_COLOR_MASK ((1u << RTI_RENDERER_COLOR_BIT_COUNT) - 1)

#ifdef VULKAN
#else
#undef mat4
#undef vec3
#endif

#endif
