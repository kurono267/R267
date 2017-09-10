//
// Created by kurono267 on 04.09.17.
//

#pragma once

#include <base/vk/pipeline.hpp>
#include <base/vk/image.hpp>
#include <base/vk/framebuffer.hpp>
#include <base/vk/shape.hpp>

using namespace r267;

// Convert image to cubemap
class ImageCube {
	struct UBO {
		glm::mat4 mat[6];
	};
	public:
		ImageCube(){}
		~ImageCube(){}

		void init(spDevice device,spImage source);
		void run();

		vk::ImageView cubemap();
		vk::ImageView irradiance();
	protected:
		spDevice   _device;

		spPipeline _pipeline;
		spPipeline _irrPipeline;
		spDescSet  _descSet;
		spDescSet  _irrDescSet;

		spImage    _source;
		spImage    _cubemap;
		spImage    _irradiance;

		glm::mat4  _mats[6];

		UBO        _uboData;
		Uniform    _uboUniform;

		spFramebuffer _framebuffers[6];
		spFramebuffer _irrFramebuffers[6];

		spShape    _cube;

		vk::CommandBuffer _cmd;
		vk::CommandBuffer _irrCmd;
};
