#pragma once

#include "buffer.hpp"
#include <stdexcept>

namespace r267 {

class Uniform {
	public:
		Uniform();
		virtual ~Uniform();

		void create(Device device,vk::Queue queue,vk::CommandPool pool);
		void set(const size_t& size,const void* data = nullptr);

		vk::Buffer vk_buffer() const;
		size_t     size() const ;
	protected:
		spBuffer _gpu; // GPU buffer
		spBuffer _cpu; // CPU staging buffer

		bool     _is;

		size_t   _size;
		void*    _data;
};

};
