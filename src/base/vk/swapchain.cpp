#include "swapchain.hpp"

using namespace r267;

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
		return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,const glm::ivec2& size) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		vk::Extent2D actualExtent(size.x, size.y);

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		
		return actualExtent;
	}
}

void Swapchain::create(const vk::PhysicalDevice& pDevice,const vk::Device& device,const vk::SurfaceKHR& surface,const glm::ivec2& size){
	_device = device;
	SwapchainSupportDetails swapChainSupport = swapchainSupport(pDevice,surface);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities,size);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.setSurface(surface);

	createInfo.setMinImageCount(imageCount);
	createInfo.setImageFormat(surfaceFormat.format);
	createInfo.setImageColorSpace(surfaceFormat.colorSpace);
	createInfo.setImageExtent(extent);
	createInfo.setImageArrayLayers(1);
	createInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

	QueueFamilyIndices indices = queueFamilies(pDevice,surface);
	uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
		createInfo.setQueueFamilyIndexCount(2);
		createInfo.setPQueueFamilyIndices(queueFamilyIndices);
	} else {
		createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
	}

	createInfo.setPreTransform(swapChainSupport.capabilities.currentTransform);
	createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
	createInfo.setPresentMode(presentMode);
	createInfo.setClipped(VK_TRUE);

	//createInfo.setOldSwapchain(SwapchainKHR());

	_swapchain = device.createSwapchainKHR(createInfo);
	_images = device.getSwapchainImagesKHR(_swapchain);
	_imageFormat = surfaceFormat.format;
	_extent = extent;

	createImageViews(device);
}

void Swapchain::createImageViews(const vk::Device& device){
	_imageViews.resize(_images.size());

	for (uint i = 0; i < _images.size(); i++) {
		vk::ImageViewCreateInfo createInfo(
			vk::ImageViewCreateFlags(),
			_images[i],
			vk::ImageViewType::e2D,
			_imageFormat,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0, 1, 0, 1)
		);

		_imageViews[i] = device.createImageView(createInfo);
	}
}

void Swapchain::release(){
	for (uint i = 0; i < _images.size(); i++) {
		_device.destroyImageView(_imageViews[i]);
	}
	_device.destroySwapchainKHR(_swapchain);
}

vk::SwapchainKHR Swapchain::getSwapchain() const {return _swapchain;}
std::vector<vk::Image> Swapchain::getImages() const {return _images;}
vk::Format    Swapchain::getFormat() const {return _imageFormat;}
vk::Extent2D  Swapchain::getExtent() const {return _extent;}
std::vector<vk::ImageView> Swapchain::getImageViews() const {return _imageViews;}
