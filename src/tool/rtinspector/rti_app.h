/*
 * Copyright © 2026 Valve Corporation
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <utility>

#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>

#include "backends/imgui_impl_vulkan.h"

#include "rti_file_view.h"
#include "rti_util.h"

#define RTI_MAX_OPEN_VIEWS 4096

#define RTI_CUBE_VERTEX_COUNT        (2 * 4 * 2 + 4 * 2)
#define RTI_FILLED_CUBE_VERTEX_COUNT (3 * 2 * 6)

struct rti_app {
   SDL_Window *window;
   VkInstance instance;
   VkPhysicalDevice pdev;
   VkDevice device;
   VkQueue queue;
   uint32_t queue_family_index;
   ImGui_ImplVulkanH_Window imgui_window;
   std::vector<std::unique_ptr<rti_file_view>> open_files;
   VkCommandBuffer command_buffer;

   PFN_vkCmdDrawMultiEXT vkCmdDrawMultiEXT;

   VkCommandPool staging_command_pool;
   VkCommandBuffer staging_command_buffer = VK_NULL_HANDLE;
   std::vector<std::pair<std::shared_ptr<rti_backed_buffer>, std::shared_ptr<rti_backed_buffer>>>
      pending_staging_buffers;

   VkDescriptorSetLayout renderer_set_layout;
   VkPipelineLayout renderer_pipeline_layout;
   VkPipeline fill_pipeline;
   VkPipeline wireframe_pipeline;
   VkPipeline thick_wireframe_pipeline;
   VkPipeline lines_pipeline;
   VkPipeline thick_lines_pipeline;

   uint32_t sample_count;
   uint32_t max_multi_draw_count;

   std::shared_ptr<rti_backed_buffer> cube_vertex_buffer;
   std::shared_ptr<rti_backed_buffer> filled_cube_vertex_buffer;

   std::chrono::high_resolution_clock::time_point last_run_time;
   bool last_run_time_valid = false;
   float t = 0;
   float dt;

   rti_vec3 selection_color;

   void *upload_memory(std::shared_ptr<rti_backed_buffer> dst);

   void flush_upload_memory();
   void finish_upload_memory();
};

int rti_app_init(rti_app *app);

void rti_app_finish(rti_app *app);

void rti_app_resize(rti_app *app, uint32_t width, uint32_t height);

bool rti_app_run(rti_app *app);
