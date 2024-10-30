#define NOB_IMPLEMENTATION
#include <nob.h>

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>

purrr_window_t *purrr_window_create(purrr_window_info_t *info) {
  if (!info || info->api >= COUNT_PURRR_APIS) return NULL;
  if (!info->title) info->title = "purrr window";

  if (glfwInit() != GLFW_TRUE) return NULL;
  // #2
  if (info->api == PURRR_API_VULKAN && !glfwVulkanSupported()) return NULL;

  GLFWmonitor *monitor = NULL;
  const GLFWvidmode *mode = NULL;
  if (info->fullscreen) {
    // #3
    monitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(monitor);
    if (info->width  == PURRR_WINDOW_SIZE_DONT_MIND) info->width  = mode->width;
    if (info->height == PURRR_WINDOW_SIZE_DONT_MIND) info->height = mode->height;
    info->x = 0;
    info->y = 0;
  } else {
    mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (info->width == PURRR_WINDOW_SIZE_DONT_MIND)  info->width  = mode->width/2;
    else if (info->width == PURRR_WINDOW_SIZE_MAX)   info->width  = mode->width;
    if (info->height == PURRR_WINDOW_SIZE_DONT_MIND) info->height = mode->height/2;
    else if (info->height == PURRR_WINDOW_SIZE_MAX)  info->height = mode->height;
    if (info->x == PURRR_WINDOW_POS_CENTER) info->x = (mode->width/2)-(info->width/2);
    if (info->y == PURRR_WINDOW_POS_CENTER) info->y = (mode->height/2)-(info->height/2);
  }

  _purrr_window_t *internal = (_purrr_window_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->api = info->api;

  switch (internal->api) {
  case PURRR_API_VULKAN: {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  } break;
  case COUNT_PURRR_APIS:
  default: {
    assert(0 && "purrr_window_create: Unreachable");
  } break;
  }

  internal->window = glfwCreateWindow(info->width, info->height, info->title, monitor, NULL);
  if (!internal->window) {
    purrr_window_destroy((purrr_window_t*)internal);
    return NULL;
  }
  glfwMakeContextCurrent(internal->window);

  glfwSetWindowMonitor(internal->window, monitor, info->x, info->y, info->width, info->height, mode->refreshRate);

  return (purrr_window_t*)internal;
}

bool purrr_window_should_close(purrr_window_t *window) {
  return glfwWindowShouldClose(((_purrr_window_t*)window)->window);
}

void purrr_window_destroy(purrr_window_t *window) {
  if (!window) return;
  _purrr_window_t *internal = (_purrr_window_t*)window;
  if (internal->window) glfwDestroyWindow(internal->window);
  free(internal);
}

// sampler

purrr_sampler_t *purrr_sampler_create(purrr_sampler_info_t *info, purrr_renderer_t *renderer) {
  if (!info ||
      info->mag_filter >= COUNT_PURRR_SAMPLER_FILTERS || info->min_filter >= COUNT_PURRR_SAMPLER_FILTERS ||
      info->address_mode_u >= COUNT_PURRR_SAMPLER_ADDRESS_MODES || info->address_mode_v >= COUNT_PURRR_SAMPLER_ADDRESS_MODES || info->address_mode_w >= COUNT_PURRR_SAMPLER_ADDRESS_MODES ||
      !renderer) return NULL;

  _purrr_sampler_t *internal = (_purrr_sampler_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->info = info;
  internal->renderer = (_purrr_renderer_t*)renderer;

  switch (((_purrr_renderer_t*)renderer)->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_sampler_vulkan_init;
    internal->cleanup = _purrr_sampler_vulkan_cleanup;
  } break;
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_sampler_free(internal);
    return NULL;
  }

  return (purrr_sampler_t*)internal;
}

void purrr_sampler_destroy(purrr_sampler_t *sampler) {
  if (sampler) _purrr_sampler_free((_purrr_sampler_t*)sampler);
}

// texture

purrr_texture_t *purrr_texture_create(purrr_texture_info_t *info, purrr_renderer_t *renderer) {
  if (!info || !info->sampler || info->format >= COUNT_PURRR_FORMATS || info->format == PURRR_FORMAT_UNDEFINED || !renderer) return NULL;

  _purrr_texture_t *internal = (_purrr_texture_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->info = info;
  internal->renderer = (_purrr_renderer_t*)renderer;

  switch (((_purrr_renderer_t*)renderer)->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_texture_vulkan_init;
    internal->cleanup = _purrr_texture_vulkan_cleanup;
    internal->load = _purrr_texture_vulkan_load;
    internal->copy = _purrr_texture_vulkan_copy;
  } break;
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_texture_free(internal);
    return NULL;
  }

  return (purrr_texture_t*)internal;
}

void purrr_texture_destroy(purrr_texture_t *texture) {
  if (texture) _purrr_texture_free((_purrr_texture_t*)texture);
}

bool purrr_texture_load(purrr_texture_t *dst, uint8_t *src, uint32_t src_width, uint32_t src_height) {
  _purrr_texture_t *internal = (_purrr_texture_t*)dst;
  assert(internal && src && internal->load);
  return internal->load(internal, src, src_width, src_height);
}

bool purrr_texture_copy(purrr_texture_t *dst, purrr_texture_t *src, uint32_t src_width, uint32_t src_height) {
  _purrr_texture_t *internal = (_purrr_texture_t*)dst;
  assert(internal && src && internal->copy);
  return internal->copy(internal, (_purrr_texture_t*)src, src_width, src_height);
}

// pipeline descriptor

purrr_pipeline_descriptor_t *purrr_pipeline_descriptor_create(purrr_pipeline_descriptor_info_t *info, purrr_renderer_t *renderer) {
  if (!info || info->color_attachment_count == 0 || !info->color_attachments || !renderer) return NULL;

  _purrr_pipeline_descriptor_t *internal = (_purrr_pipeline_descriptor_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->info = info;
  internal->renderer = (_purrr_renderer_t*)renderer;

  switch (((_purrr_renderer_t*)renderer)->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_pipeline_descriptor_vulkan_init;
    internal->cleanup = _purrr_pipeline_descriptor_vulkan_cleanup;
  } break;
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_pipeline_descriptor_free(internal);
    return NULL;
  }

  internal->initialized = true;

  return (purrr_pipeline_descriptor_t*)internal;
}

void purrr_pipeline_descriptor_destroy(purrr_pipeline_descriptor_t *pipeline_descriptor) {
  if (pipeline_descriptor) _purrr_pipeline_descriptor_free((_purrr_pipeline_descriptor_t*)pipeline_descriptor);
}

// pipeline

purrr_pipeline_t *purrr_pipeline_create(purrr_pipeline_info_t *info, purrr_renderer_t *renderer) {
  if (!info || !renderer ||
      !info->pipeline_descriptor ||
      (info->descriptor_slot_count > 0 && !info->descriptor_slots) ||
      (info->shader_info_count > 0 && !info->shader_infos))
    return NULL;

  _purrr_pipeline_t *internal = (_purrr_pipeline_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->info = info;
  internal->renderer = (_purrr_renderer_t*)renderer;

  switch (((_purrr_renderer_t*)renderer)->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_pipeline_vulkan_init;
    internal->cleanup = _purrr_pipeline_vulkan_cleanup;
  } break;
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_pipeline_free(internal);
    return NULL;
  }

  internal->initialized = true;

  return (purrr_pipeline_t*)internal;
}

void purrr_pipeline_destroy(purrr_pipeline_t *pipeline) {
  if (pipeline) _purrr_pipeline_free((_purrr_pipeline_t*)pipeline);
}

// render target

purrr_render_target_t *purrr_render_target_create(purrr_render_target_info_t *info, purrr_renderer_t *renderer) {
  if (!info || !info->pipeline_descriptor ||
      !info->width || !info->height ||
      !info->pipeline_descriptor || !((_purrr_pipeline_descriptor_t*)info->pipeline_descriptor)->initialized ||
      !renderer) return NULL;

  _purrr_render_target_t *internal = (_purrr_render_target_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->descriptor = (_purrr_pipeline_descriptor_t*)info->pipeline_descriptor;
  internal->width = info->width;
  internal->height = info->height;
  internal->renderer = (_purrr_renderer_t*)renderer;

  switch (((_purrr_renderer_t*)renderer)->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_render_target_vulkan_init;
    internal->cleanup = _purrr_render_target_vulkan_cleanup;
    internal->get_texture = _purrr_render_target_vulkan_get_texture;
  } break;
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_render_target_free(internal);
    return NULL;
  }

  internal->initialized = true;

  return (purrr_render_target_t*)internal;
}

purrr_texture_t *purrr_render_target_get_texture(purrr_render_target_t *render_target, uint32_t texture_index) {
  _purrr_render_target_t *internal = (_purrr_render_target_t*)render_target;
  assert(internal && internal->get_texture);
  _purrr_texture_t *texture = internal->get_texture(internal, texture_index);
  return (purrr_texture_t*)texture;
}

void purrr_render_target_destroy(purrr_render_target_t *render_target) {
  if (render_target) _purrr_render_target_free((_purrr_render_target_t*)render_target);
}

// mesh

purrr_mesh_t *purrr_mesh_create(purrr_mesh_info_t *info, purrr_renderer_t *renderer) {
  if (!info ||
      !info->indices_size || !info->indices ||
      !info->vertices_size || !info->vertices) return NULL;

  _purrr_mesh_t *internal = (_purrr_mesh_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->info = info;
  internal->renderer = (_purrr_renderer_t*)renderer;

  switch (internal->renderer->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_mesh_vulkan_init;
    internal->cleanup = _purrr_mesh_vulkan_cleanup;
  } break;
  case COUNT_PURRR_APIS:
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_mesh_free(internal);
    return NULL;
  }

  internal->initialized = true;

  return (purrr_mesh_t*)internal;
}

void purrr_mesh_destroy(purrr_mesh_t *mesh) {
  if (mesh) _purrr_mesh_free((_purrr_mesh_t*)mesh);
}

// renderer

purrr_renderer_t *purrr_renderer_create(purrr_renderer_info_t *info) {
  if (!info || !info->window) return NULL;

  _purrr_renderer_t *internal = (_purrr_renderer_t*)malloc(sizeof(*internal));
  if (!internal) return NULL;
  memset(internal, 0, sizeof(*internal));
  internal->info = info;
  internal->api = ((_purrr_window_t*)info->window)->api;

  switch (internal->api) {
  case PURRR_API_VULKAN: {
    internal->init = _purrr_renderer_vulkan_init;
    internal->cleanup = _purrr_renderer_vulkan_cleanup;
    internal->begin_frame = _purrr_renderer_vulkan_begin_frame;
    internal->begin_render_target = _purrr_renderer_vulkan_begin_render_target;
    internal->bind_pipeline = _purrr_renderer_vulkan_bind_pipeline;
    internal->bind_texture = _purrr_renderer_vulkan_bind_texture;
    internal->draw_mesh = _purrr_renderer_vulkan_draw_mesh;
    internal->end_render_target = _purrr_renderer_vulkan_end_render_target;
    internal->end_frame = _purrr_renderer_vulkan_end_frame;
    internal->wait = _purrr_renderer_vulkan_wait;
  } break;
  case COUNT_PURRR_APIS:
  default: {
    assert(0 && "Unreachable");
    return NULL;
  }
  }

  if (!internal->init(internal)) {
    _purrr_renderer_free(internal);
    return NULL;
  }

  internal->initialized = true;

  return (purrr_renderer_t*)internal;
}

void purrr_renderer_destroy(purrr_renderer_t *renderer) {
  if (renderer) _purrr_renderer_free((_purrr_renderer_t*)renderer);
}

void purrr_renderer_set_user_data(purrr_renderer_t *renderer, void *ptr) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal);
  internal->user_ptr = ptr;
}

