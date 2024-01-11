#include "Application.h"

// public

void Application::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

// private

void Application::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(m_width, m_height, "Vulkan Renderer", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Application::initVulkan()
{
	createVulkanInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPools();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

void Application::mainLoop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(m_device);
}

void Application::cleanup()
{
	//Vulkan
	cleanupSwapChain();

	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
	vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	vkDestroyRenderPass(m_device, m_renderPass, nullptr);

	//synchronization
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
	
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);

	vkDestroyDevice(m_device, nullptr);

	if (enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(m_vulkanInstance, m_debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
	vkDestroyInstance(m_vulkanInstance, nullptr);

	//GLFW
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::createVulkanInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport())
		throw std::runtime_error("Validation layers requested, but not available");
	//else
	//	std::cout << "Requested validation layers are available" << std::endl;

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pNext = nullptr;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto glfwExtensions = getRequiredExtensions(); // glfw + debug utils if debug enabled extensions

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();
	createInfo.enabledLayerCount = 0;

	// enumerating 2 times to get correct size of the vector and then assign
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	//glfw extensions comparision
	if (!CheckWindowExtensionsMatchVulkanExtensions(glfwExtensions, extensions))
	{
		std::cout << "GLFW window has an extensions which is not available" << std::endl;
	}
	else
		std::cout << "GLFW window needed extensions are available" << std::endl;

	//Debug messenger for specific Vulkan instance creation and deletion
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		initDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// instance creating
	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create an instance");
	}
}

void Application::setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT dmCreateInfo;
	initDebugMessengerCreateInfo(dmCreateInfo);

	if (createDebugUtilsMessengerEXT(m_vulkanInstance, &dmCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to setup debug messenger");
	}
}

void Application::initDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& dmCreateInfo)
{
	dmCreateInfo = {};
	dmCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dmCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	dmCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	dmCreateInfo.pfnUserCallback = debugCallback;
	dmCreateInfo.pUserData = nullptr; // optional, can be set to 'this'
}

void Application::createSurface()
{
	if (glfwCreateWindowSurface(m_vulkanInstance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create window surface");
}

void Application::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			m_physicalDevice = device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable GPU");
}

void Application::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}

	vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);
}

bool Application::isDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//std::cout << deviceProperties.deviceName << std::endl;

	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate &&
		(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
		 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
}

bool Application::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (std::strcmp(layerName, layerProperties.layerName))
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

bool Application::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionsCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	if (!requiredExtensions.empty()) {
		//std::cout << "These extensions aren't supported by the currently selected device" << std::endl;
		/*
		for (const auto& extension : requiredExtensions)
		{
			std::cout << extension << std::endl;
		}
		*/
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			indices.transferFamily = i;

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

		if (presentSupport)
			indices.presentFamily = i;


		if (indices.isComplete())
			break;

		i++;
	}

	return indices;
}

void Application::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapChainFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount; // Should be equal to MAX_FRAMES_IN_FLIGHT
	//std::cout << "Min image count: " << imageCount << std::endl;
	//std::cout << "Max image count: " << swapChainSupport.capabilities.maxImageCount << std::endl;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily && indices.graphicsFamily != indices.transferFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 3;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // optional
		createInfo.pQueueFamilyIndices = nullptr; // optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain");
	}

	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

void Application::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_device);
	
	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createFramebuffers();
}

void Application::cleanupSwapChain()
{
	for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(m_device, m_swapChainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(m_device, m_swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}

SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Application::chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& presentMode : availablePresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Application::createImageViews()
{
	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (int i = 0; i < m_swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views");
		}
	}
}

void Application::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear framebuffer before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store framebuffer after rendering
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // optional
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // optional

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image data layout before render pass starts
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // image data layout after render pass (to change to)

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // index of the attachment description array
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout of the attachment

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // pipeline type
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass before render pass
	dependency.dstSubpass = 0; // our subpass
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // pipeline stage
	dependency.srcAccessMask = 0; // access mask for the dependency
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // pipeline stage
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // access mask for the dependency

	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &m_renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass");
	}
}

