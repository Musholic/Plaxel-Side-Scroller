//
// Created by musholic on 11/3/23.
//

#ifndef PLAXEL_RENDERER_H
#define PLAXEL_RENDERER_H

#include "Buffer.h"
#include "base_renderer.h"

#include <glm/detail/type_mat4x4.hpp>
#include <glm/fwd.hpp>
namespace plaxel {

constexpr int BLOCK_W = 8;
constexpr int NB_BLOCKS = BLOCK_W * BLOCK_W * BLOCK_W;
constexpr int MAX_NODES = 100;
constexpr int MAX_LEAVES = MAX_NODES;
constexpr int MAX_DEBUG_VALUES = 1000;

struct VoxelTreeNode {
  uint32_t height;
  uint32_t children[4];
  uint32_t leaf;
  int32_t x;
  int32_t y;
};

struct VoxelTreeLeaf {
  uint32_t blocks[NB_BLOCKS];
};

struct VoxelTreeInfo {
  uint32_t rootNodeIndex;
  uint32_t lastNodeIndex;
  uint32_t lastLeafIndex;
};

struct AddedBlock {
  int32_t x, y, z;
};

struct DebugValue {
  int32_t key;
  int32_t value;
};

struct DebugValues {
  uint32_t lastIndex;
  DebugValue values[MAX_DEBUG_VALUES];
};

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
  [[nodiscard]] static std::string toString(float x);
  [[nodiscard]] std::string toString() const;
};

class Renderer : public BaseRenderer {
public:
  Renderer();

private:
  static int getTargetFps();

protected:
  void initVulkan() override;
  void initCustomDescriptorSetLayout() override;
  vk::raii::CommandBuffers computeCommandBuffers = nullptr;

  vk::raii::Image textureImage = nullptr;
  vk::raii::DeviceMemory textureImageMemory = nullptr;
  vk::raii::ImageView textureImageView = nullptr;
  vk::raii::Sampler textureSampler = nullptr;

  vk::raii::DescriptorPool computeDescriptorPool = nullptr;
  vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr;
  vk::raii::DescriptorSet computeDescriptorSet = nullptr;

  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorSets descriptorSets = nullptr;

  std::optional<Buffer> vertexBuffer;
  std::optional<Buffer> indexBuffer;
  std::optional<Buffer> drawCommandBuffer;
  std::optional<Buffer> voxelTreeNodesBuffer;
  std::optional<Buffer> voxelTreeLeavesBuffer;
  std::optional<Buffer> voxelTreeInfoBuffer;
  std::optional<Buffer> addedBlockBuffer;
  std::optional<Buffer> debugValuesBuffer;

  vk::raii::DescriptorSetLayout addBlockComputeDescriptorSetLayout = nullptr;
  vk::raii::DescriptorSet addBlockComputeDescriptorSet = nullptr;

  vk::raii::PipelineLayout addBlockComputePipelineLayout = nullptr;
  vk::raii::Pipeline addBlockComputePipeline = nullptr;

  void createComputeDescriptorSetLayout();
  void createComputeDescriptorSets();
  void recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) override;
  void drawCommand(vk::CommandBuffer commandBuffer) const override;
  [[nodiscard]] vk::VertexInputBindingDescription getVertexBindingDescription() const override;
  [[nodiscard]] std::vector<vk::VertexInputAttributeDescription>
  getVertexAttributeDescription() const override;
  void createComputeBuffers();
  void createComputeDescriptorPool();
  void createDescriptorSetLayout();
  void createDescriptorSets();
  void createTextureImageView();
  void createTextureSampler();
  void createTextureImage();
  void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) const;
  [[nodiscard]] vk::PipelineLayoutCreateInfo getPipelineLayoutInfo() const override;
  [[nodiscard]] vk::PipelineLayoutCreateInfo getComputePipelineLayoutInfo() const override;
  Buffer createBufferWithInitialData(vk::BufferUsageFlags usage, const void *src,
                                     vk::DeviceSize size) const;
  void createAddBlockComputePipeline();
  void createAddBlockComputeDescriptorSetLayout();
  void printDebugValues();
  void addBlock(int x, int y, int z);
  virtual void initWorld();
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
