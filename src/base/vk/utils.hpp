#pragma once

#include <base/default.hpp>
#include <fstream>

namespace r267 {

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
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

};