void Application::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("shaders/test_vert.spv");
	//std::cout << "Loaded shader test.vert bytecode with size " + vertShaderCode.size() << std::endl;
	auto fragShaderCode = readFile("shaders/test_frag.spv");
	//std::cout << "Loaded shader test.frag bytecode with size " + fragShaderCode.size() << std::endl;
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
	
	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = vertShaderModule;
	vertShaderStageCreateInfo.pName = "main";
	vertShaderStageCreateInfo.pSpecializationInfo = nullptr; // used to set constants in compile time, very useful if needed

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCreateInfo.module = fragShaderModule;
	fragShaderStageCreateInfo.pName = "main";
	fragShaderStageCreateInfo.pSpecializationInfo = nullptr; // same

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1; // binding descriptions(typically means vertex buffer count? :))
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); // setting up like this cuz vertex
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();						     // is array of 2 elements for now, maybe more later

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapChainExtent.width);
	viewport.height = static_cast<float>(m_swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.lineWidth = 1.0f;
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f; // optional
	rasterizationCreateInfo.depthBiasClamp = 0.0f; // optional
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f; // optional

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleCreateInfo.minSampleShading = 1.0f; // optional
	multisampleCreateInfo.pSampleMask = nullptr; // optional
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE; // optional
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE; // optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
										  VK_COLOR_COMPONENT_G_BIT |
										  VK_COLOR_COMPONENT_B_BIT |
										  VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; 		
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; 		
		
	
	// THIS IS USED FOR GLOBAL COLOR BLENDING, CONFIG
	
	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY; // optional
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendCreateInfo.blendConstants[0] = 0.0f; // optional
	colorBlendCreateInfo.blendConstants[1] = 0.0f; // optional
	colorBlendCreateInfo.blendConstants[2] = 0.0f; // optional
	colorBlendCreateInfo.blendConstants[3] = 0.0f; // optional
	

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // optional
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // optional

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr; // optional
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

	pipelineCreateInfo.layout = m_pipelineLayout;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.subpass = 0;

	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // optional
	pipelineCreateInfo.basePipelineIndex = -1; // optional

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
}

void Application::createFramebuffers()
{
	m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

	for (int i = 0; i < m_swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { m_swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = m_swapChainExtent.width;
		framebufferCreateInfo.height = m_swapChainExtent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer");
		}
	}	
}

void Application::createCommandPools()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics command pool");
	}

	VkCommandPoolCreateInfo transferCommandPoolCreateInfo{};
	transferCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transferCommandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();

	if (vkCreateCommandPool(m_device, &transferCommandPoolCreateInfo, nullptr, &m_transferCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create transfer command pool");
	}

}

void Application::createCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

	if (vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; // optional
	commandBufferBeginInfo.pInheritanceInfo = nullptr; // optional

	if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.framebuffer = m_swapChainFramebuffers[imageIndex];

	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_swapChainExtent;

	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapChainExtent.width);
	viewport.height = static_cast<float>(m_swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { m_vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	//if (imageIndex >= m_descriptorSets.size()) {
	//	throw std::out_of_range("Descriptor set index out of range");
	//}

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);
	//vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void Application::drawFrame()
{
	vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
	//std::cout << imageIndex << std::endl;

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	updateUniformBuffer(m_currentFrame);

	vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
	vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
	recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1; // number of semaphores to wait on
	submitInfo.pWaitSemaphores = waitSemaphores; // list of semaphores to wait on
	submitInfo.pWaitDstStageMask = waitStages; // stages to check semaphores

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1; // number of semaphores to signal
	submitInfo.pSignalSemaphores = signalSemaphores; // list of semaphores to signal

	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1; // number of semaphores to wait on
	presentInfo.pWaitSemaphores = signalSemaphores; // list of semaphores to wait on

	VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1; // number of swapchains to present to
	presentInfo.pSwapchains = swapChains; // list of swapchains to present to
	presentInfo.pImageIndices = &imageIndex; // index of images for each swapchain
	presentInfo.pResults = nullptr; // optional

	result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
	{
		m_framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::createSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start with signaled state

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects");
		}
	}
}

