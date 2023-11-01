#ifndef PLAXEL_RENDERER_H
#define PLAXEL_RENDERER_H

#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <fstream>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>

namespace plaxel {

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsAndComputeFamily;
  std::optional<uint32_t> presentFamily;

  [[nodiscard]] bool isComplete() const {
    return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

  switch (vk::DebugUtilsMessageSeverityFlagBitsEXT(messageType)) {
    using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
  case eVerbose:
    std::cerr << "(V) ";
    break;
  case eInfo:
    std::cerr << "(INFO) ";
    break;
  case eWarning:
    std::cerr << "(WARN) ";
    break;
  case eError:
    std::cerr << "(ERR) ";
    break;
  }
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

class VulkanInitializationError : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);

    return bindingDescription;
  }

  static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[1].offset = offsetof(Particle, color);

    return attributeDescriptions;
  }
};

static std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw VulkanInitializationError("failed to open file!");
  }

  auto fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
};

class Renderer {
public:
  Renderer();
  void showWindow();
  void closeWindow();
  constexpr static std::string WINDOW_TITLE = "Plaxel";
  bool fullscreen = false;

  bool shouldClose();

  void draw();

  void loadShader(std::string fileName);

private:
  GLFWwindow *window{};
  vk::Extent2D size{1280, 720};
  const bool enableValidationLayers;
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::Device device = nullptr;

  vk::raii::Queue graphicsQueue = nullptr;
  vk::raii::Queue computeQueue = nullptr;
  vk::raii::Queue presentQueue = nullptr;

  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  std::vector<vk::raii::ImageView> swapChainImageViews;
  std::vector<vk::raii::Framebuffer> swapChainFramebuffers;

  vk::Format swapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D swapChainExtent;

  vk::raii::RenderPass renderPass = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr;
  vk::raii::PipelineLayout computePipelineLayout = nullptr;
  vk::raii::Pipeline computePipeline = nullptr;

  vk::raii::CommandPool commandPool = nullptr;

  void createWindow();
  static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
  void initVulkan();

  void createInstance();
  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredExtensions();
  vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  bool isDeviceSuitable(vk::raii::PhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(vk::raii::PhysicalDevice device);
  bool checkDeviceExtensionSupport(vk::raii::PhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(vk::raii::PhysicalDevice device);
  void createLogicalDevice();
  void createSwapChain();
  vk::SurfaceFormatKHR
  chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  vk::PresentModeKHR
  chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
  void createImageViews();
  void createRenderPass();
  void createComputeDescriptorSetLayout();
  void createGraphicsPipeline();
  vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);
  void createComputePipeline();
  void createFramebuffers();
  void createCommandPool();
};

} // namespace plaxel

#endif // PLAXEL_RENDERER_H
