//
// Created by musholic on 11/3/23.
//

#include "renderer.h"
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace plaxel {

void Renderer::initVulkan() {
  BaseRenderer::initVulkan();

  createVertexBuffer();
  createIndexBuffer();
  createDrawCommandBuffer();
  createTextureImage();
  createTextureImageView();
  createTextureSampler();

  createDescriptorSets();
  createComputeDescriptorSets();
}

void Renderer::initCustomDescriptorSetLayout() {
  createDescriptorSetLayout();
  createComputeDescriptorSetLayout();
}

void Renderer::createComputeDescriptorSetLayout() {
  std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
  layoutBindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1,
                              vk::ShaderStageFlagBits::eCompute);
  layoutBindings.emplace_back(1, vk::DescriptorType::eStorageBuffer, 1,
                              vk::ShaderStageFlagBits::eCompute);
  layoutBindings.emplace_back(2, vk::DescriptorType::eStorageBuffer, 1,
                              vk::ShaderStageFlagBits::eCompute);

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  layoutInfo.pBindings = layoutBindings.data();

  computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void Renderer::createComputeDescriptorSets() {
  std::vector<vk::DescriptorSetLayout> layouts(1, *computeDescriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.descriptorPool = *descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = layouts.data();

  auto computeDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
  computeDescriptorSet = std::move(computeDescriptorSets[0]);

  std::vector<vk::WriteDescriptorSet> descriptorWrites;
  descriptorWrites.push_back(vertexBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 0));
  descriptorWrites.push_back(indexBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 1));
  descriptorWrites.push_back(
      drawCommandBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 2));

  device.updateDescriptorSets(descriptorWrites, nullptr);
}

void Renderer::recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) {
  vk::CommandBufferBeginInfo beginInfo;

  commandBuffer.begin(beginInfo);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0,
                                   *computeDescriptorSet, nullptr);
  commandBuffer.dispatch(1, 1, 1);

  commandBuffer.end();
}

void Renderer::drawCommand(vk::CommandBuffer commandBuffer) const {
  std::vector<vk::DeviceSize> offsets = {0};
  commandBuffer.bindVertexBuffers(0, vertexBuffer->getBuffer(), offsets);
  commandBuffer.bindIndexBuffer(indexBuffer->getBuffer(), 0, vk::IndexType::eUint32);

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0,
                                   *descriptorSets[currentFrame], nullptr);

  commandBuffer.drawIndexedIndirect(drawCommandBuffer->getBuffer(), 0, 1, 0);
}

std::vector<vk::VertexInputAttributeDescription> Renderer::getVertexAttributeDescription() const {
  return Vertex::getAttributeDescriptions();
}

vk::VertexInputBindingDescription Renderer::getVertexBindingDescription() const {
  return Vertex::getBindingDescription();
}

void Renderer::createVertexBuffer() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  vk::DeviceSize bufferSize = sizeof(Vertex) * MAX_VERTEX_COUNT;

  vertexBuffer.emplace(device, physicalDevice, bufferSize, eStorageBuffer | eVertexBuffer,
                       eDeviceLocal);
}

void Renderer::createIndexBuffer() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  vk::DeviceSize bufferSize = sizeof(uint32_t) * MAX_INDEX_COUNT;

  indexBuffer.emplace(device, physicalDevice, bufferSize, eStorageBuffer | eIndexBuffer,
                      eDeviceLocal);
}

void Renderer::createDrawCommandBuffer() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  vk::DeviceSize bufferSize = sizeof(VkDrawIndexedIndirectCommand);

  drawCommandBuffer.emplace(device, physicalDevice, bufferSize, eStorageBuffer | eIndirectBuffer,
                            eDeviceLocal);
}

void Renderer::createTextureImage() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  int texWidth;
  int texHeight;
  int texChannels;
  stbi_uc *pixels =
      stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  vk::DeviceSize imageSize = texWidth * texHeight * 4;

  if (!pixels) {
    throw VulkanInitializationError("failed to load texture image!");
  }

  Buffer stagingBuffer(device, physicalDevice, imageSize, eTransferSrc,
                       eHostVisible | eHostCoherent);
  vk::raii::DeviceMemory stagingBufferMemory = nullptr;

  stagingBuffer.copyToMemory(pixels);

  stbi_image_free(pixels);

  createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, eDeviceLocal,
              textureImage, textureImageMemory);

  transitionImageLayout(*textureImage, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);
  copyBufferToImage(stagingBuffer.getBuffer(), *textureImage, static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));
  transitionImageLayout(*textureImage, vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Renderer::createTextureImageView() {
  textureImageView =
      createImageView(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

void Renderer::createTextureSampler() {
  vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

  vk::SamplerCreateInfo samplerInfo;
  samplerInfo.magFilter = vk::Filter::eLinear;
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
  // TODO:anisotropy filtering can be very slow with lavapipe in some cases, we need to disable it
  // automatically when using lavapipe
  samplerInfo.anisotropyEnable = vk::False;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = vk::False;
  samplerInfo.compareEnable = vk::False;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

  textureSampler = vk::raii::Sampler(device, samplerInfo);
}

void Renderer::createDescriptorSetLayout() {
  vk::DescriptorSetLayoutBinding uboLayoutBinding;
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

  vk::DescriptorSetLayoutBinding samplerLayoutBinding;
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
  vk::DescriptorSetLayoutCreateInfo layoutInfo;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void Renderer::createDescriptorSets() {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.descriptorPool = *descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets = vk::raii::DescriptorSets(device, allocInfo);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = uniformBuffers[i].getBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = *textureImageView;
    imageInfo.sampler = *textureSampler;

    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    vk::WriteDescriptorSet &descriptorWrite = descriptorWrites.emplace_back();
    descriptorWrite.dstSet = *descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vk::WriteDescriptorSet &descriptorWrite2 = descriptorWrites.emplace_back();
    descriptorWrite2.dstSet = *descriptorSets[i];
    descriptorWrite2.dstBinding = 1;
    descriptorWrite2.dstArrayElement = 0;
    descriptorWrite2.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite2.descriptorCount = 1;
    descriptorWrite2.pImageInfo = &imageInfo;

    device.updateDescriptorSets(descriptorWrites, nullptr);
  }
}

void Renderer::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width,
                                 uint32_t height) {
  vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

  vk::BufferImageCopy region;
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = vk::Offset3D{0, 0, 0};
  region.imageExtent = vk::Extent3D{width, height, 1};

  commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

  endSingleTimeCommands(*commandBuffer);
}

vk::PipelineLayoutCreateInfo Renderer::getPipelineLayoutInfo() const {
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &*descriptorSetLayout;
  return pipelineLayoutInfo;
}

vk::PipelineLayoutCreateInfo Renderer::getComputePipelineLayoutInfo() const {
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &*computeDescriptorSetLayout;
  return pipelineLayoutInfo;
}

} // namespace plaxel