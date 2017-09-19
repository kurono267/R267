#include "ssao.hpp"

float lerp(float a, float b, float f) {
    return a + f * (b - a);
}

void ssaoKernelGen(spDevice device,ssaoUBO& ubo){
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
	    glm::vec3 sample(
	        randomFloats(generator) * 2.0 - 1.0,
	        randomFloats(generator) * 2.0 - 1.0,
	        randomFloats(generator)
	    );
	    sample  = glm::normalize(sample);
	    sample *= randomFloats(generator);
	    float scale = (float)i / 64.0;
	    scale   = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ubo.kernels[i] = glm::vec4(sample,1.0f);
	}
}

spImage ssaoKernelRotationGen(spDevice device){
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++){
	    glm::vec3 noise(
	        randomFloats(generator) * 2.0 - 1.0,
	        randomFloats(generator) * 2.0 - 1.0,
	        0.0f);
	    ssaoNoise.push_back(noise);
	}
	spImage kernelImage = device->create<Image>();
	spBuffer cpu     = device->create<Buffer>();
	cpu->create(64*sizeof(glm::vec3),vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(ssaoNoise.data(),16*sizeof(glm::vec3));
	kernelImage->create(4,4,vk::Format::eR16G16B16A16Sfloat);
	kernelImage->set(cpu);

	return kernelImage;
}

void SSAO::init(spDevice device,const GBuffer& gbuffer,const glm::ivec2& size){
	_size = size;

	auto baseRP = RenderPattern::basic(device);
    baseRP.blend(1,false);
    baseRP.depth(false,false);
    baseRP.createRenderPass(vk::Format::eR16G16B16A16Sfloat,device->depthFormat(),1);
    _main = std::make_shared<Pipeline>(baseRP,device->getDevice());

    _main->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/effects/ssao_vert.spv");
    _main->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/effects/ssao_frag.spv");

     ssaoKernelGen(device,_ssaoData);
    _rotationImage = ssaoKernelRotationGen(device);

    _ssaoUniform.create(device,sizeof(ssaoUBO),&_ssaoData);

    vk::SamplerCreateInfo info(
		vk::SamplerCreateFlags(),
		vk::Filter::eNearest, // Mag Filter
		vk::Filter::eNearest, // Min Filter
		vk::SamplerMipmapMode::eNearest, // MipMap Mode
		vk::SamplerAddressMode::eRepeat, // U Address mode
		vk::SamplerAddressMode::eRepeat, // V Address mode
		vk::SamplerAddressMode::eRepeat, // W Address mode
		0, // Mip Lod bias
		0, // Anisotropic enabled
		0, // Max anisotropy
		0, // Compare enabled
		vk::CompareOp::eAlways, // Compare Operator
		0, // Min lod
		0, // Max lod
		vk::BorderColor::eFloatTransparentBlack, // Border color
		0 // Unnormalized coordiante
    );

    _descSet = device->create<DescSet>();
	_descSet->setTexture(gbuffer.posMap(),createSampler(device->getDevice(),info),0,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(gbuffer.normalMap(),createSampler(device->getDevice(),info),1,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(gbuffer.colorMap(),createSampler(device->getDevice(),linearSampler()),2,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(_rotationImage->ImageView(),createSampler(device->getDevice(),info),3,vk::ShaderStageFlagBits::eFragment);
	_descSet->setUniformBuffer(_ssaoUniform,4,vk::ShaderStageFlagBits::eFragment);
	_descSet->create();

	_main->descSet(_descSet);
    _main->create();

    _quad = std::make_shared<Quad>();
	_quad->create(device);

	_ssaoImage = device->create<Image>();
	_ssaoImage->create(_size.x,_size.y,vk::Format::eR16G16B16A16Sfloat,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

	_framebuffer = device->create<Framebuffer>();
	_framebuffer->attachment(_ssaoImage->ImageView());
	_framebuffer->depth(_size.x,_size.y);
	_framebuffer->create(_size.x,_size.y,_main->getRenderPass());

	_finish = device->createSemaphore(vk::SemaphoreCreateInfo());

	createCommands(device);
}

void SSAO::update(spCamera camera){
    _ssaoData.proj = camera->getProj();
    _ssaoData.view = camera->getView();
    _ssaoData.invmvp = glm::inverse(camera->getProj());
    _ssaoUniform.set(sizeof(ssaoUBO),&_ssaoData);
}

vk::Semaphore SSAO::render(spDevice device,vk::Semaphore& wait){
	vk::Semaphore waitSemaphores[] = {wait};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::Semaphore signalSemaphores[] = {_finish};

    vk::SubmitInfo submitInfo(
            1, waitSemaphores,
            waitStages,
            1, &_command,
            1, signalSemaphores
    );

    device->getGraphicsQueue().submit(submitInfo, nullptr);
    return _finish;
}

void SSAO::createCommands(spDevice device){
	vk::CommandPool commandPool = device->getCommandPool();

	vk::CommandBufferAllocateInfo allocInfo(commandPool,vk::CommandBufferLevel::ePrimary, 1);
    _command = device->getDevice().allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    _command.begin(&beginInfo);

    std::array<vk::ClearValue, 2> clearValues = {};
    clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f});
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassInfo(
            _main->getRenderPass(),
            _framebuffer->vk_framebuffer(),
            vk::Rect2D(vk::Offset2D(),vk::Extent2D(_size.x,_size.y)),
            (uint32_t)clearValues.size(), clearValues.data()
    );

    _command.beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
    _command.bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

	vk::DescriptorSet descSet = _descSet->getDescriptorSet();
    _command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _main->getPipelineLayout(), 0, 1, &descSet, 0, nullptr);

    _quad->draw(_command);

    _command.endRenderPass();
    _command.end();
}

void SSAO::release(){
	_main->release();
}

vk::ImageView SSAO::ssaoImage(){
	return _ssaoImage->ImageView();
}