void purrr_renderer_set_resize_callback(purrr_renderer_t *renderer, purrr_renderer_resize_cb cb) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal);
  internal->callbacks.resize = cb;
}

void purrr_renderer_begin_frame(purrr_renderer_t *renderer) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->begin_frame);
  assert(internal->begin_frame(internal));
}

void purrr_renderer_begin_render_target(purrr_renderer_t *renderer, purrr_render_target_t *render_target) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->begin_render_target && render_target);
  assert(internal->begin_render_target(internal, (_purrr_render_target_t*)render_target));
}

void purrr_renderer_bind_pipeline(purrr_renderer_t *renderer, purrr_pipeline_t *pipeline) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->bind_pipeline && pipeline);
  assert(internal->bind_pipeline(internal, (_purrr_pipeline_t*)pipeline));
}

void purrr_renderer_bind_texture(purrr_renderer_t *renderer, purrr_texture_t *texture, uint32_t slot_index) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->bind_texture && texture);
  assert(internal->bind_texture(internal, (_purrr_texture_t*)texture, slot_index));
}

void purrr_renderer_draw_mesh(purrr_renderer_t *renderer, purrr_mesh_t *mesh) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->draw_mesh && mesh);
  assert(internal->draw_mesh(internal, (_purrr_mesh_t*)mesh));
}

