#include "additional.hpp"

using namespace r267;

std::vector<vk::Framebuffer> r267::createFrameBuffers(spDevice device,spPipeline pipeline){
	std::vector<vk::Framebuffer> framebuffers;

	auto swapchain = device->getSwapchain();
	auto imageViews = swapchain->getImageViews();
	auto extent = swapchain->getExtent();
	auto vk_device = device->getDevice();

	for(int i = 0;i<imageViews.size();++i){
		vk::ImageView attachments[] = {imageViews[i]};

		vk::FramebufferCreateInfo framebufferInfo(
			vk::FramebufferCreateFlags(), // Default
			pipeline->getRenderPass(), // Current render pass
			1, attachments, // Attachments
			extent.width, // Width
			extent.height, // Height
			1 // Layers
		);
		framebuffers.push_back(vk_device.createFramebuffer(framebufferInfo));
	}
	return framebuffers;
}
