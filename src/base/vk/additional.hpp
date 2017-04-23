#pragma once
// Additional functional for simple code write
#include "device.hpp"
#include "pipeline.hpp"

namespace r267 {

std::vector<vk::Framebuffer> createFrameBuffers(spDevice device,spPipeline pipeline);

};