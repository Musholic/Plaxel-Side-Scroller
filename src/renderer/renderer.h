//
// Created by musholic on 11/3/23.
//

#ifndef PLAXEL_RENDERER_H
#define PLAXEL_RENDERER_H

#include "base_renderer.h"

#include <glm/detail/type_mat4x4.hpp>
#include <glm/fwd.hpp>
namespace plaxel {

struct Vertex {
  // TODO: check alignment issues?
  glm::vec3 pos;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription getBindingDescription() {
    vk::VertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);

    return bindingDescription;
  }

  static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.emplace_back(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
    attributeDescriptions.emplace_back(1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord));

    return attributeDescriptions;
  }
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},  {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},   {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}}};

const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

class Renderer : public BaseRenderer {
private:
  void initVulkan() override;
  void initCustomDescriptorSetLayout() override;
  vk::raii::CommandBuffers computeCommandBuffers = nullptr;

  vk::raii::Image textureImage = nullptr;
  vk::raii::DeviceMemory textureImageMemory = nullptr;
  vk::raii::ImageView textureImageView = nullptr;
  vk::raii::Sampler textureSampler = nullptr;

  vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr;
  vk::raii::DescriptorSets computeDescriptorSets = nullptr;

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorSets descriptorSets = nullptr;

  vk::raii::Buffer vertexBuffer = nullptr;
  vk::raii::DeviceMemory vertexBufferMemory = nullptr;
  vk::raii::Buffer indexBuffer = nullptr;
  vk::raii::DeviceMemory indexBufferMemory = nullptr;

  void createComputeDescriptorSetLayout();
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
  [[nodiscard]] vk::PipelineLayoutCreateInfo getComputePipelineLayoutInfo() const override;
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
