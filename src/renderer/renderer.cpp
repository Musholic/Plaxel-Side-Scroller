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

struct Vertex {
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

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

void Renderer::initVulkan() {
  BaseRenderer::initVulkan();
  createShaderStorageBuffers();

  createVertexBuffer();
  createIndexBuffer();
  createTextureImage();
  createTextureImageView();
  createTextureSampler();

  createDescriptorSets();
  createComputeDescriptorSets();
}

void Renderer::initCustomDescriptorSetLayout() { createDescriptorSetLayout(); }

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
  commandBuffer.bindVertexBuffers(0, *vertexBuffer, offsets);
  commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0,
                                   *descriptorSets[currentFrame], nullptr);

  commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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
  vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  vk::raii::Buffer stagingBuffer = nullptr;
  vk::raii::DeviceMemory stagingBufferMemory = nullptr;
  createBuffer(bufferSize, eTransferSrc, eHostVisible | eHostCoherent, stagingBuffer,
               stagingBufferMemory);

  void *data = stagingBufferMemory.mapMemory(0, bufferSize);
  memcpy(data, vertices.data(), bufferSize);
  stagingBufferMemory.unmapMemory();

  createBuffer(bufferSize, eTransferDst | eVertexBuffer, eDeviceLocal, vertexBuffer,
               vertexBufferMemory);

  copyBuffer(*stagingBuffer, *vertexBuffer, bufferSize);
}

void Renderer::createIndexBuffer() {
  using enum vk::MemoryPropertyFlagBits;
  using enum vk::BufferUsageFlagBits;
  vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  vk::raii::Buffer stagingBuffer = nullptr;
  vk::raii::DeviceMemory stagingBufferMemory = nullptr;
  createBuffer(bufferSize, eTransferSrc, eHostVisible | eHostCoherent, stagingBuffer,
               stagingBufferMemory);

  void *data = stagingBufferMemory.mapMemory(0, bufferSize);
  memcpy(data, indices.data(), bufferSize);
  stagingBufferMemory.unmapMemory();

  createBuffer(bufferSize, eTransferDst | eIndexBuffer, eDeviceLocal, indexBuffer,
               indexBufferMemory);

  copyBuffer(*stagingBuffer, *indexBuffer, bufferSize);
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

  vk::raii::Buffer stagingBuffer = nullptr;
  vk::raii::DeviceMemory stagingBufferMemory = nullptr;
  createBuffer(imageSize, eTransferSrc, eHostVisible | eHostCoherent, stagingBuffer,
               stagingBufferMemory);

  void *data = stagingBufferMemory.mapMemory(0, imageSize);
  memcpy(data, pixels, imageSize);
  stagingBufferMemory.unmapMemory();

  stbi_image_free(pixels);

  createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, eDeviceLocal,
              textureImage, textureImageMemory);

  transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);
  copyBufferToImage(*stagingBuffer, *textureImage, static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));
  transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb,
                        vk::ImageLayout::eTransferDstOptimal,
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
  samplerInfo.anisotropyEnable = vk::True;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = vk::False;
  samplerInfo.compareEnable = vk::False;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

  textureSampler = vk::raii::Sampler(device, samplerInfo);
}

vk::raii::ImageView Renderer::createImageView(vk::Image image, vk::Format format,
                                              vk::ImageAspectFlags aspectFlags) {
  vk::ImageViewCreateInfo viewInfo;
  viewInfo.image = image;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  return {device, viewInfo};
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
    bufferInfo.buffer = *uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = *textureImageView;
    imageInfo.sampler = *textureSampler;

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites;

    descriptorWrites[0].dstSet = *descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].dstSet = *descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    device.updateDescriptorSets(descriptorWrites, nullptr);
  }
}

void Renderer::createImage(uint32_t width, uint32_t height, vk::Format format,
                           vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                           vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                           vk::raii::DeviceMemory &imageMemory) {
  vk::ImageCreateInfo imageInfo;
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.usage = usage;

  image = vk::raii::Image(device, imageInfo);

  vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  imageMemory = vk::raii::DeviceMemory(device, allocInfo);
  image.bindMemory(*imageMemory, 0);
}

void Renderer::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout) {
  vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

  vk::ImageMemoryBarrier barrier;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vk::PipelineStageFlags sourceStage;
  vk::PipelineStageFlags destinationStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }
  commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlagBits::eByRegion,
                                nullptr, nullptr, barrier);

  endSingleTimeCommands(*commandBuffer);
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

vk::raii::CommandBuffer Renderer::beginSingleTimeCommands() {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = 1;

  vk::raii::CommandBuffers commandBuffers{device, allocInfo};

  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  vk::raii::CommandBuffer commandBuffer(std::move(commandBuffers[0]));
  commandBuffer.begin(beginInfo);

  return commandBuffer;
}

void Renderer::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
  commandBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  graphicsQueue.submit(submitInfo);
  graphicsQueue.waitIdle();
}

vk::PipelineLayoutCreateInfo Renderer::getPipelineLayoutInfo() const {
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &*descriptorSetLayout;
  return pipelineLayoutInfo;
}

} // namespace plaxel