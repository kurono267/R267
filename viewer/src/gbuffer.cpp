//
// Created by kurono267 on 18.08.17.
//

#include "gbuffer.hpp"

void GBuffer::init(spDevice device,spScene scene,spCamera camera,spDescSet matDescSet,const glm::ivec2& size){
	_scene  = scene;
	_camera = camera;
	_size   = size;
	_device = device;

	// Setup Pipeline

	RenderPattern rpGBuffer;
	rpGBuffer = RenderPattern::basic(_device);
	rpGBuffer.blend(3,false);
	//rpGBuffer.rasterizer(vk::PolygonMode::eLine);
	rpGBuffer.inputAssembly(vk::PrimitiveTopology::ePatchList);
	rpGBuffer.createRenderPass(vk::Format::eR32G32B32A32Sfloat,_device->depthFormat(),3);

	_pipeline = std::make_shared<Pipeline>(rpGBuffer,device->getDevice());
	_pipeline->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/effects/gbuffer_vert.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eTessellationControl,"../shaders/effects/displace_tesc.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eTessellationEvaluation,"../shaders/effects/displace_tese.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/effects/gbuffer_frag.spv");

	// Create and fill Uniform buffer

	_cameraData.mvp = _camera->getVP();
    _cameraData.view = glm::vec4(_camera->getPos(),1.0f);
    _cameraData.proj = _camera->getProj();
    _cameraUniform.create(device,sizeof(CameraData),&_cameraData);

	_cameraDesc = device->create<DescSet>();
	_cameraDesc->setUniformBuffer(_cameraUniform,0,vk::ShaderStageFlagBits::eTessellationEvaluation|vk::ShaderStageFlagBits::eTessellationControl);
    _cameraDesc->create();

    // Finish setup pipeline and create it

    _pipeline->descSets({_cameraDesc,matDescSet});
    _pipeline->create();

    // Create textures for GBuffer
    _posMap = device->create<Image>();
    _normalMap = device->create<Image>();
    _colorMap  = device->create<Image>();
    _posMap->create(_size.x,_size.y,vk::Format::eR32G32B32A32Sfloat,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
    _normalMap->create(_size.x,_size.y,vk::Format::eR32G32B32A32Sfloat,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
    _colorMap->create(_size.x,_size.y,vk::Format::eR32G32B32A32Sfloat,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    _framebuffer = device->create<Framebuffer>();
    _framebuffer->attachment(_posMap->ImageView());
    _framebuffer->attachment(_normalMap->ImageView());
    _framebuffer->attachment(_colorMap->ImageView());
    _framebuffer->depth(_size.x,_size.y);
    _framebuffer->create(_size.x,_size.y,_pipeline->getRenderPass());

    _finish = device->createSemaphore(vk::SemaphoreCreateInfo());

    // Create command buffer
    createCommands();
}

vk::Semaphore GBuffer::render(vk::Semaphore& wait){
	_cameraData.mvp = _camera->getVP();
	_cameraData.viewMat = _camera->getView();
    _cameraData.view = glm::vec4(_camera->getPos(),1.0f);
    _cameraUniform.set(sizeof(CameraData),&_cameraData);

	vk::Semaphore waitSemaphores[] = {wait};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::Semaphore signalSemaphores[] = {_finish};

    vk::SubmitInfo submitInfo(
            1, waitSemaphores,
            waitStages,
            1, &_command,
            1, signalSemaphores
    );

    _device->getGraphicsQueue().submit(submitInfo, nullptr);
    return _finish;
}

void GBuffer::createCommands(){
	_commandPool = _device->getCommandPool();

	vk::CommandBufferAllocateInfo allocInfo(_commandPool,vk::CommandBufferLevel::ePrimary, 1);
    _command = _device->getDevice().allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    _command.begin(&beginInfo);

    std::array<vk::ClearValue, 4> clearValues = {};
    clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 0.0f});
    clearValues[1].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 0.0f});
    clearValues[2].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 0.0f});
    clearValues[3].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassInfo(
            _pipeline->getRenderPass(),
            _framebuffer->vk_framebuffer(),
            vk::Rect2D(vk::Offset2D(),vk::Extent2D(_size.x,_size.y)),
            (uint32_t)clearValues.size(), clearValues.data()
    );

    _command.beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
    _command.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);

    // Draw scene
    for(auto& model : _scene->models()){
        vk::DescriptorSet descSets[] = {_cameraDesc->getDescriptorSet(),model->material()->getDescSet()->getDescriptorSet()};

        _command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->getPipelineLayout(), 0, 2, descSets, 0, nullptr);

        model->mesh()->draw(_command);
    }

    _command.endRenderPass();
    _command.end();
}

void GBuffer::release(){
	_pipeline->release();
}

vk::ImageView GBuffer::posMap() const {
	return _posMap->ImageView();
}

vk::ImageView GBuffer::normalMap() const {
	return _normalMap->ImageView();
}

vk::ImageView GBuffer::colorMap() const {
	return _colorMap->ImageView();
}
