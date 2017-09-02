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

    auto baseRP = RenderPattern::basic(device);
    baseRP.blend();
    _main = std::make_shared<Pipeline>(baseRP,vk_device);

    _main->addShader(vk::ShaderStageFlagBits::eVertex,"assets/effects/differed_vert.spv");
    _main->addShader(vk::ShaderStageFlagBits::eFragment,"assets/effects/differed_frag.spv");

    _camera = std::make_shared<Camera>(vec3(0.0f, 0.0f, -15.0f),vec3(0.0f,0.0f,0.0f),vec3(0.0,-1.0f,0.0f));
    _camera->setProj(glm::radians(45.0f),(float)(wnd.x)/(float)(wnd.y),0.1f,10000.0f);

    // Init GBuffer
    _gbuffer.init(device,_scene,_camera,matDesc,glm::ivec2(1280,720));

	_differedDesc = device->create<DescSet>();
	_differedDesc->setTexture(_gbuffer.posMap(),createSampler(device->getDevice(),linearSampler()),0,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_gbuffer.normalMap(),createSampler(device->getDevice(),linearSampler()),1,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->setTexture(_gbuffer.colorMap(),createSampler(device->getDevice(),linearSampler()),2,vk::ShaderStageFlagBits::eFragment);
	_differedDesc->create();

	_main->descSet(_differedDesc);
    _main->create();

	_quad = std::make_shared<Quad>();
	_quad->create(device);

    _framebuffers = createFrameBuffers(device,_main);

    _guiFunc = std::bind(&Toolbar::update,std::ref(_toolbar),std::placeholders::_1);

    _gui = std::make_shared<GUI>(device);
    _gui->create(mainApp->wndSize(),vk::CommandBufferInheritanceInfo(_main->getRenderPass()));
    _gui->update(_guiFunc);

    vk::CommandBufferAllocateInfo allocInfo(_commandPool,vk::CommandBufferLevel::ePrimary, (uint32_t)_framebuffers.size());
    _commandBuffers = vk_device.allocateCommandBuffers(allocInfo);

    vk::DescriptorSet descSet = _differedDesc->getDescriptorSet();

    for(int i = 0;i<_commandBuffers.size();++i){
        // Fill Render passes
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

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

        _commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
        _commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

        _commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _main->getPipelineLayout(), 0, 1, &descSet, 0, nullptr);

		_quad->draw(_commandBuffers[i]);

        _commandBuffers[i].executeCommands(_gui->commandBuffer());

        _commandBuffers[i].endRenderPass();
        _commandBuffers[i].end();
    }

    // Create semaphores
    _imageAvailable = device->createSemaphore(vk::SemaphoreCreateInfo());
    _renderFinish = device->createSemaphore(vk::SemaphoreCreateInfo());

    prev = std::chrono::steady_clock::now();

    return true;
}

bool ViewerApp::draw(){
    unsigned int imageIndex = vk_device.acquireNextImageKHR(swapchain->getSwapchain(),std::numeric_limits<uint64_t>::max(),_imageAvailable,nullptr).value;

    auto next = std::chrono::steady_clock::now();
    _dt = std::chrono::duration_cast<std::chrono::duration<double> >(next - prev).count();
    prev = next;

    vk::Semaphore waitGBuffer = _gbuffer.render(_imageAvailable);

    vk::Semaphore waitSemaphores[] = {waitGBuffer};
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
    return true;
}