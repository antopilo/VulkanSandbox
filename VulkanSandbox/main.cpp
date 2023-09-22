#include "Logger/Logger.h"

#include "VulkanEngine.h"

int main(int argc, char* argv)
{
	VulkanEngine vulkanEngine;
	vulkanEngine.Init();

	vulkanEngine.Draw();
	
	vulkanEngine.Cleanup();
}