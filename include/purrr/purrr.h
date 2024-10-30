#ifndef   PURRR_HPP_
#define   PURRR_HPP_

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

typedef enum {
  PURRR_API_VULKAN = 0,
  COUNT_PURRR_APIS
} purrr_api_t;

typedef void *handle_t;

#define PURRR_WINDOW_POS_CENTER UINT32_MAX
#define PURRR_WINDOW_SIZE_DONT_MIND -1
#define PURRR_WINDOW_SIZE_MAX INT32_MAX

typedef struct {
  uint32_t x, y;
  int32_t width, height;
  const char *title;
  bool fullscreen;
  purrr_api_t api;
} purrr_window_info_t;

typedef struct purrr_window_s purrr_window_t;

purrr_window_t *purrr_window_create(purrr_window_info_t *info);
bool purrr_window_should_close(purrr_window_t *window);
void purrr_window_destroy(purrr_window_t *window);

typedef enum {
  PURRR_FORMAT_UNDEFINED = 0,

  // Color formats
  PURRR_FORMAT_RGBA8U,
  PURRR_FORMAT_RGBA8RGB,
  PURRR_FORMAT_BGRA8U,
  PURRR_FORMAT_BGRA8RGB,
  PURRR_FORMAT_RGBA16F,
  PURRR_FORMAT_RG32F,
  PURRR_FORMAT_RGB32F,
  PURRR_FORMAT_RGBA32F,
  PURRR_FORMAT_RGBA64F,

  // Depth formats
  PURRR_FORMAT_D24U8U,

  COUNT_PURRR_FORMATS
} purrr_format_t;

typedef enum {
  PURRR_SAMPLER_FILTER_NEAREST = 0,
  PURRR_SAMPLER_FILTER_LINEAR,
  COUNT_PURRR_SAMPLER_FILTERS
} purrr_sampler_filter_t;

typedef enum {
  PURRR_SAMPLER_ADDRESS_MODE_REPEAT = 0,
  PURRR_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
  PURRR_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  PURRR_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
  PURRR_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
  COUNT_PURRR_SAMPLER_ADDRESS_MODES
} purrr_sampler_address_mode_t;

typedef struct {
  purrr_sampler_filter_t mag_filter;
  purrr_sampler_filter_t min_filter;
  purrr_sampler_address_mode_t address_mode_u;
  purrr_sampler_address_mode_t address_mode_v;
  purrr_sampler_address_mode_t address_mode_w;
} purrr_sampler_info_t;

typedef struct purrr_renderer_s purrr_renderer_t;

typedef struct purrr_sampler_s purrr_sampler_t;

purrr_sampler_t *purrr_sampler_create(purrr_sampler_info_t *info, purrr_renderer_t *renderer);
void purrr_sampler_destroy(purrr_sampler_t *sampler);

typedef struct {
  purrr_sampler_t *sampler;
  uint32_t width, height;
  purrr_format_t format;
} purrr_texture_info_t;

typedef struct purrr_texture_s purrr_texture_t;

purrr_texture_t *purrr_texture_create(purrr_texture_info_t *info, purrr_renderer_t *renderer);
void purrr_texture_destroy(purrr_texture_t *texture);
bool purrr_texture_load(purrr_texture_t *dst, uint8_t *src, uint32_t src_width, uint32_t src_height);
bool purrr_texture_copy(purrr_texture_t *dst, purrr_texture_t *src, uint32_t src_width, uint32_t src_height);

typedef struct {
  purrr_format_t format;
  bool load;
  bool store;
  purrr_sampler_t *sampler;
} purrr_pipeline_descriptor_attachment_info_t;

typedef struct {
  purrr_pipeline_descriptor_attachment_info_t *color_attachments;
  uint32_t color_attachment_count;
  purrr_pipeline_descriptor_attachment_info_t *depth_attachment;
} purrr_pipeline_descriptor_info_t;

typedef struct purrr_pipeline_descriptor_s purrr_pipeline_descriptor_t;

purrr_pipeline_descriptor_t *purrr_pipeline_descriptor_create(purrr_pipeline_descriptor_info_t *info, purrr_renderer_t *renderer);
void purrr_pipeline_descriptor_destroy(purrr_pipeline_descriptor_t *pipeline_descriptor);

typedef struct {
  purrr_pipeline_descriptor_t *pipeline_descriptor;
  uint32_t width, height;
} purrr_render_target_info_t;

typedef struct purrr_render_target_s purrr_render_target_t;

purrr_render_target_t *purrr_render_target_create(purrr_render_target_info_t *info, purrr_renderer_t *renderer);
purrr_texture_t *purrr_render_target_get_texture(purrr_render_target_t *render_target, uint32_t texture_index);
void purrr_render_target_destroy(purrr_render_target_t *render_target);

