#include "instance.hpp"

#define ENGINE_NAME "R267"

using namespace r267;

void Instance::init(const std::string& title, GLFWwindow* window, const glm::ivec2& size){
	_size = size;
	_title = title;
	
	createInstance();
	setupDebugCallback();
	createSurface(window);

	_device.create(_instance,_surface,size);
}

Device& Instance::device(){
	return _device;
}

void Instance::release(){
	_instance.destroyDebugReportCallbackEXT(_callback,nullptr);
	_instance.destroy(nullptr); // Destroy instance
}

void Instance::createSurface(GLFWwindow* window){
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(_instance, window, nullptr, &surface) != VK_SUCCESS){
		throw std::runtime_error("failed to create window surface!");
	}
	_surface = surface;
}

VkResult vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

// Create instance
void Instance::createInstance() {
    vk::ApplicationInfo appInfo(_title.c_str(),VK_MAKE_VERSION(1, 0, 0),ENGINE_NAME,VK_MAKE_VERSION(1, 0, 0),VK_API_VERSION_1_0);

    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);

    auto extensions = getRequiredExtensions();

    createInfo.setEnabledExtensionCount(extensions.size());
    createInfo.setPpEnabledExtensionNames(extensions.data());

	if (enableValidationLayers) {
		createInfo.setEnabledLayerCount(validationLayers.size());
		createInfo.setPpEnabledLayerNames(validationLayers.data());
	} else {
		createInfo.setEnabledLayerCount(0);
	}

    _instance = vk::createInstance(createInfo);
}

// Validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

bool Instance::checkValidationLayerSupport() {
	auto availableLayers = vk::enumerateInstanceLayerProperties();

	for (const auto& layer : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layer, layerProperties.layerName) == 0) {
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

void Instance::setupDebugCallback() {
	if (!enableValidationLayers) return;

	vk::DebugReportCallbackCreateInfoEXT createInfo;
	createInfo.setFlags(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning);
	createInfo.setPfnCallback(debugCallback);

	_callback = _instance.createDebugReportCallbackEXT(createInfo,nullptr);
}

std::vector<const char*> Instance::getRequiredExtensions() {
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}
