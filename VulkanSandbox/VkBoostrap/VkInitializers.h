#pragma once
#include <Vulkan/vulkan.h>

VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t familyIndex, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);