typedef enum {
  PURRR_DESCRIPTOR_TYPE_TEXTURE = 0,
  COUNT_PURRR_DESCRIPTOR_TYPES
} purrr_descriptor_type_t;

typedef enum {
  PURRR_SHADER_TYPE_VERTEX = 0,
  PURRR_SHADER_TYPE_FRAGMENT,
  PURRR_SHADER_TYPE_COMPUTE,
  COUNT_PURRR_SHADER_TYPES
} purrr_shader_type_t;

typedef struct {
  const char *file_path;
  purrr_shader_type_t type;
} purrr_pipeline_shader_info_t;

typedef struct {
  purrr_format_t format;
  uint32_t size;
  uint32_t offset;
} purrr_vertex_info_t;

typedef struct {
  purrr_vertex_info_t *vertex_infos;
  uint32_t vertex_info_count;
} purrr_mesh_binding_info_t;

typedef struct {
  purrr_pipeline_shader_info_t *shader_infos;
  uint32_t shader_info_count;

  purrr_mesh_binding_info_t mesh_info;

  purrr_pipeline_descriptor_t *pipeline_descriptor;
  // bool depth;
  purrr_descriptor_type_t *descriptor_slots;
  uint32_t descriptor_slot_count;
} purrr_pipeline_info_t;

typedef struct purrr_pipeline_s purrr_pipeline_t;

purrr_pipeline_t *purrr_pipeline_create(purrr_pipeline_info_t *info, purrr_renderer_t *renderer);
void purrr_pipeline_destroy(purrr_pipeline_t *pipeline);

typedef struct {
  void     *vertices;
  uint32_t  vertices_size;
  uint32_t *indices;
  uint32_t  indices_size;
  uint32_t  index_count;
} purrr_mesh_info_t;

typedef struct purrr_mesh_s purrr_mesh_t;

purrr_mesh_t *purrr_mesh_create(purrr_mesh_info_t *info, purrr_renderer_t *renderer);
void purrr_mesh_destroy(purrr_mesh_t *mesh);

typedef struct {
  purrr_window_t *window;
  bool vsync;

  purrr_format_t swapchain_format;
  // Modified on create and resize, owned by renderer (DO NOT FREE OR CHANGE!)
  purrr_pipeline_descriptor_t *swapchain_pipeline_descriptor;
  // Modified on create and resize, owned by renderer (DO NOT FREE OR CHANGE!)
  purrr_render_target_t *swapchain_render_targets[2];
  // Modified by renderer (DO NOT FREE OR CHANGE!)
  purrr_render_target_t *swapchain_render_target;
} purrr_renderer_info_t;

// Callbacks
typedef void (*purrr_renderer_resize_cb)(void *user_pointer);

purrr_renderer_t *purrr_renderer_create(purrr_renderer_info_t *info);
// TODO: Add a function for recreating swapchain
void purrr_renderer_destroy(purrr_renderer_t *renderer);

void purrr_renderer_set_user_data(purrr_renderer_t *renderer, void *ptr);
void purrr_renderer_set_resize_callback(purrr_renderer_t *renderer, purrr_renderer_resize_cb cb);

void purrr_renderer_begin_frame(purrr_renderer_t *renderer);
void purrr_renderer_begin_render_target(purrr_renderer_t *renderer, purrr_render_target_t *render_target);
void purrr_renderer_bind_pipeline(purrr_renderer_t *renderer, purrr_pipeline_t *pipeline);
void purrr_renderer_bind_texture(purrr_renderer_t *renderer, purrr_texture_t *texture, uint32_t slot_index);
void purrr_renderer_draw_mesh(purrr_renderer_t *renderer, purrr_mesh_t *mesh);
void purrr_renderer_end_render_target(purrr_renderer_t *renderer);
void purrr_renderer_end_frame(purrr_renderer_t *renderer);
void purrr_renderer_wait(purrr_renderer_t *renderer);

void purrr_poll_events();

#endif // PURRR_HPP_

/*
 * TODO( #1): Add support for other graphics apis
 * TODO( #2): If device doesn't support api selected
              by user in purrr_window_info_t, choose
              other one instead of returning an error
 * TODO( #3): Let user select monitor for window
 * TODO( #4): Change function names to
              purrr_<operation>_<name> (for example:
              purrr_create_renderer)
 * TODO( #5): Add a macro for stripping `purrr`
              prefix
 * TODO( #6): Maybe change name of pipeline
              descriptor?
 * TODO( #7): Better vertex input info.
 */