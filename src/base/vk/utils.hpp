#pragma once

#include <base/default.hpp>
#include <fstream>
#include <iostream>

namespace r267 {

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	int computeFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0;
	}
};

QueueFamilyIndices queueFamilies(const vk::PhysicalDevice& device,const vk::SurfaceKHR surface);

struct SwapchainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

SwapchainSupportDetails swapchainSupport(vk::PhysicalDevice device,vk::SurfaceKHR surface);
uint32_t findMemoryType(vk::PhysicalDevice pDevice,uint32_t typeFilter, vk::MemoryPropertyFlags properties);

std::vector<char> readFile(const std::string& filename);

const vk::ColorComponentFlags RGBA = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

vk::CommandBuffer beginSingle(vk::Device device,vk::CommandPool pool);
void              endSingle(vk::Device device,vk::Queue queue,vk::CommandPool pool,vk::CommandBuffer commands);

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor, const uint& mipLevels = 1, const uint& layers = 1);
vk::Sampler   createSampler(vk::Device device, vk::SamplerCreateInfo samplerInfo);

vk::SamplerCreateInfo linearSampler(const uint& mipLevels = 1);
vk::SamplerCreateInfo anisoSampler(const uint& mipLevels = 1,const uint& aniso = 16);
vk::SamplerCreateInfo nearsetSampler(const uint& mipLevels = 1);

inline bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

inline bool hasDepthComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint
        || format == vk::Format::eD16Unorm || format == vk::Format::eD16UnormS8Uint || format == vk::Format::eD32Sfloat;
}

vk::ShaderModule createShaderModule(vk::Device device,const std::string& filename);

};

