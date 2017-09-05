#include "ImageCube.hpp"

#define CUBEMAP_SIZE 512

void ImageCube::init(spDevice device,spImage source){
	_source = source;
	_device = device;

	_cube = std::make_shared<Cube>();
	_cube->create(device);

	auto baseRP = RenderPattern::basic(device);
	baseRP.depth(true,true,vk::CompareOp::eGreater);
	baseRP.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eFront);
	_pipeline = std::make_shared<Pipeline>(baseRP,_device->getDevice());

	_descSet = device->create<DescSet>();

	_cubemap = device->create<Image>();
	_cubemap->createCubemap(CUBEMAP_SIZE,CUBEMAP_SIZE,
							vk::Format::eR32G32B32A32Sfloat,1,vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eColorAttachment,
							vk::ImageLayout::eColorAttachmentOptimal);

	spImage cubemapDepth = device->create<Image>();
	cubemapDepth->createCubemap(CUBEMAP_SIZE,CUBEMAP_SIZE,
							vk::Format::eD24UnormS8Uint,1,vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eDepthStencilAttachment,
							vk::ImageLayout::eDepthStencilAttachmentOptimal);

	// Create matrix
	glm::mat4 proj = glm::perspective(glm::radians(90.0f),1.0f,0.1f,2.0f);
	_uboData.mat[0] = glm::lookAt(glm::vec3(-1.0,0.0,0.0),glm::vec3(0.0f),glm::vec3(0.0f,0.0f,1.0f));
	_uboData.mat[1] = glm::lookAt(glm::vec3(1.0,0.0,0.0),glm::vec3(0.0f),glm::vec3(0.0f,0.0f,1.0f));
	_uboData.mat[2] = glm::lookAt(glm::vec3(0.0,-1.0,0.0),glm::vec3(0.0f),glm::vec3(0.0f,0.0f,1.0f));
	_uboData.mat[3] = glm::lookAt(glm::vec3(0.0,1.0,0.0),glm::vec3(0.0f),glm::vec3(0.0f,0.0f,1.0f));
	_uboData.mat[4] = glm::lookAt(glm::vec3(0.0,0.0,-1.0),glm::vec3(0.0f),glm::vec3(0.0f,0.0f,1.0f));
	_uboData.mat[5] = glm::lookAt(glm::vec3(0.0,0.0,1.0),glm::vec3(0.0f),glm::vec3(0.0f,0.0f,1.0f));

	_uboUniform.create(_device,sizeof(UBO),&_uboData);

	_descSet->setUniformBuffer(_uboUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSet->setTexture(_source->ImageView(),createSampler(_device,linearSampler(source->mipLevels())),1,vk::ShaderStageFlagBits::eFragment);

	_pipeline->addShader(vk::ShaderStageFlagBits::eVertex,"assets/cubemap/image2cube_vert.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eFragment,"assets/cubemap/image2cube_frag.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eGeometry,"assets/cubemap/image2cube_geom.spv");
	_pipeline->create();

	vk::CommandBufferAllocateInfo allocInfo(_device->getCommandPool(),vk::CommandBufferLevel::ePrimary, 1);
	_cmd = _device->getDevice().allocateCommandBuffers(allocInfo)[0];

	// Fill Render passes
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

	_cmd.begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.5f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(0.0f, 0);

	vk::RenderPassBeginInfo renderPassInfo(
		_pipeline->getRenderPass(),
		_framebuffer->vk_framebuffer(),
		vk::Rect2D(vk::Offset2D(),vk::Extent2D(CUBEMAP_SIZE,CUBEMAP_SIZE)),
		clearValues.size(), clearValues.data()
	);

	_cmd.beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
	_cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);

	vk::DescriptorSet descSets[] = {_descSet->getDescriptorSet()};

	_cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

	_cube->draw(_cmd);

	_cmd.endRenderPass();
	_cmd.end();
}

void ImageCube::run(){
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_cmd;

	vk::Fence fence;
	_device->getGraphicsQueue().submit(submitInfo,fence);
	_device->getDevice().waitForFences(fence,true,1000000000);
}

vk::ImageView ImageCube::cubemap(){
	return _cubemap;
}
