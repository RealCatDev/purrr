#include "purrr/purrr.h"

#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static uint32_t gIndices[] = {
  0, 1, 2, 2, 3, 0
};

typedef struct {
  float pos[3];
  float uv[2];
} vertex_t;

static vertex_t gVertices[] = {
  { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, },
  { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, },
  { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }, },
  { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, },
};

void resize(void *data) {
  (void)data;
  printf("Resized!\n");
}

int main(void) {
  purrr_window_info_t window_info = {
    .api = PURRR_API_VULKAN,
    .title = "UwU",
    .width = PURRR_WINDOW_SIZE_DONT_MIND,
    .height = PURRR_WINDOW_SIZE_DONT_MIND,
    .x = PURRR_WINDOW_POS_CENTER,
    .y = PURRR_WINDOW_POS_CENTER,
  };

  purrr_window_t *window = purrr_window_create(&window_info);
  assert(window);

  purrr_renderer_info_t renderer_info = {
    .window = window,
    .vsync = true,
  };

  purrr_renderer_t *renderer = purrr_renderer_create(&renderer_info);
  assert(renderer);

  purrr_pipeline_shader_info_t shaders[] = {
    (purrr_pipeline_shader_info_t){
      .file_path = "./example/vertex.spv",
      .type = PURRR_SHADER_TYPE_VERTEX
    },
    (purrr_pipeline_shader_info_t){
      .file_path = "./example/fragment.spv",
      .type = PURRR_SHADER_TYPE_FRAGMENT
    },
  };

  purrr_vertex_info_t vertex_infos[] = {
    (purrr_vertex_info_t){
      .format = PURRR_FORMAT_RGB32F,
      .size = 12,
      .offset = 0,
    },
    (purrr_vertex_info_t){
      .format = PURRR_FORMAT_RG32F,
      .size = 8,
      .offset = 12,
    },
  };

  purrr_pipeline_info_t pipeline_info = {
    .shader_infos = shaders,
    .shader_info_count = 2,
    .pipeline_descriptor = renderer_info.swapchain_pipeline_descriptor,
    .mesh_info = (purrr_mesh_binding_info_t){
      .vertex_infos = vertex_infos,
      .vertex_info_count = 2,
    },
    .descriptor_slots = (purrr_descriptor_type_t[]){ PURRR_DESCRIPTOR_TYPE_TEXTURE },
    .descriptor_slot_count = 1,
  };

  purrr_pipeline_t *pipeline = purrr_pipeline_create(&pipeline_info, renderer);
  assert(pipeline);

  purrr_mesh_info_t mesh_info = {
    .index_count = 6,
    .indices_size = sizeof(gIndices),
    .indices = gIndices,
    .vertices_size = sizeof(gVertices),
    .vertices = gVertices,
  };

  purrr_mesh_t *mesh = purrr_mesh_create(&mesh_info, renderer);

  purrr_sampler_info_t sampler_info = {
    .mag_filter = PURRR_SAMPLER_FILTER_LINEAR,
    .min_filter = PURRR_SAMPLER_FILTER_LINEAR,
    .address_mode_u = PURRR_SAMPLER_ADDRESS_MODE_REPEAT,
    .address_mode_v = PURRR_SAMPLER_ADDRESS_MODE_REPEAT,
    .address_mode_w = PURRR_SAMPLER_ADDRESS_MODE_REPEAT,
  };

  purrr_sampler_t *sampler = purrr_sampler_create(&sampler_info, renderer);
  assert(sampler);

  purrr_texture_info_t texture_info = {
    .sampler = sampler,
  };

  purrr_texture_t *texture = NULL;
  {
    int w, h, c;
    stbi_uc *pixels = stbi_load("./example/chp.png", &w, &h, &c, STBI_rgb_alpha);
    texture_info.width  = (uint32_t)w;
    texture_info.height = (uint32_t)h;
    texture_info.format = PURRR_FORMAT_RGBA8RGB;

    texture = purrr_texture_create(&texture_info, renderer);
    assert(texture);

    assert(purrr_texture_load(texture, pixels, (uint32_t)w, (uint32_t)h));
    stbi_image_free(pixels);
  }

  purrr_pipeline_descriptor_attachment_info_t color_attachments[] = {
    (purrr_pipeline_descriptor_attachment_info_t){
      .format = PURRR_FORMAT_RGBA8RGB,
      .load = false,
      .store = true,
      .sampler = sampler,
    },
  };

  purrr_pipeline_descriptor_attachment_info_t depth_attachment = {
    .format = PURRR_FORMAT_DEPTH,
    .load = false,
    .store = false,
    .sampler = sampler
  };

  purrr_pipeline_descriptor_info_t offscreen_pipeline_descriptor_info = {
    .color_attachments = color_attachments,
    .color_attachment_count = 1,
    .depth_attachment = &depth_attachment,
  };

  purrr_pipeline_descriptor_t *offscreen_pipeline_descriptor = purrr_pipeline_descriptor_create(&offscreen_pipeline_descriptor_info, renderer);
  assert(offscreen_pipeline_descriptor);

  purrr_render_target_info_t offscreen_render_target_info = {
    .pipeline_descriptor = offscreen_pipeline_descriptor,
    .width = 1920,
    .height = 1080,
  };

  purrr_render_target_t *offscreen_render_target = purrr_render_target_create(&offscreen_render_target_info, renderer);
  assert(offscreen_render_target);

  purrr_pipeline_info_t offscreen_pipeline_info = {
    .shader_infos = shaders,
    .shader_info_count = 2,
    .pipeline_descriptor = offscreen_pipeline_descriptor,
    .mesh_info = (purrr_mesh_binding_info_t){
      .vertex_infos = vertex_infos,
      .vertex_info_count = 2,
    },
    .descriptor_slots = (purrr_descriptor_type_t[]){ PURRR_DESCRIPTOR_TYPE_TEXTURE },
    .descriptor_slot_count = 1,
  };

  purrr_pipeline_t *offscreen_pipeline = purrr_pipeline_create(&offscreen_pipeline_info, renderer);
  assert(offscreen_pipeline);

  purrr_renderer_set_user_data(renderer, NULL);
  purrr_renderer_set_resize_callback(renderer, resize);

  while (!purrr_window_should_close(window)) {
    purrr_renderer_begin_frame(renderer);

    purrr_renderer_begin_render_target(renderer, offscreen_render_target);
    purrr_renderer_bind_pipeline(renderer, offscreen_pipeline);
    purrr_renderer_bind_texture(renderer, texture, 0);
    purrr_renderer_draw_mesh(renderer, mesh);
    purrr_renderer_end_render_target(renderer);

    purrr_renderer_begin_render_target(renderer, renderer_info.swapchain_render_target);
    purrr_renderer_bind_pipeline(renderer, pipeline);
    purrr_texture_t *offscreen_texture = purrr_render_target_get_texture(offscreen_render_target, 0);
    assert(offscreen_texture);
    purrr_renderer_bind_texture(renderer, offscreen_texture, 0);
    purrr_renderer_draw_mesh(renderer, mesh);
    purrr_renderer_end_render_target(renderer);

    purrr_renderer_end_frame(renderer);
    purrr_poll_events();
  }
  purrr_renderer_wait(renderer);

  purrr_pipeline_destroy(offscreen_pipeline);
  purrr_pipeline_descriptor_destroy(offscreen_pipeline_descriptor);
  purrr_render_target_destroy(offscreen_render_target);
  purrr_mesh_destroy(mesh);
  purrr_sampler_destroy(sampler);
  purrr_texture_destroy(texture);
  purrr_pipeline_destroy(pipeline);
  purrr_renderer_destroy(renderer);
  purrr_window_destroy(window);

  return 0;
}