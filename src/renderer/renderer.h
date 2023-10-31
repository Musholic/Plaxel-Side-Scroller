#ifndef PLAXEL_RENDERER_H
#define PLAXEL_RENDERER_H

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

namespace plaxel {

class Renderer {
public:
  Renderer();
  ~Renderer();
  void showWindow();
  constexpr static std::string WINDOW_TITLE = "Plaxel";
  bool fullscreen = false;

  bool shouldClose();

  void draw();

  void loadShader(std::string fileName);

private:
  GLFWwindow *window{};
  vk::Extent2D size{1280, 720};
  const bool enableValidationLayers;
  vk::Instance instance;
  vk::DispatchLoaderDynamic dispatchLoader;
  vk::DebugUtilsMessengerEXT debugMessenger;
  vk::SurfaceKHR surface;

  void createWindow();
  static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
  void initVulkan();
  void createInstance();
  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredExtensions();
  vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    switch (vk::DebugUtilsMessageSeverityFlagBitsEXT(messageType)) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
      std::cerr << "(V) ";
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
      std::cerr << "(INFO) ";
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
      std::cerr << "(WARN) ";
      break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
      std::cerr << "(ERR) ";
      break;
    }
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }
  void setupDebugMessenger();
  VkResult createDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                        const VkAllocationCallbacks *pAllocator,
                                        VkDebugUtilsMessengerEXT *pDebugMessenger);
  void destroyDebugUtilsMessengerEXT(const VkAllocationCallbacks *pAllocator);
  void createSurface();
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
