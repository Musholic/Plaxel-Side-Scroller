//
// Created by musholic on 11/3/23.
//

#ifndef PLAXEL_RENDERER_H
#define PLAXEL_RENDERER_H

#include "base_renderer.h"

#include <glm/detail/type_mat4x4.hpp>
#include <glm/fwd.hpp>
namespace plaxel {


struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);

    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[1].offset = offsetof(Particle, color);

    return attributeDescriptions;
  }
};

class Renderer : public BaseRenderer {
private:
  void createShaderStorageBuffers();

  void initVulkan() override;
  void initCustomDescriptorSetLayout() override;
  vk::raii::CommandBuffers computeCommandBuffers = nullptr;

  vk::raii::DescriptorSets computeDescriptorSets = nullptr;

  vk::raii::Buffer shaderStorageBuffer = nullptr;
  vk::raii::DeviceMemory shaderStorageBufferMemory = nullptr;

  vk::raii::Image textureImage = nullptr;
  vk::raii::DeviceMemory textureImageMemory = nullptr;
  vk::raii::ImageView textureImageView = nullptr;
  vk::raii::Sampler textureSampler = nullptr;

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorSets descriptorSets = nullptr;

  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

  void createComputeDescriptorSets();
  void recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) override;
  void drawCommand(vk::CommandBuffer commandBuffer) const override;
  [[nodiscard]] vk::VertexInputBindingDescription getVertexBindingDescription() const override;
  [[nodiscard]] std::vector<vk::VertexInputAttributeDescription>
  getVertexAttributeDescription() const override;
  void createVertexBuffer();
  void createIndexBuffer();
  void createDescriptorSetLayout();
  void createDescriptorSets();
  void createTextureImageView();
  void createTextureSampler();
  void createTextureImage();
  void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                   vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);
  vk::raii::CommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(vk::CommandBuffer commandBuffer);
  void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout);
  void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
  vk::raii::ImageView createImageView(vk::Image image, vk::Format format,
                                      vk::ImageAspectFlags aspectFlags);
  [[nodiscard]] vk::PipelineLayoutCreateInfo getPipelineLayoutInfo() const override;
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
