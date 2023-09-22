#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>


class GLFWwindow;

class VulkanEngine {
private:
	const std::string& WINDOW_TITLE = "Vulkan Sandbox";

	bool m_IsInit = false;

	GLFWwindow* m_GLFWWindow;

	uint32_t m_WindowWidth = 1280;
	uint32_t m_WindowHeight = 720;
	uint32_t m_FrameNumber = 0;

public:
	VkInstance m_VKInstance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkPhysicalDevice m_GPU;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface;

	// Swapchain
	VkSwapchainKHR m_Swapchain;
	VkFormat m_SwapchainImageFormat;
	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily;
	VkCommandPool m_CommandPool;
	VkCommandBuffer m_MainCommandBuffer;

	VkSemaphore m_PresentSemaphore;
	VkSemaphore m_RenderSemaphore;
	VkFence m_RenderFence;

public:
	void Init();
	void Draw();
	void Cleanup();

private:
	void InitVK();
	void InitSwapchain();
	void InitCommands();

	void InitSyncStructure();
};