void purrr_renderer_end_render_target(purrr_renderer_t *renderer) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->end_render_target);
  assert(internal->end_render_target(internal));
}

void purrr_renderer_end_frame(purrr_renderer_t *renderer) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->end_frame);
  assert(internal->end_frame(internal));
}

void purrr_renderer_wait(purrr_renderer_t *renderer) {
  _purrr_renderer_t *internal = (_purrr_renderer_t*)renderer;
  assert(internal && internal->wait);
  internal->wait(internal);
}

void purrr_poll_events() {
  glfwPollEvents();
}

// struct ContextData {
//   GLFWwindow *window;

//   VkInstance instance;
//   VkSurfaceKHR surface;
//   VkPhysicalDevice physicalDevice;
//   uint32_t graphicsFamily;
//   uint32_t presentFamily;
//   VkDevice device;
//   VkQueue graphicsQueue;
//   VkQueue presentQueue;
//   VkSwapchainKHR swapchain;
//   VkCommandPool commandPool;
//   VkImage swapchainImages[2];
//   VkImageView swapchainImageViews[2];
//   VkRenderPass swapchainRenderPass;
//   VkFramebuffer swapchainFramebuffers[2];
//   VkCommandBuffer renderBuffers[2];
//   VkSemaphore imageSemaphores[2];
//   VkSemaphore renderSemaphores[2];
//   VkFence fences[2];

//   VkSurfaceFormatKHR swapchainFormat;
//   VkPresentModeKHR swapchainPresentMode;
//   VkExtent2D swapchainExtent;

//   uint8_t frame = 0;
//   uint32_t imageIndex = 0;

//   VkDescriptorPool texturePool = VK_NULL_HANDLE;
//   VkDescriptorSetLayout textureLayout = VK_NULL_HANDLE;

//   purrenderer::purrResizeCb resizeCb;
// };

// namespace purrenderer {
//   static ContextData *sContextData = nullptr;
// }

// namespace purrenderer { // Utils

//   static VkFormat purrFormatToVk(purrFormat format) {
//     switch (format) {
//     case purrFormat::RGBA8U:  return VK_FORMAT_R8G8B8A8_UNORM;
//     case purrFormat::RGBA8S:  return VK_FORMAT_R8G8B8A8_SRGB;
//     case purrFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
//     case purrFormat::R32F:    return VK_FORMAT_R32_SFLOAT;
//     case purrFormat::RG32F:   return VK_FORMAT_R32G32_SFLOAT;
//     case purrFormat::RGB32F:  return VK_FORMAT_R32G32B32_SFLOAT;
//     case purrFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
//     case purrFormat::RGBA64F: return VK_FORMAT_R64G64B64A64_SFLOAT;
//     }
//     return VK_FORMAT_UNDEFINED;
//   }

//   static VkShaderStageFlagBits purrShaderTypeToVk(purrShaderType type) {
//     switch (type) {
//     case purrShaderType::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
//     case purrShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
//     }
//     assert(0 && "Unreachable");
//   }

//   static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
//     VkPhysicalDeviceMemoryProperties memProperties{};
//     vkGetPhysicalDeviceMemoryProperties(sContextData->physicalDevice, &memProperties);

//     for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
//       if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
//         return i;
//     return UINT32_MAX;
//   }

//   static bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
//     VkBufferCreateInfo bufferInfo{};
//     bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//     bufferInfo.size = size;
//     bufferInfo.usage = usage;
//     bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//     if (vkCreateBuffer(sContextData->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) return NULL;
//
//     VkMemoryRequirements memRequirements;
//     vkGetBufferMemoryRequirements(sContextData->device, buffer, &memRequirements);
//
//     VkMemoryAllocateInfo allocInfo{};
//     allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//     allocInfo.allocationSize = memRequirements.size;
//     allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
//
//     if (vkAllocateMemory(sContextData->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) return false;
//
//     vkBindBufferMemory(sContextData->device, buffer, bufferMemory, 0);
//
//     return true;
//   }
//
//   static void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
//     VkCommandBuffer cmdBuf = beginSingleTimeCommands();
//
//     VkBufferCopy copyRegion{};
//     copyRegion.srcOffset = 0;
//     copyRegion.dstOffset = 0;
//     copyRegion.size = size;
//     vkCmdCopyBuffer(cmdBuf, src, dst, 1, &copyRegion);
//
//     endSingleTimeCommands(cmdBuf);
//   }
//
//   static void copyBufferToImage(VkBuffer src, VkImage dst, uint32_t width, uint32_t height) {
//     VkCommandBuffer cmdBuf = beginSingleTimeCommands();
//
//     VkBufferImageCopy region{};
//     region.bufferOffset = 0;
//     region.bufferRowLength = 0;
//     region.bufferImageHeight = 0;
//
//     region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//     region.imageSubresource.mipLevel = 0;
//     region.imageSubresource.baseArrayLayer = 0;
//     region.imageSubresource.layerCount = 1;
//
//     region.imageOffset = {0, 0, 0};
//     region.imageExtent = {
//       width,
//       height,
//       1
//     };
//
//     vkCmdCopyBufferToImage(
//       cmdBuf,
//       src,
//       dst,
//       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//       1,
//       &region
//     );
//
//     endSingleTimeCommands(cmdBuf);
//   }
//
// }

