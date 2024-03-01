#ifndef PLAXEL_BUFFER_H
#define PLAXEL_BUFFER_H

#include "vulkan_dispatch_loader.h"

namespace plaxel {

class BufferInitializationError final : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

class Buffer {
public:
  Buffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
         vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vk::MemoryPropertyFlags properties);
  [[nodiscard]] vk::Buffer getBuffer() const;
  vk::WriteDescriptorSet &getDescriptorWriteForCompute(vk::DescriptorSet computeDescriptorSet,
                                                       int dstBinding);
  void copyToMemory(const void *src);

  [[nodiscard]] static uint32_t findMemoryType(uint32_t typeFilter,
                                               vk::MemoryPropertyFlags properties,
                                               const vk::raii::PhysicalDevice &physicalDevice);

private:
  vk::BufferUsageFlags usage;
  vk::raii::Buffer buffer;
  vk::raii::DeviceMemory bufferMemory;
  vk::DeviceSize bufferSize;
  void *mappedMemory = nullptr;

  vk::WriteDescriptorSet descriptorWrite{};
  vk::DescriptorBufferInfo storageBufferInfoCurrentFrame{};

  vk::raii::Buffer initBuffer(const vk::raii::Device &device, unsigned long size) const;
  [[nodiscard]] vk::raii::DeviceMemory
  initBufferMemory(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
                   const vk::MemoryPropertyFlags &properties) const;

public:
  template <typename T> std::vector<T> getVectorData() {
    if (!mappedMemory) {
      mappedMemory = bufferMemory.mapMemory(0, bufferSize);
    }
    const size_t size = bufferSize / sizeof(T);
    std::vector<T> mem;
    mem.reserve(size);
    memcpy(mem.data(), mappedMemory, bufferSize);
    return mem;
  }

  template <typename T> T getData() {
    if (!mappedMemory) {
      mappedMemory = bufferMemory.mapMemory(0, bufferSize);
    }
    T mem;
    memcpy(&mem, mappedMemory, bufferSize);
    return mem;
  }
};

} // namespace plaxel

#endif // PLAXEL_BUFFER_H
