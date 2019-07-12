#ifndef EXPORTED_VULKAN_FUNCTION
#define EXPORTED_VULKAN_FUNCTION(function)
#endif

EXPORTED_VULKAN_FUNCTION (vkGetInstanceProcAddr)

#undef EXPORTED_VULKAN_FUNCTION

#ifndef GLOBAL_LEVEL_VULKAN_FUNCTION
#define GLOBAL_LEVEL_VULKAN_FUNCTION(function)
#endif

GLOBAL_LEVEL_VULKAN_FUNCTION (vkEnumerateInstanceVersion)
GLOBAL_LEVEL_VULKAN_FUNCTION (vkEnumerateInstanceExtensionProperties)
GLOBAL_LEVEL_VULKAN_FUNCTION (vkEnumerateInstanceLayerProperties)
GLOBAL_LEVEL_VULKAN_FUNCTION (vkCreateInstance)

#undef GLOBAL_LEVEL_VULKAN_FUNCTION

#ifndef INSTANCE_LEVEL_VULKAN_FUNCTION
#define INSTANCE_LEVEL_VULKAN_FUNCTION(function)
#endif

INSTANCE_LEVEL_VULKAN_FUNCTION (vkEnumeratePhysicalDevices)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkGetPhysicalDeviceProperties)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkGetPhysicalDeviceFeatures)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkGetPhysicalDeviceQueueFamilyProperties)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkCreateDevice)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkGetDeviceProcAddr)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkDestroyInstance)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkEnumerateDeviceLayerProperties)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkEnumerateDeviceExtensionProperties)
INSTANCE_LEVEL_VULKAN_FUNCTION (vkGetPhysicalDeviceMemoryProperties)

#undef INSTANCE_LEVEL_VULKAN_FUNCTION

#ifndef INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(function, extension)
#endif

INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkCreateDebugUtilsMessengerEXT, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceSurfaceSupportKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceSurfacePresentModesKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VK_KHR_SURFACE_EXTENSION_NAME)
INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceSurfaceFormatsKHR, VK_KHR_SURFACE_EXTENSION_NAME)

#undef INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION

#ifndef PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
#define PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION(function, extension)
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceXlibPresentationSupportKHR,
	 VK_KHR_XLIB_SURFACE_EXTENSION_NAME)
PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
	(vkCreateXlibSurfaceKHR,
	 VK_KHR_XLIB_SURFACE_EXTENSION_NAME)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceWin32PresentationSupportKHR,
	 VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
	(vkCreateWin32SurfaceKHR,
	 VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetPhysicalDeviceXcbPresentationSupportKHR,
	 VK_KHR_XCB_SURFACE_EXTENSION_NAME)
PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION
	(vkCreateXcbSurfaceKHR,
	 VK_KHR_XCB_SURFACE_EXTENSION_NAME)
#endif

#undef PRESENTATION_VULKAN_FUNCTION_FROM_EXTENSION

#ifndef DEVICE_LEVEL_VULKAN_FUNCTION
#define DEVICE_LEVEL_VULKAN_FUNCTION(function)
#endif

DEVICE_LEVEL_VULKAN_FUNCTION (vkDestroyDevice)
DEVICE_LEVEL_VULKAN_FUNCTION (vkGetDeviceQueue)

#undef DEVICE_LEVEL_VULKAN_FUNCTION

#ifndef DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(function, extension)
#endif

DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkCreateSwapchainKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkDestroySwapchainKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
	(vkGetSwapchainImagesKHR, VK_KHR_SWAPCHAIN_EXTENSION_NAME)

#undef DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