// namespace purrenderer { // purrMesh

//   purrMesh::purrMesh()
//   {}

//   purrMesh::~purrMesh() {
//     vkDestroyBuffer(sContextData->device, mVBuffer, VK_NULL_HANDLE);
//     vkFreeMemory(sContextData->device, mVMemory, VK_NULL_HANDLE);
//     vkDestroyBuffer(sContextData->device, mIBuffer, VK_NULL_HANDLE);
//     vkFreeMemory(sContextData->device, mIMemory, VK_NULL_HANDLE);
//   }

//   bool purrMesh::init(void *vertices, size_t vertexSize, size_t vertexCount, uint32_t *indices, size_t indexCount) {
//     {
//       VkDeviceSize size = vertexSize*vertexCount;

//       VkBuffer stagingBuffer;
//       VkDeviceMemory stagingMemory;
//       if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory)) return false;

//       void *bufData;
//       vkMapMemory(sContextData->device, stagingMemory, 0, size, 0, &bufData);
//         memcpy(bufData, vertices, size);
//       vkUnmapMemory(sContextData->device, stagingMemory);

//       if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVBuffer, mVMemory)) return false;
//       copyBuffer(stagingBuffer, mVBuffer, size);

//       vkDestroyBuffer(sContextData->device, stagingBuffer, VK_NULL_HANDLE);
//       vkFreeMemory(sContextData->device, stagingMemory, VK_NULL_HANDLE);
//     }

//     {
//       VkDeviceSize size = sizeof(uint32_t)*(mIndexCount = indexCount);

//       VkBuffer stagingBuffer;
//       VkDeviceMemory stagingMemory;
//       if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory)) return false;

//       void *bufData;
//       vkMapMemory(sContextData->device, stagingMemory, 0, size, 0, &bufData);
//         memcpy(bufData, indices, size);
//       vkUnmapMemory(sContextData->device, stagingMemory);

//       if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIBuffer, mIMemory)) return false;
//       copyBuffer(stagingBuffer, mIBuffer, size);

//       vkDestroyBuffer(sContextData->device, stagingBuffer, VK_NULL_HANDLE);
//       vkFreeMemory(sContextData->device, stagingMemory, VK_NULL_HANDLE);
//     }

//     return true;
//   }

// }

// namespace purrenderer { // purrSampler
//
//   purrSampler::purrSampler()
//   {}
//
//   purrSampler::~purrSampler() {
//     vkDestroySampler(sContextData->device, mSampler, VK_NULL_HANDLE);
//   }
//
//   bool purrSampler::init() {
//     VkSamplerCreateInfo samplerInfo{};
//     samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//     samplerInfo.magFilter = VK_FILTER_LINEAR;
//     samplerInfo.minFilter = VK_FILTER_LINEAR;
//     samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//     samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//     samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//     samplerInfo.anisotropyEnable = VK_FALSE; // TODO: Add anisotropy
//     samplerInfo.maxAnisotropy = 1.0f;
//     samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
//     samplerInfo.unnormalizedCoordinates = VK_FALSE;
//     samplerInfo.compareEnable = VK_FALSE;
//     samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
//     samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//     samplerInfo.mipLodBias = 0.0f;
//     samplerInfo.minLod = 0.0f;
//     samplerInfo.maxLod = 0.0f;
//
//     return vkCreateSampler(sContextData->device, &samplerInfo, nullptr, &mSampler) == VK_SUCCESS;
//   }
//
// }

