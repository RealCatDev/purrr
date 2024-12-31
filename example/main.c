#include "purrr/purrr.h"

/*
 * Press `escape` to close.
 */

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

static struct {
  purrr_buffer_info_t vertex_buffer_info;
  purrr_buffer_t *vertex_buffer;
  purrr_buffer_info_t index_buffer_info;
  purrr_buffer_t *index_buffer;
  uint32_t index_count;
} s_mesh;

static bool s_running = true;

void initialize_mesh(purrr_renderer_t *);
void cleanup_mesh();

void key_callback(purrr_window_t *window, int key, int scancode, int action, int mods) {
  if (action != 1) return;
  if (key == PURRR_KEY_T) {
    printf("%s\n", (char*)purrr_window_get_user_ptr(window));
  } else if (key == PURRR_KEY_ESCAPE) {
    s_running = false; // Why are you running?
  }
}

int main(void) {
  int w, h, c;
  stbi_uc *pixels = stbi_load("./chp.png", &w, &h, &c, STBI_rgb_alpha);

  purrr_window_callbacks_t *callbacks = NULL;

  purrr_window_info_t window_info = {
    .options = (purrr_window_options_t)(PURRR_WINDOW_OPTION_BORDERLESS | PURRR_WINDOW_OPTION_NOT_RESIZABLE | PURRR_WINDOW_OPTION_TRANSPARENT), // I don't want to deal with resizing the render targets rn.
    .api = PURRR_API_VULKAN,
    .title = "purrr example",
    .width = w,
    .height = h,
    .x = PURRR_WINDOW_POS_CENTER,
    .y = PURRR_WINDOW_POS_CENTER,
    .callbacks_ptr = &callbacks,
  };

  purrr_window_t *window = purrr_window_create(&window_info);
  assert(window);

  char *text = "It is good day to be not dead!";
  purrr_window_set_user_ptr(window, text);

  callbacks->key = &key_callback;

  purrr_cursor_t *cursor = purrr_cursor_create_standard(PURRR_STANDARD_CURSOR_RESIZE);
  assert(cursor);

  purrr_window_set_cursor(window, cursor);

  purrr_window_icon_info_t icon_info = {
    .pixels = pixels,
    .width = w,
    .height = h,
  };

  purrr_window_set_icons(window, &icon_info, NULL);

  purrr_format_t swap_format = PURRR_FORMAT_UNDEFINED;
  purrr_image_t **swap_images = NULL;
  purrr_renderer_info_t renderer_info = {
    .window = window,
    .vsync = true,
    .image_count = 2,
    .swapchain_format = &swap_format,
    .swapchain_images = &swap_images,
  };

  purrr_renderer_t *renderer = purrr_renderer_create(&renderer_info);
  assert(renderer);

  purrr_pipeline_descriptor_attachment_info_t color_attachment = {
    .format = swap_format,
    .load = false,
    .store = true,
    .present_src = true,
  };

  purrr_pipeline_descriptor_info_t pipeline_descriptor_info = {
    .color_attachments = &color_attachment,
    .color_attachment_count = 1,
  };

  purrr_pipeline_descriptor_t *pipeline_descriptor = purrr_pipeline_descriptor_create(&pipeline_descriptor_info, renderer);
  assert(pipeline_descriptor);

  purrr_render_target_t *render_targets[2];

  purrr_render_target_info_t render_target_info = {
    .pipeline_descriptor = pipeline_descriptor,
  };
  purrr_window_get_size(window, &render_target_info.width, &render_target_info.height);

  for (uint32_t i = 0; i < 2; ++i) {
    render_target_info.images = &swap_images[i];

    render_targets[i] = purrr_render_target_create(&render_target_info, renderer);
    assert(render_targets[i]);
  }

  purrr_shader_info_t vertex_shader_info = {
    .filename = "./vertex.spv",
    .type = PURRR_SHADER_TYPE_VERTEX,
  };
  purrr_shader_t *vertex_shader = purrr_shader_create(&vertex_shader_info, renderer);
  assert(vertex_shader);

  purrr_shader_info_t fragment_shader_info = {
    .filename = "./fragment.spv",
    .type = PURRR_SHADER_TYPE_FRAGMENT,
  };
  purrr_shader_t *fragment_shader = purrr_shader_create(&fragment_shader_info, renderer);
  assert(fragment_shader);

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
    .shaders = (purrr_shader_t*[]){ vertex_shader, fragment_shader },
    .shader_count = 2,
    .pipeline_descriptor = pipeline_descriptor,
    .mesh_info = (purrr_mesh_binding_info_t){
      .vertex_infos = vertex_infos,
      .vertex_info_count = 2,
    },
    .descriptor_slots = (purrr_descriptor_type_t[]){ PURRR_DESCRIPTOR_TYPE_TEXTURE },
    .descriptor_slot_count = 1,
  };

  purrr_pipeline_t *pipeline = purrr_pipeline_create(&pipeline_info, renderer);
  assert(pipeline);

  purrr_shader_destroy(vertex_shader);
  purrr_shader_destroy(fragment_shader);

  initialize_mesh(renderer);

  purrr_image_info_t image_info = {
    .width  = (uint32_t)w,
    .height = (uint32_t)h,
    .format = PURRR_FORMAT_RGBA8RGB,
  };

  purrr_image_t *image = purrr_image_create(&image_info, renderer);
  assert(image);

  assert(purrr_image_load(image, pixels, (uint32_t)w, (uint32_t)h));

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
    .image = image,
    .sampler = sampler,
  };

  purrr_texture_t *texture = purrr_texture_create(&texture_info, renderer);
  assert(texture);

  uint32_t image_index = 0;
  while (!purrr_window_should_close(window) && s_running) {
    purrr_renderer_begin_frame(renderer, &image_index);

    purrr_renderer_begin_render_target(renderer, render_targets[image_index]);
    purrr_renderer_bind_pipeline(renderer, pipeline);

    purrr_renderer_bind_buffer(renderer, s_mesh.vertex_buffer, 0);
    purrr_renderer_bind_buffer(renderer, s_mesh.index_buffer, 0);

    purrr_renderer_bind_texture(renderer, texture, 0);

    purrr_renderer_draw_indexed(renderer, 1, 0, s_mesh.index_count, 0, 0);

    purrr_renderer_end_render_target(renderer);

    purrr_renderer_end_frame(renderer);
    purrr_poll_events();
  }
  purrr_renderer_wait(renderer);

  cleanup_mesh();
  purrr_sampler_destroy(sampler);
  purrr_image_destroy(image);
  purrr_pipeline_destroy(pipeline);
  purrr_pipeline_descriptor_destroy(pipeline_descriptor);
  for (uint32_t i = 0; i < 2; ++i) purrr_render_target_destroy(render_targets[i]);
  purrr_renderer_destroy(renderer);
  purrr_window_destroy(window);
  stbi_image_free(pixels);

  return 0;
}

void initialize_mesh(purrr_renderer_t *renderer) {
  s_mesh.vertex_buffer_info = (purrr_buffer_info_t){
    .type = PURRR_BUFFER_TYPE_VERTEX,
    .size = sizeof(gVertices),
  };
  s_mesh.vertex_buffer = purrr_buffer_create(&s_mesh.vertex_buffer_info, renderer);
  assert(purrr_buffer_copy(s_mesh.vertex_buffer, gVertices, sizeof(gVertices), 0));

  s_mesh.index_buffer_info = (purrr_buffer_info_t){
    .type = PURRR_BUFFER_TYPE_INDEX,
    .size = sizeof(gIndices),
  };
  s_mesh.index_buffer = purrr_buffer_create(&s_mesh.index_buffer_info, renderer);
  assert(purrr_buffer_copy(s_mesh.index_buffer, gIndices, sizeof(gIndices), 0));
  s_mesh.index_count = sizeof(gIndices)/sizeof(gIndices[0]);
}

void cleanup_mesh() {
  purrr_buffer_destroy(s_mesh.vertex_buffer);
  purrr_buffer_destroy(s_mesh.index_buffer);
}