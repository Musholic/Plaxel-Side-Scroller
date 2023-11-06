#include "base_renderer.h"
#include <chrono>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <limits>
#include <random>
#include <set>

using namespace plaxel;

BaseRenderer::BaseRenderer()
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
void BaseRenderer::showWindow() {
  createWindow();
  initVulkan();

  lastTime = glfwGetTime();
}

void BaseRenderer::closeWindow() {
  device.waitIdle();

  glfwDestroyWindow(window);
  glfwTerminate();
}

void BaseRenderer::createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto monitor = glfwGetPrimaryMonitor();
  auto mode = glfwGetVideoMode(monitor);
  windowSize.width = mode->width;
  windowSize.height = mode->height;

  if (fullscreen) {
    window = glfwCreateWindow(windowSize.width, windowSize.height, WINDOW_TITLE.c_str(), monitor,
                              nullptr);
  } else {
    windowSize.width /= 2;
    windowSize.height /= 2;
    window = glfwCreateWindow(windowSize.width, windowSize.height, WINDOW_TITLE.c_str(), nullptr,
                              nullptr);
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  if (!window) {
    throw VulkanInitializationError("Could not create window");
  }
}

void BaseRenderer::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto pRenderer = std::bit_cast<BaseRenderer *>(glfwGetWindowUserPointer(window));
  pRenderer->framebufferResized = true;
}

bool BaseRenderer::shouldClose() { return glfwWindowShouldClose(window); }

void BaseRenderer::loadShader(std::string fileName) {
  // TODO
}

void BaseRenderer::initVulkan() {
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();

  initCustomDescriptorSetLayout();

  createGraphicsPipeline();
  createComputePipeline();
  createFramebuffers();
  createCommandPool();
  createUniformBuffers();
  createDescriptorPool();
  createCommandBuffers();
  createComputeCommandBuffers();
  createSyncObjects();
}

void BaseRenderer::initCustomDescriptorSetLayout() {}