// namespace purrenderer { // purrTexture
//
//   purrTexture::purrTexture()
//   {}
//
//   purrTexture::~purrTexture() {
//     vkDestroyImage(sContextData->device, mImage, VK_NULL_HANDLE);
//     vkDestroyImageView(sContextData->device, mView, VK_NULL_HANDLE);
//     vkFreeMemory(sContextData->device, mMemory, VK_NULL_HANDLE);
//   }
//
//   bool purrTexture::init(uint32_t width, uint32_t height, purrFormat format, purrSampler *sampler) {
//     mFormat = purrFormatToVk(format);
//     mSize = (mWidth = width)*(mHeight = height)*4;
//
//     VkImageCreateInfo imageInfo{};
//     imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//     imageInfo.imageType = VK_IMAGE_TYPE_2D;
//     imageInfo.extent.width = width;
//     imageInfo.extent.height = height;
//     imageInfo.extent.depth = 1;
//     imageInfo.mipLevels = 1;
//     imageInfo.arrayLayers = 1;
//     imageInfo.format = mFormat;
//     imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//     imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//     imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//     imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//     imageInfo.flags = 0;
//
//     if (vkCreateImage(sContextData->device, &imageInfo, nullptr, &mImage) != VK_SUCCESS) return false;
//
//     VkMemoryRequirements memRequirements{};
//     vkGetImageMemoryRequirements(sContextData->device, mImage, &memRequirements);
//
//     VkMemoryAllocateInfo allocInfo{};
//     allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//     allocInfo.allocationSize = memRequirements.size;
//     allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//     if (vkAllocateMemory(sContextData->device, &allocInfo, nullptr, &mMemory) != VK_SUCCESS) return false;
//
//     vkBindImageMemory(sContextData->device, mImage, mMemory, 0);
//
//     VkImageViewCreateInfo createInfo{};
//     createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//     createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//     createInfo.format = mFormat;
//     createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//     createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//     createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//     createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//     createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//     createInfo.subresourceRange.baseMipLevel = 0;
//     createInfo.subresourceRange.levelCount = 1;
//     createInfo.subresourceRange.baseArrayLayer = 0;
//     createInfo.subresourceRange.layerCount = 1;
//     createInfo.image = mImage;
//
//     if (vkCreateImageView(sContextData->device, &createInfo, nullptr, &mView) != VK_SUCCESS) return false;
//
//     { // Set
//       VkDescriptorSetAllocateInfo allocInfo{};
//       allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//       allocInfo.descriptorPool = sContextData->texturePool;
//       allocInfo.descriptorSetCount = 1;
//       allocInfo.pSetLayouts = &sContextData->textureLayout;
//
//       if (vkAllocateDescriptorSets(sContextData->device, &allocInfo, &mSet) != VK_SUCCESS) return false;
//
//       VkDescriptorImageInfo imageInfo{};
//       imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//       imageInfo.imageView = mView;
//       imageInfo.sampler = sampler->get();
//
//       VkWriteDescriptorSet descriptorWrite{};
//       descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//       descriptorWrite.dstSet = mSet;
//       descriptorWrite.dstBinding = 0;
//       descriptorWrite.dstArrayElement = 0;
//       descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//       descriptorWrite.descriptorCount = 1;
//       descriptorWrite.pBufferInfo = nullptr;
//       descriptorWrite.pImageInfo = &imageInfo;
//       descriptorWrite.pTexelBufferView = nullptr;
//
//       vkUpdateDescriptorSets(sContextData->device, 1, &descriptorWrite, 0, nullptr);
//     }
//
//     return true;
//   }
//
//   bool purrTexture::copy(uint8_t *data) {
//     VkBuffer stagingBuffer;
//     VkDeviceMemory stagingMemory;
//     VkDeviceSize size = mWidth * mHeight * 4;
//     if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory)) return false;
//
//     void *bufData;
//     vkMapMemory(sContextData->device, stagingMemory, 0, size, 0, &bufData);
//       memcpy(bufData, data, size);
//     vkUnmapMemory(sContextData->device, stagingMemory);
//
//     transitionImageLayout(mImage, mFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
//     copyBufferToImage(stagingBuffer, mImage, mWidth, mHeight);
//     transitionImageLayout(mImage, mFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//
//     vkDestroyBuffer(sContextData->device, stagingBuffer, VK_NULL_HANDLE);
//     vkFreeMemory(sContextData->device, stagingMemory, VK_NULL_HANDLE);
//
//     return true;
//   }
//
// }

// namespace purrenderer { // purrPipeline
//
//   purrPipeline::purrPipeline()
//   {}
//
//   purrPipeline::~purrPipeline() {
//     vkDestroyPipeline(sContextData->device, mPipeline, VK_NULL_HANDLE);
//     vkDestroyPipelineLayout(sContextData->device, mLayout, VK_NULL_HANDLE);
//   }
//
//   bool purrPipeline::init(purrPipelineInfo info) {
//     std::vector<VkShaderModule> modules{};
//     std::vector<VkPipelineShaderStageCreateInfo> stages{};
//
//     for (std::pair<purrShaderType, const char *> shader: info.shaders) {
//       std::ifstream file(shader.second, std::ios::ate | std::ios::binary);
//       if (!file.is_open()) return false;
//       size_t fileSize = (size_t) file.tellg();
//       std::vector<char> buffer(fileSize);
//       file.seekg(0);
//       file.read(buffer.data(), fileSize);
//       file.close();
//
//       VkShaderModuleCreateInfo createInfo{};
//       createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//       createInfo.codeSize = buffer.size();
//       createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

//       VkShaderModule module;
//       if (vkCreateShaderModule(sContextData->device, &createInfo, nullptr, &module) != VK_SUCCESS) return false;

//       modules.push_back(module);

//       stages.push_back(VkPipelineShaderStageCreateInfo{
//         VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, VK_NULL_HANDLE, 0,
//         purrShaderTypeToVk(shader.first),
//         module,
//         "main"
//       });
//     }

//     VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//     vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

//     VkVertexInputBindingDescription binding{};
//     std::vector<VkVertexInputAttributeDescription> attributes{};

//     uint32_t i = 0, uhh = 0;
//     for (purrVertexAttrib attrib: info.vertexInfo.attributes) {
//       attributes.push_back(VkVertexInputAttributeDescription{
//         i++, 0,
//         purrFormatToVk(attrib.format), uhh
//       });
//       uhh += attrib.size;
//     }
//     binding.binding = 0;
//     binding.stride = uhh;
//     binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

//     vertexInputInfo.vertexBindingDescriptionCount = 1;
//     vertexInputInfo.pVertexBindingDescriptions = &binding;
//     vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
//     vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

//     VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//     inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     inputAssembly.primitiveRestartEnable = VK_FALSE;

//     VkPipelineViewportStateCreateInfo viewportState{};
//     viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     viewportState.viewportCount = 1;
//     viewportState.scissorCount = 1;

//     VkPipelineRasterizationStateCreateInfo rasterizer{};
//     rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     rasterizer.depthClampEnable = VK_FALSE;
//     rasterizer.rasterizerDiscardEnable = VK_FALSE;
//     rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//     rasterizer.lineWidth = 1.0f;
//     rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//     rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//     rasterizer.depthBiasEnable = VK_FALSE;

//     VkPipelineMultisampleStateCreateInfo multisampling{};
//     multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     multisampling.sampleShadingEnable = VK_FALSE;
//     multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

//     VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//     colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//     colorBlendAttachment.blendEnable = VK_FALSE;

//     VkPipelineColorBlendStateCreateInfo colorBlending{};
//     colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     colorBlending.logicOpEnable = VK_FALSE;
//     colorBlending.logicOp = VK_LOGIC_OP_COPY;
//     colorBlending.attachmentCount = 1;
//     colorBlending.pAttachments = &colorBlendAttachment;
//     colorBlending.blendConstants[0] = 0.0f;
//     colorBlending.blendConstants[1] = 0.0f;
//     colorBlending.blendConstants[2] = 0.0f;
//     colorBlending.blendConstants[3] = 0.0f;

