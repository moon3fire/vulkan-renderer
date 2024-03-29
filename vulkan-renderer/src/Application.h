#pragma once

#define GLFW_INCLUDE_VULKAN // includes vulkan.h
#include <GLFW/include/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
//std
#include <stdexcept>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

//Graphics specific
struct GlobalUBO
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	bool operator==(const Vertex& other) const
	{
		return position == other.position && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

//Vulkan specific
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transferFamily;

	bool isComplete() const { return transferFamily.has_value() && graphicsFamily.has_value() && presentFamily.has_value(); }
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
	void recreateSwapChain();
	void cleanupSwapChain();
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createImageViews();
	void createRenderPass();

	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();

	void createGraphicsPipeline();
	void createFramebuffers();
	
	void createTextureImage();
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags, VkImage& image, VkDeviceMemory& imageMemory);
	void createTextureImageView();
	void createTextureSampler();

	void createCommandPools();
	void createCommandBuffers();

	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void drawFrame();
	//synchronization
	void createSyncObjects();

	void generateMipmaps(VkImage image, VkFormat format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

	void createColorResources();

	VkShaderModule createShaderModule(const std::vector<char>& bytecode);

	VkSampleCountFlagBits getMaxUsableSampleCount();

	// this function is simply adding debug utils extension if debug mode is enabled
	std::vector<const char*> getRequiredExtensions();
	//very enthusiastic ones
	bool CheckWindowExtensionsMatchVulkanExtensions(const std::vector<const char*> glfwExts, const std::vector<VkExtensionProperties>& vulkanExts);
	
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

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

	//helper functions
	static std::vector<char> readFile(const std::string& filename);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	void createDepthResources();
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
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
	
	VkQueue m_graphicsQueue, m_presentQueue, m_transferQueue;


	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;
	VkPipeline m_graphicsPipeline;
	std::vector<VkFramebuffer> m_swapChainFramebuffers;

	VkCommandPool m_commandPool, m_transferCommandPool; // TODO: add this one , m_temporaryOperationsCommandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	//synchronization
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	bool m_framebufferResized = false;

	VkSwapchainKHR m_swapChain;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t m_currentFrame = 0;

	//rendering
	VkSampleCountFlagBits m_msaaSamples;

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	std::vector<void*> m_mappedUniformBuffersMemory;

	uint32_t m_mipLevels;
	VkImage m_textureImage;
	VkDeviceMemory m_textureImageMemory;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;

	VkImage m_colorImage;
	VkDeviceMemory m_colorImageMemory;
	VkImageView m_colorImageView;

	/*
	const std::vector<Vertex> m_vertices =
	{
		{ {-0.5f, -0.5f, 0.0f}, {0.1f, 0.9f, 0.2f},   {1.0f, 0.0f} },
		{ { 0.5f, -0.5f, 0.0f}, {0.2f, 0.7f, 0.3f},   {0.0f, 0.0f} },
		{ { 0.5f,  0.5f, 0.0f}, {0.32f, 0.8f, 0.21f}, {0.0f, 1.0f} },
		{ {-0.5f,  0.5f, 0.0f}, {0.3f, 1.0f, 0.2f},   {1.0f, 1.0f} },

		{ {-0.5f, -0.5f, -0.5f}, {0.1f, 0.9f, 0.2f},   {1.0f, 0.0f} },
		{ { 0.5f, -0.5f, -0.5f}, {0.2f, 0.7f, 0.3f},   {0.0f, 0.0f} },
		{ { 0.5f,  0.5f, -0.5f}, {0.32f, 0.8f, 0.21f}, {0.0f, 1.0f} },
		{ {-0.5f,  0.5f, -0.5f}, {0.3f, 1.0f, 0.2f},   {1.0f, 1.0f} }
	};

	const std::vector<uint32_t> m_indices = 
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};
	*/ // Saved for future reference

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	
	const std::string m_modelPath = "textures/obj/viking_room.obj";
	const std::string m_modelTexturePath = "textures/lain.jpg";
};