void BaseRenderer::createInstance() {
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

bool BaseRenderer::checkValidationLayerSupport() {
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

std::vector<const char *> BaseRenderer::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

vk::DebugUtilsMessengerCreateInfoEXT BaseRenderer::createDebugMessengerCreateInfo() {
  vk::DebugUtilsMessengerCreateInfoEXT createInfo;
  createInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;

  using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
  createInfo.messageSeverity = eVerbose | eWarning | eError;

  using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
  createInfo.messageType = eGeneral | eValidation | ePerformance;

  createInfo.pfnUserCallback = debugCallback;
  return createInfo;
}

void BaseRenderer::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();
  vk::raii::DebugUtilsMessengerEXT(instance, debugCreateInfo);
}

void BaseRenderer::createSurface() {
  VkSurfaceKHR natSurface = nullptr;
  if (glfwCreateWindowSurface(*instance, window, nullptr, &natSurface) != VK_SUCCESS) {
    throw VulkanInitializationError("failed to create window surface!");
  }
  surface = vk::raii::SurfaceKHR(instance, natSurface);
}

void BaseRenderer::pickPhysicalDevice() {
  vk::raii::PhysicalDevices devices(instance);

  for (const auto &device : devices) {
    if (isDeviceSuitable(*device)) {
      physicalDevice = device;
      break;
    }
  }

  if (*physicalDevice == nullptr) {
    throw VulkanInitializationError("failed to find a suitable GPU!");
  }
}

bool BaseRenderer::isDeviceSuitable(vk::PhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices BaseRenderer::findQueueFamilies(vk::PhysicalDevice device) {
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

bool BaseRenderer::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
  std::vector<vk::ExtensionProperties> availableExtensions =
      device.enumerateDeviceExtensionProperties();

  std::set<std::string, std::less<>> requiredExtensions(deviceExtensions.begin(),
                                                        deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName.data());
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails BaseRenderer::querySwapChainSupport(vk::PhysicalDevice device) {
  SwapChainSupportDetails details;

  details.capabilities = device.getSurfaceCapabilitiesKHR(*surface);
  details.formats = device.getSurfaceFormatsKHR(*surface);
  details.presentModes = device.getSurfacePresentModesKHR(*surface);

  return details;
}

void BaseRenderer::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(*physicalDevice);

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
  deviceFeatures.samplerAnisotropy = vk::True;

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

void BaseRenderer::createSwapChain() {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(*physicalDevice);

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

  QueueFamilyIndices indices = findQueueFamilies(*physicalDevice);
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
BaseRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR
BaseRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D BaseRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
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

void BaseRenderer::createImageViews() {
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

void BaseRenderer::createRenderPass() {
  vk::AttachmentDescription colorAttachment;
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference colorAttachmentRef;
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  vk::SubpassDependency dependency;
  dependency.srcSubpass = vk::SubpassExternal;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.srcAccessMask = vk::AccessFlagBits::eNone;
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  renderPass = vk::raii::RenderPass(device, renderPassInfo);
}


void BaseRenderer::createGraphicsPipeline() {
  auto vertShaderCode = readFile("shaders/shader.vert.spv");
  auto fragShaderCode = readFile("shaders/shader.frag.spv");

  vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
  vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
  vertShaderStageInfo.module = *vertShaderModule;
  vertShaderStageInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
  fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
  fragShaderStageInfo.module = *fragShaderModule;
  fragShaderStageInfo.pName = "main";

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo,
                                                                 fragShaderStageInfo};

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

  auto bindingDescription = getVertexBindingDescription();
  auto attributeDescriptions = getVertexAttributeDescription();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = vk::False;

  vk::PipelineViewportStateCreateInfo viewportState;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  vk::PipelineRasterizationStateCreateInfo rasterizer;
  rasterizer.depthClampEnable = vk::False;
  rasterizer.rasterizerDiscardEnable = vk::False;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.depthBiasEnable = vk::False;

  vk::PipelineMultisampleStateCreateInfo multisampling;
  multisampling.sampleShadingEnable = vk::False;

  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  using enum vk::ColorComponentFlagBits;
  colorBlendAttachment.colorWriteMask = eR | eG | eB | eA;
  colorBlendAttachment.blendEnable = vk::False;

  vk::PipelineColorBlendStateCreateInfo colorBlending;
  colorBlending.logicOpEnable = vk::False;
  colorBlending.logicOp = vk::LogicOp::eCopy;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                 vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicState;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo = getPipelineLayoutInfo();

  pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = *pipelineLayout;
  pipelineInfo.renderPass = *renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

vk::PipelineLayoutCreateInfo BaseRenderer::getPipelineLayoutInfo() const {
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  return pipelineLayoutInfo;
}

vk::PipelineLayoutCreateInfo BaseRenderer::getComputePipelineLayoutInfo() const {
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  return pipelineLayoutInfo;
}

vk::raii::ShaderModule BaseRenderer::createShaderModule(const std::vector<char> &code) {
  vk::ShaderModuleCreateInfo createInfo;
  createInfo.codeSize = code.size();
  createInfo.pCode = std::bit_cast<const uint32_t *>(code.data());

  return {device, createInfo};
}

void BaseRenderer::createComputePipeline() {
  auto computeShaderCode = readFile("shaders/shader.comp.spv");

  vk::raii::ShaderModule computeShaderModule = createShaderModule(computeShaderCode);

  vk::PipelineShaderStageCreateInfo computeShaderStageInfo;
  computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
  computeShaderStageInfo.module = *computeShaderModule;
  computeShaderStageInfo.pName = "main";

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo = getComputePipelineLayoutInfo();

  computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

  vk::ComputePipelineCreateInfo pipelineInfo;
  pipelineInfo.layout = *computePipelineLayout;
  pipelineInfo.stage = computeShaderStageInfo;

  computePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void BaseRenderer::createFramebuffers() {

  for (const auto &swapChainImageView : swapChainImageViews) {
    std::vector<vk::ImageView> attachments = {*swapChainImageView};

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = *renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;
    swapChainFramebuffers.emplace_back(device, framebufferInfo);
  }
}

void BaseRenderer::createCommandPool() {
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(*physicalDevice);

  vk::CommandPoolCreateInfo poolInfo;
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

  commandPool = vk::raii::CommandPool(device, poolInfo);
}


void BaseRenderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties, vk::raii::Buffer &buffer,
                            vk::raii::DeviceMemory &bufferMemory) {
  vk::BufferCreateInfo bufferInfo;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  buffer = vk::raii::Buffer(device, bufferInfo);

  vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
  buffer.bindMemory(*bufferMemory, 0);
}

uint32_t BaseRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
  vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw VulkanInitializationError("failed to find suitable memory type!");
}

void BaseRenderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = 1;

  vk::raii::CommandBuffers commandBuffers(device, allocInfo);
  vk::raii::CommandBuffer commandBuffer(std::move(commandBuffers[0]));

  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer.begin(beginInfo);

  vk::BufferCopy copyRegion;
  copyRegion.size = size;
  commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

  commandBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*commandBuffer;

  graphicsQueue.submit(submitInfo);
  graphicsQueue.waitIdle();
}


void BaseRenderer::createDescriptorPool() {
  std::array<vk::DescriptorPoolSize, 3> poolSizes;
  poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

  poolSizes[1].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

  poolSizes[2].type = vk::DescriptorType::eCombinedImageSampler;
  poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  vk::DescriptorPoolCreateInfo poolInfo;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

  descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void BaseRenderer::createCommandBuffers() {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void BaseRenderer::createComputeCommandBuffers() {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  computeCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void BaseRenderer::createSyncObjects() {
  vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo;
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    imageAvailableSemaphores[i] = vk::raii::Semaphore(device, semaphoreInfo);
    renderFinishedSemaphores[i] = vk::raii::Semaphore(device, semaphoreInfo);
    inFlightFences[i] = vk::raii::Fence(device, fenceInfo);

    computeFinishedSemaphores[i] = vk::raii::Semaphore(device, semaphoreInfo);
    computeInFlightFences[i] = vk::raii::Fence(device, fenceInfo);
  }
}

void BaseRenderer::draw() {
  glfwPollEvents();
  drawFrame();
  // We want to animate the particle system using the last frames time to get smooth, frame-rate
  // independent animation
  double currentTime = glfwGetTime();
  lastTime = currentTime;

  fpsCount++;
  if (currentTime - lastFpsCountTime >= 1.0) {
    std::cout << "FPS: " << fpsCount << std::endl;

    lastFpsCountTime = currentTime;
    fpsCount = 0;
  }
}

void BaseRenderer::drawFrame() {
  vk::SubmitInfo submitInfo;

  // Compute submission
  waitForFence(*computeInFlightFences[currentFrame]);

  updateUniformBuffer(currentFrame);

  device.resetFences(*computeInFlightFences[currentFrame]);

  computeCommandBuffers[currentFrame].reset();
  recordComputeCommandBuffer(*computeCommandBuffers[currentFrame]);

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*computeCommandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*computeFinishedSemaphores[currentFrame];

  computeQueue.submit(submitInfo, *computeInFlightFences[currentFrame]);

  // Graphics submission
  waitForFence(*inFlightFences[currentFrame]);

  uint32_t imageIndex;
  vk::Result result;
  std::tie(result, imageIndex) =
      swapChain.acquireNextImage(FENCE_TIMEOUT, *imageAvailableSemaphores[currentFrame]);
  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
    throw VulkanDrawingError("failed to acquire swap chain image!");
  }

  device.resetFences(*inFlightFences[currentFrame]);

  commandBuffers[currentFrame].reset();
  recordCommandBuffer(*commandBuffers[currentFrame], imageIndex);

  std::vector<vk::Semaphore> waitSemaphores = {*computeFinishedSemaphores[currentFrame],
                                               *imageAvailableSemaphores[currentFrame]};
  std::vector<vk::PipelineStageFlags> waitStages = {
      vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo = vk::SubmitInfo{};

  submitInfo.waitSemaphoreCount = 2;
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*commandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*renderFinishedSemaphores[currentFrame];

  graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

  vk::PresentInfoKHR presentInfo;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &*renderFinishedSemaphores[currentFrame];

  std::vector<vk::SwapchainKHR> swapChains = {*swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains.data();

  presentInfo.pImageIndices = &imageIndex;

  result = presentQueue.presentKHR(presentInfo);

  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
      framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (result != vk::Result::eSuccess) {
    throw VulkanDrawingError("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
void BaseRenderer::waitForFence(vk::Fence fence) {
  while (vk::Result::eTimeout == device.waitForFences({fence}, vk::True, FENCE_TIMEOUT))
    ;
}


void BaseRenderer::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  device.waitIdle();

  createSwapChain();
  swapChainImageViews.clear();
  createImageViews();
  swapChainFramebuffers.clear();
  createFramebuffers();
}

void BaseRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo;

  commandBuffer.begin(beginInfo);

  vk::RenderPassBeginInfo renderPassInfo;
  renderPassInfo.renderPass = *renderPass;
  renderPassInfo.framebuffer = *swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  vk::ClearValue clearColor = {vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

  vk::Viewport viewport;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  commandBuffer.setViewport(0, viewport);

  vk::Rect2D scissor;
  scissor.offset = vk::Offset2D{0, 0};
  scissor.extent = swapChainExtent;
  commandBuffer.setScissor(0, scissor);

  drawCommand(commandBuffer);

  commandBuffer.endRenderPass();
  commandBuffer.end();
}

void BaseRenderer::createUniformBuffers() {
  vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 uniformBuffers[i], uniformBuffersMemory[i]);
    uniformBuffersMapped[i] = uniformBuffersMemory[i].mapMemory(0, bufferSize);
  }
}

void BaseRenderer::updateUniformBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time =
      std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                         glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f),
                              swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
