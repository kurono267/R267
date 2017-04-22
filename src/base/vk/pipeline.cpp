#include "pipeline.hpp"

using namespace r267;

RenderPattern::RenderPattern(){}
RenderPattern::RenderPattern(const RenderPattern& r){
	_assembly = r._assembly;
	_viewport = r._viewport;
	_scissor  = r._scissor;
	_rasterizer = r._rasterizer;
	_multisampling = r._multisampling;
	_blendAttacment = r._blendAttacment;
	_blend          = r._blend;
	
	_colorAttachment    = r._colorAttachment;
	_colorAttachmentRef = r._colorAttachmentRef;
	_subPass            = r._subPass;
	_renderPassInfo     = r._renderPassInfo;
}
RenderPattern::~RenderPattern(){

}

void RenderPattern::inputAssembly(vk::PrimitiveTopology topology){
	_assembly = vk::PipelineInputAssemblyStateCreateInfo(vk::PipelineInputAssemblyStateCreateFlags(),(vk::PrimitiveTopology)topology);
}

void RenderPattern::viewport(const float& x,const float& y,const float& width,const float& height,const float& minDepth,const float& maxDepth){
	_viewport = vk::Viewport(x,y,width,height,minDepth,maxDepth);
}

void RenderPattern::scissor(const glm::ivec2& offset,const glm::ivec2& size){
	scissor(vk::Offset2D(offset.x,offset.y),vk::Extent2D(size.x,size.y));
}

void RenderPattern::scissor(const vk::Offset2D& offset,const vk::Extent2D& extent){
	_scissor = vk::Rect2D(offset,extent);
}

void RenderPattern::rasterizer(const vk::PolygonMode& pmode,
						const vk::CullModeFlagBits& cmode,
						const vk::FrontFace& face,
						const float& lineWidth,
						const bool& depthClamp,
						const bool& discard,
						const bool& depthBias,const glm::vec3& depthBiasFactor){
	_rasterizer = vk::PipelineRasterizationStateCreateInfo(vk::PipelineRasterizationStateCreateFlags(),(vk::Bool32)depthClamp,(vk::Bool32)discard,(vk::PolygonMode)pmode,(vk::CullModeFlagBits)cmode,(vk::FrontFace)face,(vk::Bool32)depthBias,depthBiasFactor.x,depthBiasFactor.y,depthBiasFactor.z,lineWidth);
}

void RenderPattern::multisampling(const vk::SampleCountFlagBits& count,
						   const bool& sampleShading,
						   const float& minSampleShading,
						   const bool& alphaToCoverageEnable,
						   const bool& alphaToOneEnable,
						   const vk::SampleMask* sampleMask){
	_multisampling = vk::PipelineMultisampleStateCreateInfo(vk::PipelineMultisampleStateCreateFlags(),(vk::SampleCountFlagBits)count,(vk::Bool32)sampleShading,minSampleShading,sampleMask,alphaToCoverageEnable,alphaToOneEnable);
}

void RenderPattern::blend(const bool& enable,const vk::ColorComponentFlags& writeMask,
					const vk::BlendFactor& srcColorBlendFactor,const vk::BlendFactor& dstColorBlendFactor,
					const vk::BlendOp& colorBlendOp,
					const vk::BlendFactor& srcAlphaBlendFactor,const vk::BlendFactor& dstAlphaBlendFactor,
					const vk::BlendOp& alphaBlendOp){
	_blendAttacment = vk::PipelineColorBlendAttachmentState(enable,srcColorBlendFactor,dstColorBlendFactor,colorBlendOp,srcAlphaBlendFactor,dstAlphaBlendFactor,alphaBlendOp,writeMask);

	_blend = vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(),0,vk::LogicOp::eCopy,1,&_blendAttacment);
}
// TODO improve functional for create Render Pass
void RenderPattern::createRenderPass(const vk::Format& swapchainFormat){
	_colorAttachment.setFormat(swapchainFormat);
	_colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
	_colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	_colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	_colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	_colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	_colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	_colorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	_colorAttachmentRef.setAttachment(0);
	_colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	_subPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	_subPass.setColorAttachmentCount(1);
	_subPass.setPColorAttachments(&_colorAttachmentRef);

	_renderPassInfo.setAttachmentCount(1);
	_renderPassInfo.setPAttachments(&_colorAttachment);
	_renderPassInfo.setSubpassCount(1);
	_renderPassInfo.setPSubpasses(&_subPass);
}

