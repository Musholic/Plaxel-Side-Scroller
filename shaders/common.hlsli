#line 1
#define W 8
#define WU 8u
#define MAX_B WU *WU *WU

struct VoxelTreeNode {
  uint height;
  uint children[4];
  uint leaf;
  int x;
  int y;
};

RWStructuredBuffer<VoxelTreeNode> nodes : register(u0);

struct VoxelTreeLeaf {
  uint blocks[MAX_B];
};

RWStructuredBuffer<VoxelTreeLeaf> leaves : register(u1);

struct VoxelTreeInfo {
  uint rootNodeIndex;
  uint lastNodeIndex;
  uint lastLeafIndex;
};

RWStructuredBuffer<VoxelTreeInfo> voxelTreeInfo : register(u2);
#define rootNodeIndex voxelTreeInfo[0].rootNodeIndex
#define lastNodeIndex voxelTreeInfo[0].lastNodeIndex
#define lastLeafIndex voxelTreeInfo[0].lastLeafIndex

uint createChildNode(uint parent, uint childIndex);

uint getBlockInd(int x, int y, int z) { return x + W * y + W * W * z; }

uint findChildIndex(uint index, inout int x, inout int y) {
  uint height = nodes[index].height;
  int halfSize = WU << (height - 1);
  uint childIndex = 0;
  if (x >= halfSize) {
    childIndex++;
    x -= halfSize;
  }
  if (y >= halfSize) {
    childIndex += 2;
    y -= halfSize;
  }
  uint nextIndex = nodes[index].children[childIndex];
#ifdef ADD_BLOCK
  if (nextIndex == -1) {
    nextIndex = createChildNode(index, childIndex);
  }
#endif
  return nextIndex;
}

uint findLeafNode(inout int x, inout int y) {
  uint index = rootNodeIndex;

  int startX = nodes[index].x * W;
  int startY = nodes[index].y * W;
  x -= startX;
  y -= startY;

  uint maxSize = (1u << nodes[index].height) * W;

  // Eliminate cases not covered by the root node
  if (x < 0 || y < 0 || x >= maxSize || y >= maxSize) {
    return -1;
  }

  // This for loop should be replace by a while loop, but the while loop seems bugged by only
  // iterating once
  // TODO: check if the problem is also there on an actual graphic card
  [unroll(20)] for (int i = 0; i < 20; i++) if (index != -1 && nodes[index].height > 0) {
    index = findChildIndex(index, x, y);
  }
  return index;
}
