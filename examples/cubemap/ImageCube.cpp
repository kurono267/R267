#include "ImageCube.hpp"

#define CUBEMAP_SIZE 1024
#define IRRADIANCE_SIZE 64

void ImageCube::init(spDevice device,spImage source){
	_source = source;
	_device = device;

	_cube = std::make_shared<Cube>();
	_cube->create(device);

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
}

void ImageCube::initConvert(){
	auto baseRP = RenderPattern::basic(_device);
	baseRP.depth(true,true,vk::CompareOp::eLess);
	baseRP.scissor(glm::ivec2(0),glm::ivec2(CUBEMAP_SIZE));
	baseRP.viewport(0,0,CUBEMAP_SIZE,CUBEMAP_SIZE);
	baseRP.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eFront);
	baseRP.createRenderPass(vk::Format::eR16G16B16A16Sfloat,_device->depthFormat(),1);

	_pipelines[Convert] = std::make_shared<Pipeline>(baseRP,_device->getDevice());

	_cubemap = _device->create<Image>();
	_cubemap->createCubemap(CUBEMAP_SIZE,CUBEMAP_SIZE,
							vk::Format::eR16G16B16A16Sfloat,1,vk::ImageTiling::eOptimal,
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
		_framebuffers[i]->attachment(_cubemap->ImageView(i,0,1));
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

void ImageCube::initIrradiance(){
	auto irradiancePattern = RenderPattern::basic(_device);
	irradiancePattern.depth(true,true,vk::CompareOp::eLess);
	irradiancePattern.scissor(glm::ivec2(0),glm::ivec2(IRRADIANCE_SIZE));
	irradiancePattern.viewport(0,0,IRRADIANCE_SIZE,IRRADIANCE_SIZE);
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

void ImageCube::run(){
	auto vk_device = _device->getDevice();

	vk::FenceCreateInfo fenceCreateInfo;
	vk::Fence fence;
	fence = vk_device.createFence(fenceCreateInfo);

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	for(int i = 0;i<2;++i){
		submitInfo.pCommandBuffers = &_cmds[i];

		_device->getGraphicsQueue().submit(submitInfo,fence);
		vk_device.waitForFences(fence,true,1000000000);

		vk_device.resetFences(fence);
	}
}

vk::ImageView ImageCube::cubemap(){
	return _cubemap->ImageView();
}

vk::ImageView ImageCube::irradiance(){
	return _irradiance->ImageView();
}
