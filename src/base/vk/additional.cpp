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

spImage r267::checkboardTexture(spDevice device,const uint& width, const uint& height, const uint& step){
	// Create data
	glm::vec4* pixels = new glm::vec4[width*height];
	for(uint y = 0;y<height;++y){
		uint yStep = (y/step);
		bool isLine = yStep%2;
		for(uint x = 0;x<width;++x){
			uint xStep = (x/step);
			bool isX = (xStep+isLine)%2;
			if(isX){
				pixels[y*width+x] = glm::vec4(1.0f,1.0f,1.0f,1.0f);
			} else pixels[y*width+x] = glm::vec4(0,0,0,1.0f);
		}
	}

	size_t pixelSize = sizeof(glm::vec4);
	vk::DeviceSize size = width*height*pixelSize;

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(pixels,size);

	spImage image = device->create<Image>();
	image->create(width,height,vk::Format::eR32G32B32A32Sfloat);
	image->set(cpu);

	return image;
}
