#include "VulkanEngine.h"
#include "VkBoostrap/VkBootstrap.h"

#include <GLFW/glfw3.h>
#include "Logger/Logger.h"
#include "VkBoostrap/VkInitializers.h"

void VulkanEngine::Init()
{
	if (!glfwInit())
	{
		Logger::Log("Failed to initialize GLFW");
		return;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_GLFWWindow = glfwCreateWindow(m_WindowWidth, m_WindowHeight, WINDOW_TITLE.c_str(), NULL, NULL);

	if(!m_GLFWWindow)
	{
		Logger::Log("Failed to create window");
		return;
	}

	glfwMakeContextCurrent(m_GLFWWindow);

	InitVK();
	InitSwapchain();
	InitCommands();
	InitSyncStructure();

	m_IsInit = true;
}

void VulkanEngine::InitVK()
{
	vkb::InstanceBuilder builder;

	auto instanceReturn = builder.set_app_name("VulkanSandbox")
		.request_validation_layers(true)
		.require_api_version(1, 2, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkbInst = instanceReturn.value();

	// Store instance & debug messenger
	m_VKInstance = vkbInst.instance;
	m_DebugMessenger = vkbInst.debug_messenger;

	// Create surface of window that allows us to draw to
	VkResult err = glfwCreateWindowSurface(m_VKInstance, m_GLFWWindow, NULL, &m_Surface);
	if (err)
	{
		Logger::Log("Window surface creation failed");
		return;
	}

	// Select GPU
	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(m_Surface)
		.add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
		.select()
		.value();


	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	m_Device = vkbDevice.device;
	m_GPU = physicalDevice.physical_device;

	m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::InitSwapchain()
{
	// NOTE: We need to rebuild the swapchain if the image is resized.
	vkb::SwapchainBuilder swapchainBuilder{ m_GPU, m_Device, m_Surface};

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(m_WindowWidth, m_WindowHeight)
		.build()
		.value();

	m_Swapchain = vkbSwapchain.swapchain;
	m_SwapchainImages = vkbSwapchain.get_images().value();
	m_SwapchainImageViews = vkbSwapchain.get_image_views().value();

	m_SwapchainImageFormat = vkbSwapchain.image_format;
}

void VulkanEngine::InitCommands()
{
	// Create a command pool for graphics queue
	VkCommandPoolCreateInfo commandPoolInfo = CommandPoolCreateInfo(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_CommandPool);

	// Create command buffer now
	VkCommandBufferAllocateInfo cmdAllocInfo = CommandBufferAllocateInfo(m_CommandPool, 1);
	vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_MainCommandBuffer);
}

void VulkanEngine::InitSyncStructure()
{
	// Create fence
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;

	//we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_RenderFence);

	// Create semapore
	//for the semaphores we don't need any flags
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_PresentSemaphore);
	vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderSemaphore);
}

void VulkanEngine::Draw()
{
	while (!glfwWindowShouldClose(m_GLFWWindow))
	{
		vkWaitForFences(m_Device, 1, &m_RenderFence, true, 1000000000);
		vkResetFences(m_Device, 1, &m_RenderFence);

		uint32_t swapchainImageIndex;
		vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, m_PresentSemaphore, nullptr, &swapchainImageIndex);

		vkResetCommandBuffer(m_MainCommandBuffer, 0);

		// Now that the command buffer is reset and empty, we can submit commands to it
		VkCommandBuffer cmd = m_MainCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = nullptr;
		cmdBeginInfo.pInheritanceInfo = nullptr;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &cmdBeginInfo);

		VkImageMemoryBarrier image_memory_barrierBegin{};
		image_memory_barrierBegin.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrierBegin.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_memory_barrierBegin.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrierBegin.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_memory_barrierBegin.image = m_SwapchainImages[swapchainImageIndex];
		image_memory_barrierBegin.subresourceRange = {};
		image_memory_barrierBegin.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrierBegin.subresourceRange.baseMipLevel = 0;
		image_memory_barrierBegin.subresourceRange.levelCount = 1;
		image_memory_barrierBegin.subresourceRange.baseArrayLayer = 0;
		image_memory_barrierBegin.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // srcStageMask
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
			0,
			0,
			nullptr,
			0,
			nullptr,
			1, // imageMemoryBarrierCount
			&image_memory_barrierBegin // pImageMemoryBarriers
		);


		VkClearValue clearValue;
		float flash = abs(sin(m_FrameNumber / 120.f));
		clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

		VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachmentInfo.imageView = m_SwapchainImageViews[swapchainImageIndex];
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue = clearValue;

		VkRenderingInfoKHR renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = VkRect2D{ {0, 0}, {1, 1} };
		renderingInfo.pNext = nullptr;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;

		vkCmdBeginRendering(cmd, &renderingInfo);



		vkCmdEndRendering(cmd);

		VkImageMemoryBarrier image_memory_barrier{};
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image_memory_barrier.image = m_SwapchainImages[swapchainImageIndex];
		image_memory_barrier.subresourceRange = {};
		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
			

		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
			0, // dstStageMask
			0,
			0,
			nullptr,
			0,
			nullptr,
			1, // imageMemoryBarrierCount
			&image_memory_barrier // pImageMemoryBarriers
		);


		vkEndCommandBuffer(cmd);


		// Submit to GPU
		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = nullptr;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		submit.pWaitDstStageMask = &waitStage;

		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &m_PresentSemaphore;

		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &m_RenderSemaphore;

		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;

		vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_RenderFence);

		// Present image to window
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;

		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &m_RenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);

		m_FrameNumber++;
	}
	
}

void VulkanEngine::Cleanup()
{
	if (!m_IsInit)
	{
		return;
	}

	vkDestroyCommandPool(m_Device, m_CommandPool, 0);

	vkDestroySwapchainKHR(m_Device, m_Swapchain, 0);

	for (uint32_t i = 0; i < m_SwapchainImageViews.size(); i++)
	{
		vkDestroyImageView(m_Device, m_SwapchainImageViews[i], 0);
	}

	vkDestroyDevice(m_Device, 0);
	vkDestroySurfaceKHR(m_VKInstance, m_Surface, 0);
	vkb::destroy_debug_utils_messenger(m_VKInstance, m_DebugMessenger);
	vkDestroyInstance(m_VKInstance, nullptr);

	glfwDestroyWindow(m_GLFWWindow);
}