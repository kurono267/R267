#include "device.hpp"

using namespace r267;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHX_MULTIVIEW_EXTENSION_NAME
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

	vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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
	deviceFeatures.samplerAnisotropy = true;
	deviceFeatures.tessellationShader = true;
	deviceFeatures.multiViewport = true;

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
	_computeQueue  = _device.getQueue(indices.computeFamily,0);
}

void Device::create(const vk::Instance& instance,const vk::SurfaceKHR& surface,const glm::ivec2& size){
	_instance = instance;_surface = surface;_size = size;
	pickPhysicalDevice();
	createLogicalDevice();
	_swapchain = std::make_shared<Swapchain>();
	_swapchain->create(_pDevice,_device,_surface,size);

	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,queueFamiliesIndices().graphicsFamily);
	_pool = _device.createCommandPool(poolInfo); 
	vk::CommandPoolCreateInfo poolComputeInfo(vk::CommandPoolCreateFlags(),queueFamiliesIndices().computeFamily);
	_poolCompute = _device.createCommandPool(poolComputeInfo); 
}

vk::Semaphore Device::createSemaphore(vk::SemaphoreCreateInfo info){
	auto result = _device.createSemaphore(vk::SemaphoreCreateInfo());
	_semaphores.push_back(result);
	return result;
}

vk::CommandPool Device::getCommandPool(const bool isCompute){
	if(!isCompute)return _pool;
	else return _poolCompute;
}

void Device::release(){
	_swapchain->release();
	for(auto s : _semaphores){ // Release Semaphores
		_device.destroySemaphore(s);
	}
	// Release Managed object
	for(auto m : _release){
		m(shared_from_this());
	}
	_device.destroyCommandPool(_pool);
	_device.destroyCommandPool(_poolCompute);
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

vk::Queue  Device::getComputeQueue(){
	return _computeQueue;
}

void Device::print(){
	vk::PhysicalDeviceProperties pdProp = _pDevice.getProperties();
	std::cout << "Selected Device " <<  pdProp.deviceName << std::endl;
}

vk::Format Device::supportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features){
	for (vk::Format format : candidates) {
		vk::FormatProperties props = _pDevice.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

vk::Format Device::depthFormat(){
	return supportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}
