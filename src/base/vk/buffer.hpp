#pragma once

// Vulkan buffer
#include <base/default.hpp>
#include "utils.hpp"
#include "device.hpp"

namespace r267 {

// Vertex Buffer
struct sVertex {
	sVertex(const glm::vec3& pos_ = glm::vec3(0.0f),const glm::vec2& uv_ = glm::vec2(0.0f),const glm::vec3& normal_ = glm::vec3(0.0f));
	sVertex(const float& px,const float& py,const float& pz,const float& u = 0.0f,const float& v = 0.0f,const float& nx = 0.0f,const float& ny = 0.0f,const float& nz = 0.0f);

	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;

	static vk::VertexInputBindingDescription bindingDesc(){
		return vk::VertexInputBindingDescription(0,sizeof(sVertex));
	}

	static std::array<vk::VertexInputAttributeDescription,3> attributes(){
		std::array<vk::VertexInputAttributeDescription,3> attrs;
		attrs[0] = vk::VertexInputAttributeDescription(0,0,vk::Format::eR32G32B32Sfloat,offsetof(sVertex,pos));
		attrs[1] = vk::VertexInputAttributeDescription(1,0,vk::Format::eR32G32B32Sfloat,offsetof(sVertex,normal));
		attrs[2] = vk::VertexInputAttributeDescription(2,0,vk::Format::eR32G32Sfloat,offsetof(sVertex,uv));
		return attrs;
	}
};

class Buffer {
	public: // Base data simple to access
		vk::Buffer       buffer;
		vk::DeviceMemory memory;
	public:
		Buffer(Device device,vk::Queue queue,vk::CommandPool pool);

		void create(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

		void createVB(const std::vector<sVertex>& vertices);
		void createIB(const std::vector<uint>& indices);

		void set(const void* data,const size_t& size);
		void copy(const Buffer& src,const Buffer& dst,const size_t& size);
	protected:
		Buffer createOther(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

		Device _lvuDevice; // Out inner using device

		vk::Device      _device;
		vk::PhysicalDevice _pDevice;
		vk::CommandPool _pool;
		vk::Queue       _queue;

		void copy(vk::Buffer src,vk::Buffer dst,vk::DeviceSize size);
};

typedef std::shared_ptr<Buffer> spBuffer;

};
