//
// Created by kurono267 on 17.09.17.
//

#pragma once

#include <base/wnd/MainApp.hpp>
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>

#include <base/scene/scene.hpp>
#include <base/scene/camera.hpp>
#include <base/scene/ibl.hpp>
#include "ssao.hpp"

class Differed {
	struct UBO {
	    glm::vec4 view;
	    glm::mat4 viewMat;
	    glm::mat4 proj;
	    glm::mat4 invview;
	    glm::mat4 invproj;
	};
	public:
		Differed(){}
		~Differed(){}

		void init(spDevice device);

		vk::ImageView result();
		vk::Semaphore run(vk::Semaphore wait);
	protected:
		vk::Semaphore _finish;

		spPipeline    _pipeline;
		spDescSet     _descSet;

		Uniform _uniform;
		UBO     _ubo;

		spShape _quad;
};
