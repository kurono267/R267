#pragma once

#include <base/default.hpp>
#include <base/vk/device.hpp>
#include <base/vk/image.hpp>
#include <base/vk/pipeline.hpp>
#include <base/vk/compute.hpp>

using namespace r267;

class IBL {
	public:
		IBL(){}
		~IBL(){}

		void init(spDevice device,spImage image);

		vk::ImageView irradiance();
	protected:
		void irradianceCompute(spDevice device,spImage image);

		spCompute  _irrCompute;
		std::vector<spDescSet>  _irrDescSets;

		spImage    _irrImage;

		vk::CommandBuffer _irrCmd;
};