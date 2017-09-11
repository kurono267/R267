#include "ibl.hpp"

#define CUBEMAP_SIZE 1024
#define IRRADIANCE_SIZE 64
#define BRDF_SIZE 128

using namespace r267;

void IBL::init(spDevice device,spImage source){
	_source = source;
	_device = device;

	_cube = std::make_shared<Cube>();
	_cube->create(device);

	_quad = std::make_shared<Quad>();
	_quad->create(device);

	// Create matrix
	glm::mat4 proj = glm::perspective(glm::radians(90.0f),1.0f,0.1f,100.0f);
	_uboData.mat[0] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f));
    _uboData.mat[1] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f));
    _uboData.mat[2] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
    _uboData.mat[3] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f, 1.0f));
    _uboData.mat[4] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f));
    _uboData.mat[5] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, 1.0f), glm::vec3(0.0f, 1.0f,  0.0f));

	_uboUniform.create(_device,sizeof(UBO),&_uboData);

	vk::CommandBufferAllocateInfo allocInfo(_device->getCommandPool(),vk::CommandBufferLevel::ePrimary, NumSteps);
	auto cmds = _device->getDevice().allocateCommandBuffers(allocInfo);
	for(int i = 0;i<NumSteps;++i)
		_cmds[i] = cmds[i];

	initConvert();
	initIrradiance();
	initFilter();
	initBRDF();
}

void IBL::initConvert(){
	auto baseRP = RenderPattern::basic(_device);
	baseRP.depth(true,true,vk::CompareOp::eLess);
	baseRP.scissor(glm::ivec2(0),glm::ivec2(CUBEMAP_SIZE));
	baseRP.viewport(0,0,CUBEMAP_SIZE,CUBEMAP_SIZE);
	baseRP.constants(0,sizeof(int),vk::ShaderStageFlagBits::eAllGraphics);
	baseRP.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eFront);
	baseRP.createRenderPass(vk::Format::eR16G16B16A16Sfloat,_device->depthFormat(),1);

	_pipelines[Convert] = std::make_shared<Pipeline>(baseRP,_device->getDevice());

	_cubemap = _device->create<Image>();
	_cubemap->createCubemap(CUBEMAP_SIZE,CUBEMAP_SIZE,
							vk::Format::eR16G16B16A16Sfloat,log2(CUBEMAP_SIZE),vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eColorAttachment);
	_cubemap->transition(vk::ImageLayout::eColorAttachmentOptimal);

	_descSets[Convert] = _device->create<DescSet>();
	_descSets[Convert]->setUniformBuffer(_uboUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSets[Convert]->setTexture(_source->ImageView(),createSampler(_device->getDevice(),linearSampler(_source->mipLevels())),1,vk::ShaderStageFlagBits::eFragment);
	_descSets[Convert]->create();

	_pipelines[Convert]->addShader(vk::ShaderStageFlagBits::eVertex,"assets/cubemap/image2cube_vert.spv");
	_pipelines[Convert]->addShader(vk::ShaderStageFlagBits::eFragment,"assets/cubemap/image2cube_frag.spv");
	_pipelines[Convert]->descSet(_descSets[Convert]);
	_pipelines[Convert]->create();

	for(int i = 0;i<6;++i){
		_framebuffers[i] = _device->create<Framebuffer>();
		_framebuffers[i]->attachment(_cubemap->ImageView(i,0,1,1));
		_framebuffers[i]->depth(CUBEMAP_SIZE,CUBEMAP_SIZE);
		_framebuffers[i]->create(CUBEMAP_SIZE,CUBEMAP_SIZE,_pipelines[Convert]->getRenderPass());
	}

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
	_cmds[Convert].begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	for(int f = 0;f<6;++f){
		vk::RenderPassBeginInfo renderPassInfo(
			_pipelines[Convert]->getRenderPass(),
			_framebuffers[f]->vk_framebuffer(),
			vk::Rect2D(vk::Offset2D(),vk::Extent2D(CUBEMAP_SIZE,CUBEMAP_SIZE)),
			clearValues.size(), clearValues.data()
		);

		_cmds[Convert].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
		_cmds[Convert].bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipelines[Convert]);

		vk::DescriptorSet descSets[] = {_descSets[Convert]->getDescriptorSet()};

		_cmds[Convert].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines[Convert]->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

		_cmds[Convert].pushConstants(_pipelines[Convert]->getPipelineLayout(),vk::ShaderStageFlagBits::eAllGraphics,0,sizeof(int),&f);
		_cube->draw(_cmds[Convert]);

		_cmds[Convert].endRenderPass();
	}

	_cmds[Convert].end();
}

