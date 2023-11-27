#include "Buffer.h"
// TODO: Remove dependency with base renderer
#include "base_renderer.h"

namespace plaxel {

Buffer::Buffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
               vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
    : buffer(initBuffer(device, size, usage)),
      bufferMemory(initBufferMemory(device, physicalDevice, properties)), bufferSize(size) {

  buffer.bindMemory(*bufferMemory, 0);
}

vk::raii::Buffer Buffer::initBuffer(const vk::raii::Device &device, unsigned long size,
                                    const vk::BufferUsageFlags &usage) {
  vk::BufferCreateInfo bufferInfo;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  return {device, bufferInfo};
}
vk::raii::DeviceMemory Buffer::initBufferMemory(const vk::raii::Device &device,
                                                const vk::raii::PhysicalDevice &physicalDevice,
                                                const vk::MemoryPropertyFlags &properties) {
  vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

  return {device, allocInfo};
}

vk::Buffer Buffer::getBuffer() const { return *buffer; }

uint32_t Buffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties,
                                const vk::raii::PhysicalDevice &physicalDevice) {
  vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw VulkanInitializationError("failed to find suitable memory type!");
}

vk::WriteDescriptorSet &Buffer::getDescriptorWriteForCompute(vk::DescriptorSet computeDescriptorSet,
                                                             int dstBinding) {
  storageBufferInfoCurrentFrame.buffer = *buffer;
  storageBufferInfoCurrentFrame.offset = 0;
  storageBufferInfoCurrentFrame.range = bufferSize;

  descriptorWrite.dstSet = computeDescriptorSet;
  descriptorWrite.dstBinding = dstBinding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &storageBufferInfoCurrentFrame;

  return descriptorWrite;
}
} // namespace plaxel