void Application::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer, m_vertexBufferMemory);

	copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

VkShaderModule Application::createShaderModule(const std::vector<char>& bytecode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytecode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module");
	}

	return shaderModule;
}

void Application::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_indices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indexBuffer, m_indexBufferMemory);

	copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void Application::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0; // binding number in shader
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1; // number of descriptors, if there would be an array of ubos, this would be > 1

	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // shader stage to bind to
	uboLayoutBinding.pImmutableSamplers = nullptr; // optional

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = 1;
	layoutCreateInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

std::vector<const char*> Application::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = { 0 };
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}


bool Application::CheckWindowExtensionsMatchVulkanExtensions(
	const std::vector<const char*> glfwExts,														 
	const std::vector<VkExtensionProperties>& vulkanExts)
{
	if (glfwExts.size() > vulkanExts.size())
		return false;

	/*
	for (const char* glfwExt : glfwExts)
		std::cout << "GLFW Extension: " << glfwExt << std::endl;
	*/
	
	/*
	for (const VkExtensionProperties& vulkanExt : vulkanExts)
		std::cout << "Vulkan Extension: " << vulkanExt.extensionName << std::endl;
	*/

	for (const char* glfwExt : glfwExts)
	{
		bool extensionFound = false;

		for (const VkExtensionProperties& vulkanExt : vulkanExts)
		{

			if (std::strcmp(glfwExt, vulkanExt.extensionName))
			{
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)
			return false;
	}

	return true;
}

void Application::createDescriptorPool()
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &poolSize;
	descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void Application::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
	
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	descriptorSetAllocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_device, &descriptorSetAllocInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(GlobalUBO);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0; // binding number in shader
		descriptorWrite.dstArrayElement = 0; // index in array of descriptors
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1; // number of descriptors
		descriptorWrite.pBufferInfo = &bufferInfo; // buffer info
		//descriptorWrite.pImageInfo = nullptr; // optional
		//descriptorWrite.pTexelBufferView = nullptr; // optional

		vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
	}

}

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->m_framebufferResized = true;

}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* userData)
{
	std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void Application::updateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	GlobalUBO ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 10.0f);

	ubo.proj[1][1] *= -1; // flip y coordinate
	memcpy(m_mappedUniformBuffersMemory[currentImage], &ubo, sizeof(ubo));
}

void Application::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(GlobalUBO);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	m_mappedUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_uniformBuffers[i], m_uniformBuffersMemory[i]);

		vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_mappedUniformBuffersMemory[i]);
	}
}

VkResult Application::createDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Application::destroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

std::vector<char> Application::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Failed to open file");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

uint32_t Application::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &deviceMemoryProperties);

	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory");
	}

	vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = m_transferCommandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // optional
	copyRegion.dstOffset = 0; // optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_transferQueue); // TODO: think about replacing this with vkWaitForFences, it will allow to schedule multiple transfers

	vkFreeCommandBuffers(m_device, m_transferCommandPool, 1, &commandBuffer);
}

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0; // index of the binding in the array of bindings
	bindingDescription.stride = sizeof(Vertex); // number of bytes from one entry to the next
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // move to the next data entry after each vertex

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

	// position attribute
	attributeDescriptions[0].binding = 0; // index of the binding in the array of bindings
	attributeDescriptions[0].location = 0; // location directive of the input in the vertex shader
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // format of the data
	attributeDescriptions[0].offset = offsetof(Vertex, position); // number of bytes since the start of the per-vertex data to read from

	// color attribute
	attributeDescriptions[1].binding = 0; // index of the binding in the array of bindings
	attributeDescriptions[1].location = 1; // location directive of the input in the vertex shader
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // format of the data
	attributeDescriptions[1].offset = offsetof(Vertex, color); // number of bytes since the start of the per-vertex data to read from

	return attributeDescriptions;
}
