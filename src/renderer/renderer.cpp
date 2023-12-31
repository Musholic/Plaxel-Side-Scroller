#include "renderer.h"
#include "file_utils.h"
#include <cmrc/cmrc.hpp>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace plaxel {

void Renderer::initVulkan() {
  BaseRenderer::initVulkan();

  createComputeBuffers();

  createTextureImage();
  createTextureImageView();
  createTextureSampler();

  createComputeDescriptorPool();
  createDescriptorSets();
  createComputeDescriptorSets();
}

void Renderer::initCustomDescriptorSetLayout() {
  createDescriptorSetLayout();
  createComputeDescriptorSetLayout();
}

void Renderer::createComputeDescriptorSetLayout() {
  std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
  for (int i = 0; i < NB_COMPUTE_BUFFERS; ++i) {
    layoutBindings.emplace_back(i, vk::DescriptorType::eStorageBuffer, 1,
                                vk::ShaderStageFlagBits::eCompute);
  }

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  layoutInfo.pBindings = layoutBindings.data();

  computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void Renderer::createComputeDescriptorSets() {
  const std::vector layouts(1, *computeDescriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.descriptorPool = *computeDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = layouts.data();

  auto computeDescriptorSets = vk::raii::DescriptorSets(device, allocInfo);
  computeDescriptorSet = std::move(computeDescriptorSets[0]);

  std::vector<vk::WriteDescriptorSet> descriptorWrites;
  descriptorWrites.push_back(vertexBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 0));
  descriptorWrites.push_back(indexBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 1));
  descriptorWrites.push_back(
      drawCommandBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 2));
  descriptorWrites.push_back(
      testDataBuffer->getDescriptorWriteForCompute(*computeDescriptorSet, 3));

  device.updateDescriptorSets(descriptorWrites, nullptr);
}

void Renderer::recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) {
  constexpr vk::CommandBufferBeginInfo beginInfo;

  commandBuffer.begin(beginInfo);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0,
                                   *computeDescriptorSet, nullptr);
  commandBuffer.dispatch(1, 1, 1);

  commandBuffer.end();
}

void Renderer::drawCommand(vk::CommandBuffer commandBuffer) const {
  const std::vector<vk::DeviceSize> offsets = {0};
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

void Renderer::createComputeBuffers() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;

  vertexBuffer.emplace(device, physicalDevice, sizeof(Vertex) * MAX_VERTEX_COUNT,
                       eStorageBuffer | eVertexBuffer, eDeviceLocal);

  indexBuffer.emplace(device, physicalDevice, sizeof(uint32_t) * MAX_INDEX_COUNT,
                      eStorageBuffer | eIndexBuffer, eDeviceLocal);

  drawCommandBuffer.emplace(device, physicalDevice, sizeof(VkDrawIndexedIndirectCommand),
                            eStorageBuffer | eIndirectBuffer, eDeviceLocal);
  constexpr TestData src = {0};
  testDataBuffer = createBufferWithInitialData(eStorageBuffer, &src, sizeof(src));
}

Buffer Renderer::createBufferWithInitialData(const vk::BufferUsageFlags usage, const void *src,
                                             // ReSharper disable once CppDFAConstantParameter
                                             const vk::DeviceSize size) const {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  Buffer stagingBuffer(device, physicalDevice, size, eTransferSrc, eHostVisible | eHostCoherent);
  stagingBuffer.copyToMemory(src);
  Buffer buffer(device, physicalDevice, size, usage | eTransferDst, eDeviceLocal);
  copyBuffer(stagingBuffer.getBuffer(), buffer.getBuffer(), size);
  return buffer;
}

void Renderer::createTextureImage() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  int texWidth;
  int texHeight;
  int texChannels;
  const auto texture = files::readFile("textures/texture.jpg");
  stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(texture.begin()),
                                          static_cast<int>(texture.size()), &texWidth, &texHeight,
                                          &texChannels, STBI_rgb_alpha);
  const vk::DeviceSize imageSize = texWidth * texHeight * 4;

  if (!pixels) {
    throw VulkanInitializationError("failed to load texture image!");
  }

  Buffer stagingBuffer(device, physicalDevice, imageSize, eTransferSrc,
                       eHostVisible | eHostCoherent);

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
  const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

  vk::SamplerCreateInfo samplerInfo;
  samplerInfo.magFilter = vk::Filter::eLinear;
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
  if (const auto deviceName = std::string(properties.deviceName.data());
      deviceName.starts_with("llvmpipe")) {
    // anisotropy filtering can be very slow with lavapipe in some cases, we need to disable it
    samplerInfo.anisotropyEnable = vk::False;
  } else {
    samplerInfo.anisotropyEnable = vk::True;
  }
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

  const std::array bindings = {uboLayoutBinding, samplerLayoutBinding};
  vk::DescriptorSetLayoutCreateInfo layoutInfo;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void Renderer::createComputeDescriptorPool() {
  std::array<vk::DescriptorPoolSize, 1> poolSizes;
  poolSizes[0].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[0].descriptorCount = NB_COMPUTE_BUFFERS;

  vk::DescriptorPoolCreateInfo poolInfo;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

  computeDescriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void Renderer::createDescriptorSets() {
  const std::vector layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
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

void Renderer::copyBufferToImage(const vk::Buffer buffer, const vk::Image image,
                                 const uint32_t width, const uint32_t height) const {
  const vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

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