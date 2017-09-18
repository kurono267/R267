//
// Created by kurono267 on 05.08.17.
//

#include "viewer.hpp"

using namespace r267;

bool ViewerApp::init(){
    vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
    _commandPool = device->getCommandPool();
    glm::ivec2 wnd = mainApp->wndSize();

    // Load scene
    _scene = std::make_shared<Scene>();
    _scene->load(_filename);
    spDescSet matDesc;
    // Create Vulkan meshes
    for(auto& m : _scene->models()){
        m->mesh()->create(device);
        m->material()->create(device,_imagesBuffer);
        if(!matDesc)matDesc = m->material()->getDescSet();
    }

    _toolbar.setScene(_scene);

    _background = loadImage(device,"models/temple/JerusalemPanorama.jpg");

    _ibl.init(device,_background);
    _ibl.run();

    auto baseRP = RenderPattern::basic(device);
    baseRP.blend(1,false);
    _main = std::make_shared<Pipeline>(baseRP,vk_device);

    _main->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/effects/differed_vert.spv");
    _main->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/effects/differed_frag.spv");

    _camera = std::make_shared<Camera>(vec3(0.0f, 0.0f, -15.0f),vec3(0.0f,0.0f,0.0f),vec3(0.0,-1.0f,0.0f));
    _camera->setProj(glm::radians(45.0f),(float)(wnd.x)/(float)(wnd.y),0.1f,10000.0f);

    // Init GBuffer
    _gbuffer.init(device,_scene,_camera,matDesc,glm::ivec2(1280,720));
    _ssao.init(device,_gbuffer,glm::ivec2(1280,720));

    _ubo.view = glm::vec4(_camera->getPos(),1.0f);
    _ubo.viewMat = _camera->getView();
    _ubo.proj = _camera->getProj();
    _ubo.invview  = inverse(_camera->getView());
    _ubo.invproj  = inverse(_camera->getProj());
    _uniform.create(device,sizeof(UBO),&_ubo);

	_differedDesc = device->create<DescSet>();
	_differedDesc->setTexture(_gbuffer.posMap(),createSampler(device->getDevice(),linearSampler()),0,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_gbuffer.normalMap(),createSampler(device->getDevice(),linearSampler()),1,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_gbuffer.colorMap(),createSampler(device->getDevice(),linearSampler()),2,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_ssao.ssaoImage(),createSampler(device->getDevice(),linearSampler()),3,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_ibl.cubemap(),createSampler(device->getDevice(),linearSampler(10)),4,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_ibl.irradiance(),createSampler(device->getDevice(),linearSampler(1)),5,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_ibl.brdf(),createSampler(device->getDevice(),linearSampler(1)),6,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setUniformBuffer(_uniform,7,vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eVertex);
	_differedDesc->create();

	_main->descSet(_differedDesc);
    _main->create();

	_quad = std::make_shared<Quad>();
	_quad->create(device);

    _framebuffers = createFrameBuffers(device,_main);

    _guiFunc = std::bind(&Toolbar::update,std::ref(_toolbar),std::placeholders::_1);

    _gui = std::make_shared<GUI>(device);
    vk::CommandBufferInheritanceInfo inheritanceInfo(_main->getRenderPass());
    _gui->create(mainApp->wndSize(),inheritanceInfo);
    _gui->update(_guiFunc);

    vk::CommandBufferAllocateInfo allocInfo(_commandPool,vk::CommandBufferLevel::ePrimary, (uint32_t)_framebuffers.size());
    _commandBuffers = vk_device.allocateCommandBuffers(allocInfo);

    // Create semaphores
    _imageAvailable = device->createSemaphore(vk::SemaphoreCreateInfo());
    _renderFinish = device->createSemaphore(vk::SemaphoreCreateInfo());

    prev = std::chrono::steady_clock::now();

    _differedBuffer = vk_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(_commandPool,vk::CommandBufferLevel::eSecondary,1))[0];
    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse | vk::CommandBufferUsageFlagBits::eRenderPassContinue);
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    _differedBuffer.begin(beginInfo);

    _differedBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

    vk::DescriptorSet descSet = _differedDesc->getDescriptorSet();
    _differedBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _main->getPipelineLayout(), 0, 1, &descSet, 0, nullptr);

    _quad->draw(_differedBuffer);

    _differedBuffer.end();

    return true;
}

void ViewerApp::updateCommandBuffers(){
    vk::DescriptorSet descSet = _differedDesc->getDescriptorSet();
    for(int i = 0;i<_commandBuffers.size();++i){
        // Fill Render passes
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        _commandBuffers[i].reset(vk::CommandBufferResetFlagBits::eReleaseResources);
        _commandBuffers[i].begin(&beginInfo);

        std::array<vk::ClearValue, 2> clearValues = {};
        clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.5f, 0.0f, 1.0f});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        vk::RenderPassBeginInfo renderPassInfo(
                _main->getRenderPass(),
                _framebuffers[i]->vk_framebuffer(),
                vk::Rect2D(vk::Offset2D(),swapchain->getExtent()),
                (uint32_t)clearValues.size(), clearValues.data()
        );

        _commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eSecondaryCommandBuffers);

        _commandBuffers[i].executeCommands(_differedBuffer);
        _commandBuffers[i].executeCommands(_gui->commandBuffer());

        _commandBuffers[i].endRenderPass();
        _commandBuffers[i].end();
    }
}

bool ViewerApp::draw(){
	_uniform.set(sizeof(UBO),&_ubo);
    unsigned int imageIndex = vk_device.acquireNextImageKHR(swapchain->getSwapchain(),std::numeric_limits<uint64_t>::max(),_imageAvailable,nullptr).value;

    auto next = std::chrono::steady_clock::now();
    _dt = std::chrono::duration_cast<std::chrono::duration<double> >(next - prev).count();
    std::stringstream title;
    title << "Viewer " << " FPS: " << 1.0f/_dt;
    glfwSetWindowTitle(mainApp->window(),title.str().c_str());
    prev = next;

    vk::Semaphore waitGBuffer = _gbuffer.render(_imageAvailable);
	vk::Semaphore waitSSAO    = _ssao.render(device,waitGBuffer);

    vk::Semaphore waitSemaphores[] = {waitSSAO};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::Semaphore signalSemaphores[] = {_renderFinish};

    vk::SubmitInfo submitInfo(
            1, waitSemaphores,
            waitStages,
            1, &_commandBuffers[imageIndex],
            1, signalSemaphores
    );

    device->getGraphicsQueue().submit(submitInfo, nullptr);

    vk::SwapchainKHR swapChains[] = {swapchain->getSwapchain()};

    vk::PresentInfoKHR presentInfo(
            1, signalSemaphores,
            1, swapChains,
            &imageIndex
    );

    device->getPresentQueue().presentKHR(presentInfo);

    return true;
}

bool ViewerApp::update(){
    _guiEvents = _gui->actionUpdate(mainApp->window());
    _gui->update(_guiFunc);
    updateCommandBuffers();
    _ssao.update(_camera);
    _ubo.view = glm::vec4(_camera->getPos(),1.0f);
    _ubo.viewMat = _camera->getView();
    _ubo.invview  = inverse(_camera->getView());
    return true;
}