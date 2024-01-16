#ifndef TESTRENDERER_H
#define TESTRENDERER_H
#include "../../src/renderer/renderer.h"

#include <set>

namespace plaxel::test {
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
  explicit VoxelTreeNode(const plaxel::VoxelTreeNode &node,
                         const plaxel::VoxelTreeNode gpuNodes[MAX_NODES],
                         const VoxelTreeLeaf gpuLeaves[MAX_LEAVES]) {
    height = node.height;
    x = node.x;
    y = node.y;
    for (int i = 0; i < 4; ++i) {
      const uint32_t child = node.children[i];
      if (child != -1) {
        children[i].emplace(gpuNodes[child], gpuNodes, gpuLeaves);
      }
    }
    if (node.leaf != -1) {
      for (int i = 0; i < NB_BLOCKS; ++i) {
        if (gpuLeaves[node.leaf].blocks[i] == 1) {
          const int x = i % BLOCK_W;
          const int y = (i / BLOCK_W) % BLOCK_W;
          const int z = i / BLOCK_W / BLOCK_W;
          std::string block;
          block += std::to_string(x) + ";";
          block += std::to_string(y) + ";";
          block += std::to_string(z);

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
  void getVoxelTree();

protected:
  void initWorld() override;
};

} // namespace plaxel::test

#endif // TESTRENDERER_H
