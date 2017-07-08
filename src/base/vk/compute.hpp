#pragma once

#include "pipeline.hpp"
#include "additional.hpp"

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

		void create(const std::string& filename,const std::vector<spDescSet>& descSets);  // Create Compute shader
		void run(); // Run compute shader

		vk::Pipeline pipeline(){return _pipeline;}
	protected:
		void initCommandBuffer(); // Create command buffer 

		spDevice _device;

		std::vector<spDescSet>    _descSets;
		vk::Pipeline _pipeline;
		vk::PipelineLayout _pipelineLayout;

		glm::ivec3        _size;
		bool              _isCmd;
		vk::CommandBuffer _commandBuffer; 

		vk::Semaphore     _finish;
		vk::Fence         _fence;
};

typedef std::shared_ptr<Compute> spCompute;

} // r267
