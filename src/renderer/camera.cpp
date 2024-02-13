#include "camera.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>

namespace plaxel {

glm::mat4 Camera::getViewMatrix() const {
  auto rotM = glm::mat4(1.0f);

  rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
  rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
  rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

  const auto transM = glm::translate(glm::mat4(1.0f), -position);
  return rotM * transM;
}

glm::ivec3 Camera::getLeftDirection() const {
  if (rotation.y < 45 || rotation.y > 315) {
    return {-1, 0, 0};
  }
  if (rotation.y < 135) {
    return {0, 0, -1};
  }
  if (rotation.y < 225) {
    return {1, 0, 0};
  }
  return {0, 0, 1};
}

void Camera::rotate(const float &dx, const float &dy) {
  rotation += glm::vec3(-dy * rotationSpeed, -dx * rotationSpeed, 0.0f);
  rotation.x = std::clamp(rotation.x, -90.0f, 90.0f);
  if (rotation.y < 360) {
    rotation.y += 360;
  }
  if (rotation.y > 360) {
    rotation.y -= 360;
  }
}

glm::vec3 Camera::getFront() const {
  glm::vec3 camFront;
  camFront.x = static_cast<float>(-cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y)));
  camFront.y = static_cast<float>(sin(glm::radians(rotation.x)));
  camFront.z = static_cast<float>(cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y)));
  camFront = normalize(camFront);
  return camFront;
}

void Camera::translate(const glm::vec3 &delta) {
  const glm::vec3 camFront = getFront();
  constexpr auto camUp = glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec3 camLeft = cross(camFront, camUp);
  const glm::vec3 result = camLeft * delta.x + camUp * delta.y + camFront * delta.z;
  position += result;
}

void Camera::update() {
  static double lastUpdateTime = glfwGetTime();

  const double startTime = glfwGetTime();
  const auto deltaTime = static_cast<float>(startTime - lastUpdateTime);
  lastUpdateTime = startTime;

  if (moving()) {
    glm::vec3 direction{0.f, 0.f, 0.f};
    if (keys.forward != keys.backward) {
      direction.z = keys.forward ? -1.0f : 1.0f;
    }
    if (keys.left != keys.right) {
      direction.x = keys.left ? 1.0f : -1.0f;
    }
    if (keys.up != keys.down) {
      direction.y = keys.up ? 1.0f : -1.0f;
    }
    translate(direction * deltaTime * movementSpeed);
  }
}

bool Camera::moving() const {
  return keys.left || keys.right || keys.up || keys.down || keys.forward || keys.backward;
}
void Camera::printDebug() const {
  std::cout << "Camera debug info:\n";
  std::cout << "Rotation:{" << rotation.x << ";" << rotation.y << ";" << rotation.z << "}\n";
  std::cout << "Position:{" << position.x << ";" << position.y << ";" << position.z << "}"
            << std::endl;
}

} // namespace plaxel