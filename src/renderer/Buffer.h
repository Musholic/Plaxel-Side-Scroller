#ifndef PLAXEL_BUFFER_H
#define PLAXEL_BUFFER_H

#include <vulkan/vulkan_raii.hpp>
namespace plaxel {

class BufferInitializationError final : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

class Buffer {
public:
  Buffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
         vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
  [[nodiscard]] vk::Buffer getBuffer() const;
  vk::WriteDescriptorSet &getDescriptorWriteForCompute(vk::DescriptorSet computeDescriptorSet,
                                                       int dstBinding);
  void copyToMemory(const void *src);

  [[nodiscard]] static uint32_t findMemoryType(uint32_t typeFilter,
                                               vk::MemoryPropertyFlags properties,
                                               const vk::raii::PhysicalDevice &physicalDevice);

private:
  vk::raii::Buffer buffer;
  vk::raii::DeviceMemory bufferMemory;
  vk::DeviceSize bufferSize;
  void *mappedMemory = nullptr;

  vk::WriteDescriptorSet descriptorWrite{};
  vk::DescriptorBufferInfo storageBufferInfoCurrentFrame{};

  static vk::raii::Buffer initBuffer(const vk::raii::Device &device, unsigned long size,
                                     const vk::BufferUsageFlags &usage);
  [[nodiscard]] vk::raii::DeviceMemory
  initBufferMemory(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
                   const vk::MemoryPropertyFlags &properties) const;

public:
  template <typename T> std::vector<T> getData() {
    if (!mappedMemory) {
      mappedMemory = bufferMemory.mapMemory(0, bufferSize);
    }
    const size_t size = bufferSize / sizeof(T);
    T mem[size];
    memcpy(mem, mappedMemory, bufferSize);
    std::vector<T> result;
    for (int i = 0; i < size; ++i) {
      result.push_back(mem[i]);
    }
    return result;
  }
};

} // namespace plaxel

#endif // PLAXEL_BUFFER_H
