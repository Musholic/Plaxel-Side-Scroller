//
// Created by musholic on 11/3/23.
//

#ifndef PLAXEL_RENDERER_H
#define PLAXEL_RENDERER_H

#include "base_renderer.h"
namespace plaxel {

struct UniformBufferObject {
  float deltaTime = 1.0f;
};

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

  std::array<vk::raii::Buffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers{nullptr, nullptr};
  std::array<vk::raii::DeviceMemory, MAX_FRAMES_IN_FLIGHT> uniformBuffersMemory{nullptr, nullptr};
  std::array<void *, MAX_FRAMES_IN_FLIGHT> uniformBuffersMapped{};

  vk::raii::CommandBuffers computeCommandBuffers = nullptr;

  vk::raii::DescriptorSets computeDescriptorSets = nullptr;

  vk::raii::Buffer shaderStorageBuffer = nullptr;
  vk::raii::DeviceMemory shaderStorageBufferMemory = nullptr;
  void createUniformBuffers();
  void createComputeDescriptorSets();
  void updateUniformBuffer(uint32_t currentImage) override;
  void recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) override;
  void drawCommand(vk::CommandBuffer commandBuffer) const override;
  [[nodiscard]] vk::VertexInputBindingDescription getVertexBindingDescription() const override;
  [[nodiscard]] std::array<vk::VertexInputAttributeDescription, 2>
  getVertexAttributeDescription() const override;
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
