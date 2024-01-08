#include "base_renderer.h"
#include <chrono>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <limits>
#include <random>
#include <set>
#include <thread>

using namespace plaxel;

BaseRenderer::BaseRenderer(const int targetFps) : frameTimeS(1.0 / targetFps) {}

/**
 * Setup the bare minimum to open a new window
 */
void BaseRenderer::showWindow() {
  createWindow();
  initVulkan();
}

void BaseRenderer::closeWindow() const {
  device.waitIdle();

  glfwDestroyWindow(window);
  glfwTerminate();
}

void BaseRenderer::createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWmonitor *monitor = nullptr;
  if (fullscreen) {
    monitor = glfwGetPrimaryMonitor();
  }
  window = glfwCreateWindow(static_cast<int>(windowSize.width), static_cast<int>(windowSize.height),
                            WINDOW_TITLE.data(), monitor, nullptr);

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetCursorPosCallback(window, mouseMoveHandler);
  glfwSetKeyCallback(window, keyboardHandler);
  glfwSetMouseButtonCallback(window, mouseHandler);
  if (!window) {
    throw VulkanInitializationError("Could not create window");
  }
}

void BaseRenderer::framebufferResizeCallback(GLFWwindow *window, [[maybe_unused]] int width,
                                             [[maybe_unused]] int height) {
  const auto pRenderer = static_cast<BaseRenderer *>(glfwGetWindowUserPointer(window));
  pRenderer->framebufferResized = true;
}

bool BaseRenderer::shouldClose() const { return glfwWindowShouldClose(window); }

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
  createCommandPool();
  createDepthResources();
  createFramebuffers();
  createUniformBuffers();
  createDescriptorPool();
  createCommandBuffers();
  createComputeCommandBuffers();
  createSyncObjects();
}

void BaseRenderer::initCustomDescriptorSetLayout() {
  // Overridden for additional descriptor set layout
}

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

  const auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // This variable is placed outside the if statement to ensure it is not destroyed before creating
  // the instance
  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&debugCreateInfo);
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  instance = vk::raii::Instance(context, createInfo);
}

