#include "pipeline.hpp"

using namespace r267;

RenderPattern::RenderPattern() : _dynamicScissor(true),_dynamicViewport(true){}
RenderPattern::RenderPattern(const RenderPattern& r){
	_assembly = r._assembly;
	_viewport = r._viewport;
	_scissor  = r._scissor;
	_rasterizer = r._rasterizer;
	_multisampling = r._multisampling;
	_blendAttachments = r._blendAttachments;
	_blend          = r._blend;

	_subPass            = r._subPass;
	_renderPassInfo     = r._renderPassInfo;
	_depthStencil       = r._depthStencil;
	_dynamicScissor     = r._dynamicScissor;
	_dynamicViewport    = r._dynamicViewport;
	for(int i = 0;i<r._attachmentsRef.size();++i){
		_attachmentsRef.push_back(r._attachmentsRef[i]);
		_attachmentsDesc.push_back(r._attachmentsDesc[i]);
	}
	for(int i = 0;i<r._pushConsts.size();++i){
		_pushConsts.push_back(r._pushConsts[i]);
	}
}
RenderPattern::~RenderPattern(){

}

void RenderPattern::inputAssembly(vk::PrimitiveTopology topology){
	_assembly = vk::PipelineInputAssemblyStateCreateInfo(vk::PipelineInputAssemblyStateCreateFlags(),(vk::PrimitiveTopology)topology);
}

void RenderPattern::viewport(const float& x,const float& y,const float& width,const float& height,const float& minDepth,const float& maxDepth){
	_viewport = vk::Viewport(x,y,width,height,minDepth,maxDepth);
	_dynamicViewport = false;
}

void RenderPattern::scissor(const glm::ivec2& offset,const glm::ivec2& size){
	scissor(vk::Offset2D(offset.x,offset.y),vk::Extent2D(size.x,size.y));
}

void RenderPattern::scissor(const vk::Offset2D& offset,const vk::Extent2D& extent){
	_scissor = vk::Rect2D(offset,extent);
	_dynamicScissor = false;
}

void RenderPattern::dynamicScissor(){
	_dynamicScissor = true;
}

void RenderPattern::dynamicViewport(){
	_dynamicViewport = true;
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

void RenderPattern::blend(const uint& attachments,const bool& enable,const vk::ColorComponentFlags& writeMask,
					const vk::BlendFactor& srcColorBlendFactor,const vk::BlendFactor& dstColorBlendFactor,
					const vk::BlendOp& colorBlendOp,
					const vk::BlendFactor& srcAlphaBlendFactor,const vk::BlendFactor& dstAlphaBlendFactor,
					const vk::BlendOp& alphaBlendOp){
	_blendAttachments.clear();
	for(int i = 0;i<attachments;++i){
		_blendAttachments.push_back(vk::PipelineColorBlendAttachmentState(enable,srcColorBlendFactor,dstColorBlendFactor,colorBlendOp,srcAlphaBlendFactor,dstAlphaBlendFactor,alphaBlendOp,writeMask));
	}

	_blend = vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(),0,vk::LogicOp::eCopy,_blendAttachments.size(),_blendAttachments.data());
}

void RenderPattern::depth(const bool& enable, const bool& write,const vk::CompareOp& comp){
	_depthStencil = vk::PipelineDepthStencilStateCreateInfo(
		vk::PipelineDepthStencilStateCreateFlags(),
		enable,
		write,
		comp
	);
}

RenderPattern::Attachment r267::createAttachment(const vk::Format& format, const bool& depth, const int index){
	RenderPattern::Attachment att;
	att.desc.setFormat(format);
	att.desc.setSamples(vk::SampleCountFlagBits::e1);
	att.desc.setLoadOp(vk::AttachmentLoadOp::eClear);
	if(!depth)att.desc.setStoreOp(vk::AttachmentStoreOp::eStore);
	else att.desc.setStoreOp(vk::AttachmentStoreOp::eDontCare);
	att.desc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	att.desc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	if(!depth)att.desc.setInitialLayout(vk::ImageLayout::ePresentSrcKHR);
	else att.desc.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	if(!depth)att.desc.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	else att.desc.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	att.ref.setAttachment(index);
	if(!depth)att.ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	else att.ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	return att;
}

void RenderPattern::constants(const size_t& offset, const size_t& size, const vk::ShaderStageFlagBits& stage){
	_pushConsts.push_back(vk::PushConstantRange(stage,offset,size));
}

