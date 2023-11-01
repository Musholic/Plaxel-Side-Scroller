#include "renderer.h"
#include <limits>
#include <set>

using namespace plaxel;

Renderer::Renderer()
    :
#ifdef NDEBUG
      enableValidationLayers(false)
#else
      enableValidationLayers(true)
#endif
{
}

/**
 * Setup the bare minimum to open a new window
 */
void Renderer::showWindow() {
  createWindow();
  initVulkan();
}

void Renderer::closeWindow() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Renderer::createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto monitor = glfwGetPrimaryMonitor();
  auto mode = glfwGetVideoMode(monitor);
  size.width = mode->width;
  size.height = mode->height;

  if (fullscreen) {
    window = glfwCreateWindow(size.width, size.height, WINDOW_TITLE.c_str(), monitor, nullptr);
  } else {
    size.width /= 2;
    size.height /= 2;
    window = glfwCreateWindow(size.width, size.height, WINDOW_TITLE.c_str(), nullptr, nullptr);
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  if (!window) {
    throw VulkanInitializationError("Could not create window");
  }
}

void Renderer::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  // TODO
}

bool Renderer::shouldClose() { return glfwWindowShouldClose(window); }

void Renderer::draw() { glfwPollEvents(); }

void Renderer::loadShader(std::string fileName) {
  // TODO
}

void Renderer::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
}

void Renderer::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw VulkanInitializationError("validation layers requested, but not available!");
  }

  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = "Plaxel";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "PlaxelEngine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // This variable is placed outside the if statement to ensure it is not destroyed before creating
  // the instance
  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  instance = vk::raii::Instance(context, createInfo);
}

bool Renderer::checkValidationLayerSupport() {
  auto availableLayers = vk::enumerateInstanceLayerProperties();

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

std::vector<const char *> Renderer::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

vk::DebugUtilsMessengerCreateInfoEXT Renderer::createDebugMessengerCreateInfo() {
  vk::DebugUtilsMessengerCreateInfoEXT createInfo;
  createInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;

  using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
  createInfo.messageSeverity = eVerbose | eWarning | eError;

  using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
  createInfo.messageType = eGeneral | eValidation | ePerformance;

  createInfo.pfnUserCallback = debugCallback;
  return createInfo;
}

void Renderer::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();
  vk::raii::DebugUtilsMessengerEXT(instance, debugCreateInfo);
}

void Renderer::createSurface() {
  VkSurfaceKHR natSurface = nullptr;
  if (glfwCreateWindowSurface(*instance, window, nullptr, &natSurface) != VK_SUCCESS) {
    throw VulkanInitializationError("failed to create window surface!");
  }
  surface = vk::raii::SurfaceKHR(instance, natSurface);
}

void Renderer::pickPhysicalDevice() {
  vk::raii::PhysicalDevices devices(instance);

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (*physicalDevice == nullptr) {
    throw VulkanInitializationError("failed to find a suitable GPU!");
  }
}

bool Renderer::isDeviceSuitable(vk::raii::PhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices Renderer::findQueueFamilies(vk::raii::PhysicalDevice device) {
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
        (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
      indices.graphicsAndComputeFamily = i;
    }

    if (device.getSurfaceSupportKHR(i, *surface)) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool Renderer::checkDeviceExtensionSupport(vk::raii::PhysicalDevice device) {
  std::vector<vk::ExtensionProperties> availableExtensions =
      device.enumerateDeviceExtensionProperties();

  std::set<std::string, std::less<>> requiredExtensions(deviceExtensions.begin(),
                                                        deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName.data());
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer::querySwapChainSupport(vk::raii::PhysicalDevice device) {
  SwapChainSupportDetails details;

  details.capabilities = device.getSurfaceCapabilitiesKHR(*surface);
  details.formats = device.getSurfaceFormatsKHR(*surface);
  details.presentModes = device.getSurfacePresentModesKHR(*surface);

  return details;
}

void Renderer::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(),
                                            indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::PhysicalDeviceFeatures deviceFeatures{};

  vk::DeviceCreateInfo createInfo{};
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  device = vk::raii::Device(physicalDevice, createInfo);

  graphicsQueue = device.getQueue(indices.graphicsAndComputeFamily.value(), 0);
  computeQueue = device.getQueue(indices.graphicsAndComputeFamily.value(), 0);
  presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}

void Renderer::createSwapChain() {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{};
  createInfo.surface = *surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  std::vector<uint32_t> queueFamilyIndices = {indices.graphicsAndComputeFamily.value(),
                                              indices.presentFamily.value()};

  if (indices.graphicsAndComputeFamily != indices.presentFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  swapChain = vk::raii::SwapchainKHR(device, createInfo);

  swapChainImages = swapChain.getImages();

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

vk::SurfaceFormatKHR
Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR
Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void Renderer::createImageViews() {
  for (auto swapChainImage : swapChainImages) {
    vk::ImageViewCreateInfo createInfo;
    createInfo.image = swapChainImage;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = swapChainImageFormat;
    createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    swapChainImageViews.emplace_back(device, createInfo);
  }
}
