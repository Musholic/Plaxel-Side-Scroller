//
// Created by musholic on 11/3/23.
//

#include "renderer.h"
#include <glm/geometric.hpp>
#include <random>

namespace plaxel {
void Renderer::initVulkan() {
  BaseRenderer::initVulkan();
  createShaderStorageBuffers();
  createUniformBuffers();
  createComputeDescriptorSets();
}

void Renderer::createShaderStorageBuffers() {
  // Initialize particles
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

  // Initial particle positions on a circle
  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto &particle : particles) {
    float r = 0.25f * sqrt(rndDist(rndEngine));
    float theta = rndDist(rndEngine) * 2.0f * std::numbers::pi_v<float>;
    float x = r * cos(theta) * windowSize.height / windowSize.width;
    float y = r * sin(theta);
    particle.position = glm::vec2(x, y);
    particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
  }

  vk::DeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

  // Create a staging buffer used to upload data to the gpu
  vk::raii::Buffer stagingBuffer = nullptr;
  vk::raii::DeviceMemory stagingBufferMemory = nullptr;
  createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
               stagingBuffer, stagingBufferMemory);

  void *data = stagingBufferMemory.mapMemory(0, bufferSize);
  memcpy(data, particles.data(), bufferSize);
  stagingBufferMemory.unmapMemory();

  createBuffer(bufferSize,
               vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
                   vk::BufferUsageFlagBits::eTransferDst,
               vk::MemoryPropertyFlagBits::eDeviceLocal, shaderStorageBuffer,
               shaderStorageBufferMemory);
  copyBuffer(*stagingBuffer, *shaderStorageBuffer, bufferSize);
}

void Renderer::createUniformBuffers() {
  vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 uniformBuffers[i], uniformBuffersMemory[i]);
    uniformBuffersMapped[i] = uniformBuffersMemory[i].mapMemory(0, bufferSize);
  }
}

void Renderer::createComputeDescriptorSets() {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *computeDescriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.descriptorPool = *descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  computeDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo uniformBufferInfo;
    uniformBufferInfo.buffer = *uniformBuffers[i];
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(UniformBufferObject);

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
    descriptorWrites[0].dstSet = *computeDescriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

    vk::DescriptorBufferInfo storageBufferInfoCurrentFrame;
    storageBufferInfoCurrentFrame.buffer = *shaderStorageBuffer;
    storageBufferInfoCurrentFrame.offset = 0;
    storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    descriptorWrites[1].dstSet = *computeDescriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageBuffer;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &storageBufferInfoCurrentFrame;

    device.updateDescriptorSets(descriptorWrites, nullptr);
  }
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
  UniformBufferObject ubo;
  ubo.deltaTime = lastFrameTime * 2.0f;

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) {
  vk::CommandBufferBeginInfo beginInfo;

  commandBuffer.begin(beginInfo);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0,
                                   *computeDescriptorSets[currentFrame], nullptr);
  commandBuffer.dispatch(PARTICLE_COUNT / 256, 1, 1);

  commandBuffer.end();
}

void Renderer::drawCommand(vk::CommandBuffer commandBuffer) const {
  std::vector<vk::DeviceSize> offsets = {0};
  commandBuffer.bindVertexBuffers(0, *shaderStorageBuffer, offsets);

  commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);
}

std::array<vk::VertexInputAttributeDescription, 2> Renderer::getVertexAttributeDescription() const {
  return Particle::getAttributeDescriptions();
}

vk::VertexInputBindingDescription Renderer::getVertexBindingDescription() const {
  return Particle::getBindingDescription();
}

} // namespace plaxel