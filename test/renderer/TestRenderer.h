#ifndef TESTRENDERER_H
#define TESTRENDERER_H
#include "../../src/renderer/renderer.h"

#include <set>

namespace plaxel::test {
constexpr uint32_t UINT32_NULL_VALUE = -1;

struct Triangle {
  Vertex vertices[3];
  [[nodiscard]] std::string toString() const;
};

struct VoxelTreeNode {
  unsigned int height{};
  std::vector<std::optional<VoxelTreeNode>> children{4};
  std::set<std::string> blocks;
  int x{};
  int y{};
  VoxelTreeNode(const plaxel::VoxelTreeNode &node,
                         const std::vector<plaxel::VoxelTreeNode> &gpuNodes,
                         const std::vector<VoxelTreeLeaf> &gpuLeaves) {
    height = node.height;
    x = node.x;
    y = node.y;
    for (int i = 0; i < 4; ++i) {
      const uint32_t child = node.children[i];
      if (child != UINT32_NULL_VALUE) {
        children[i].emplace(gpuNodes[child], gpuNodes, gpuLeaves);
      }
    }
    if (node.leaf != UINT32_NULL_VALUE) {
      for (int i = 0; i < NB_BLOCKS; ++i) {
        if (gpuLeaves[node.leaf].blocks[i] == 1) {
          const int xBlock = i % BLOCK_W;
          const int yBlock = (i / BLOCK_W) % BLOCK_W;
          const int zBlock = i / BLOCK_W / BLOCK_W;
          std::string block;
          block += std::to_string(xBlock) + ";";
          block += std::to_string(yBlock) + ";";
          block += std::to_string(zBlock);

          blocks.emplace(block);
        }
      }
    }
  }
};

class TestRenderer : public Renderer {
public:
  std::vector<Triangle> getTriangles();
  void addBlock(int x, int y, int z);
  VoxelTreeNode getVoxelTree();
  void moveCursor(glm::ivec3 pos);

protected:
  void initWorld() override;
};

} // namespace plaxel::test

#endif // TESTRENDERER_H
