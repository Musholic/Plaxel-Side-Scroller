
#include "TestRenderer.h"

namespace plaxel::test {

std::vector<Triangle> TestRenderer::getTriangles() {
  const auto indexBufferData = indexBuffer->getData<uint32_t>();
  const auto vertexBufferData = vertexBuffer->getData<Vertex>();
  const auto drawCommandBufferData = drawCommandBuffer->getData<VkDrawIndexedIndirectCommand>();
  const uint indexCount = drawCommandBufferData[0].indexCount;
  std::vector<Triangle> triangles;
  for (uint i = 0; i < indexCount / 3; ++i) {
    const uint index = indexBufferData[i * 3];
    const uint index2 = indexBufferData[i * 3 + 1];
    const uint index3 = indexBufferData[i * 3 + 2];
    triangles.emplace_back(
        Triangle{vertexBufferData[index], vertexBufferData[index2], vertexBufferData[index3]});
  }
  return triangles;
}

void TestRenderer::setVoxelTreeNode(const VoxelTreeNode &newNode) { voxelTreeNode = newNode; }

std::string Triangle::toString() const {
  std::string result = "{";
  result += vertices[0].toString() + ",";
  result += vertices[1].toString() + ",";
  result += vertices[2].toString() + "}";

  return result;
}

} // namespace plaxel::test