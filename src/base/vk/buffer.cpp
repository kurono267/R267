#include "buffer.hpp"

using namespace r267;

sVertex::sVertex(const glm::vec3& pos_,const glm::vec2& uv_,const glm::vec3& normal_)
: pos(pos_),uv(uv_),normal(normal_){}

sVertex::sVertex(const float& px,const float& py,const float& pz,const float& u,const float& v,const float& nx,const float& ny,const float& nz) : 
	pos(px,py,pz),uv(u,v),normal(nx,ny,nz)
{}

Buffer::Buffer(spDevice device,vk::Queue queue,vk::CommandPool pool) : _lvuDevice(device),_device(device->getDevice()), _pDevice(device->getPhysicalDevice()), _queue(queue), _pool(pool) {}

Buffer Buffer::createOther(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties){
    Buffer result(_lvuDevice,_queue,_pool);
  	result.create(size,usage,properties);
    return result;
}

void Buffer::create(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties){
	vk::BufferCreateInfo bufferInfo(vk::BufferCreateFlags(),size,usage);

    buffer = _device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = _device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo(memRequirements.size,findMemoryType(_pDevice, memRequirements.memoryTypeBits, properties));
    memory = _device.allocateMemory(allocInfo);

    _device.bindBufferMemory(buffer,memory, 0);
}

void Buffer::createVB(const std::vector<sVertex>& vertices){
	vk::DeviceSize bufferSize = sizeof(sVertex)*vertices.size();

	Buffer stagingBuffer = createOther(bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.set(vertices.data(),(size_t)bufferSize);

	Buffer vertexBuffer = createOther(bufferSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	copy(stagingBuffer.buffer,vertexBuffer.buffer,bufferSize);

	buffer = vertexBuffer.buffer;
	memory = vertexBuffer.memory;
}

void Buffer::createIB(const std::vector<uint>& indices) {
    vk::DeviceSize bufferSize = sizeof(uint) * indices.size();

	Buffer stagingBuffer = createOther(bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	stagingBuffer.set(indices.data(),(size_t)bufferSize);

	Buffer indexBuffer = createOther(bufferSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	copy(stagingBuffer.buffer,indexBuffer.buffer,bufferSize);

	buffer = indexBuffer.buffer;
	memory = indexBuffer.memory;
}

void Buffer::copy(vk::Buffer src,vk::Buffer dst,vk::DeviceSize size){
	vk::CommandBufferAllocateInfo allocInfo(_pool,vk::CommandBufferLevel::ePrimary,1);

	auto cmdBuffers = _device.allocateCommandBuffers(allocInfo);

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	cmdBuffers[0].begin(beginInfo);
		cmdBuffers[0].copyBuffer(src,dst,vk::BufferCopy(0,0,size));
	cmdBuffers[0].end();

	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBufferCount(1);
	submitInfo.setPCommandBuffers(&cmdBuffers[0]);

	_queue.submit(submitInfo,vk::Fence());
	_queue.waitIdle();

	_device.freeCommandBuffers(_pool,cmdBuffers);
}

void Buffer::set(const void* data,const size_t& size){
	void* map_data = _device.mapMemory(memory,0,(vk::DeviceSize)size);
		memcpy(map_data,data,size);
	_device.unmapMemory(memory);
}

void Buffer::copy(const Buffer& src,const Buffer& dst,const size_t& size){
	copy(src.buffer,dst.buffer,(vk::DeviceSize)size);
}