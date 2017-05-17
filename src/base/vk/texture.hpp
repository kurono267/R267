#pragma once

#include "base/default.hpp"
#include "buffer.hpp"

namespace r267 {

class Texture {
	public:
		Texture(spDevice device,vk::Queue queue,vk::CommandPool pool);
		~Texture();

		void create(const uint& width,const uint& height,const void* pixels,const vk::Format& format,const size_t& pixelSize);
		void release();
	protected:
		void createTexture(const uint& width,const uint& height,const vk::Format& format,const void* pixels,const size_t& pixelSize);
		// Inner used function for create Image and allocate memory
		Image createImage(const uint& width,const uint& height,
					const vk::Format& format,const vk::ImageTiling& tiling,
					const vk::ImageUsageFlags& usage,const vk::MemoryProperyFlags& properties);

		spDevice        _device;
		vk::CommandPool _pool;
		vk::Queue       _queue;
		
		Image  _image; // Image at GPU
		spBuffer _cpu;  // Image at CPU
};

} // r267