void IBL::initIrradiance(){
	auto irradiancePattern = RenderPattern::basic(_device);
	irradiancePattern.depth(true,true,vk::CompareOp::eLess);
	irradiancePattern.scissor(glm::ivec2(0),glm::ivec2(IRRADIANCE_SIZE));
	irradiancePattern.viewport(0,0,IRRADIANCE_SIZE,IRRADIANCE_SIZE);
	irradiancePattern.constants(0,sizeof(int),vk::ShaderStageFlagBits::eAllGraphics);
	irradiancePattern.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eFront);
	irradiancePattern.createRenderPass(vk::Format::eR16G16B16A16Sfloat,_device->depthFormat(),1);
	_pipelines[Irradiance] = std::make_shared<Pipeline>(irradiancePattern,_device->getDevice());

	_irradiance = _device->create<Image>();
	_irradiance->createCubemap(IRRADIANCE_SIZE,IRRADIANCE_SIZE,
							vk::Format::eR16G16B16A16Sfloat,1,vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eColorAttachment);
	_irradiance->transition(vk::ImageLayout::eColorAttachmentOptimal);

	_descSets[Irradiance] = _device->create<DescSet>();
	_descSets[Irradiance]->setUniformBuffer(_uboUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSets[Irradiance]->setTexture(_cubemap->ImageView(),createSampler(_device->getDevice(),linearSampler(_cubemap->mipLevels())),1,vk::ShaderStageFlagBits::eFragment);
	_descSets[Irradiance]->create();

	_pipelines[Irradiance]->addShader(vk::ShaderStageFlagBits::eVertex,"assets/cubemap/image2cube_vert.spv");
	_pipelines[Irradiance]->addShader(vk::ShaderStageFlagBits::eFragment,"assets/cubemap/irradiance_frag.spv");
	_pipelines[Irradiance]->descSet(_descSets[Irradiance]);
	_pipelines[Irradiance]->create();

	for(int i = 0;i<6;++i){
		_irrFramebuffers[i] = _device->create<Framebuffer>();
		_irrFramebuffers[i]->attachment(_irradiance->ImageView(i,0,1));
		_irrFramebuffers[i]->depth(IRRADIANCE_SIZE,IRRADIANCE_SIZE);
		_irrFramebuffers[i]->create(IRRADIANCE_SIZE,IRRADIANCE_SIZE,_pipelines[Irradiance]->getRenderPass());
	}

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
	_cmds[Irradiance].begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	for(int f = 0;f<6;++f){
		vk::RenderPassBeginInfo renderPassInfo(
			_pipelines[Irradiance]->getRenderPass(),
			_irrFramebuffers[f]->vk_framebuffer(),
			vk::Rect2D(vk::Offset2D(),vk::Extent2D(IRRADIANCE_SIZE,IRRADIANCE_SIZE)),
			clearValues.size(), clearValues.data()
		);

		_cmds[Irradiance].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
		_cmds[Irradiance].bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipelines[Irradiance]);

		vk::DescriptorSet descSets[] = {_descSets[Irradiance]->getDescriptorSet()};

		_cmds[Irradiance].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines[Irradiance]->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

		_cmds[Irradiance].pushConstants(_pipelines[Irradiance]->getPipelineLayout(),vk::ShaderStageFlagBits::eAllGraphics,0,sizeof(int),&f);
		_cube->draw(_cmds[Irradiance]);

		_cmds[Irradiance].endRenderPass();
	}
	_cmds[Irradiance].end();
}

struct FilterConsts {
	int face;
	float rough;
};