static RenderPattern&& basic(const spSwapchain& swapchain){
	// Create Basic Render Pattern
	RenderPattern pattern;
	pattern.inputAssembly(vk::PrimitiveTopology::eTriangleList);
	auto extent = swapchain->getExtent();
	pattern.viewport(0,0,extent.width,extent.height);
	pattern.scissor(vk::Offset2D(),extent);
	pattern.rasterizer();
	pattern.multisampling();
	pattern.blend();

	pattern.createRenderPass(swapchain->getFormat());
	return std::move(pattern);
}

vk::ShaderModule createShaderModule(vk::Device device,const std::string& filename){
	auto shaderCode = readFile(filename);
	vk::ShaderModuleCreateInfo createInfo(vk::ShaderModuleCreateFlags(),shaderCode.size(),(uint32_t*) shaderCode.data());

	return device.createShaderModule(createInfo);
}

void Pipeline::addShaderModule(const vk::ShaderStageFlagBits& type,const std::string& filename){
	vk::PipelineShaderStageCreateInfo shaderStageInfo(
		vk::PipelineShaderStageCreateFlags(),
		type,
		createShaderModule(_device,filename),
		"main"
	);
	_shaders.push_back(shaderStageInfo);
}

void Pipeline::setUniformBuffer(const Uniform& buffer,const size_t& binding,const vk::ShaderStageFlags& stage){
	// Create descriptor set
	vk::DescriptorSetLayoutBinding layoutBind(binding,vk::DescriptorType::eUniformBuffer,1,stage);
	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(),1,&layoutBind);
	_uboLayout = _device.createDescriptorSetLayout(layoutInfo);

	vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer,1);
	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(),1,1,&poolSize);

	vk::DescriptorPool descPool = _device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo allocInfo(descPool,1,&_uboLayout);
	vk::DescriptorSet descSet = _device.allocateDescriptorSets(allocInfo)[0];

	vk::DescriptorBufferInfo bufferInfo(buffer.vk_buffer(),0,buffer.size());
	vk::WriteDescriptorSet descriptorWrite(descSet,0,0,1,vk::DescriptorType::eUniformBuffer,nullptr,&bufferInfo,nullptr);

	_device.updateDescriptorSets({descriptorWrite},nullptr);
}

void Pipeline::create(){
	_viewportState = vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(),1,&(_renderpattern._viewport),1,&(_renderpattern._scissor));
	_renderPass    = _device.createRenderPass(_renderpattern._renderPassInfo);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		vk::PipelineLayoutCreateFlags(),
		1, &_uboLayout);

	auto bindingDescription = sVertex::bindingDesc();
    auto attributeDescriptions = sVertex::attributes();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
    	vk::PipelineVertexInputStateCreateFlags(),
    	1,&bindingDescription,
    	attributeDescriptions.size(),attributeDescriptions.data()
    );

	vk::PipelineLayout pLayout = _device.createPipelineLayout(pipelineLayoutInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(),
		_shaders.size(),_shaders.data(),
		&vertexInputInfo,&_renderpattern._assembly,
		nullptr,
		&_viewportState,
		&_renderpattern._rasterizer,
		&_renderpattern._multisampling,
		nullptr, // Depth and stencil
		&_renderpattern._blend,
		nullptr,
		pLayout,
		_renderPass,0);

	_pipeline = _device.createGraphicsPipelines(nullptr,pipelineInfo)[0];
}
