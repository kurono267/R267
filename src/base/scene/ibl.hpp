//
// Created by kurono267 on 04.09.17.
//

#pragma once

#include <base/vk/pipeline.hpp>
#include <base/vk/image.hpp>
#include <base/vk/framebuffer.hpp>
#include <base/vk/shape.hpp>

namespace r267 {

// IBL precompute class
// Convert image to cubemap
// Compute roughness mipmaps
// Compute irradiance
// Compute BRDF
class IBL {
	struct UBO {
		glm::mat4 mat[6];
	};
	public:
		IBL(){}
		~IBL(){}

		void init(spDevice device,spImage source);
		void run();

		vk::ImageView cubemap();
		vk::ImageView irradiance();
		vk::ImageView brdf();
	protected:
		enum Step {
			Convert = 0,
			Irradiance,
			Filter,
			BRDF,
			NumSteps
		};

		void initConvert();
		void initIrradiance();
		void initFilter();
		void initBRDF();

		spDevice   _device;

		spPipeline _pipelines[NumSteps];
		spDescSet  _descSets[NumSteps];

		spImage    _source;
		spImage    _cubemap;
		spImage    _irradiance;
		spImage    _brdf;

		glm::mat4  _mats[6];

		UBO        _uboData;
		Uniform    _uboUniform;

		spFramebuffer _framebuffers[6];
		spFramebuffer _irrFramebuffers[6];
		std::vector<spFramebuffer> _roughFramebuffers;
		spFramebuffer _brdfFramebuffer;

		spShape    _cube;
		spShape    _quad;

		vk::CommandBuffer _cmds[NumSteps];
};

};