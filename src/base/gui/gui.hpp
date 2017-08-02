#pragma once 

#include "base/vk/buffer.hpp"
#include "base/vk/image.hpp"
#include "base/vk/framebuffer.hpp"
#include "base/vk/pipeline.hpp"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear.h>

namespace r267 {

typedef std::function<bool(nk_context&)> updateGUI;

class GUI {
	public:
		GUI(spDevice device);
		virtual ~GUI();

		void create(const glm::ivec2& size);
		vk::Semaphore render(const vk::Semaphore& wait);

		void update(updateGUI _update);
		void actionUpdate(GLFWwindow* win);

		nk_context* nkContext(){return &_ctx;}

		vk::ImageView getImageView(); // Frame buffer image view
	protected:
		void createBuffer();
		void updateBuffer();
		void commands();
		// Framebuffer size
		glm::ivec2 _size;

		spDevice   _device;
		spPipeline _pipeline;
		spDescSet  _descSet;

		spImage    _vkAtlas;

		spImage    _fbImage;
		vk::ImageView _fbView;
		spFramebuffer _fb;

		// Vertex and Index Buffers
		vk::DeviceSize _vbSize;
		spBuffer  _cpuVB;
		spBuffer  _gpuVB;
		vk::DeviceSize _ibSize;
		spBuffer  _cpuIB;
		spBuffer  _gpuIB;

		vk::Semaphore _renderFinish;
		vk::CommandBuffer _commandBuffer;

		// NK Objects
		nk_context    _ctx;
		nk_font_atlas _atlas;
		nk_convert_config _convertConfig;
		nk_buffer _cmds;
};

typedef std::shared_ptr<GUI> spGUI;

} // r267
