#include "renderer.h"
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

Renderer::~Renderer() {
  if (window) {

    if (enableValidationLayers) {
      instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, dispatchLoader);
    }

    instance.destroySurfaceKHR(surface);
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
  }
}

/**
 * Setup the bare minimum to open a new window
 */
void Renderer::showWindow() {
  createWindow();
  initVulkan();
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
    throw std::runtime_error("Could not create window");
  }
}

void Renderer::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  // TODO
}

bool Renderer::shouldClose() { return glfwWindowShouldClose(window); }

void Renderer::draw() { glfwPollEvents(); }

void Renderer::loadShader(std::string fileName) {}

void Renderer::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
}

void Renderer::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
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

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vk::createInstance(&createInfo, nullptr, &instance) != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create instance!");
  }

  dispatchLoader = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
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
  vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
  createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
  createInfo.pfnUserCallback = debugCallback;
  return createInfo;
}

void Renderer::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  vk::DebugUtilsMessengerCreateInfoEXT createInfo = createDebugMessengerCreateInfo();
  debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo, nullptr, dispatchLoader);
}

void Renderer::createSurface() {
  VkSurfaceKHR natSurface = nullptr;
  if (glfwCreateWindowSurface(instance, window, nullptr, &natSurface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
  surface = natSurface;
}

void Renderer::pickPhysicalDevice() {
  std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

bool Renderer::isDeviceSuitable(vk::PhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices Renderer::findQueueFamilies(vk::PhysicalDevice device) {
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
        (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
      indices.graphicsAndComputeFamily = i;
    }

    VkBool32 presentSupport = device.getSurfaceSupportKHR(i, surface);

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool Renderer::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
  std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName.data());
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer::querySwapChainSupport(vk::PhysicalDevice device) {
  SwapChainSupportDetails details;

  details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
  details.formats = device.getSurfaceFormatsKHR(surface);
  details.presentModes = device.getSurfacePresentModesKHR(surface);

  return details;
}
