#define W 8u
#define MAX_B W *W *W

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

uint getBlockInd(int x, int y, int z) { return x + W * y + W * W * z; }
