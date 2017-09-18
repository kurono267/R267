#include "framebuffer.hpp"

using namespace r267;

Framebuffer::Framebuffer(spDevice device,vk::Queue queue,vk::CommandPool pool) : _device(device) {}
Framebuffer::~Framebuffer(){}

void Framebuffer::attachment(const vk::ImageView& imageView){
	_imageViews.push_back(imageView);
}

void Framebuffer::depth(const uint& width,const uint& height){
	vk::Format depthFormat = _device->depthFormat();

	_depthBuffer = _device->create<Image>();
	_depthBuffer->create(width,height,depthFormat,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eDepthStencilAttachment,vk::ImageLayout::eUndefined);
	_depthView  = createImageView(_device->getDevice(),_depthBuffer->vk_image(),depthFormat,vk::ImageAspectFlagBits::eDepth);
	_depthBuffer->transition(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	attachment(_depthView);
}

void Framebuffer::create(const uint& width,const uint& height,const vk::RenderPass& renderPass){
	vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo(
		vk::FramebufferCreateFlags(), // Default
		renderPass, // Current render pass
		_imageViews.size(), _imageViews.data(), // Attachments
		width, // Width
		height, // Height
		1 // Layers
	);
	_framebuffer = _device->getDevice().createFramebuffer(framebufferInfo);
}

vk::Framebuffer Framebuffer::vk_framebuffer(){
	return _framebuffer;
}

void Framebuffer::release(spDevice device){
	device->getDevice().destroyImageView(_depthView);
	device->getDevice().destroyFramebuffer(_framebuffer);
}

