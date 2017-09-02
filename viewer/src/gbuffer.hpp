//
// Created by kurono267 on 18.08.17.
//
#pragma once

// Current huge GBuffer

#include <base/default.hpp>
#include <base/vk/device.hpp>
#include <base/vk/image.hpp>
#include <base/vk/pipeline.hpp>
#include <base/scene/scene.hpp>
#include <base/scene/camera.hpp>

using namespace r267;

struct CameraData {
	glm::mat4 mvp;
	glm::vec4 view;
};

class GBuffer {
	public:
		GBuffer(){}
		~GBuffer(){}

		void init(spDevice device,spScene scene,spCamera camera,spDescSet matDescSet,const glm::ivec2& size);

		vk::ImageView posMap();
		vk::ImageView normalMap();
		vk::ImageView colorMap();

		vk::Semaphore render(vk::Semaphore& wait);
	protected:
		spDevice _device;

		spImage _posMap;
		spImage _colorMap;
		spImage _normalMap;

		spFramebuffer _framebuffer;

		spPipeline _pipeline;
		spScene    _scene;
		spCamera   _camera;

		spDescSet  _cameraDesc;
		Uniform    _cameraUniform;
		CameraData _cameraData;

		glm::ivec2 _size;

		void createCommands();

		vk::CommandPool   _commandPool;
		vk::CommandBuffer _command;

		vk::Semaphore     _finish;
};