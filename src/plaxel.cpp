#include "plaxel.h"
#include "renderer/renderer.h"
using namespace plaxel;

void Plaxel::start() {
  Renderer renderer;
  renderer.showWindow();

  while (!renderer.shouldClose()) {
    renderer.draw();
  }

  renderer.closeWindow();
}
