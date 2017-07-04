#include "compute.hpp"

using namespace r267;

void Compute::create(const std::string& filename,const std::vector<spDescSet>& descSets){
	auto vk_device = _device->getDevice();

	_descSets = descSets;
	std::vector<vk::DescriptorSetLayout> descLayouts;
	for(auto d : _descSets)descLayouts.push_back(d->getLayout());

	auto layoutInfo = vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		descLayouts.size(), descLayouts.data());

	_pipelineLayout = vk_device.createPipelineLayout(layoutInfo);

	auto SM = createShaderModule(vk_device,filename);
	vk::PipelineShaderStageCreateInfo shaderStageInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eCompute,
		SM,
		"main"
	);

	vk::ComputePipelineCreateInfo info(
		vk::PipelineCreateFlags(),
		shaderStageInfo,
        _pipelineLayout
	);

	_pipeline = vk_device.createComputePipeline(nullptr,info);
	_finish = _device->createSemaphore(vk::SemaphoreCreateInfo());
}

void Compute::dispatch(const glm::ivec3& size){
	_size = size;
}

void Compute::dispatch(const glm::ivec2& size){
	dispatch(glm::ivec3(size.x,size.y,1));
}

void Compute::dispatch(const int&        size){
	dispatch(glm::ivec3(size,1,1));
}

void Compute::initCommandBuffer(){
	auto vk_device = _device->getDevice();
	auto commandPool = _device->getCommandPool(true);

	vk::CommandBufferAllocateInfo allocInfo(commandPool,vk::CommandBufferLevel::ePrimary, 1);
	_commandBuffer = vk_device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

	_commandBuffer.begin(&beginInfo);

	std::vector<vk::DescriptorSet> descSets;
	for(auto d : _descSets)descSets.push_back(d->getDescriptorSet());

	_commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
    _commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, descSets.size(), descSets.data(), 0, nullptr);
	_commandBuffer.dispatch(_size.x,_size.y,_size.z);

	_commandBuffer.end();
	_isCmd = true;
}

vk::Semaphore Compute::run(const vk::Semaphore& wait){
	if(!_isCmd)initCommandBuffer(); // Init command buffer if that still don't made

	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

	vk::SubmitInfo submitInfo(
		1, &wait,
		waitStages, 
		1, &_commandBuffer,
		1, &_finish
	);

	_device->getComputeQueue().submit(submitInfo, nullptr);
    return _finish;
}
