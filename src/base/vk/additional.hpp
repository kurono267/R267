#pragma once
// Additional functional for simple code write
#include "device.hpp"
#include "pipeline.hpp"
#include "image.hpp"
#include "framebuffer.hpp"

namespace r267 {

std::vector<spFramebuffer> createFrameBuffers(spDevice device,spPipeline pipeline);
// Create RGBA32F Checkboard texture
spImage checkboardTexture(spDevice device,const uint& width, const uint& height, const uint& step);
spImage whiteTexture(spDevice device,const uint& width, const uint& height);
spImage loadImage(spDevice device,const std::string& filename);
spImage defaultCubemap(spDevice device,const uint& width,const uint& height);

struct AccessTransfer {
    AccessTransfer(){}
    AccessTransfer(const AccessTransfer& a) : src(a.src),dst(a.dst) {}
    AccessTransfer(const vk::AccessFlags& _src) : src(_src),dst(_src){}
    AccessTransfer(const vk::AccessFlags& _src,const vk::AccessFlags& _dst) : src(_src),dst(_dst){}

    vk::AccessFlags src;
    vk::AccessFlags dst;
};

struct ImageLayoutTransfer {
    ImageLayoutTransfer(){}
    ImageLayoutTransfer(const ImageLayoutTransfer& t) : src(t.src),dst(t.dst) {}
    ImageLayoutTransfer(const vk::ImageLayout& _src) : src(_src), dst(_src) {}
    ImageLayoutTransfer(const vk::ImageLayout& _src, const vk::ImageLayout& _dst) : src(_src), dst(_dst) {}

    vk::ImageLayout src;
    vk::ImageLayout dst;
};

// Image memory barrier, full size
vk::ImageMemoryBarrier imageBarrier(spImage image, AccessTransfer access, ImageLayoutTransfer layout);

};