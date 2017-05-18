#pragma once
// Additional functional for simple code write
#include "device.hpp"
#include "pipeline.hpp"
#include "image.hpp"

namespace r267 {

std::vector<vk::Framebuffer> createFrameBuffers(spDevice device,spPipeline pipeline);
// Create RGBA32F Checkboard texture
spImage checkboardTexture(spDevice device,const uint& width, const uint& height, const uint& step);
spImage loadImage(spDevice device,const std::string& filename);

};