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

vk::ShaderModule createShaderModule(vk::Device device,const std::string& filename){
	auto shaderCode = readFile(filename);
	vk::ShaderModuleCreateInfo createInfo(vk::ShaderModuleCreateFlags(),shaderCode.size(),(uint32_t*) shaderCode.data());

	return device.createShaderModule(createInfo);
}

void Pipeline::addShader(const vk::ShaderStageFlagBits& type,const std::string& filename){
	auto SM = createShaderModule(_device,filename);
	_shaderModules.push_back(SM);
	vk::PipelineShaderStageCreateInfo shaderStageInfo(
		vk::PipelineShaderStageCreateFlags(),
		type,
		SM,
		"main"
	);
	_shaders.push_back(shaderStageInfo);
}

// TODO Move to Uniform
void Pipeline::setUniformBuffer(const Uniform& buffer,const size_t& binding,const vk::ShaderStageFlags& stage){
	_uboBinds.push_back({buffer,binding,stage});
}

void Pipeline::setTexture(const vk::ImageView& imageView,const vk::Sampler& sampler, const size_t& binding, const vk::ShaderStageFlags& stage){
	_samplerBinds.push_back({imageView,sampler,binding,stage});
}

void Pipeline::createDescSet(){
	// Create Decription Set Layout
	std::vector<vk::DescriptorSetLayoutBinding> layoutBinds;
	std::vector<vk::DescriptorPoolSize>         poolSizes;

	// Add Layout binding for UBO
	for(auto u : _uboBinds){
		std::cout << "UBO at binding " << u.binding << std::endl;
		layoutBinds.push_back(vk::DescriptorSetLayoutBinding(u.binding,vk::DescriptorType::eUniformBuffer,1,u.stage));
		poolSizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer,1));
	}
	for(auto s : _samplerBinds){
		layoutBinds.push_back(vk::DescriptorSetLayoutBinding(s.binding,vk::DescriptorType::eCombinedImageSampler,1,s.stage));
		poolSizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,1));
	}

	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(),layoutBinds.size(),layoutBinds.data());
	_descLayout = _device.createDescriptorSetLayout(layoutInfo);

	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(),1,poolSizes.size(),poolSizes.data());
	_descPool = _device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo allocInfo(_descPool,1,&_descLayout);
	_descSet = _device.allocateDescriptorSets(allocInfo)[0];

	std::vector<vk::WriteDescriptorSet>         descWrites;
	for(auto u : _uboBinds){
		vk::DescriptorBufferInfo bufferInfo(u.buffer.vk_buffer(),0,u.buffer.size());
		descWrites.push_back(vk::WriteDescriptorSet(_descSet,0,0,1,vk::DescriptorType::eUniformBuffer,nullptr,&bufferInfo,nullptr));
	}
	for(auto s : _samplerBinds){
		vk::DescriptorImageInfo imageInfo(s.sampler, s.imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
		descWrites.push_back(vk::WriteDescriptorSet(_descSet,0,0,1,vk::DescriptorType::eCombinedImageSampler,&imageInfo,nullptr,nullptr));
	}
	_device.updateDescriptorSets(descWrites,nullptr);
}

void Pipeline::create(){
	createDescSet();
	_viewportState = vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(),1,&(_renderpattern._viewport),1,&(_renderpattern._scissor));
	_renderPass    = _device.createRenderPass(_renderpattern._renderPassInfo);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		vk::PipelineLayoutCreateFlags(),
		1, &_descLayout);

	auto bindingDescription = sVertex::bindingDesc();
    auto attributeDescriptions = sVertex::attributes();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
    	vk::PipelineVertexInputStateCreateFlags(),
    	1,&bindingDescription,
    	attributeDescriptions.size(),attributeDescriptions.data()
    );

	_pLayout = _device.createPipelineLayout(pipelineLayoutInfo);

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
		_pLayout,
		_renderPass,0);

	_pipeline = _device.createGraphicsPipelines(nullptr,pipelineInfo)[0];
}

void Pipeline::release(){
	for(auto m : _shaderModules){
		_device.destroyShaderModule(m);
	}
	_device.destroyRenderPass(_renderPass);
	_device.destroyDescriptorSetLayout(_descLayout);
	_device.destroyDescriptorPool(_descPool);
	_device.destroyPipelineLayout(_pLayout);
	_device.destroyPipeline(_pipeline);
}

vk::RenderPass Pipeline::getRenderPass(){
	return _renderPass;
}
