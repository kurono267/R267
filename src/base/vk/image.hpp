#pragma once 

#include "buffer.hpp"
#include "utils.hpp"

namespace r267 {

class Image {
	public:
		Image(spDevice device,vk::Queue queue,vk::CommandPool pool);
		~Image();

		void set(const spBuffer& buffer); // Set without mip levels
		void set(const spBuffer& buffer, const std::vector<uint>& offsets, const std::vector<glm::ivec2>& sizes);

		void create(const uint& width,const uint& height,
					const vk::Format& format,const uint& mipLevels = 1,const vk::ImageTiling& tiling = vk::ImageTiling::eOptimal,
					const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
					const vk::MemoryPropertyFlags& properties = vk::MemoryPropertyFlagBits::eDeviceLocal);
		void release(spDevice device);

		vk::ImageView createImageView();

		uint mipLevels();

		void transition(const vk::Format& format,const vk::ImageLayout& oldLayout,const vk::ImageLayout& newLayout);

		vk::Image vk_image();

		uint width();
		uint height();
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
		uint             _mipLevels;

		void setBuffer(const spBuffer& buffer);
};

typedef std::shared_ptr<Image> spImage;

};