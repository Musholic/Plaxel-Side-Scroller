#ifndef PLAXEL_BASE_RENDERER_H
#define PLAXEL_BASE_RENDERER_H

#include <vulkan/vulkan_raii.hpp>

#include "Buffer.h"
#include "UIOverlay.h"
#include "camera.h"
#include "file_utils.h"

#include "cmrc/cmrc.hpp"
#include <GLFW/glfw3.h>
#include <boost/iostreams/filter/zlib.hpp>
#include <fstream>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>

static constexpr int NB_COMPUTE_BUFFERS = 6;
static constexpr int NB_ADD_BLOCK_COMPUTE_BUFFERS = 3;
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
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME};
constexpr uint32_t MAX_VERTEX_COUNT = 8192;
constexpr uint32_t MAX_INDEX_COUNT = 8192;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint64_t FENCE_TIMEOUT = 100000000;
constexpr int TARGET_FPS = 60;

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

class BaseRenderer {
public:
  explicit BaseRenderer(int targetFps);
  virtual ~BaseRenderer() = default;

  constexpr static std::string_view WINDOW_TITLE = "Plaxel";

  void closeWindow() const;
  [[nodiscard]] bool shouldClose() const;
  void initializeOverlay();
  void draw();
  void showWindow();

  void saveScreenshot(const char *filename) const;
  bool showOverlay = true;

private:
  // These objects needs to be destructed last
  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugReportCallbackEXT debugReportCallbackHandle = nullptr;

protected:
  virtual void initVulkan();

  vk::Extent2D windowSize{1280, 720};

  void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) const;
  [[nodiscard]] virtual vk::PipelineLayoutCreateInfo getPipelineLayoutInfo() const;
  [[nodiscard]] virtual vk::PipelineLayoutCreateInfo getComputePipelineLayoutInfo() const;
  virtual void initCustomDescriptorSetLayout();
  void setupDebugReportCallback();
  void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                   vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory) const;
  vk::raii::ImageView createImageView(vk::Image image, vk::Format format,
                                      vk::ImageAspectFlags aspectFlags);
  [[nodiscard]] vk::raii::CommandBuffer beginSingleTimeCommands() const;
  void endSingleTimeCommands(vk::CommandBuffer commandBuffer) const;
  void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout) const;
  vk::raii::ShaderModule createShaderModule(const cmrc::file &code);

  uint32_t currentFrame = 0;
  const double frameTimeS;
  int lastFps = 0;

  vk::raii::Device device = nullptr;

  vk::raii::PipelineLayout computePipelineLayout = nullptr;
  vk::raii::Pipeline computePipeline = nullptr;

  vk::raii::DescriptorPool descriptorPool = nullptr;

  vk::raii::CommandPool commandPool = nullptr;
  vk::raii::Queue graphicsQueue = nullptr;
  vk::raii::PhysicalDevice physicalDevice = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  std::vector<Buffer> uniformBuffers{};
  vk::raii::CommandBuffer computeCommandBuffer = nullptr;
  vk::raii::Queue computeQueue = nullptr;

  UIOverlay overlay;

private:
  GLFWwindow *window{};
  bool fullscreen = false;
#ifdef NDEBUG
  bool enableValidationLayers = false;
#else
  bool enableValidationLayers = true;
#endif
  vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;

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

  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{nullptr, nullptr};
  std::array<vk::raii::Semaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{nullptr, nullptr};
  vk::raii::Semaphore computeFinishedSemaphore = nullptr;
  std::array<vk::raii::Fence, MAX_FRAMES_IN_FLIGHT> inFlightFences{nullptr, nullptr};
  vk::raii::Fence computeFence = nullptr;

  bool framebufferResized = false;

  glm::vec2 mousePos{};
  Camera camera;
  MouseButtons mouseButtons;

  void createWindow();
  static void framebufferResizeCallback(GLFWwindow *window, [[maybe_unused]] int width,
                                        [[maybe_unused]] int height);
  static void mouseMoveHandler(GLFWwindow *window, double posx, double posy);
  static void keyboardHandler(GLFWwindow *window, int key, [[maybe_unused]] int scancode,
                              int action, [[maybe_unused]] int mods);
  static void mouseHandler(GLFWwindow *window, int button, int action, int mods);

  void createInstance();
  static bool checkValidationLayerSupport();
  [[nodiscard]] std::vector<const char *> getRequiredExtensions() const;
  static vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();
  void setupDebugMessenger() const;
  void createSurface();
  void pickPhysicalDevice();
  [[nodiscard]] bool isDeviceSuitable(vk::PhysicalDevice physicalDeviceCandidate) const;
  [[nodiscard]] QueueFamilyIndices
  findQueueFamilies(vk::PhysicalDevice physicalDeviceCandidate) const;
  static bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);
  [[nodiscard]] SwapChainSupportDetails
  querySwapChainSupport(vk::PhysicalDevice physicalDeviceCandidate) const;
  void createLogicalDevice();
  void createSwapChain();
  static vk::SurfaceFormatKHR
  chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  static vk::PresentModeKHR
  chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
  [[nodiscard]] vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;
  void createImageViews();
  void createRenderPass();
  void createGraphicsPipeline();
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
  [[nodiscard]] vk::Format findDepthFormat() const;
  [[nodiscard]] vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates,
                                               vk::ImageTiling tiling,
                                               vk::FormatFeatureFlags features) const;
  void createImage(const vk::ImageCreateInfo &imageInfo, const vk::MemoryPropertyFlags &properties,
                   vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory) const;
  static vk::AccessFlags accessFlagsForLayout(vk::ImageLayout layout);
  static vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout);
  void mouseMoved(const glm::vec2 &newPos);
  void keyPressed(int key);
  void keyReleased(int key);
  void mouseAction(int button, int action, int mods);
  void handleCameraKeys(int key, bool pressed);

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData, void * /*pUserData*/);
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags,
                                                            VkDebugReportObjectTypeEXT objType,
                                                            uint64_t srcObject, size_t location,
                                                            int32_t msgCode,
                                                            const char *pLayerPrefix,
                                                            const char *pMsg, void *pUserData);
  void manageFps() const;
  void computeFps();
};

} // namespace plaxel

#endif // PLAXEL_BASE_RENDERER_H
