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
	public:
		Differed(){}
		~Differed(){}

		void init(spDevice device);

		vk::ImageView result();
		vk::Semaphore run(vk::Semaphore wait);
	protected:
		vk::Semaphore _finish;


};
