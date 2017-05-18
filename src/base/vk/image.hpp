#pragma once 

#include "buffer.hpp"
#include "utils.hpp"

namespace r267 {

class Image {
	public:
		Image(spDevice device,vk::Queue queue,vk::CommandPool pool);
		~Image();

		void set(const spBuffer& buffer);

		void create(const uint& width,const uint& height,
					const vk::Format& format,const uint& mipLevels = 1,const vk::ImageTiling& tiling = vk::ImageTiling::eOptimal,
					const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
					const vk::MemoryPropertyFlags& properties = vk::MemoryPropertyFlagBits::eDeviceLocal);
		void release(spDevice device);

		vk::ImageView createImageView();

		vk::Image vk_image();
	protected:
		spDevice        _device;
		vk::CommandPool _pool;
		vk::Queue       _queue;

		vk::Image        _image;
		vk::DeviceMemory _memory;
		vk::ImageView    _imageView;

		uint             _width;
		uint             _height;
		vk::Format       _format;

		void transition(const vk::Format& format,const vk::ImageLayout& oldLayout,const vk::ImageLayout& newLayout);
		void setBuffer(const spBuffer& buffer);
};

typedef std::shared_ptr<Image> spImage;

};