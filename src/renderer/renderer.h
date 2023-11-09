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
  alignas(16) glm::vec3 pos;
  alignas(16) glm::vec2 texCoord;

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
  void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
  [[nodiscard]] vk::PipelineLayoutCreateInfo getPipelineLayoutInfo() const override;
  [[nodiscard]] vk::PipelineLayoutCreateInfo getComputePipelineLayoutInfo() const override;
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
