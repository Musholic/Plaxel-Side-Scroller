#ifndef PLAXEL_BASE_RENDERER_H
#define PLAXEL_BASE_RENDERER_H

#include <vulkan/vulkan_raii.hpp>

#include "camera.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>

namespace plaxel {

struct MouseButtons {
  bool left = false;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

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
const uint32_t MAX_VERTEX_COUNT = 8192;
const uint32_t MAX_INDEX_COUNT = 8192;
const int MAX_FRAMES_IN_FLIGHT = 2;
const uint64_t FENCE_TIMEOUT = 100000000;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData, void * /*pUserData*/) {
  std::ostringstream message;

  message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity))
          << ": " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes))
          << ":\n";
  message << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
  message << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
  message << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
  if (0 < pCallbackData->queueLabelCount) {
    message << std::string("\t") << "Queue Labels:\n";
    for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
      message << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName
              << ">\n";
    }
  }
  if (0 < pCallbackData->cmdBufLabelCount) {
    message << std::string("\t") << "CommandBuffer Labels:\n";
    for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
      message << std::string("\t\t") << "labelName = <"
              << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
    }
  }
  if (0 < pCallbackData->objectCount) {
    message << std::string("\t") << "Objects:\n";
    for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
      message << std::string("\t\t") << "Object " << i << "\n";
      message << std::string("\t\t\t") << "objectType   = "
              << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
              << "\n";
      message << std::string("\t\t\t")
              << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
      if (pCallbackData->pObjects[i].pObjectName) {
        message << std::string("\t\t\t") << "objectName   = <"
                << pCallbackData->pObjects[i].pObjectName << ">\n";
      }
    }
  }

  std::cout << message.str() << std::endl;

  return false;
}

class VulkanInitializationError : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

class VulkanDrawingError : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

class NotImplementedError : public std::runtime_error {
public:
  using runtime_error::runtime_error;
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

class BaseRenderer {
public:
  BaseRenderer();
  virtual ~BaseRenderer() = default;

  constexpr static std::string_view WINDOW_TITLE = "Plaxel";
  bool fullscreen = false;

  void closeWindow();
  bool shouldClose();
  void draw();
  void showWindow();

  void saveScreenshot(const char *filename);

private:
  // These objects needs to be destructed last
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;

protected:
  virtual void initVulkan();

  vk::Extent2D windowSize{1280, 720};

  void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties, vk::raii::Buffer &buffer,
                    vk::raii::DeviceMemory &bufferMemory);

  [[maybe_unused]] void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
  [[nodiscard]] virtual vk::PipelineLayoutCreateInfo getPipelineLayoutInfo() const;
  [[nodiscard]] virtual vk::PipelineLayoutCreateInfo getComputePipelineLayoutInfo() const;
  virtual void initCustomDescriptorSetLayout();
  uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
  void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                   vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);
  vk::raii::ImageView createImageView(vk::Image image, vk::Format format,
                                      vk::ImageAspectFlags aspectFlags);
  vk::raii::CommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(vk::CommandBuffer commandBuffer);
  void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

  uint32_t currentFrame = 0;

  vk::raii::Device device = nullptr;

  vk::raii::PipelineLayout computePipelineLayout = nullptr;
  vk::raii::Pipeline computePipeline = nullptr;

  vk::raii::DescriptorPool descriptorPool = nullptr;

  vk::raii::CommandPool commandPool = nullptr;
  vk::raii::Queue graphicsQueue = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::array<vk::raii::Buffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers{nullptr, nullptr};

private:
  GLFWwindow *window{};
  const bool enableValidationLayers;
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;

  vk::raii::Queue computeQueue = nullptr;
  vk::raii::Queue presentQueue = nullptr;

  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  std::vector<vk::raii::ImageView> swapChainImageViews;
  std::vector<vk::raii::Framebuffer> swapChainFramebuffers;

  vk::raii::Image depthImage = nullptr;
  vk::raii::DeviceMemory depthImageMemory = nullptr;
  vk::raii::ImageView depthImageView = nullptr;

  vk::Format swapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D swapChainExtent;

  vk::raii::RenderPass renderPass = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::CommandBuffers mainCommandBuffers = nullptr;
  vk::raii::CommandBuffers computeCommandBuffers = nullptr;

  std::array<vk::raii::DeviceMemory, MAX_FRAMES_IN_FLIGHT> uniformBuffersMemory{nullptr, nullptr};
  std::array<void *, MAX_FRAMES_IN_FLIGHT> uniformBuffersMapped{};

  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{nullptr, nullptr};
  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{nullptr, nullptr};
  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> computeFinishedSemaphores{nullptr, nullptr};
  std::array<vk::raii::Fence, MAX_FRAMES_IN_FLIGHT> inFlightFences{nullptr, nullptr};
  std::array<vk::raii::Fence, MAX_FRAMES_IN_FLIGHT> computeInFlightFences{nullptr, nullptr};

  bool framebufferResized = false;

  double lastFpsCountTime = 0.0f;
  int fpsCount = 0;
  glm::vec2 mousePos;
  Camera camera;
  MouseButtons mouseButtons;

  void createWindow();
  static void framebufferResizeCallback(GLFWwindow *window, [[maybe_unused]] int width,
                                        [[maybe_unused]] int height);
  static void mouseMoveHandler(GLFWwindow *window, double posx, double posy);
  static void keyboardHandler(GLFWwindow *window, int key, int scancode, int action, int mods);
  static void mouseHandler(GLFWwindow *window, int button, int action, int mods);

  void createInstance();
  static bool checkValidationLayerSupport();
  [[nodiscard]] std::vector<const char *> getRequiredExtensions() const;
  static vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();
  void setupDebugMessenger() const;
  void createSurface();
  void pickPhysicalDevice();
  bool isDeviceSuitable(vk::PhysicalDevice physicalDeviceCandidate) const;
  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDeviceCandidate) const;
  static bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);
  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDeviceCandidate) const;
  void createLogicalDevice();
  void createSwapChain();
  static vk::SurfaceFormatKHR
  chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  static vk::PresentModeKHR
  chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
  void createImageViews();
  void createRenderPass();
  void createGraphicsPipeline();
  vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);
  void createComputePipeline();
  void createFramebuffers();
  void createCommandPool();
  void createDescriptorPool();
  void createCommandBuffers();
  void createComputeCommandBuffers();
  void createSyncObjects();
  void createUniformBuffers();
  void updateUniformBuffer(uint32_t currentImage);
  virtual void recordComputeCommandBuffer(vk::CommandBuffer commandBuffer) = 0;
  void waitForFence(vk::Fence fence) const;
  void recreateSwapChain();
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

  void drawFrame();
  virtual void drawCommand(vk::CommandBuffer commandBuffer) const = 0;
  [[nodiscard]] virtual vk::VertexInputBindingDescription getVertexBindingDescription() const = 0;
  [[nodiscard]] virtual std::vector<vk::VertexInputAttributeDescription>
  getVertexAttributeDescription() const = 0;
  void createDepthResources();
  vk::Format findDepthFormat() const;
  vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features) const;
  void createImage(const vk::ImageCreateInfo &imageInfo, const vk::MemoryPropertyFlags &properties,
                   vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);
  static vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout);
  static vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout);
  void mouseMoved(const glm::vec2 &newPos);
  void keyPressed(int key);
  void keyReleased(int key);
  void mouseAction(int button, int action, int mods);
  void handleCameraKeys(int key, bool pressed);
};

} // namespace plaxel

#endif // PLAXEL_BASE_RENDERER_H