void IBL::initFilter(){
	auto pattern = RenderPattern::basic(_device);
	pattern.depth(true,true,vk::CompareOp::eLess);
	pattern.dynamicScissor();
	pattern.dynamicViewport();
	pattern.constants(0,sizeof(FilterConsts),vk::ShaderStageFlagBits::eAllGraphics);
	pattern.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eFront);
	pattern.createRenderPass(vk::Format::eR16G16B16A16Sfloat,_device->depthFormat(),1);
	_pipelines[Filter] = std::make_shared<Pipeline>(pattern,_device->getDevice());

	_descSets[Filter] = _device->create<DescSet>();
	_descSets[Filter]->setUniformBuffer(_uboUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSets[Filter]->setTexture(_cubemap->ImageView(0,0,-1,1,vk::ImageViewType::eCube),createSampler(_device->getDevice(),linearSampler(1)),1,vk::ShaderStageFlagBits::eFragment);
	_descSets[Filter]->create();

	_pipelines[Filter]->addShader(vk::ShaderStageFlagBits::eVertex,"assets/cubemap/filter_vert.spv");
	_pipelines[Filter]->addShader(vk::ShaderStageFlagBits::eFragment,"assets/cubemap/filter_frag.spv");
	_pipelines[Filter]->descSet(_descSets[Filter]);
	_pipelines[Filter]->create();

	const int roughLevels = _cubemap->mipLevels()-1;
	_roughFramebuffers.resize(roughLevels*6);
	for(int m = 0;m<roughLevels;++m){
		int mipsize = CUBEMAP_SIZE >> m+1;
		for(int i = 0;i<6;++i){
			_roughFramebuffers[m*6+i] = _device->create<Framebuffer>();
			_roughFramebuffers[m*6+i]->attachment(_cubemap->ImageView(i,m+1,1,1));
			_roughFramebuffers[m*6+i]->depth(mipsize,mipsize);
			_roughFramebuffers[m*6+i]->create(mipsize,mipsize,_pipelines[Filter]->getRenderPass());
		}
	}

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
	_cmds[Filter].begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	FilterConsts consts;

	for(int m = 0;m<roughLevels;++m){
		int mipsize = CUBEMAP_SIZE >> m+1;
		for(int f = 0;f<6;++f){
			vk::RenderPassBeginInfo renderPassInfo(
				_pipelines[Filter]->getRenderPass(),
				_roughFramebuffers[m*6+f]->vk_framebuffer(),
				vk::Rect2D(vk::Offset2D(),vk::Extent2D(mipsize,mipsize)),
				clearValues.size(), clearValues.data()
			);

			_cmds[Filter].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
			_cmds[Filter].bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipelines[Filter]);

			vk::Viewport viewport(0,0,mipsize,mipsize);
			vk::Rect2D   scissor(vk::Offset2D(),vk::Extent2D(mipsize,mipsize));
			_cmds[Filter].setViewport(0,viewport);
			_cmds[Filter].setScissor(0,1,&scissor);

			vk::DescriptorSet descSets[] = {_descSets[Filter]->getDescriptorSet()};

			_cmds[Filter].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelines[Filter]->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

			consts.face = f;
			consts.rough = (float)(m)/((float)roughLevels-1.0f);
			_cmds[Filter].pushConstants(_pipelines[Filter]->getPipelineLayout(),vk::ShaderStageFlagBits::eAllGraphics,0,sizeof(FilterConsts),&consts);
			_cube->draw(_cmds[Filter]);

			_cmds[Filter].endRenderPass();
		}
	}
	_cmds[Filter].end();
}

void IBL::initBRDF(){
	auto pattern = RenderPattern::basic(_device);
	pattern.depth(true,true,vk::CompareOp::eLess);
	pattern.scissor(glm::ivec2(0),glm::ivec2(BRDF_SIZE,BRDF_SIZE));
	pattern.viewport(0,0,BRDF_SIZE,BRDF_SIZE);
	pattern.blend(1,false);
	pattern.createRenderPass(vk::Format::eR16G16Sfloat,_device->depthFormat(),1);
	_pipelines[BRDF] = std::make_shared<Pipeline>(pattern,_device->getDevice());

	_brdf = _device->create<Image>();
	_brdf->create(BRDF_SIZE,BRDF_SIZE,
							vk::Format::eR16G16Sfloat,1,vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eColorAttachment);
	_brdf->transition(vk::ImageLayout::eColorAttachmentOptimal);

	_pipelines[BRDF]->addShader(vk::ShaderStageFlagBits::eVertex,"assets/cubemap/brdf_vert.spv");
	_pipelines[BRDF]->addShader(vk::ShaderStageFlagBits::eFragment,"assets/cubemap/brdf_frag.spv");
	_pipelines[BRDF]->create();

	_brdfFramebuffer = _device->create<Framebuffer>();
	_brdfFramebuffer->attachment(_brdf->ImageView());
	_brdfFramebuffer->depth(BRDF_SIZE,BRDF_SIZE);
	_brdfFramebuffer->create(BRDF_SIZE,BRDF_SIZE,_pipelines[BRDF]->getRenderPass());

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
	_cmds[BRDF].begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderPassBeginInfo renderPassInfo(
		_pipelines[BRDF]->getRenderPass(),
		_brdfFramebuffer->vk_framebuffer(),
		vk::Rect2D(vk::Offset2D(),vk::Extent2D(BRDF_SIZE,BRDF_SIZE)),
		clearValues.size(), clearValues.data()
	);

	_cmds[BRDF].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
	_cmds[BRDF].bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipelines[BRDF]);

	_quad->draw(_cmds[BRDF]);

	_cmds[BRDF].endRenderPass();
	_cmds[BRDF].end();
}

void IBL::run(){
	auto vk_device = _device->getDevice();

	vk::FenceCreateInfo fenceCreateInfo;
	vk::Fence fence;
	fence = vk_device.createFence(fenceCreateInfo);

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	for(int i = 0;i<NumSteps;++i){
		submitInfo.pCommandBuffers = &_cmds[i];

		_device->getGraphicsQueue().submit(submitInfo,fence);
		vk_device.waitForFences(fence,true,1000000000);

		vk_device.resetFences(fence);
	}
}

vk::ImageView IBL::cubemap(){
	return _cubemap->ImageView();
}

vk::ImageView IBL::brdf(){
	return _brdf->ImageView();
}

vk::ImageView IBL::irradiance(){
	return _irradiance->ImageView();
}
