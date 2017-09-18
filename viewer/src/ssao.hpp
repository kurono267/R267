#pragma once

#include "gbuffer.hpp"
#include <random>

struct ssaoUBO {
    glm::mat4 proj;
    glm::mat4 view;
	glm::mat4 invmvp;
    glm::vec4 kernels[64];
};

class SSAO {
	public:
		SSAO(){}
		~SSAO(){}

		void init(spDevice device,const GBuffer& gbuffer,const glm::ivec2& size);
		void update(spCamera camera);

		vk::ImageView ssaoImage();

		vk::Semaphore render(spDevice device,vk::Semaphore& wait);

		void release();
	protected:
		void createCommands(spDevice device);

		glm::ivec2 _size;

		vk::CommandBuffer _command;
		vk::Semaphore     _finish;

		spImage       _ssaoImage;
		spFramebuffer _framebuffer;

		spPipeline _main;
		spDescSet  _descSet;

		ssaoUBO _ssaoData;
        Uniform _ssaoUniform;

        spImage _rotationImage;

        spShape _quad;
};
