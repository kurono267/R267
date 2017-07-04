#pragma once

#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>

namespace r267 {

// Compute Pipeline
class Compute {
	public:
		Compute(const spDevice& device) : _device(device),_isCmd(false) {}
		~Compute(){}

		// Set size for compute buffer
		void dispatch(const glm::ivec3& size);
		void dispatch(const glm::ivec2& size);
		void dispatch(const int&        size);

		void create(const std::string& filename,const spDescSet& descSet);  // Create Compute shader
		vk::Semaphore run(const vk::Semaphore& wait); // Run compute shader

		vk::Pipeline pipeline(){return _pipeline;}
	protected:
		void initCommandBuffer(); // Create command buffer 

		spDevice _device;

		spDescSet    _descSet;
		vk::Pipeline _pipeline;
		vk::PipelineLayout _pipelineLayout;

		glm::ivec3        _size;
		bool              _isCmd;
		vk::CommandBuffer _commandBuffer; 

		vk::Semaphore     _finish;
};

typedef std::shared_ptr<Compute> spCompute;

} // r267
