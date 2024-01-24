#include "Buffer.h"

namespace plaxel {

Buffer::Buffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
               const vk::DeviceSize size, const vk::BufferUsageFlags usage,
               const vk::MemoryPropertyFlags properties)
    : usage(usage), buffer(initBuffer(device, size)),
      bufferMemory(initBufferMemory(device, physicalDevice, properties)), bufferSize(size) {

  buffer.bindMemory(*bufferMemory, 0);
}

vk::raii::Buffer Buffer::initBuffer(const vk::raii::Device &device, unsigned long size) const {
  vk::BufferCreateInfo bufferInfo;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  return {device, bufferInfo};
}
vk::raii::DeviceMemory Buffer::initBufferMemory(const vk::raii::Device &device,
                                                const vk::raii::PhysicalDevice &physicalDevice,
                                                const vk::MemoryPropertyFlags &properties) const {
  const vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

  return {device, allocInfo};
}

vk::Buffer Buffer::getBuffer() const { return *buffer; }

uint32_t Buffer::findMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags properties,
                                const vk::raii::PhysicalDevice &physicalDevice) {
  const vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw BufferInitializationError("failed to find suitable memory type!");
}

vk::WriteDescriptorSet &Buffer::getDescriptorWriteForCompute(vk::DescriptorSet computeDescriptorSet,
                                                             int dstBinding) {
  storageBufferInfoCurrentFrame.buffer = *buffer;
  storageBufferInfoCurrentFrame.offset = 0;
  storageBufferInfoCurrentFrame.range = bufferSize;

  descriptorWrite.dstSet = computeDescriptorSet;
  descriptorWrite.dstBinding = dstBinding;
  descriptorWrite.dstArrayElement = 0;
  if (usage & vk::BufferUsageFlagBits::eStorageBuffer) {
    descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
  } else {
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  }
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &storageBufferInfoCurrentFrame;

  return descriptorWrite;
}

void Buffer::copyToMemory(const void *src) {
  if (!mappedMemory) {
    mappedMemory = bufferMemory.mapMemory(0, bufferSize);
  }
  memcpy(mappedMemory, src, bufferSize);
}
} // namespace plaxel