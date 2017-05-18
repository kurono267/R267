#include "uniform.hpp"

using namespace r267;

Uniform::Uniform() : _is(false), _size(0), _data(nullptr) {}

Uniform::~Uniform(){}

void Uniform::create(spDevice device,const size_t& size,const void* data){
	_size = size;

	vk::DeviceSize bufferSize = _size;

	_gpu = device->create<Buffer>();//std::make_shared<Buffer>(_device,queue,pool);
	_cpu = device->create<Buffer>();//std::make_shared<Buffer>(_device,queue,pool);

	_cpu->create(bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	_gpu->create(bufferSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	_is = true;

	if(data)set(size,data);
}

void Uniform::set(const size_t& size,const void* data){
	if(_size != size)throw std::logic_error("Uniform failed: different size");
	else if(!data)throw std::logic_error("Data is nullptr");
	else if(_is){
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
