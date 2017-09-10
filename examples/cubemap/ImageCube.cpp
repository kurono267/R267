#include "ImageCube.hpp"

#define CUBEMAP_SIZE 512

void ImageCube::init(spDevice device,spImage source){
	_source = source;
	_device = device;

	_cube = std::make_shared<Cube>();
	_cube->create(device);

	auto baseRP = RenderPattern::basic(device);
	baseRP.depth(true,true,vk::CompareOp::eLess);
	baseRP.viewport(0,0,CUBEMAP_SIZE,CUBEMAP_SIZE);
	baseRP.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eFront);
	baseRP.createRenderPass(vk::Format::eR32G32B32A32Sfloat,_device->depthFormat(),1);
	_pipeline = std::make_shared<Pipeline>(baseRP,_device->getDevice());

	_descSet = device->create<DescSet>();

	_cubemap = device->create<Image>();
	_cubemap->createCubemap(CUBEMAP_SIZE,CUBEMAP_SIZE,
							vk::Format::eR32G32B32A32Sfloat,1,vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eColorAttachment);
	_cubemap->transition(vk::ImageLayout::eColorAttachmentOptimal);

	// Create matrix
	glm::mat4 proj = glm::perspective(glm::radians(90.0f),1.0f,0.1f,100.0f);
	_uboData.mat[0] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f));
    _uboData.mat[1] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f));
    _uboData.mat[2] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
    _uboData.mat[3] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f, 1.0f));
    _uboData.mat[4] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f));
    _uboData.mat[5] = proj*glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, 1.0f), glm::vec3(0.0f, 1.0f,  0.0f));

	_uboUniform.create(_device,sizeof(UBO),&_uboData);

	_descSet->setUniformBuffer(_uboUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSet->setTexture(_source->ImageView(),createSampler(_device->getDevice(),linearSampler(source->mipLevels())),1,vk::ShaderStageFlagBits::eFragment);
	_descSet->create();

	_pipeline->addShader(vk::ShaderStageFlagBits::eVertex,"assets/cubemap/image2cube_vert.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eFragment,"assets/cubemap/image2cube_frag.spv");
	_pipeline->descSet(_descSet);
	_pipeline->create();

	for(int i = 0;i<6;++i){
		_framebuffers[i] = device->create<Framebuffer>();
		_framebuffers[i]->attachment(_cubemap->ImageView(i,0,1));
		_framebuffers[i]->depth(CUBEMAP_SIZE,CUBEMAP_SIZE);
		_framebuffers[i]->create(CUBEMAP_SIZE,CUBEMAP_SIZE,_pipeline->getRenderPass());
	}

	vk::CommandBufferAllocateInfo allocInfo(_device->getCommandPool(),vk::CommandBufferLevel::ePrimary, 1);
	_cmd = _device->getDevice().allocateCommandBuffers(allocInfo)[0];

	// Fill Render passes
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

	_cmd.begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	for(int f = 0;f<6;++f){
		vk::RenderPassBeginInfo renderPassInfo(
			_pipeline->getRenderPass(),
			_framebuffers[f]->vk_framebuffer(),
			vk::Rect2D(vk::Offset2D(),vk::Extent2D(CUBEMAP_SIZE,CUBEMAP_SIZE)),
			clearValues.size(), clearValues.data()
		);

		_cmd.beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
		_cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);

		vk::DescriptorSet descSets[] = {_descSet->getDescriptorSet()};

		_cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

		_cmd.pushConstants(_pipeline->getPipelineLayout(),vk::ShaderStageFlagBits::eAllGraphics,0,sizeof(int),&f);
		_cube->draw(_cmd);

		_cmd.endRenderPass();
	}
	_cmd.end();
}

void ImageCube::run(){
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_cmd;

	vk::FenceCreateInfo fenceCreateInfo;
	vk::Fence fence;
	fence = _device->getDevice().createFence(fenceCreateInfo);
	_device->getGraphicsQueue().submit(submitInfo,fence);
	_device->getDevice().waitForFences(fence,true,1000000000);
}

vk::ImageView ImageCube::cubemap(){
	return _cubemap->ImageView();
}
