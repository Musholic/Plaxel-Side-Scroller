#ifndef TESTRENDERER_H
#define TESTRENDERER_H
#include "../../src/renderer/renderer.h"

namespace plaxel::test {
struct Triangle {
  Vertex vertices[3];
  [[nodiscard]] std::string toString() const;
};

class TestRenderer : public Renderer {
public:
  std::vector<Triangle> getTriangles();
  void addBlock(int x, int y, int z);

protected:
  void initWorld() override;
};

} // namespace plaxel::test

#endif // TESTRENDERER_H