// TODO improve functional for create Render Pass
void RenderPattern::createRenderPass(const vk::Format& swapchainFormat,const vk::Format& depthFormat,const uint& attachments){
	_attachmentsDesc.clear();
	_attachmentsRef.clear();
	for(int i = 0;i<attachments;++i){
		auto a = createAttachment(swapchainFormat,false,i);
		_attachmentsDesc.push_back(a.desc);
		_attachmentsRef.push_back(a.ref);
	}
	std::cout << _attachmentsRef.size() << std::endl;
	auto depthAtt = createAttachment(depthFormat,true,attachments);
	_attachmentsDesc.push_back(depthAtt.desc);
	_attachmentsRef.push_back(depthAtt.ref);

	_subPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	_subPass.setColorAttachmentCount(attachments);
	_subPass.setPColorAttachments(_attachmentsRef.data());
	_subPass.setPDepthStencilAttachment(&_attachmentsRef[attachments]);

	_subPassDep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	_subPassDep[0].dstSubpass = 0;
	_subPassDep[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	_subPassDep[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	_subPassDep[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	_subPassDep[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead| vk::AccessFlagBits::eColorAttachmentWrite;
	_subPassDep[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	_subPassDep[1].srcSubpass = 0;
	_subPassDep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	_subPassDep[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	_subPassDep[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	_subPassDep[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead| vk::AccessFlagBits::eColorAttachmentWrite;
	_subPassDep[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	_subPassDep[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	_renderPassInfo.setAttachmentCount(_attachmentsDesc.size());
	_renderPassInfo.setPAttachments(_attachmentsDesc.data());
	_renderPassInfo.setSubpassCount(1);
	_renderPassInfo.setPSubpasses(&_subPass);
	_renderPassInfo.setDependencyCount(2);
	_renderPassInfo.setPDependencies(_subPassDep);
}

void RenderPattern::createRenderPass(const std::vector<Attachment>& attachments,const Attachment& depth){
	_attachmentsDesc.clear();
	_attachmentsRef.clear();
	for(auto a : attachments){
		_attachmentsDesc.push_back(a.desc);
		_attachmentsRef.push_back(a.ref);
	}
	_attachmentsDesc.push_back(depth.desc);
	_attachmentsRef.push_back(depth.ref);

	_subPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	_subPass.setColorAttachmentCount(attachments.size());
	_subPass.setPColorAttachments(_attachmentsRef.data());
	_subPass.setPDepthStencilAttachment(&_attachmentsRef[attachments.size()]);

	_subPassDep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	_subPassDep[0].dstSubpass = 0;
	_subPassDep[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	_subPassDep[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	_subPassDep[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	_subPassDep[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead| vk::AccessFlagBits::eColorAttachmentWrite;
	_subPassDep[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	_subPassDep[1].srcSubpass = 0;
	_subPassDep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	_subPassDep[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	_subPassDep[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	_subPassDep[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead| vk::AccessFlagBits::eColorAttachmentWrite;
	_subPassDep[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	_subPassDep[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	_renderPassInfo.setAttachmentCount(_attachmentsDesc.size());
	_renderPassInfo.setPAttachments(_attachmentsDesc.data());
	_renderPassInfo.setSubpassCount(1);
	_renderPassInfo.setPSubpasses(&_subPass);
	_renderPassInfo.setDependencyCount(2);
	_renderPassInfo.setPDependencies(_subPassDep);
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

void Pipeline::create(const vk::VertexInputBindingDescription& vertexBinding, const std::vector<vk::VertexInputAttributeDescription>& vertexAttrib){
	_viewportState = vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(),1,&(_renderpattern._viewport),1,&(_renderpattern._scissor));
	std::vector<vk::DynamicState> dynamicStates;
	if(_renderpattern._dynamicScissor){
		dynamicStates.push_back(vk::DynamicState::eScissor);
	}
	if(_renderpattern._dynamicViewport){
		dynamicStates.push_back(vk::DynamicState::eViewport);
	}
	vk::PipelineDynamicStateCreateInfo dynamicStatesCreteInfo;
	dynamicStatesCreteInfo.pDynamicStates    = dynamicStates.data();
	dynamicStatesCreteInfo.dynamicStateCount = dynamicStates.size();

	_renderPass    = _device.createRenderPass(_renderpattern._renderPassInfo);

	_pLayout = _device.createPipelineLayout(_pipelineLayoutInfo);

	auto bindingDescription = vertexBinding;
    auto attributeDescriptions = vertexAttrib;

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
    	vk::PipelineVertexInputStateCreateFlags(),
    	1,&bindingDescription,
    	attributeDescriptions.size(),attributeDescriptions.data()
    );

	vk::PipelineTessellationStateCreateInfo tessInfo(vk::PipelineTessellationStateCreateFlags(),3);

	vk::GraphicsPipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(),
		_shaders.size(),_shaders.data(),
		&vertexInputInfo,&_renderpattern._assembly,
		&tessInfo,
		&_viewportState,
		&_renderpattern._rasterizer,
		&_renderpattern._multisampling,
		&_renderpattern._depthStencil, // Depth and stencil
		&_renderpattern._blend,
		dynamicStates.size()==0?nullptr:&dynamicStatesCreteInfo,
		_pLayout,
		_renderPass,0);

	_pipeline = _device.createGraphicsPipelines(nullptr,pipelineInfo)[0];
}

void Pipeline::descSets(const std::vector<spDescSet>& descSets){
	for(auto d : descSets){
		_descLayouts.push_back(d->getLayout());
	}
	_pipelineLayoutInfo = vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		_descLayouts.size(), _descLayouts.data());
}

void Pipeline::descSet(const spDescSet& d){
	_descLayouts.push_back(d->getLayout());

	_pipelineLayoutInfo = vk::PipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		_descLayouts.size(), _descLayouts.data(),_renderpattern._pushConsts.size(),_renderpattern._pushConsts.data());
}

void Pipeline::release(){
	for(auto m : _shaderModules){
		_device.destroyShaderModule(m);
	}
	_device.destroyRenderPass(_renderPass);
	_device.destroyPipelineLayout(_pLayout);
	_device.destroyPipeline(_pipeline);
}

vk::RenderPass Pipeline::getRenderPass(){
	return _renderPass;
}

DescSet::DescSet(spDevice device,vk::Queue queue,vk::CommandPool pool) : _device(device) {}
DescSet::~DescSet(){}

void DescSet::create(){
	// Create Decription Set Layout
	std::vector<vk::DescriptorSetLayoutBinding> layoutBinds;
	std::vector<vk::DescriptorPoolSize>         poolSizes;

	auto vk_device = _device->getDevice();

	std::unordered_map<int,int> typesDesc;
	// Add Layout binding for UBO
	for(auto u : _uboBinds){
		layoutBinds.push_back(vk::DescriptorSetLayoutBinding(u.binding,u.descType,1,u.stage));
		poolSizes.push_back(vk::DescriptorPoolSize(u.descType,1));
		if(typesDesc.find((int)u.descType) == typesDesc.end()){
			typesDesc.insert(std::pair<int,int>((int)u.descType,1));
		} else {
			typesDesc[(int)u.descType] += 1;
		}
	}
	for(auto s : _samplerBinds) {
		layoutBinds.push_back(
				vk::DescriptorSetLayoutBinding(s.binding, s.descType, 1, s.stage));
		poolSizes.push_back(vk::DescriptorPoolSize(s.descType, 1));
		if(typesDesc.find((int)s.descType) == typesDesc.end()){
			typesDesc.insert(std::pair<int,int>((int)s.descType,1));
		} else {
			typesDesc[(int)s.descType] += 1;
		}
	}
	/*int poolSize = 0;
	for(auto t : typesDesc){
		std::cout << to_string((vk::DescriptorType)t.first) << " " << t.second << std::endl;
		poolSizes.push_back(vk::DescriptorPoolSize((vk::DescriptorType)t.first,t.second));
		poolSize++;
	}*/

	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(),layoutBinds.size(),layoutBinds.data());
	_descLayout = vk_device.createDescriptorSetLayout(layoutInfo);

	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(),1,poolSizes.size(),poolSizes.data());
	_descPool = vk_device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo allocInfo(_descPool,1,&_descLayout);
	_descSet = vk_device.allocateDescriptorSets(allocInfo)[0];

	std::vector<vk::WriteDescriptorSet>         descWrites;
	_descBufferInfos.resize(_uboBinds.size());
	for(auto u : _uboBinds){
		_descBufferInfos.push_back(vk::DescriptorBufferInfo(u.buffer.vk_buffer(),0,u.buffer.size()));
		descWrites.push_back(vk::WriteDescriptorSet(_descSet,u.binding,0,1,u.descType,nullptr,&_descBufferInfos[_descBufferInfos.size()-1],nullptr));
	}
	_descImageInfos.resize(_samplerBinds.size());
	for(auto s : _samplerBinds){
		_descImageInfos.push_back(vk::DescriptorImageInfo(s.sampler, s.imageView, s.layout));
		descWrites.push_back(vk::WriteDescriptorSet(_descSet,s.binding,0,1,s.descType,&_descImageInfos[_descImageInfos.size()-1],nullptr,nullptr));
	}
	vk_device.updateDescriptorSets(descWrites,nullptr);
}

void DescSet::setUniformBuffer(const Uniform& buffer,const size_t& binding,const vk::ShaderStageFlags& stage,
							   const vk::DescriptorType& descType){
	_uboBinds.push_back({buffer,binding,stage,descType});
}

void DescSet::setTexture(const vk::ImageView& imageView,const vk::Sampler& sampler, const size_t& binding, const vk::ShaderStageFlags& stage,
						 const vk::DescriptorType& descType,
						 const vk::ImageLayout& imageLayout){
	_samplerBinds.push_back({imageView,sampler,binding,stage,descType,imageLayout});
}

void DescSet::release(spDevice device){
	auto vk_device = device->getDevice();

	std::set<vk::Sampler> samplerSet;
	for(auto s : _samplerBinds){
		samplerSet.insert(s.sampler);
	}

	for(auto s : samplerSet){
		vk_device.destroySampler(s);
	}


	vk_device.destroyDescriptorSetLayout(_descLayout);
	vk_device.destroyDescriptorPool(_descPool);
}
