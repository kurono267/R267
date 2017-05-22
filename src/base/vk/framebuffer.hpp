#pragma once

#include "device.hpp"
#include "image.hpp"

namespace r267 {

class Framebuffer {
	public:
		Framebuffer(spDevice device,vk::Queue queue,vk::CommandPool pool);
		virtual ~Framebuffer();
		
		void attachment(const vk::ImageView& imageView);
		void depth(const uint& width,const uint& height); // Create Depth Buffer

		void create(const uint& width,const uint& height,const vk::RenderPass& renderPass);
		vk::Framebuffer vk_framebuffer();

		void release(spDevice device);
	protected:
		spDevice _device;
		std::vector<vk::ImageView> _imageViews;
		vk::Framebuffer _framebuffer;

		bool    _isDepthBuffer;
		spImage _depthBuffer;
		vk::ImageView _depthView;
};

typedef std::shared_ptr<Framebuffer> spFramebuffer;

} // r267