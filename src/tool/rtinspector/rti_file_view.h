/*
 * Copyright © 2026 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#include "imgui.h"

#include "rti_util.h"

#include "shaders/rti_shader_interface.h"
#include "util/rti_format.h"
#include "vulkan/vulkan_core.h"

struct rti_app;

enum rti_render_task_type {
   rti_render_task_type_solid,
   rti_render_task_type_wireframe,
   rti_render_task_type_thick_wireframe,
   rti_render_task_type_lines,
   rti_render_task_type_thick_lines,
   rti_render_task_type_render_list,
};

struct rti_render_list;

struct rti_render_task {
   rti_render_task_type type = rti_render_task_type_solid;
   union {
      const rti_backed_buffer *vertex_buffer = nullptr;
      rti_render_list *render_list;
   };
   uint32_t first_vertex = 0;
   uint32_t vertex_count = 0;
   rti_render_params params;
   uint32_t flags = 0;
   bool wait_for_prev = false;
   bool override_flags = false;
};

struct rti_render_list {
   rti_app *app;
   std::vector<rti_render_task> tasks;
   std::shared_ptr<rti_backed_buffer> constants = nullptr;
   VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
   VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

   rti_render_list(rti_app *app): app(app)
   {
   }

   ~rti_render_list();

   void build();
};

struct rti_imgui_texture {
   VkDescriptorSet descriptor_set;

   rti_imgui_texture(VkDescriptorSet descriptor_set): descriptor_set(descriptor_set)
   {
   }

   ~rti_imgui_texture();
};

struct rti_viewport_state {
   std::shared_ptr<rti_imgui_texture> surface = nullptr;
   std::shared_ptr<rti_backed_image> resolved_image = nullptr;
   std::shared_ptr<rti_backed_image> cb = nullptr;
   std::shared_ptr<rti_backed_image> db = nullptr;
   uint32_t width = 0;
   uint32_t height = 0;
   std::shared_ptr<rti_render_list> render_list = nullptr;
};

enum rti_visualization_color {
   rti_visualization_color_primitive_index,
   rti_visualization_color_geometry_index,
   rti_visualization_color_instance_index,
};

static inline uint32_t
rti_visualization_color_to_renderer_color(rti_visualization_color color)
{
   switch (color) {
   case rti_visualization_color_primitive_index:
      return RTI_RENDERER_COLOR_PRIMITIVE_INDEX;
   case rti_visualization_color_geometry_index:
      return RTI_RENDERER_COLOR_GEOMETRY_INDEX;
   case rti_visualization_color_instance_index:
      return RTI_RENDERER_COLOR_PUSH_CONSTANT;
   }
   return RTI_RENDERER_COLOR_PRIMITIVE_INDEX;
}

static inline rti_vec3
rti_index_to_color(uint32_t index)
{
   return {
      .x = ((index * 0xd83f0930) >> 24) / 255.0f,
      .y = ((index * 0x8fa9836b) >> 24) / 255.0f,
      .z = ((index * 0x3037f8ad) >> 24) / 255.0f,
   };
}

enum rti_camera_type {
   rti_camera_type_first_person,
   rti_camera_type_pivot,
};

struct rti_file_view;

struct rti_acceleration_structure {
   rti_app *app = nullptr;
   rti_file_view *view = nullptr;
   rti_acceleration_structure_header header;
   char *name = nullptr;
   void *data = nullptr;

   uint32_t *primitive_counts = nullptr;
   uint32_t *primitive_counts_exclusive_sum = nullptr;
   uint32_t primitive_count = 0;

   rti_aabb aabb;

   struct {
      bool opened = false;
      bool request_focus = false;
      bool prev_focused = false;

      rti_viewport_state viewport;
      std::vector<rti_viewport_state> pending_viewports;

      std::shared_ptr<rti_backed_buffer> vertex_buffer = nullptr;
      std::shared_ptr<rti_render_list> instance_render_list = nullptr;

      rti_camera_type camera_type = rti_camera_type_first_person;
      float camera_phi = 0;
      float camera_theta = 0;
      rti_vec3 camera_pivot = {0, 0, 0};
      float camera_pivot_distance = 0;
      bool camera_pivot_valid = false;
      rti_vec3 camera_position = {0, 0, 0};
      float camera_speed = 0.3;
      float camera_near = 0.0001;
      float camera_far = 5.0;
      float camera_fov = 60.0;

      rti_visualization_color visualization_color = rti_visualization_color_primitive_index;

      bool is_right_dragging_viewport;
      bool is_middle_dragging_viewport;
      ImVec2 prev_mouse_pos;
      float middle_drag_start_z_distance;
   } ui;

   rti_acceleration_structure(rti_file_view *view);

   virtual ~rti_acceleration_structure()
   {
      free(name);
      free(data);
   }

   void init_camera();

   void load(FILE *file, uint32_t chunk_size);
};

struct rti_file_view {
   rti_app *app;
   std::filesystem::path path;

   std::vector<std::unique_ptr<rti_acceleration_structure>> acceleration_structures;
   std::map<uint64_t, uint32_t> address_map;

   uint32_t up_axis = 1 + 3; /* +x, +y, +z, -x, -y, -z */

   struct {
      ImGuiID dockspace_id;
      ImGuiWindowClass dockspace_class;
      bool request_focus = true;
      bool initialized;
      VkAccelerationStructureTypeKHR filter_acceleration_structure_type;
      rti_acceleration_structure *focused_acceleration_structure = nullptr;
   } ui;

   struct {
      rti_acceleration_structure *acceleration_structure;
      rti_mat4 projection_matrix;
      rti_mat4 inv_camera_rotation;
      rti_mat4 view_projection_matrix;
      ImVec2 viewport_offset;
      ImVec2 viewport_size;
      bool render;
      rti_vec3 camera_pivot;
   } rendering_state;

   virtual void load(FILE *file, const rti_header *header);

   virtual void load_driver_specific(FILE *file, const rti_chunk_header *chunk_header, const rti_header *header)
   {
   }

   virtual std::unique_ptr<rti_acceleration_structure> create_acceleration_structure() = 0;

   virtual void dock_driver_specific(uint32_t left_top, uint32_t left_bottom, uint32_t center, uint32_t right_top,
                                     uint32_t right_bottom)
   {
   }

   virtual float handle_mouse_click(rti_acceleration_structure *acceleration_structure, rti_ray ray, bool select)
   {
      return INFINITY;
   }

   virtual void run() = 0;

   /* Helper functions for updating and drawing the UI */
   void begin();
   void end();

   bool begin_viewport(rti_acceleration_structure *acceleration_structure);

   void render(const rti_render_task &task);
   void render_aabb(const rti_render_task &base_task, rti_aabb aabb, bool fill = false);

   void render_viewport();

   void draw_point(rti_vec3 pos, rti_vec3 color, float stroke);
   void draw_line(rti_vec3 start, rti_vec3 end, rti_vec3 color, float stroke);

   void end_viewport();

   void begin_bvh_tree();
   void end_bvh_tree();

   void begin_node_info();
   void end_node_info();

   rti_acceleration_structure *addr_to_acceleration_structure(uint64_t address);
};

std::unique_ptr<rti_file_view> rti_create_file_view(rti_app *app, const char *file);

/* Create functions for rti_file_views specialized for drivers. */

std::unique_ptr<rti_file_view> rti_create_file_view_radv();