//     std::vector<VkDynamicState> dynamicStates = {
//         VK_DYNAMIC_STATE_VIEWPORT,
//         VK_DYNAMIC_STATE_SCISSOR
//     };
//     VkPipelineDynamicStateCreateInfo dynamicState{};
//     dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//     dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
//     dynamicState.pDynamicStates = dynamicStates.data();

//     VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//     pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

//     std::vector<VkDescriptorSetLayout> layouts{};
//     for (purrDescriptorType type: info.descriptors) {
//       VkDescriptorSetLayout layout = VK_NULL_HANDLE;
//       if (type == purrDescriptorType::Texture) layout = sContextData->textureLayout;
//       layouts.push_back(layout);
//     }
//     pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
//     pipelineLayoutInfo.pSetLayouts = layouts.data();
//     pipelineLayoutInfo.pushConstantRangeCount = 0;

//     if (vkCreatePipelineLayout(sContextData->device, &pipelineLayoutInfo, nullptr, &mLayout) != VK_SUCCESS) return false;

//     VkGraphicsPipelineCreateInfo pipelineInfo{};
//     pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//     pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
//     pipelineInfo.pStages = stages.data();
//     pipelineInfo.pVertexInputState = &vertexInputInfo;
//     pipelineInfo.pInputAssemblyState = &inputAssembly;
//     pipelineInfo.pViewportState = &viewportState;
//     pipelineInfo.pRasterizationState = &rasterizer;
//     pipelineInfo.pMultisampleState = &multisampling;
//     pipelineInfo.pColorBlendState = &colorBlending;
//     pipelineInfo.pDynamicState = &dynamicState;
//     pipelineInfo.layout = mLayout;
//     pipelineInfo.renderPass = sContextData->swapchainRenderPass;
//     pipelineInfo.subpass = 0;
//     pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

//     if (vkCreateGraphicsPipelines(sContextData->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) return false;

//     for (VkShaderModule module: modules) vkDestroyShaderModule(sContextData->device, module, nullptr);

//     return true;
//   }

// }

// namespace purrenderer { // purrWindow && purrContext

//   purrWindow::purrWindow(const char *title, int width, int height)
//   {
//     assert(glfwInit());
//     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//     assert(glfwVulkanSupported());
//     mWindow = glfwCreateWindow(width, height, title, NULL, NULL);
//     assert(mWindow);
//   }

//   purrWindow::~purrWindow() {
//     glfwDestroyWindow(mWindow);
//   }

//   void purrWindow::setFullscreen(bool fs) {
//     GLFWmonitor *monitor = glfwGetPrimaryMonitor();
//     const GLFWvidmode *videMode = glfwGetVideoMode(monitor);
//     GLFWmonitor *windowMonitor = glfwGetWindowMonitor(mWindow);
//     if (fs && windowMonitor == NULL) {
//       glfwGetWindowSize(mWindow, &mWidth, &mHeight);
//       glfwGetWindowPos(mWindow, &mX, &mY);
//       glfwSetWindowMonitor(mWindow, monitor, 0, 0, videMode->width, videMode->height, videMode->refreshRate);
//     } else if (!fs && windowMonitor) glfwSetWindowMonitor(mWindow, NULL, mX, mY, mWidth, mHeight, GLFW_DONT_CARE);
//   }

//   void purrWindow::update() {
//     glfwPollEvents();
//   }

//   bool purrWindow::isKeyDown(int key) {
//     return glfwGetKey(mWindow, key) == GLFW_PRESS;
//   }

//   bool purrWindow::isKeyUp(int key) {
//     return glfwGetKey(mWindow, key) == GLFW_RELEASE;
//   }

//   bool purrWindow::shouldClose() const {
//     return glfwWindowShouldClose(mWindow);
//   }

//   static bool findQueueFamilies(VkPhysicalDevice device, uint32_t *o_graphics, uint32_t *o_present) {
//     uint32_t queueFamilyCount = 0;
//     vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
//     std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
//     vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

//     uint32_t graphics = UINT32_MAX;
//     uint32_t present = UINT32_MAX;

//     uint32_t i = 0;
//     for (const auto &queueFamily: queueFamilies) {
//       if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphics = i;

//       VkBool32 presentSupport = false;
//       vkGetPhysicalDeviceSurfaceSupportKHR(device, i, sContextData->surface, &presentSupport);
//       if (presentSupport) present = i;

//       if (graphics != UINT32_MAX && present != UINT32_MAX) break;

//       ++i;
//     }

//     if (o_graphics) *o_graphics = graphics;
//     if (o_present)  *o_present  = present;
//     return graphics != UINT32_MAX
//         && present  != UINT32_MAX;
//   }

//   static uint32_t rateDevice(VkPhysicalDevice device, const std::vector<const char *> &extensions) {
//     if (!findQueueFamilies(device, nullptr, nullptr)) return 0;

//     VkPhysicalDeviceProperties deviceProperties;
//     VkPhysicalDeviceFeatures deviceFeatures;
//     vkGetPhysicalDeviceProperties(device, &deviceProperties);
//     vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


//     { // Extension support
//       uint32_t extensionCount;
//       vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

//       std::vector<VkExtensionProperties> availableExtensions(extensionCount);
//       vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

//       std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

//       for (const auto& extension: availableExtensions) requiredExtensions.erase(extension.extensionName);

//       if (!requiredExtensions.empty()) return 0;
//     }

//     uint32_t score = 0;

//     switch (deviceProperties.deviceType) {
//     case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
//       score += 1000;
//       break;
//     case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
//       score += 500;
//       break;
//     case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
//       score += 100;
//       break;
//     case VK_PHYSICAL_DEVICE_TYPE_CPU:
//       score += 50;
//       break;
//     case VK_PHYSICAL_DEVICE_TYPE_OTHER:
//     default: {}
//     }

//     score += deviceProperties.limits.maxImageDimension2D;

//     return score;
//   }

//   static bool createSwapchainObjects() {
//     for (size_t i = 0; i < 2; ++i) {
//       VkImageView attachments[] = {
//         sContextData->swapchainImageViews[i]
//       };

