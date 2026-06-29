/*
 * Copyright © 2026 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <imgui.h>
#include <map>
#include <memory>
#include <stdint.h>
#include "bvh_defines.h"
#include "radv_rti.h"
#include "rti_file_view.h"
#include "rti_util.h"
#include <unordered_set>

struct rti_acceleration_structure_radv : public rti_acceleration_structure {
   rti_acceleration_structure_radv(rti_file_view *view): rti_acceleration_structure(view)
   {
   }

   struct {
      uint32_t selected_node_id = RADV_BVH_INVALID_NODE;
      bool scroll_to_selected_node = false;
      std::unordered_set<uint32_t> open_path;
   } radv_ui;

   const uint8_t *bvh;

   uint32_t first_vertex;
   uint32_t wireframe_first_vertex;
};

struct rti_dispatch_view_settings {
   uint32_t width = 0;
   uint32_t height = 0;
   uint32_t depth = 0;
   uint32_t z = 0;
   uint32_t max_iteration_count = 0;
   bool show_box_iteration_count = true;
   bool show_instance_iteration_count = true;
   bool show_primitive_iteration_count = true;
};

struct rti_invocation_data {
   std::vector<uint32_t> offsets;
   uint32_t selected_ray = 0;
   uint32_t box_iteration_count = 0;
   uint32_t instance_iteration_count = 0;
   uint32_t primitive_iteration_count = 0;
};

struct rti_dispatch_view {
   struct {
      bool open = false;
      bool request_focus = false;

      bool initialize_viewport = true;

      std::vector<std::shared_ptr<ImTextureData>> texture_data;
      std::vector<std::shared_ptr<ImTextureData>> destroy_texture_data;

      float image_scale;
      rti_vec2 viewport_center;
      bool is_dragging_viewport = false;
      ImVec2 prev_mouse_pos;
      uint32_t selection_x = UINT32_MAX;
      uint32_t selection_y = UINT32_MAX;

      rti_dispatch_view_settings prev_settings;
      rti_dispatch_view_settings settings;
   } ui;

   const radv_rti_dispatch_info *info;

   uint32_t invocation_count;
   uint32_t max_iteration_count = 0;

   std::vector<rti_invocation_data> invocations;

   std::map<uint32_t, std::map<uint32_t, uint32_t>> valid_dispatch_sizes;
};

struct rti_file_view_radv : public rti_file_view {
   std::vector<std::shared_ptr<rti_dispatch_view>> dispatch_views;

   rti_dispatch_view *selected_dispatch = nullptr;

   void *ray_history_data = nullptr;
   const radv_rti_ray_history_header *history_header = nullptr;
   const radv_rti_dispatch_info *dispatch_infos = nullptr;
   const uint8_t *history_data = nullptr;
   uint32_t dispatch_count = 0;
   uint64_t ray_history_data_size;

   radv_rti_trace_info *trace_info;

   virtual std::unique_ptr<rti_acceleration_structure> create_acceleration_structure() override;

   virtual void load(FILE *file, const rti_header *header) override;

   virtual void load_driver_specific(FILE *file, const rti_chunk_header *chunk_header,
                                     const rti_header *header) override;

   virtual void dock_driver_specific(uint32_t left_top, uint32_t left_bottom, uint32_t center, uint32_t right_top,
                                     uint32_t right_bottom) override;

   virtual float handle_mouse_click(rti_acceleration_structure *acceleration_structure, rti_ray ray,
                                    bool select) override;

   virtual void run() override;

   rti_invocation_data *get_selected_invocation();

   /* bvh4 */
   void bvh4_traverse(rti_acceleration_structure_radv *acceleration_structure, rti_ray ray, uint32_t id, float *tmax,
                      const std::unordered_set<uint32_t> &path, bool select);

   rti_aabb bvh4_scene_aabb(rti_acceleration_structure_radv *acceleration_structure);

   void bvh4_get_vertices(rti_acceleration_structure_radv *acceleration_structure, uint32_t id, rti_vertex *vertices,
                          rti_vertex *wireframe_vertices);

   void bvh4_get_instances(rti_acceleration_structure_radv *tlas, uint32_t id);

   void bvh4_render_selection(rti_acceleration_structure_radv *acceleration_structure);

   void bvh4_draw(rti_acceleration_structure_radv *acceleration_structure, uint32_t id);

   void bvh4_draw_node_info(const rti_acceleration_structure_radv *acceleration_structure, uint32_t id);

   void bvh4_get_instance_info(const rti_acceleration_structure_radv *acceleration_structure, uint32_t id,
                               rti_mat4 *transform, rti_acceleration_structure_radv **blas);

   /* bvh8 */
   void bvh8_traverse(rti_acceleration_structure_radv *acceleration_structure, rti_ray ray, uint32_t id, float *tmax,
                      const std::unordered_set<uint32_t> &path, bool select);

   rti_aabb bvh8_scene_aabb(rti_acceleration_structure_radv *acceleration_structure);

   void bvh8_get_vertices(rti_acceleration_structure_radv *acceleration_structure, uint32_t id, rti_vertex *vertices,
                          rti_vertex *wireframe_vertices);

   void bvh8_get_instances(rti_acceleration_structure_radv *tlas, uint32_t id);

   void bvh8_render_selection(rti_acceleration_structure_radv *acceleration_structure);

   void bvh8_draw(rti_acceleration_structure_radv *acceleration_structure, uint32_t id);

   void bvh8_draw_node_info(const rti_acceleration_structure_radv *acceleration_structure, uint32_t id);

   void bvh8_get_instance_info(const rti_acceleration_structure_radv *acceleration_structure, uint32_t id,
                               rti_mat4 *transform, rti_acceleration_structure_radv **blas);
};

static inline uint64_t
radv_node_to_addr(uint64_t node)
{
   node &= ~7ull;
   node <<= 19;
   return ((int64_t)node) >> 16;
}

static inline rti_mat4
mat3x4_to_rti(mat3x4 m)
{
   rti_mat4 result;
   for (uint32_t i = 0; i < 3; i++) {
      for (uint32_t j = 0; j < 4; j++) {
         result.elements[i + j * 4] = m.values[i][j];
      }
   }
   return result;
}
