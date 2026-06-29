/*
 * Copyright © 2026 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <imgui.h>
#include <memory>
#include <vector>
#include "util/macros.h"
#include <vulkan/vulkan_core.h>

void rti_check_vk_result_internal(VkResult result, const char *file, uint32_t line);

#define rti_check_vk_result(result) rti_check_vk_result_internal(result, __FILE__, __LINE__)

struct rti_app;

struct rti_backed_buffer {
   rti_app *app;
   VkDeviceMemory memory = VK_NULL_HANDLE;
   VkBuffer buffer = VK_NULL_HANDLE;
   void *map = nullptr;
   uint64_t size = 0;

   ~rti_backed_buffer();
};

enum rti_memory_type {
   rti_memory_type_device_local,
   rti_memory_type_host_visible,
   rti_memory_type_host_visible_cached,
};

std::shared_ptr<rti_backed_buffer> rti_create_backed_buffer(rti_app *app, uint64_t size,
                                                            enum rti_memory_type memory_type, VkBufferUsageFlags usage,
                                                            bool map);

struct rti_backed_image {
   rti_app *app = nullptr;
   VkImage image = VK_NULL_HANDLE;
   VkDeviceMemory memory = VK_NULL_HANDLE;
   VkImageView image_view = VK_NULL_HANDLE;

   ~rti_backed_image();
};

std::shared_ptr<rti_backed_image> rti_create_backed_image(rti_app *app, VkExtent3D extent,
                                                          const std::vector<VkFormat> &formats,
                                                          VkFormatFeatureFlags format_features, VkImageUsageFlags usage,
                                                          VkImageAspectFlagBits aspect_mask, uint32_t samples,
                                                          uint32_t levels);

struct rti_vec2 {
   float x = 0;
   float y = 0;
};

struct rti_vec3 {
   float x = 0;
   float y = 0;
   float z = 0;

   static inline rti_vec3 sub(rti_vec3 a, rti_vec3 b)
   {
      return {
         .x = a.x - b.x,
         .y = a.y - b.y,
         .z = a.z - b.z,
      };
   }

   static inline rti_vec3 cross(rti_vec3 a, rti_vec3 b)
   {
      return {
         .x = a.y * b.z - a.z * b.y,
         .y = a.z * b.x - a.x * b.z,
         .z = a.x * b.y - a.y * b.x,
      };
   }

   static inline float dot(rti_vec3 a, rti_vec3 b)
   {
      return a.x * b.x + a.y * b.y + a.z * b.z;
   }
};

struct rti_vec4 {
   float x = 0;
   float y = 0;
   float z = 0;
   float w = 0;
};

struct rti_mat4 {
   float elements[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

   static inline rti_mat4 zero()
   {
      return {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
   }

   static inline rti_mat4 mul(rti_mat4 a, rti_mat4 b)
   {
      rti_mat4 result = rti_mat4::zero();

      for (uint32_t i = 0; i < 4; i++) {
         for (uint32_t j = 0; j < 4; j++) {
            for (uint32_t k = 0; k < 4; k++)
               result.elements[i + j * 4] += a.elements[i + k * 4] * b.elements[k + j * 4];
         }
      }

      return result;
   }

   static inline rti_mat4 transpose(rti_mat4 m)
   {
      rti_mat4 result;

      for (uint32_t i = 0; i < 4; i++) {
         for (uint32_t j = 0; j < 4; j++)
            result.elements[i + j * 4] = m.elements[j + i * 4];
      }

      return result;
   }

   static inline rti_vec4 mul_vec4(rti_mat4 a, rti_vec4 v)
   {
      return (rti_vec4){
         .x = a.elements[0 + 0 * 4] * v.x + a.elements[0 + 1 * 4] * v.y + a.elements[0 + 2 * 4] * v.z +
              a.elements[0 + 3 * 4] * v.w,
         .y = a.elements[1 + 0 * 4] * v.x + a.elements[1 + 1 * 4] * v.y + a.elements[1 + 2 * 4] * v.z +
              a.elements[1 + 3 * 4] * v.w,
         .z = a.elements[2 + 0 * 4] * v.x + a.elements[2 + 1 * 4] * v.y + a.elements[2 + 2 * 4] * v.z +
              a.elements[2 + 3 * 4] * v.w,
         .w = a.elements[3 + 0 * 4] * v.x + a.elements[3 + 1 * 4] * v.y + a.elements[3 + 2 * 4] * v.z +
              a.elements[3 + 3 * 4] * v.w,
      };
   }
};

struct rti_ray {
   rti_vec3 origin;
   rti_vec3 direction;

   static inline rti_ray transform(rti_ray ray, rti_mat4 transform)
   {
      rti_vec4 origin = {ray.origin.x, ray.origin.y, ray.origin.z, 1};
      origin = rti_mat4::mul_vec4(transform, origin);

      rti_vec4 direction = {ray.direction.x, ray.direction.y, ray.direction.z, 0};
      direction = rti_mat4::mul_vec4(transform, direction);

      return {
         .origin = {origin.x, origin.y, origin.z},
         .direction = {direction.x, direction.y, direction.z},
      };
   }
};

struct rti_aabb {
   rti_vec3 min;
   rti_vec3 max;

   static inline rti_aabb combine(rti_aabb a, rti_aabb b)
   {
      return {
         .min =
            {
               .x = MIN2(a.min.x, b.min.x),
               .y = MIN2(a.min.y, b.min.y),
               .z = MIN2(a.min.z, b.min.z),
            },
         .max =
            {
               .x = MAX2(a.max.x, b.max.x),
               .y = MAX2(a.max.y, b.max.y),
               .z = MAX2(a.max.z, b.max.z),
            },
      };
   }

   static inline float intersect_ray(rti_aabb aabb, rti_ray ray)
   {
      float t1 = (aabb.min.x - ray.origin.x) / ray.direction.x;
      float t2 = (aabb.max.x - ray.origin.x) / ray.direction.x;
      float t3 = (aabb.min.y - ray.origin.y) / ray.direction.y;
      float t4 = (aabb.max.y - ray.origin.y) / ray.direction.y;
      float t5 = (aabb.min.z - ray.origin.z) / ray.direction.z;
      float t6 = (aabb.max.z - ray.origin.z) / ray.direction.z;

      float tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
      float tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));

      if (tmax < 0 || tmax < tmin)
         return INFINITY;

      return tmin;
   }
};

struct rti_triangle {
   rti_vec3 vertices[3];
   uint32_t geometry_index = 0;
   uint32_t primitive_index = 0;

   static inline float intersect_ray(rti_triangle triangle, rti_ray ray)
   {
      rti_vec3 edge1 = rti_vec3::sub(triangle.vertices[1], triangle.vertices[0]);
      rti_vec3 edge2 = rti_vec3::sub(triangle.vertices[2], triangle.vertices[0]);
      rti_vec3 direction_cross_edge2 = rti_vec3::cross(ray.direction, edge2);
      float det = rti_vec3::dot(edge1, direction_cross_edge2);
      rti_vec3 s = rti_vec3::sub(ray.origin, triangle.vertices[0]);
      float u = rti_vec3::dot(s, direction_cross_edge2) / det;
      rti_vec3 s_cross_edge1 = rti_vec3::cross(s, edge1);
      float v = rti_vec3::dot(ray.direction, s_cross_edge1) / det;
      if (u < 0 || v < 0 || u + v > 1)
         return INFINITY;

      float t = rti_vec3::dot(edge2, s_cross_edge1) / det;
      if (t < 0)
         t = INFINITY;

      return t;
   }
};

#define RTI_PRIMITIVE_INDEX_INACTIVE 0xffffffff

struct rti_vertex {
   rti_vec3 position;
   uint32_t geometry_index;
   uint32_t primitive_index;
};

void rti_generate_cube_vertices(rti_vertex *vertices, rti_aabb aabb);

void rti_generate_filled_cube_vertices(rti_vertex *vertices, rti_aabb aabb, uint32_t geometry_index,
                                       uint32_t primitive_index);

/* Helpers to remove some imgui boilerplate. */
namespace ImGui {

static inline void
TableNextColumnText(const char *fmt, ...)
{
   TableNextColumn();

   va_list args;
   va_start(args, fmt);
   TextV(fmt, args);
   va_end(args);
}

}; // namespace ImGui
