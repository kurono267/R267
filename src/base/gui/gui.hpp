#pragma once 

#include "base/vk/buffer.hpp"
#include "base/vk/image.hpp"
#include "base/vk/framebuffer.hpp"
#include "base/vk/pipeline.hpp"

#ifndef NUKLEAR_INC
#define NUKLEAR_INC
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>
#endif

namespace r267 {

typedef std::function<bool(nk_context*)> updateGUI;

class GUI {
	public:
		GUI(spDevice device);
		virtual ~GUI();

		void create(const glm::ivec2& size,vk::CommandBufferInheritanceInfo inheritanceInfo);

		void update(updateGUI _update);
		bool actionUpdate(GLFWwindow* win);
		vk::CommandBuffer commandBuffer();

		struct nk_image addImage(spImage image);

		void release();

		nk_context* nkContext(){return &_ctx;}
	protected:
		void createBuffer();
		void updateBuffer();
		void commands();
		// Framebuffer size
		glm::ivec2 _size;

		spDevice   _device;
		spPipeline _pipeline;
		spDescSet  _descSet;

		std::vector<spDescSet> _imageSet;
		std::vector<spImage>   _images;

		// Vertex and Index Buffers
		vk::DeviceSize _vbSize;
		spBuffer  _cpuVB;
		spBuffer  _gpuVB;
		vk::DeviceSize _ibSize;
		spBuffer  _cpuIB;
		spBuffer  _gpuIB;

		vk::Semaphore _renderFinish;
		vk::CommandBuffer _commandBuffer;
		vk::CommandBufferInheritanceInfo _inheritanceInfo;

		// NK Objects
		nk_context    _ctx;
		nk_font_atlas _atlas;
		nk_convert_config _convertConfig;
		nk_buffer _cmds;
};

typedef std::shared_ptr<GUI> spGUI;

} // r267