//       VkFramebufferCreateInfo framebufferInfo{};
//       framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//       framebufferInfo.renderPass = sContextData->swapchainRenderPass;
//       framebufferInfo.attachmentCount = 1;
//       framebufferInfo.pAttachments = attachments;
//       framebufferInfo.width = sContextData->swapchainExtent.width;
//       framebufferInfo.height = sContextData->swapchainExtent.height;
//       framebufferInfo.layers = 1;

//       if (vkCreateFramebuffer(sContextData->device, &framebufferInfo, nullptr, &sContextData->swapchainFramebuffers[i]) != VK_SUCCESS) return false;
//     }

//     return true;
//   }

//   static bool cleanupSwapchain() {
//     for (size_t i = 0; i < 2; ++i) {
//       vkDestroyImageView(sContextData->device, sContextData->swapchainImageViews[i], nullptr);
//       vkDestroyFramebuffer(sContextData->device, sContextData->swapchainFramebuffers[i], nullptr);
//     }
//     vkDestroySwapchainKHR(sContextData->device, sContextData->swapchain, nullptr);

//     return true;
//   }

//   static bool recreateSwapchain() {
//     int width = 0, height = 0;
//     glfwGetFramebufferSize(sContextData->window, &width, &height);
//     while (width == 0 || height == 0) {
//       if (glfwWindowShouldClose(sContextData->window)) return false;
//       glfwGetFramebufferSize(sContextData->window, &width, &height);
//       glfwWaitEvents();
//     }

//     vkDeviceWaitIdle(sContextData->device);

//     cleanupSwapchain();

//     return createSwapchain() && createSwapchainObjects();
//   }

//   static bool initData(purrContextInfo info) {
//     assert(sContextData);

//     { // Synchronization objects
//       VkSemaphoreCreateInfo semaphoreInfo{};
//       semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

//       VkFenceCreateInfo fenceInfo{};
//       fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//       fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

//       for (size_t i = 0; i < 2; i++)
//         if (vkCreateSemaphore(sContextData->device, &semaphoreInfo, nullptr, &sContextData->imageSemaphores[i]) != VK_SUCCESS ||
//             vkCreateSemaphore(sContextData->device, &semaphoreInfo, nullptr, &sContextData->renderSemaphores[i]) != VK_SUCCESS ||
//             vkCreateFence(sContextData->device, &fenceInfo, nullptr, &sContextData->fences[i]) != VK_SUCCESS) return false;
//     }

//     if (!createSwapchain()) return false;

//     { // Render pass
//       VkAttachmentDescription colorAttachment{};
//       colorAttachment.format = sContextData->swapchainFormat.format;
//       colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//       colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//       colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//       colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//       colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//       colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//       colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

//       VkAttachmentReference colorAttachmentRef{};
//       colorAttachmentRef.attachment = 0;
//       colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

//       VkSubpassDescription subpass{};
//       subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//       subpass.colorAttachmentCount = 1;
//       subpass.pColorAttachments = &colorAttachmentRef;

//       VkRenderPassCreateInfo renderPassInfo{};
//       renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//       renderPassInfo.attachmentCount = 1;
//       renderPassInfo.pAttachments = &colorAttachment;
//       renderPassInfo.subpassCount = 1;
//       renderPassInfo.pSubpasses = &subpass;

//       if (vkCreateRenderPass(sContextData->device, &renderPassInfo, nullptr, &sContextData->swapchainRenderPass) != VK_SUCCESS) return false;
//     }

//     { // Texture pool
//       VkDescriptorPoolSize poolSize{};
//       poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//       poolSize.descriptorCount = 1024; // TODO: Maybe add an option for max texture count

//       VkDescriptorPoolCreateInfo poolInfo{};
//       poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//       poolInfo.poolSizeCount = 1;
//       poolInfo.pPoolSizes = &poolSize;
//       poolInfo.maxSets = 1024;

//       if (vkCreateDescriptorPool(sContextData->device, &poolInfo, nullptr, &sContextData->texturePool) != VK_SUCCESS) return false;
//     }

//     { // Texture layout
//       VkDescriptorSetLayoutBinding binding{};
//       binding.binding = 0;
//       binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//       binding.descriptorCount = 1;
//       binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//       binding.pImmutableSamplers = nullptr;

//       VkDescriptorSetLayoutCreateInfo layoutInfo{};
//       layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//       layoutInfo.bindingCount = 1;
//       layoutInfo.pBindings = &binding;

//       if (vkCreateDescriptorSetLayout(sContextData->device, &layoutInfo, nullptr, &sContextData->textureLayout) != VK_SUCCESS) return false;
//     }

//     return createSwapchainObjects();
//   }

//   bool purrContext::initialize(purrContextInfo info) {
//     if (sContextData) return true;
//     sContextData = new ContextData();
//     sContextData->resizeCb = info.resizeCb;
//     return initData(info);
//   }

//   void purrContext::cleanup() {
//     assert(sContextData);

//     vkDeviceWaitIdle(sContextData->device);

//     cleanupSwapchain();

//     vkDestroyDescriptorPool(sContextData->device, sContextData->texturePool, VK_NULL_HANDLE);
//     vkDestroyDescriptorSetLayout(sContextData->device, sContextData->textureLayout, VK_NULL_HANDLE);

//     vkDestroyRenderPass(sContextData->device, sContextData->swapchainRenderPass, VK_NULL_HANDLE);

//     vkFreeCommandBuffers(sContextData->device, sContextData->commandPool, 2, sContextData->renderBuffers);
//     vkDestroyCommandPool(sContextData->device, sContextData->commandPool, VK_NULL_HANDLE);

//     for (uint32_t i = 0; i < 2; ++i) {
//       vkDestroyFence(sContextData->device, sContextData->fences[i], VK_NULL_HANDLE);
//       vkDestroySemaphore(sContextData->device, sContextData->imageSemaphores[i], VK_NULL_HANDLE);
//       vkDestroySemaphore(sContextData->device, sContextData->renderSemaphores[i], VK_NULL_HANDLE);
//     }

