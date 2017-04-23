#include "device.hpp"

using namespace r267;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void Device::pickPhysicalDevice(){
	auto devices = _instance.enumeratePhysicalDevices();

	if (devices.size() == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	bool haveDevice = false;
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			_pDevice = device;
			haveDevice = true;
			break;
		}
	}

	if (!haveDevice) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	print();
}

QueueFamilyIndices Device::queueFamiliesIndices(){
	return queueFamilies(_pDevice,_surface);
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
	auto availableExtensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool Device::isDeviceSuitable(const vk::PhysicalDevice& device){
	QueueFamilyIndices indices = queueFamilies(device,_surface);

	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if(extensionsSupported){
		SwapchainSupportDetails swapChainSupport = swapchainSupport(device,_surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void Device::createLogicalDevice() {
	QueueFamilyIndices indices = queueFamilies(_pDevice,_surface);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

	float queuePriority = 1.0f;
	for(int queueFamily : uniqueQueueFamilies){
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.setQueueFamilyIndex(queueFamily);
		queueCreateInfo.setQueueCount(1);
		queueCreateInfo.setPQueuePriorities(&queuePriority);
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures;

	vk::DeviceCreateInfo createInfo;

	createInfo.setPQueueCreateInfos(queueCreateInfos.data());
	createInfo.setQueueCreateInfoCount(queueCreateInfos.size());

	createInfo.setPEnabledFeatures(&deviceFeatures);

	createInfo.setPpEnabledExtensionNames(deviceExtensions.data());
	createInfo.setEnabledExtensionCount(deviceExtensions.size());
	
	if (enableValidationLayers) {
		createInfo.setEnabledLayerCount(validationLayers.size());
		createInfo.setPpEnabledLayerNames(validationLayers.data());
	} else {
		createInfo.setEnabledLayerCount(0);
	}

	_device = _pDevice.createDevice(createInfo);
	_presentQueue  = _device.getQueue(indices.presentFamily,0);
	_graphicsQueue  = _device.getQueue(indices.graphicsFamily,0);
}

void Device::create(const vk::Instance& instance,const vk::SurfaceKHR& surface,const glm::ivec2& size){
	_instance = instance;_surface = surface;_size = size;
	pickPhysicalDevice();
	createLogicalDevice();
	_swapchain = std::make_shared<Swapchain>();
	_swapchain->create(_pDevice,_device,_surface,size);
}

void Device::release(){
	_swapchain->release();
	_device.destroy();
}

spSwapchain  Device::getSwapchain(){
	return _swapchain;
}

vk::Device Device::getDevice(){
	return _device;
}

vk::PhysicalDevice Device::getPhysicalDevice(){
	return _pDevice;
}

vk::Queue  Device::getGraphicsQueue(){
	return _graphicsQueue;
}

vk::Queue  Device::getPresentQueue(){
	return _presentQueue;
}

void Device::print(){
	vk::PhysicalDeviceProperties pdProp = _pDevice.getProperties();
	std::cout << "Selected Device " <<  pdProp.deviceName << std::endl;
}
