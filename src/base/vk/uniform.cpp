#include "uniform.hpp"

using namespace r267;

Uniform::Uniform() : _is(false), _size(0), _data(nullptr) {}

Uniform::~Uniform(){}

void Uniform::create(spDevice device,vk::Queue queue,vk::CommandPool pool){
	if(!_size)throw std::logic_error("Uniform failed: don't set size");

	vk::DeviceSize bufferSize = _size;

	_gpu = std::make_shared<Buffer>(device,queue,pool);
	_cpu = std::make_shared<Buffer>(device,queue,pool);

	_gpu->create(bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	_cpu->create(bufferSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	_is = true;
}

void Uniform::set(const size_t& size,const void* data){
	if(!_size)_size = size;
	if(!_data)return;
	if(_is){
		if(_size != size)throw std::logic_error("Uniform failed: different size");
		_cpu->set(data,size);
		_cpu->copy(*_cpu,*_gpu,size);
	} else throw std::logic_error("Uniform failed: wrong set data, buffer don't exist");
}

vk::Buffer Uniform::vk_buffer() const {
	return _gpu->buffer;
}

size_t     Uniform::size() const {
	return _size;
}
