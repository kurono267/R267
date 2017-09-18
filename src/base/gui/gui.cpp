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

void GUI::update(updateGUI _update){
    if(!_update(&_ctx))return;
    updateBuffer();
    commands();
    nk_clear(&_ctx);
}

bool GUI::actionUpdate(GLFWwindow* win){
    int i;
    double x, y;
    nk_context *ctx = &_ctx;

    nk_input_begin(ctx);
    //for (i = 0; i < glfw.text_len; ++i)
    //    nk_input_unicode(ctx, glfw.text[i]);

#if NK_GLFW_GL3_MOUSE_GRABBING
    /* optional grabbing behavior */
    if (ctx->input.mouse.grab)
        glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else if (ctx->input.mouse.ungrab)
        glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif

    nk_input_key(ctx, NK_KEY_DEL, glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_ENTER, glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TAB, glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_BACKSPACE, glfwGetKey(win, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_UP, glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SHIFT, glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS||
                                    glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    glfwGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);
#if NK_GLFW_GL3_MOUSE_GRABBING
    if (ctx->input.mouse.grabbed) {
        glfwSetCursorPos(glfw.win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
#endif
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    //nk_input_scroll(ctx, glfw.scroll);
    nk_input_end(&_ctx);

    return nk_item_is_any_active(ctx);
}

vk::CommandBuffer GUI::commandBuffer(){
    return _commandBuffer;
}

enum theme {THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK};

void set_style(struct nk_context *ctx, enum theme theme) {
    struct nk_color table[NK_COLOR_COUNT];
    if (theme == THEME_WHITE) {
        table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
        table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
        nk_style_from_table(ctx, table);
    } else if (theme == THEME_RED) {
        table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
        table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
        table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
        table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
        nk_style_from_table(ctx, table);
    } else if (theme == THEME_BLUE) {
        table[NK_COLOR_TEXT] = nk_rgba(20, 20, 20, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(202, 212, 214, 215);
        table[NK_COLOR_HEADER] = nk_rgba(137, 182, 224, 220);
        table[NK_COLOR_BORDER] = nk_rgba(140, 159, 173, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(142, 187, 229, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(147, 192, 234, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(177, 210, 210, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(182, 215, 215, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_SELECT] = nk_rgba(177, 210, 210, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(177, 210, 210, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(137, 182, 224, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(142, 188, 229, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(147, 193, 234, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_EDIT] = nk_rgba(210, 210, 210, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(20, 20, 20, 255);
        table[NK_COLOR_COMBO] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_CHART] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(137, 182, 224, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(190, 200, 200, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(156, 193, 220, 255);
        nk_style_from_table(ctx, table);
    } else if (theme == THEME_DARK) {
        table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
        table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
        table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
        nk_style_from_table(ctx, table);
    } else {
        nk_style_default(ctx);
    }
}

const vk::SamplerCreateInfo defaultSampler = vk::SamplerCreateInfo(
        vk::SamplerCreateFlags(),
        vk::Filter::eLinear, // Mag Filter
        vk::Filter::eLinear, // Min Filter
        vk::SamplerMipmapMode::eLinear, // MipMap Mode
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

void GUI::create(const glm::ivec2& size,vk::CommandBufferInheritanceInfo inheritanceInfo){
	_size = size;
	_inheritanceInfo = inheritanceInfo;
	nk_init_default(&_ctx, 0);
    //set_style(&_ctx,THEME_RED);
	nk_buffer_init_default(&_cmds);
	// Create pipeline pattern
	RenderPattern pattern;
	pattern.inputAssembly(vk::PrimitiveTopology::eTriangleList);
	pattern.viewport(0,0,size.x,size.y);
	pattern.rasterizer(vk::PolygonMode::eFill,vk::CullModeFlagBits::eNone);
	pattern.multisampling();
    pattern.scissor(vk::Offset2D(),vk::Extent2D(size.x,size.y));
	pattern.blend(1,true,RGBA,
					vk::BlendFactor::eSrcAlpha,vk::BlendFactor::eOneMinusSrcAlpha,
					vk::BlendOp::eAdd,
					vk::BlendFactor::eSrcAlpha,vk::BlendFactor::eOneMinusSrcAlpha,
					vk::BlendOp::eAdd);
	pattern.depth(false);
	pattern.createRenderPass(_device->getSwapchain()->getFormat(),_device->depthFormat());
	// Create Pipeline
	_pipeline = std::make_shared<Pipeline>(pattern,_device->getDevice());
	_pipeline->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/gui/main_vert.spv");
	_pipeline->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/gui/main_frag.spv");

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

    spImage vkAtlas = _device->create<Image>();
	vkAtlas->create(wAtlas,hAtlas,vk::Format::eR8G8B8A8Unorm);
 	vkAtlas->set(cpu);
 	_images.push_back(vkAtlas);
   
    nk_draw_null_texture null_tex;
    nk_font_atlas_end(&_atlas, nk_handle_id((int)0), &null_tex);
    if (_atlas.default_font)
        nk_style_set_font(&_ctx, &_atlas.default_font->handle);

    // Create Uniform 
	glm::mat4 ortho = glm::mat4({
        2.0f/(float)size.x, 0.0f, 0.0f, 0.0f,
        0.0f,2.0f/(float)size.y, 0.0f, 0.0f,
        0.0f, 0.0f,-1.0f, 0.0f,
        -1.0f,-1.0f, 0.0f, 1.0f
    });
    Uniform orthoUniform;
	orthoUniform.create(_device,sizeof(glm::mat4),&ortho);
	_descSet->setUniformBuffer(orthoUniform,0,vk::ShaderStageFlagBits::eVertex);
	_descSet->create();

	spDescSet imageDescSet = _device->create<DescSet>();
	imageDescSet->setTexture(_images[0]->ImageView(),createSampler(_device->getDevice(),defaultSampler),0,vk::ShaderStageFlagBits::eFragment);
	imageDescSet->create();
	_imageSet.push_back(imageDescSet);

	_pipeline->descSets({_descSet,imageDescSet});
	_pipeline->create(guiVertex::bindingDesc(),guiVertex::attributes());

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

    // Init command buffer
    vk::CommandBufferAllocateInfo allocInfo(_device->getCommandPool(),vk::CommandBufferLevel::eSecondary, 1);
    _commandBuffer = _device->getDevice().allocateCommandBuffers(allocInfo)[0];

    // Create VB and IB buffers
    createBuffer();

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
    }
    _cpuVB->unmap();
    _cpuIB->unmap();

    // Copy buffers to device
    _cpuVB->copy(*_cpuVB,*_gpuVB,_vbSize);
    _cpuIB->copy(*_cpuIB,*_gpuIB,_ibSize);
}

struct nk_image GUI::addImage(spImage image){
	_images.push_back(image);
	int id = _images.size()-1;

	spDescSet imageDescSet = _device->create<DescSet>();
	imageDescSet->setTexture(_images[id]->ImageView(),createSampler(_device->getDevice(),defaultSampler),0,vk::ShaderStageFlagBits::eFragment);
	imageDescSet->create();
	_imageSet.push_back(imageDescSet);

	return nk_image_id(id);
}

void GUI::release(){
	_pipeline->release();
}

void GUI::commands(){
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse | vk::CommandBufferUsageFlagBits::eRenderPassContinue);
	beginInfo.pInheritanceInfo = &_inheritanceInfo;

    _commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	_commandBuffer.begin(&beginInfo);

    _commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);

	vk::DeviceSize buffer = 0;
    _commandBuffer.bindVertexBuffers(0,1,&_gpuVB->buffer,&buffer);
    _commandBuffer.bindIndexBuffer(_gpuIB->buffer,0,vk::IndexType::eUint16);

	uint offset = 0;

	const nk_draw_command* cmd;
	nk_draw_foreach(cmd, &_ctx, &_cmds) {
		if (!cmd->elem_count) continue;
        //vk::Rect2D rect(vk::Offset2D(cmd->clip_rect.x,_size.y - (cmd->clip_rect.y + cmd->clip_rect.h)),vk::Extent2D(cmd->clip_rect.w,cmd->clip_rect.h));
        //_commandBuffer.setScissor(0,1,&rect);
        if(cmd->texture.id >= _imageSet.size())continue;
        vk::DescriptorSet descSets[] = {_descSet->getDescriptorSet(),_imageSet[cmd->texture.id]->getDescriptorSet()};
        _commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->getPipelineLayout(), 0, 2, descSets, 0, nullptr);

        _commandBuffer.drawIndexed((uint)cmd->elem_count,1,offset,0,0);
		offset += cmd->elem_count;
	}

    _commandBuffer.end();
}
