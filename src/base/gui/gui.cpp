#include "gui.hpp"

using namespace r267;

struct guiVertex {
	glm::vec2 pos;
	glm::vec2 uv;
	glm::u8vec4 color;

	static vk::VertexInputBindingDescription bindingDesc(){
		return vk::VertexInputBindingDescription(0,sizeof(guiVertex));
	}

	static std::vector<vk::VertexInputAttributeDescription> attributes(){
		std::vector<vk::VertexInputAttributeDescription> attrs(3);
		attrs[0] = vk::VertexInputAttributeDescription(0,0,vk::Format::eR32G32Sfloat,offsetof(guiVertex,pos));
		attrs[1] = vk::VertexInputAttributeDescription(1,0,vk::Format::eR32G32Sfloat,offsetof(guiVertex,uv));
		attrs[2] = vk::VertexInputAttributeDescription(2,0,vk::Format::eR8G8B8A8Unorm,offsetof(guiVertex,color));
		return attrs;
	}
};

#define MAX_VERTEX_BUFFER_GUI  512 * 1024
#define MAX_ELEMENT_BUFFER_GUI 128 * 1024

GUI::GUI(spDevice device) : _device(device) {}
GUI::~GUI(){}

void GUI::create(const glm::ivec2& size){
	_size = size;
	nk_init_default(&_ctx, 0);
	nk_buffer_init_default(&_cmds);
	// Create pipeline pattern
	RenderPattern pattern;
	pattern.inputAssembly(vk::PrimitiveTopology::eTriangleList);
	pattern.viewport(0,0,size.x,size.y);
	pattern.scissor(vk::Offset2D(),vk::Extent2D(size.x,size.y));
	pattern.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eNone);
	pattern.multisampling();
	pattern.blend(true,RGBA,
					vk::BlendFactor::eSrcAlpha,vk::BlendFactor::eOneMinusSrcAlpha,
					vk::BlendOp::eAdd,
					vk::BlendFactor::eSrcAlpha,vk::BlendFactor::eOneMinusSrcAlpha,
					vk::BlendOp::eAdd);
	pattern.depth(false);
	pattern.createRenderPass(_device->getSwapchain()->getFormat(),_device->depthFormat());
	// Create Pipeline
	_pipeline = std::make_shared<Pipeline>(pattern,_device->getDevice());
	_pipeline->addShader(vk::ShaderStageFlagBits::eVertex,"assets/gui/main_vert.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eFragment,"assets/gui/main_frag.spv");

	_descSet = _device->create<DescSet>();
	
	// Create NK atlas
	nk_font_atlas_init_default(&_atlas);
    nk_font_atlas_begin(&_atlas);
    const void *image; int wAtlas, hAtlas;
    image = nk_font_atlas_bake(&_atlas, &wAtlas, &hAtlas, NK_FONT_ATLAS_RGBA32);

	vk::DeviceSize atlasSize = wAtlas*hAtlas*4;

	spBuffer cpu = _device->create<Buffer>();
	cpu->create(atlasSize,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(image,atlasSize);

    _vkAtlas = _device->create<Image>();
	_vkAtlas->create(wAtlas,hAtlas,vk::Format::eR8G8B8A8Unorm);
	_vkAtlas->set(cpu);
   
    nk_draw_null_texture null_tex;
    nk_font_atlas_end(&_atlas, nk_handle_id((int)1), &null_tex);
    if (_atlas.default_font)
        nk_style_set_font(&_ctx, &_atlas.default_font->handle);

    	nk_color background = nk_rgb(28,48,62);
	if (nk_begin(&_ctx, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            nk_layout_row_static(&_ctx, 30, 80, 1);
            if (nk_button_label(&_ctx, "button"))
                fprintf(stdout, "button pressed\n");

            nk_layout_row_dynamic(&_ctx, 30, 2);
            if (nk_option_label(&_ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(&_ctx, "hard", op == HARD)) op = HARD;

            nk_layout_row_dynamic(&_ctx, 25, 1);
            nk_property_int(&_ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(&_ctx, 20, 1);
            nk_label(&_ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(&_ctx, 25, 1);
            if (nk_combo_begin_color(&_ctx, background, nk_vec2(nk_widget_width(&_ctx),400))) {
                nk_layout_row_dynamic(&_ctx, 120, 1);
                background = nk_color_picker(&_ctx, background, NK_RGBA);
                nk_layout_row_dynamic(&_ctx, 25, 1);
                background.r = (nk_byte)nk_propertyi(&_ctx, "#R:", 0, background.r, 255, 1,1);
                background.g = (nk_byte)nk_propertyi(&_ctx, "#G:", 0, background.g, 255, 1,1);
                background.b = (nk_byte)nk_propertyi(&_ctx, "#B:", 0, background.b, 255, 1,1);
                background.a = (nk_byte)nk_propertyi(&_ctx, "#A:", 0, background.a, 255, 1,1);
                nk_combo_end(&_ctx);
            }
        }
        nk_end(&_ctx);

    // Create Uniform 
	glm::mat4 ortho = glm::mat4({
        2.0f/(float)size.x, 0.0f, 0.0f, 0.0f,
        0.0f,-2.0f/(float)size.y, 0.0f, 0.0f,
        0.0f, 0.0f,-1.0f, 0.0f,
        -1.0f,1.0f, 0.0f, 1.0f
    });
    Uniform orthoUniform;
	orthoUniform.create(_device,sizeof(glm::mat4),&ortho);
	_descSet->setUniformBuffer(orthoUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSet->setTexture(_vkAtlas->createImageView(),createSampler(_device->getDevice(),linearSampler()),1,vk::ShaderStageFlagBits::eFragment);
	_descSet->create();

	_pipeline->descSet(_descSet);
	_pipeline->create(guiVertex::bindingDesc(),guiVertex::attributes());

		// Create framebuffer for gui
	_fbImage = _device->create<Image>();
	_fbImage->create(size.x,size.y,_device->getSwapchain()->getFormat());
	_fb = _device->create<Framebuffer>();
	_fbView = _fbImage->createImageView();
	_fb->attachment(_fbView);
	_fb->depth(size.x,size.y);
	_fb->create(size.x,size.y,_pipeline->getRenderPass());

    const nk_anti_aliasing AA = NK_ANTI_ALIASING_ON;

    // Config for convert vertex and index data from nuklear to vulkan
    static const nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(guiVertex,pos)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(guiVertex,uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(guiVertex,color)},
        {NK_VERTEX_LAYOUT_END}
    };
    NK_MEMSET(&_convertConfig, 0, sizeof(_convertConfig));
    _convertConfig.vertex_layout = vertex_layout;
    _convertConfig.vertex_size = sizeof(guiVertex);
    _convertConfig.vertex_alignment = NK_ALIGNOF(guiVertex);
    //_convertConfig.null = dev->null;
    _convertConfig.circle_segment_count = 22;
    _convertConfig.curve_segment_count = 22;
    _convertConfig.arc_segment_count = 22;
    _convertConfig.global_alpha = 1.0f;
    _convertConfig.shape_AA = AA;
    _convertConfig.line_AA = AA;

    createBuffer();
    updateBuffer();
    _commandBuffer = commands();

    _renderFinish = _device->createSemaphore(vk::SemaphoreCreateInfo());
}

void GUI::createBuffer(){
	_cpuVB = _device->create<Buffer>();
	_gpuVB = _device->create<Buffer>();
	_cpuIB = _device->create<Buffer>();
	_gpuIB = _device->create<Buffer>();

	_vbSize = sizeof(guiVertex)*MAX_VERTEX_BUFFER_GUI;

	_cpuVB->create(_vbSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	_gpuVB->create(_vbSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	_ibSize = sizeof(uint16_t)*MAX_ELEMENT_BUFFER_GUI;

	_cpuIB->create(_ibSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);

	_gpuIB->create(_ibSize,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void GUI::updateBuffer(){
	guiVertex *vertices;
	uint16_t* elements;

	// Store Nuklear buffers to VK buffers at Host
    vertices = (guiVertex*)_cpuVB->map(_vbSize);
    elements = (uint16_t*)_cpuIB->map(_ibSize);
    {
    	nk_buffer vbuf, ebuf;
        nk_buffer_init_fixed(&vbuf, vertices, (size_t)MAX_VERTEX_BUFFER_GUI);
        nk_buffer_init_fixed(&ebuf, elements, (size_t)MAX_ELEMENT_BUFFER_GUI);
        nk_convert(&_ctx, &_cmds, &vbuf, &ebuf, &_convertConfig);
        for(uint i = 0;i<2000;++i){
        	std::cout << vertices[i].pos.x << ", " << vertices[i].pos.y << std::endl;
        }
    }
    _cpuVB->unmap();
    _cpuIB->unmap();

    // Copy buffers to device
    _cpuVB->copy(*_cpuVB,*_gpuVB,_vbSize);
    _cpuIB->copy(*_cpuIB,*_gpuIB,_ibSize);
}

vk::CommandBuffer GUI::commands(){
	vk::CommandBufferAllocateInfo allocInfo(_device->getCommandPool(),vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer commandBuffer = _device->getDevice().allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

	commandBuffer.begin(&beginInfo);

	std::array<vk::ClearValue, 2> clearValues = {};
	clearValues[0].color = vk::ClearColorValue(std::array<float,4>{28.0/255.0,48.0/255.0,62.0/255.0, 0.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderPassBeginInfo renderPassInfo(
		_pipeline->getRenderPass(),
		_fb->vk_framebuffer(),
		vk::Rect2D(vk::Offset2D(),vk::Extent2D(_size.x,_size.y)),
		clearValues.size(), clearValues.data()
	);

	commandBuffer.beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);

	vk::DescriptorSet descSets[] = {_descSet->getDescriptorSet()};
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

	vk::DeviceSize buffer = 0;
	commandBuffer.bindVertexBuffers(0,1,&_gpuVB->buffer,&buffer);
	commandBuffer.bindIndexBuffer(_gpuIB->buffer,0,vk::IndexType::eUint16);

	uint offset = 0;

	const nk_draw_command* cmd;
	nk_draw_foreach(cmd, &_ctx, &_cmds) {
		if (!cmd->elem_count) continue;
		commandBuffer.drawIndexed((uint)cmd->elem_count*3,1,offset,0,0);
		offset += cmd->elem_count;
	}
	//nk_clear(&_ctx);

	commandBuffer.endRenderPass();
	commandBuffer.end();

	return commandBuffer;
}

vk::ImageView GUI::getImageView(){
	return _fbView;
}

// Return semaphore for waiting
vk::Semaphore GUI::render(const vk::Semaphore& wait){
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

	vk::SubmitInfo submitInfo(
		1, &wait,
		waitStages, 
		1, &_commandBuffer,
		1, &_renderFinish
	);

	_device->getGraphicsQueue().submit(submitInfo, nullptr);
	return _renderFinish;
}
