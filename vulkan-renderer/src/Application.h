#pragma once

#define GLFW_INCLUDE_VULKAN // includes vulkan.h
#include <GLFW/include/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Application
{
public:
	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	//vulkan part

	void createVulkanInstance();
	void setupDebugMessenger();
	void initDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& dmCreateInfo);
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	
	void createSwapChain();
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createImageViews();

	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& bytecode);


	// this function is simply adding debug utils extension if debug mode is enabled
	std::vector<const char*> getRequiredExtensions();
	//very enthusiastic ones
	bool CheckWindowExtensionsMatchVulkanExtensions(const std::vector<const char*> glfwExts, const std::vector<VkExtensionProperties>& vulkanExts);
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* userData);

	static VkResult createDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	static void destroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	static std::vector<char> readFile(const std::string& filename);
private:
	//window
	GLFWwindow* m_window = nullptr;
	uint32_t m_width = 1200, m_height = 800;

	//vulkan
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkInstance m_vulkanInstance;
	VkSurfaceKHR m_surface;
	
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	
	VkQueue m_graphicsQueue, m_presentQueue;

	VkSwapchainKHR m_swapChain;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
};