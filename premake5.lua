workspace "VulkanSandbox"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "VulkanSandbox"
    location "VulkanSandbox"
    kind "ConsoleApp"
    language "C++"
    
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	debugdir ("%{prj.name}")

    files
    {
        "%{prj.name}/**.h",
        "%{prj.name}/**.cpp"
    }

    includedirs
    {
        "Dependencies/glfw/include",
		"Dependencies/glm",
		"Dependencies/Vulkan/Include/"
    }
    
    libdirs 
    { 
		"Dependencies/glfw/lib",
		"Dependencies/Vulkan/Lib"
    }

    links
    {
		"vulkan-1.lib",
		"glfw3_mt.lib"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"