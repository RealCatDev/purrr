#include "internal.h"

void _purrr_sampler_free(_purrr_sampler_t *sampler) {
  if (!sampler || !sampler->initialized) return;
  if (sampler->cleanup) sampler->cleanup(sampler);
  free(sampler);
}

void _purrr_texture_free(_purrr_texture_t *texture) {
  if (!texture || !texture->initialized) return;
  if (texture->cleanup) texture->cleanup(texture);
  free(texture);
}

void _purrr_pipeline_descriptor_free(_purrr_pipeline_descriptor_t *pipeline_descriptor) {
  if (!pipeline_descriptor || !pipeline_descriptor->initialized) return;
  if (pipeline_descriptor->cleanup) pipeline_descriptor->cleanup(pipeline_descriptor);
  free(pipeline_descriptor);
}

void _purrr_pipeline_free(_purrr_pipeline_t *pipeline) {
  if (!pipeline || !pipeline->initialized) return;
  if (pipeline->cleanup) pipeline->cleanup(pipeline);
  free(pipeline);
}

void _purrr_render_target_free(_purrr_render_target_t *render_target) {
  if (!render_target || !render_target->initialized) return;
  if (render_target->cleanup) render_target->cleanup(render_target);
  free(render_target);
}

void _purrr_mesh_free(_purrr_mesh_t *mesh) {
  if (!mesh || !mesh->initialized) return;
  if (mesh->cleanup) mesh->cleanup(mesh);
  free(mesh);
}

void _purrr_renderer_free(_purrr_renderer_t *renderer) {
  if (!renderer || !renderer->initialized) return;
  if (renderer->cleanup) renderer->cleanup(renderer);
  free(renderer);
}