bool BaseRenderer::checkValidationLayerSupport() {
  const auto availableLayers = vk::enumerateInstanceLayerProperties();

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

std::vector<const char *> BaseRenderer::getRequiredExtensions() const {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

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

void BaseRenderer::setupDebugMessenger() const {
  if (!enableValidationLayers)
    return;

  const vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();
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
  for (auto &physicalDeviceCandidate : vk::raii::PhysicalDevices{instance}) {
    if (isDeviceSuitable(*physicalDeviceCandidate)) {
      physicalDevice = std::move(physicalDeviceCandidate);
      break;
    }
  }

  if (!*physicalDevice) {
    throw VulkanInitializationError("failed to find a suitable GPU!");
  }
}

bool BaseRenderer::isDeviceSuitable(vk::PhysicalDevice physicalDeviceCandidate) const {
  const QueueFamilyIndices indices = findQueueFamilies(physicalDeviceCandidate);

  const bool extensionsSupported = checkDeviceExtensionSupport(physicalDeviceCandidate);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDeviceCandidate);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices
BaseRenderer::findQueueFamilies(vk::PhysicalDevice physicalDeviceCandidate) const {
  QueueFamilyIndices indices;

  const std::vector<vk::QueueFamilyProperties> queueFamilies =
      physicalDeviceCandidate.getQueueFamilyProperties();

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
        (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
      indices.graphicsAndComputeFamily = i;
    }

    if (physicalDeviceCandidate.getSurfaceSupportKHR(i, *surface)) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool BaseRenderer::checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice) {
  const std::vector<vk::ExtensionProperties> availableExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();

  std::set<std::string, std::less<>> requiredExtensions(deviceExtensions.begin(),
                                                        deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName.data());
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails
BaseRenderer::querySwapChainSupport(vk::PhysicalDevice physicalDeviceCandidate) const {
  SwapChainSupportDetails details;

  details.capabilities = physicalDeviceCandidate.getSurfaceCapabilitiesKHR(*surface);
  details.formats = physicalDeviceCandidate.getSurfaceFormatsKHR(*surface);
  details.presentModes = physicalDeviceCandidate.getSurfacePresentModesKHR(*surface);

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
  const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(*physicalDevice);

  const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

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
  createInfo.imageUsage =
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

  const QueueFamilyIndices indices = findQueueFamilies(*physicalDevice);
  const std::vector<uint32_t> queueFamilyIndices = {indices.graphicsAndComputeFamily.value(),
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

vk::Extent2D BaseRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
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
  for (const auto swapChainImage : swapChainImages) {
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

  vk::AttachmentDescription depthAttachment;
  depthAttachment.format = findDepthFormat();
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference colorAttachmentRef;
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::AttachmentReference depthAttachmentRef;
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::SubpassDescription subpass;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  vk::SubpassDependency dependency;
  dependency.srcSubpass = vk::SubpassExternal;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.srcAccessMask = vk::AccessFlagBits::eNone;
  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  const std::array attachments = {colorAttachment, depthAttachment};
  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.attachmentCount = attachments.size();
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  renderPass = vk::raii::RenderPass(device, renderPassInfo);
}

void BaseRenderer::createGraphicsPipeline() {
  auto vertShaderCode = files::readFile("shaders/shader.vert.spv");
  auto fragShaderCode = files::readFile("shaders/shader.frag.spv");

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

  vk::PipelineDepthStencilStateCreateInfo depthStencil;
  depthStencil.depthTestEnable = vk::True;
  depthStencil.depthWriteEnable = vk::True;
  depthStencil.depthCompareOp = vk::CompareOp::eLess;
  depthStencil.depthBoundsTestEnable = vk::False;
  depthStencil.stencilTestEnable = vk::False;

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
  pipelineInfo.pDepthStencilState = &depthStencil;
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

vk::raii::ShaderModule BaseRenderer::createShaderModule(const cmrc::file &code) {
  vk::ShaderModuleCreateInfo createInfo;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.begin());

  return {device, createInfo};
}

void BaseRenderer::createComputePipeline() {
  const auto computeShaderCode = files::readFile("shaders/shader.comp.spv");

  const vk::raii::ShaderModule computeShaderModule = createShaderModule(computeShaderCode);

  vk::PipelineShaderStageCreateInfo computeShaderStageInfo;
  computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
  computeShaderStageInfo.module = *computeShaderModule;
  computeShaderStageInfo.pName = "main";

  const vk::PipelineLayoutCreateInfo pipelineLayoutInfo = getComputePipelineLayoutInfo();

  computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

  vk::ComputePipelineCreateInfo pipelineInfo;
  pipelineInfo.layout = *computePipelineLayout;
  pipelineInfo.stage = computeShaderStageInfo;

  computePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void BaseRenderer::createFramebuffers() {

  for (const auto &swapChainImageView : swapChainImageViews) {
    std::vector<vk::ImageView> attachments = {*swapChainImageView, *depthImageView};

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = *renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;
    swapChainFramebuffers.emplace_back(device, framebufferInfo);
  }
}

void BaseRenderer::createCommandPool() {
  const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(*physicalDevice);

  vk::CommandPoolCreateInfo poolInfo;
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

  commandPool = vk::raii::CommandPool(device, poolInfo);
}

void BaseRenderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                              vk::DeviceSize size) const {
  const vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

  vk::BufferCopy copyRegion;
  copyRegion.size = size;
  commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

  endSingleTimeCommands(*commandBuffer);
}

void BaseRenderer::createDescriptorPool() {
  std::array<vk::DescriptorPoolSize, 2> poolSizes;
  poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  vk::DescriptorPoolCreateInfo poolInfo;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

  descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void BaseRenderer::createCommandBuffers() {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  mainCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void BaseRenderer::createComputeCommandBuffers() {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = 1;

  auto computeCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
  computeCommandBuffer = std::move(computeCommandBuffers[0]);
}

void BaseRenderer::createSyncObjects() {
  constexpr vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo;
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    imageAvailableSemaphores[i] = vk::raii::Semaphore(device, semaphoreInfo);
    renderFinishedSemaphores[i] = vk::raii::Semaphore(device, semaphoreInfo);
    inFlightFences[i] = vk::raii::Fence(device, fenceInfo);
  }
  computeFinishedSemaphore = vk::raii::Semaphore(device, semaphoreInfo);
  computeFence = vk::raii::Fence(device, fenceInfo);
}

void BaseRenderer::draw() {
  glfwPollEvents();
  drawFrame();
  manageFps();
  printFps();
}

void BaseRenderer::manageFps() {
  static double frameStartTime = 0;

  double frameEndTime = glfwGetTime();
  const double frameTime = frameEndTime - frameStartTime;

  if (frameTime < frameTimeS) {
    std::this_thread::sleep_for(std::chrono::duration<double>(frameTimeS - frameTime));
    frameEndTime = glfwGetTime();
  }

  frameStartTime = frameEndTime;
}

void BaseRenderer::printFps() {
  static double lastFpsCountTime = 0.0f;
  static int fpsCount = 0;
  const double currentTime = glfwGetTime();

  fpsCount++;
  if (currentTime - lastFpsCountTime >= 1.0) {
    std::cout << "FPS: " << fpsCount << std::endl;

    lastFpsCountTime = currentTime;
    fpsCount = 0;
  }
}

void BaseRenderer::drawFrame() {
  vk::SubmitInfo submitInfo;

  updateUniformBuffer(currentFrame);

  // Compute submission
  waitForFence(*computeFence);

  device.resetFences(*computeFence);

  computeCommandBuffer.reset();
  recordComputeCommandBuffer(*computeCommandBuffer);

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*computeCommandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*computeFinishedSemaphore;

  computeQueue.submit(submitInfo, *computeFence);

  // Graphics submission
  waitForFence(*inFlightFences[currentFrame]);

  auto [result, imageIndex] =
      swapChain.acquireNextImage(FENCE_TIMEOUT, *imageAvailableSemaphores[currentFrame]);
  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
    throw VulkanDrawingError("failed to acquire swap chain image!");
  }

  device.resetFences(*inFlightFences[currentFrame]);

  mainCommandBuffers[currentFrame].reset();
  recordCommandBuffer(*mainCommandBuffers[currentFrame], imageIndex);

  const std::vector waitSemaphores = {*computeFinishedSemaphore,
                                      *imageAvailableSemaphores[currentFrame]};
  const std::vector<vk::PipelineStageFlags> waitStages = {
      vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo = vk::SubmitInfo{};

  submitInfo.waitSemaphoreCount = static_cast<int32_t>(waitSemaphores.size());
  submitInfo.pWaitSemaphores = waitSemaphores.data();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*mainCommandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*renderFinishedSemaphores[currentFrame];

  graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

  vk::PresentInfoKHR presentInfo;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &*renderFinishedSemaphores[currentFrame];

  const std::vector swapChains = {*swapChain};
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
void BaseRenderer::waitForFence(vk::Fence fence) const {
  while (vk::Result::eTimeout == device.waitForFences({fence}, vk::True, FENCE_TIMEOUT))
    ;
}

void BaseRenderer::recreateSwapChain() {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  device.waitIdle();

  createSwapChain();
  swapChainImageViews.clear();
  createImageViews();
  createDepthResources();
  swapChainFramebuffers.clear();
  createFramebuffers();
}

void BaseRenderer::recordCommandBuffer(const vk::CommandBuffer commandBuffer,
                                       const uint32_t imageIndex) const {
  constexpr vk::CommandBufferBeginInfo beginInfo;

  commandBuffer.begin(beginInfo);

  vk::RenderPassBeginInfo renderPassInfo;
  renderPassInfo.renderPass = *renderPass;
  renderPassInfo.framebuffer = *swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  std::array<vk::ClearValue, 2> clearValues{};
  clearValues[0].color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
  clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

  vk::Viewport viewport;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
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
    uniformBuffers.emplace_back(
        device, physicalDevice, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
  }
}

void BaseRenderer::updateUniformBuffer(uint32_t currentImage) {
  camera.update();

  UniformBufferObject ubo{};
  ubo.model = glm::mat4(1.0f);

  ubo.view = camera.getViewMatrix();

  ubo.proj = glm::perspective(glm::radians(45.0f),
                              static_cast<float>(swapChainExtent.width) /
                                  static_cast<float>(swapChainExtent.height),
                              0.001f, 256.0f);
  ubo.proj[1][1] *= -1;

  uniformBuffers[currentImage].copyToMemory(&ubo);
}

void BaseRenderer::createDepthResources() {
  const vk::Format depthFormat = findDepthFormat();

  createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eDepthStencilAttachment,
              vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
  depthImageView = createImageView(*depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

vk::Format BaseRenderer::findDepthFormat() const {
  using enum vk::Format;
  return findSupportedFormat({eD32Sfloat, eD32SfloatS8Uint, eD24UnormS8Uint},
                             vk::ImageTiling::eOptimal,
                             vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format BaseRenderer::findSupportedFormat(const std::vector<vk::Format> &candidates,
                                             vk::ImageTiling tiling,
                                             vk::FormatFeatureFlags features) const {
  for (const vk::Format format : candidates) {
    vk::FormatProperties props = physicalDevice.getFormatProperties(format);

    if ((tiling == vk::ImageTiling::eLinear &&
         (props.linearTilingFeatures & features) == features) ||
        (tiling == vk::ImageTiling::eOptimal &&
         (props.optimalTilingFeatures & features) == features)) {
      return format;
    }
  }

  throw VulkanInitializationError("failed to find supported format!");
}

void BaseRenderer::createImage(uint32_t width, uint32_t height, vk::Format format,
                               vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                               vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                               vk::raii::DeviceMemory &imageMemory) const {
  vk::ImageCreateInfo imageInfo;
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.usage = usage;

  createImage(imageInfo, properties, image, imageMemory);
}

void BaseRenderer::createImage(const vk::ImageCreateInfo &imageInfo,
                               const vk::MemoryPropertyFlags &properties, vk::raii::Image &image,
                               vk::raii::DeviceMemory &imageMemory) const {
  image = vk::raii::Image(device, imageInfo);

  const vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      Buffer::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

  imageMemory = vk::raii::DeviceMemory(device, allocInfo);
  image.bindMemory(*imageMemory, 0);
}

vk::raii::ImageView BaseRenderer::createImageView(vk::Image image, vk::Format format,
                                                  vk::ImageAspectFlags aspectFlags) {
  vk::ImageViewCreateInfo viewInfo;
  viewInfo.image = image;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  return {device, viewInfo};
}

void BaseRenderer::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout,
                                         vk::ImageLayout newLayout) const {
  const vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

  vk::ImageMemoryBarrier barrier;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  barrier.srcAccessMask = accessFlagsForLayout(oldLayout);
  barrier.dstAccessMask = accessFlagsForLayout(newLayout);
  const vk::PipelineStageFlags sourceStage = pipelineStageForLayout(oldLayout);
  const vk::PipelineStageFlags destinationStage = pipelineStageForLayout(newLayout);

  commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlagBits::eByRegion,
                                nullptr, nullptr, barrier);

  endSingleTimeCommands(*commandBuffer);
}

vk::raii::CommandBuffer BaseRenderer::beginSingleTimeCommands() const {
  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = *commandPool;
  allocInfo.commandBufferCount = 1;

  vk::raii::CommandBuffers commandBuffers{device, allocInfo};

  vk::CommandBufferBeginInfo beginInfo;
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  vk::raii::CommandBuffer commandBuffer(std::move(commandBuffers[0]));
  commandBuffer.begin(beginInfo);

  return commandBuffer;
}

void BaseRenderer::endSingleTimeCommands(vk::CommandBuffer commandBuffer) const {
  commandBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  graphicsQueue.submit(submitInfo);
  graphicsQueue.waitIdle();
}

void BaseRenderer::saveScreenshot(const char *filename) const {
  bool supportsBlit = true;

  // Check blit support for source and destination

  // Get format properties for the swapchain color format
  // Check if the device supports blitting from optimal images (the swapchain images are in optimal
  // format)
  vk::FormatProperties formatProps;
  formatProps = physicalDevice.getFormatProperties(swapChainImageFormat);
  if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc)) {
    supportsBlit = false;
  }

  // Check if the device supports blitting to linear images
  formatProps = physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
  if (!(formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst)) {
    supportsBlit = false;
  }

  // Source for the copy is the last rendered swapchain image
  // The current frame is not in use yet, so we can use it here
  auto srcImage = swapChainImages[currentFrame];

  // Create the linear tiled destination image to copy to and to read the memory from
  vk::ImageCreateInfo imgCreateInfo;
  imgCreateInfo.imageType = vk::ImageType::e2D;
  // Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color
  // format would differ
  imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
  imgCreateInfo.extent.width = windowSize.width;
  imgCreateInfo.extent.height = windowSize.height;
  imgCreateInfo.extent.depth = 1;
  imgCreateInfo.arrayLayers = 1;
  imgCreateInfo.mipLevels = 1;
  imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
  imgCreateInfo.tiling = vk::ImageTiling::eLinear;
  imgCreateInfo.usage = vk::ImageUsageFlagBits::eTransferDst;
  // Create the image
  // Memory must be host visible to copy from
  vk::raii::Image dstImage = nullptr;
  vk::raii::DeviceMemory dstImageMemory = nullptr;
  createImage(imgCreateInfo,
              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
              dstImage, dstImageMemory);

  // Do the actual blit from the swapchain image to our host visible destination image
  // Transition destination image to transfer destination layout
  transitionImageLayout(*dstImage, vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);
  // Transition swapchain image from present to transfer source layout
  transitionImageLayout(srcImage, vk::ImageLayout::ePresentSrcKHR,
                        vk::ImageLayout::eTransferSrcOptimal);

  const vk::raii::CommandBuffer &commandBuffer = beginSingleTimeCommands();
  // If source and destination support blit we'll blit as this also does automatic format conversion
  // (e.g. from BGR to RGB)
  if (supportsBlit) {
    // Define the region to blit (we will blit the whole swapchain image)
    vk::Offset3D blitSize;
    blitSize.x = static_cast<int32_t>(windowSize.width);
    blitSize.y = static_cast<int32_t>(windowSize.height);
    blitSize.z = 1;
    vk::ImageBlit imageBlitRegion;
    imageBlitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSize;

    // Issue the blit command
    commandBuffer.blitImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, *dstImage,
                            vk::ImageLayout::eTransferDstOptimal, imageBlitRegion,
                            vk::Filter::eNearest);
  } else {
    throw NotImplementedError("only blit support should be necessary");
  }
  endSingleTimeCommands(*commandBuffer);

  // Transition destination image to general layout, which is the required layout for mapping the
  // image memory later on
  transitionImageLayout(*dstImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
  // Transition back the swap chain image after the blit is done
  transitionImageLayout(srcImage, vk::ImageLayout::eTransferSrcOptimal,
                        vk::ImageLayout::ePresentSrcKHR);

  // Get layout of the image (including row pitch)
  vk::SubresourceLayout subResourceLayout =
      dstImage.getSubresourceLayout({vk::ImageAspectFlagBits::eColor});

  // Map image memory so we can start copying from it
  auto *data = static_cast<char *>(dstImageMemory.mapMemory(0, vk::WholeSize));
  data += subResourceLayout.offset;

  std::ofstream file(filename, std::ios::out | std::ios::binary);

  // ppm header
  file << "P6\n" << windowSize.width << "\n" << windowSize.height << "\n" << 255 << "\n";

  // ppm binary pixel data
  for (uint32_t y = 0; y < windowSize.height; y++) {
    auto *row = reinterpret_cast<unsigned int *>(data);
    for (uint32_t x = 0; x < windowSize.width; x++) {
      file.write(reinterpret_cast<char *>(row), 3);
      row++;
    }
    data += subResourceLayout.rowPitch;
  }
  file.close();

  std::cout << "Screenshot saved to disk" << std::endl;
}

inline vk::AccessFlags BaseRenderer::accessFlagsForLayout(vk::ImageLayout layout) {
  switch (layout) {
    using enum vk::AccessFlagBits;
    using enum vk::ImageLayout;
  case ePreinitialized:
    return eHostWrite;
  case eTransferDstOptimal:
    return eTransferWrite;
  case eTransferSrcOptimal:
    return eTransferRead;
  case eColorAttachmentOptimal:
    return eColorAttachmentWrite;
  case eDepthStencilAttachmentOptimal:
    return eDepthStencilAttachmentWrite;
  case eShaderReadOnlyOptimal:
    return eShaderRead;
  default:
    return {};
  }
}

inline vk::PipelineStageFlags BaseRenderer::pipelineStageForLayout(vk::ImageLayout layout) {
  switch (layout) {
    using enum vk::PipelineStageFlagBits;
    using enum vk::ImageLayout;
  case eTransferDstOptimal:
  case eTransferSrcOptimal:
    return eTransfer;
  case eColorAttachmentOptimal:
    return eColorAttachmentOutput;
  case eDepthStencilAttachmentOptimal:
    return eEarlyFragmentTests;
  case eShaderReadOnlyOptimal:
    return eFragmentShader;
  case ePreinitialized:
    return eHost;
  case eUndefined:
    return eTopOfPipe;
  default:
    return eBottomOfPipe;
  }
}

void BaseRenderer::mouseMoveHandler(GLFWwindow *window, double posx, double posy) {
  const auto renderer = static_cast<BaseRenderer *>(glfwGetWindowUserPointer(window));
  renderer->mouseMoved(glm::vec2(posx, posy));
}

void BaseRenderer::mouseMoved(const glm::vec2 &newPos) {
  const glm::vec2 deltaPos = mousePos - newPos;
  mousePos = newPos;
  if (mouseButtons.left) {
    if (deltaPos == glm::vec2()) {
      return;
    }

    camera.rotate(deltaPos.x, deltaPos.y);
  }
}

void BaseRenderer::keyboardHandler(GLFWwindow *window, int key, [[maybe_unused]] int scancode,
                                   int action, [[maybe_unused]] int mods) {
  const auto renderer = static_cast<BaseRenderer *>(glfwGetWindowUserPointer(window));
  switch (action) {
  case GLFW_PRESS:
    renderer->keyPressed(key);
    break;

  case GLFW_RELEASE:
    renderer->keyReleased(key);
    break;

  default:
    break;
  }
}

void BaseRenderer::keyPressed(int key) { handleCameraKeys(key, true); }
void BaseRenderer::keyReleased(int key) {
  handleCameraKeys(key, false);
  if (key == GLFW_KEY_P) {
    camera.printDebug();
  }
}
void BaseRenderer::handleCameraKeys(int key, bool pressed) {
  switch (key) {
  case GLFW_KEY_W:
    camera.keys.forward = pressed;
    break;
  case GLFW_KEY_S:
    camera.keys.backward = pressed;
    break;
  case GLFW_KEY_A:
    camera.keys.left = pressed;
    break;
  case GLFW_KEY_D:
    camera.keys.right = pressed;
    break;
  case GLFW_KEY_LEFT_SHIFT:
    camera.keys.up = pressed;
    break;
  case GLFW_KEY_LEFT_CONTROL:
    camera.keys.down = pressed;
    break;
  default:
    break;
  }
}

void BaseRenderer::mouseHandler(GLFWwindow *window, int button, int action, int mods) {
  const auto renderer = static_cast<BaseRenderer *>(glfwGetWindowUserPointer(window));
  renderer->mouseAction(button, action, mods);
}

void BaseRenderer::mouseAction(int button, int action, int) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    mouseButtons.left = action == GLFW_PRESS;
  }
}

VkBool32 BaseRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                     const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                     void *) {
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
