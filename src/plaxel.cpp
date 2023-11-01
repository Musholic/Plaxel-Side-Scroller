#include "plaxel.h"
#include "renderer/renderer.h"
using namespace plaxel;

void Plaxel::start() {
  Renderer renderer;
  renderer.showWindow();

  // Create compute shader
  renderer.loadShader("shaders/shader.comp");

  // Fill initial values
  // Attach shader and fragments with appropriate bindings

  while(!renderer.shouldClose()) {
    renderer.draw();
  }

  renderer.closeWindow();
}
