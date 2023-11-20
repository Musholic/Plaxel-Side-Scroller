#ifndef PLAXEL_CAMERA_H
#define PLAXEL_CAMERA_H

#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
namespace plaxel {
class Camera {
public:
  [[nodiscard]] glm::mat4 getViewMatrix() const;
  struct {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool forward = false;
    bool backward = false;
  } keys;

  void rotate(const float &d, const float &d1);

  void update();

  void printDebug();

private:
  const float rotationSpeed = .3f;
  const float movementSpeed = 3.f;
  glm::vec3 rotation{-30.f, -210.f, 0.f};
  glm::vec3 position{1.8f, 1.9f, 2.8f};

  double lastUpdateTime = glfwGetTime();
  bool moving();
  [[nodiscard]] glm::vec3 getFront() const;
  void translate(const glm::vec3 &delta);
};

} // namespace plaxel
#endif // PLAXEL_CAMERA_H