//     vkDestroyDevice(sContextData->device, VK_NULL_HANDLE);
//     vkDestroySurfaceKHR(sContextData->instance, sContextData->surface, VK_NULL_HANDLE);
//     vkDestroyInstance(sContextData->instance, VK_NULL_HANDLE);
//   }

//   bool purrContext::prepare() {
//     vkWaitForFences(sContextData->device, 1, &sContextData->fences[sContextData->frame], VK_TRUE, UINT64_MAX);

//     VkResult result = vkAcquireNextImageKHR(sContextData->device, sContextData->swapchain, UINT64_MAX, sContextData->imageSemaphores[sContextData->frame], VK_NULL_HANDLE, &sContextData->imageIndex);

//     if (result == VK_ERROR_OUT_OF_DATE_KHR) {
//       recreateSwapchain();
//       if (sContextData->resizeCb) sContextData->resizeCb();
//       return true;
//     } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) return false;

//     vkResetFences(sContextData->device, 1, &sContextData->fences[sContextData->frame]);

//     vkResetCommandBuffer(sContextData->renderBuffers[sContextData->frame], 0);

//     VkCommandBufferBeginInfo beginInfo{};
//     beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

//     return vkBeginCommandBuffer(sContextData->renderBuffers[sContextData->frame], &beginInfo) == VK_SUCCESS;
//   }

//   bool purrContext::present() {
//     if (vkEndCommandBuffer(sContextData->renderBuffers[sContextData->frame]) != VK_SUCCESS) return false;

//     VkSubmitInfo submitInfo{};
//     submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

//     VkSemaphore waitSemaphores[] = {sContextData->imageSemaphores[sContextData->frame]};
//     VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//     submitInfo.waitSemaphoreCount = 1;
//     submitInfo.pWaitSemaphores = waitSemaphores;
//     submitInfo.pWaitDstStageMask = waitStages;

//     submitInfo.commandBufferCount = 1;
//     submitInfo.pCommandBuffers = &sContextData->renderBuffers[sContextData->frame];

//     VkSemaphore signalSemaphores[] = {sContextData->renderSemaphores[sContextData->frame]};
//     submitInfo.signalSemaphoreCount = 1;
//     submitInfo.pSignalSemaphores = signalSemaphores;

//     if (vkQueueSubmit(sContextData->graphicsQueue, 1, &submitInfo, sContextData->fences[sContextData->frame]) != VK_SUCCESS) return false;

//     VkPresentInfoKHR presentInfo{};
//     presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

//     presentInfo.waitSemaphoreCount = 1;
//     presentInfo.pWaitSemaphores = signalSemaphores;

//     VkSwapchainKHR swapchains[] = {sContextData->swapchain};
//     presentInfo.swapchainCount = 1;
//     presentInfo.pSwapchains = swapchains;

//     presentInfo.pImageIndices = &sContextData->imageIndex;

//     VkResult result = vkQueuePresentKHR(sContextData->presentQueue, &presentInfo);

//     if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
//       recreateSwapchain();
//       if (sContextData->resizeCb) sContextData->resizeCb();
//     } else if (result != VK_SUCCESS) return false;

//     sContextData->frame = (sContextData->frame + 1) % 2;

//     return true;
//   }

//   void purrContext::begin(float r, float g, float b, float a) {
//     VkRenderPassBeginInfo beginInfo{};
//     beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//     beginInfo.pNext = VK_NULL_HANDLE;
//     beginInfo.renderPass = sContextData->swapchainRenderPass;
//     beginInfo.framebuffer = sContextData->swapchainFramebuffers[sContextData->imageIndex];
//     beginInfo.renderArea = VkRect2D{VkOffset2D{0,0},sContextData->swapchainExtent};
//     beginInfo.clearValueCount = 1;
//     VkClearValue clearValue = VkClearValue{VkClearColorValue{r,g,b,a}};
//     beginInfo.pClearValues = &clearValue;

//     vkCmdBeginRenderPass(sContextData->renderBuffers[sContextData->frame], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

//     VkViewport viewport{
//       0.0f,0.0f,
//       static_cast<float>(sContextData->swapchainExtent.width), static_cast<float>(sContextData->swapchainExtent.height),
//       0.0f, 1.0f
//     };

//     VkRect2D scissor{
//       VkOffset2D{0,0},
//       sContextData->swapchainExtent
//     };
//     vkCmdSetViewport(sContextData->renderBuffers[sContextData->frame], 0, 1, &viewport);
//     vkCmdSetScissor(sContextData->renderBuffers[sContextData->frame], 0, 1, &scissor);
//   }

//   void purrContext::bind(purrPipeline *pipeline) {
//     vkCmdBindPipeline(sContextData->renderBuffers[sContextData->frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get());
//   }

//   void purrContext::bind(purrPipeline *pipeline, uint32_t slot, purrTexture *texture) {
//     VkDescriptorSet dscSet = texture->getSet();
//     vkCmdBindDescriptorSets(sContextData->renderBuffers[sContextData->frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), slot, 1, &dscSet, 0, nullptr);
//   }

//   void purrContext::draw(purrMesh *mesh) {
//     VkBuffer vertexBuffer = mesh->getVertexBuffer();
//     VkDeviceSize offset = 0;
//     vkCmdBindVertexBuffers(sContextData->renderBuffers[sContextData->frame], 0, 1, &vertexBuffer, &offset);
//     vkCmdBindIndexBuffer(sContextData->renderBuffers[sContextData->frame], mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
//     vkCmdDrawIndexed(sContextData->renderBuffers[sContextData->frame], static_cast<uint32_t>(mesh->indexCount()), 1, 0, 0, 0);
//   }

//   void purrContext::end() {
//     vkCmdEndRenderPass(sContextData->renderBuffers[sContextData->frame]);
//   }

//   void purrContext::wait() {
//     vkDeviceWaitIdle(sContextData->device);
//   }

// }