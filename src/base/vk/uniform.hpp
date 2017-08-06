#pragma once

#include "buffer.hpp"
#include <stdexcept>

namespace r267 {

class Uniform {
	public:
		Uniform();
		Uniform(const Uniform& u) : _gpu(u._gpu), _cpu(u._cpu), _is(u._is), _size(u._size), _data(u._data){}
		virtual ~Uniform();

		void create(spDevice device,const size_t& size,const void* data = nullptr, const vk::BufferUsageFlags& bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer);
		void set(const size_t& size,const void* data);

		vk::Buffer vk_buffer() const;
		size_t     size() const ;

		operator bool(){return _is;}
	protected:
		spBuffer _gpu; // GPU buffer
		spBuffer _cpu; // CPU staging buffer

		bool     _is;

		size_t   _size;
		void*    _data;
};

};
