#ifndef PLAXEL_BASE_RENDERER_H
#define PLAXEL_BASE_RENDERER_H

#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <fstream>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>

namespace plaxel {

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

  void closeWindow();
  constexpr static std::string WINDOW_TITLE = "Plaxel";
  bool fullscreen = false;

  bool shouldClose();

  void draw();

  void loadShader(std::string fileName);

  void showWindow();

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

  void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
  [[nodiscard]] virtual vk::PipelineLayoutCreateInfo getPipelineLayoutInfo() const;
  [[nodiscard]] virtual vk::PipelineLayoutCreateInfo getComputePipelineLayoutInfo() const;
  virtual void initCustomDescriptorSetLayout();
  uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

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

  vk::Format swapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D swapChainExtent;

  vk::raii::RenderPass renderPass = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  vk::raii::CommandBuffers commandBuffers = nullptr;
  vk::raii::CommandBuffers computeCommandBuffers = nullptr;

  std::array<vk::raii::DeviceMemory, MAX_FRAMES_IN_FLIGHT> uniformBuffersMemory{nullptr, nullptr};
  std::array<void *, MAX_FRAMES_IN_FLIGHT> uniformBuffersMapped{};

  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{nullptr, nullptr};
  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{nullptr, nullptr};
  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> computeFinishedSemaphores{nullptr, nullptr};
  std::array<vk::raii::Fence, MAX_FRAMES_IN_FLIGHT> inFlightFences{nullptr, nullptr};
  std::array<vk::raii::Fence, MAX_FRAMES_IN_FLIGHT> computeInFlightFences{nullptr, nullptr};

  bool framebufferResized = false;

  double lastTime = 0.0f;
  double lastFpsCountTime = 0.0f;
  int fpsCount = 0;

  void createWindow();
  static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

  void createInstance();
  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredExtensions();
  vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  bool isDeviceSuitable(vk::PhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
  bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
  void createLogicalDevice();
  void createSwapChain();
  vk::SurfaceFormatKHR
  chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  vk::PresentModeKHR
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
  void waitForFence(vk::Fence fence);
  void recreateSwapChain();
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

  void drawFrame();
  virtual void drawCommand(vk::CommandBuffer commandBuffer) const = 0;
  [[nodiscard]] virtual vk::VertexInputBindingDescription getVertexBindingDescription() const = 0;
  [[nodiscard]] virtual std::vector<vk::VertexInputAttributeDescription>
  getVertexAttributeDescription() const = 0;
};

} // namespace plaxel

#endif // PLAXEL_BASE_RENDERER_H
