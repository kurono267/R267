#pragma once 

#include "buffer.hpp"
#include "utils.hpp"

namespace r267 {

class Image {
	public:
		Image(spDevice device,vk::Queue queue,vk::CommandPool pool);
		~Image();

		void set(const spBuffer& buffer); // Set without mip levels
		void set(const spBuffer& buffer, const glm::ivec2 size, const uint mipLevel = 0, const uint layer = 0, const uint offsetBuffer = 0);

		void setMipmaps(const spBuffer& buffer, const std::vector<uint>& offsets, const std::vector<glm::ivec2>& sizes);

		void create(const uint& width,const uint& height,
					const vk::Format& format,const uint& mipLevels = 1,const vk::ImageTiling& tiling = vk::ImageTiling::eOptimal,
					const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
					const vk::ImageLayout& layout = vk::ImageLayout::ePreinitialized,const vk::MemoryPropertyFlags& properties = vk::MemoryPropertyFlagBits::eDeviceLocal);
		void createCubemap(const uint& width,const uint& height,
					const vk::Format& format,const uint& mipLevels = 1,const vk::ImageTiling& tiling = vk::ImageTiling::eOptimal,
					const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
					const vk::ImageLayout& layout = vk::ImageLayout::ePreinitialized,const vk::MemoryPropertyFlags& properties = vk::MemoryPropertyFlagBits::eDeviceLocal);
		void release(spDevice device);

		vk::ImageView ImageView();
		vk::ImageView ImageView(const int layer,const int level = 0,const int numLayers = -1,const int numLevels = -1,const vk::ImageViewType type = vk::ImageViewType::e2D);

		uint mipLevels();

		void transition(const vk::ImageLayout& newLayout); // Transition for all levels and layers
		void transition(const vk::ImageLayout& newLayout, const vk::ImageLayout& oldLayout, const int level = 0, const int numLevels = -1, const int layer = 0, const int numLayers = -1);
		void transition(vk::CommandBuffer& cmd,const vk::ImageLayout& newLayout, const vk::ImageLayout& oldLayout, const int level = 0, const int numLevels = -1, const int layer = 0, const int numLayers = -1);

		vk::Image vk_image();

		uint width();
		uint height();
	protected:
		spDevice        _device;
		vk::CommandPool _pool;
		vk::Queue       _queue;

		vk::Image        _image;
		vk::DeviceMemory _memory;
		std::vector<vk::ImageView> _imageViews;

		uint             _width;
		uint             _height;
		vk::Format       _format;
		uint             _mipLevels;
		uint             _layers;

		vk::ImageLayout  _currLayout;

		vk::ImageViewCreateInfo _imageViewCreateInfo;

		void setBuffer(const spBuffer& buffer, const glm::ivec2& size, const uint& mipLevel, const uint& layer, const uint& offsetBuffer);
};

typedef std::shared_ptr<Image> spImage;

};