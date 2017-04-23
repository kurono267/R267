#pragma once

#include <base/default.hpp>
#include "swapchain.hpp"
#include <iostream>
#include <set>
#include "utils.hpp"

namespace r267 {

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

class Device {
	public:
		Device(){}
		~Device(){}

		void create(const vk::Instance& instance,const vk::SurfaceKHR& surface,const glm::ivec2& size);
		void release();

		QueueFamilyIndices queueFamiliesIndices();

		vk::Device getDevice();
		vk::PhysicalDevice getPhysicalDevice();
		vk::Queue  getGraphicsQueue();
		vk::Queue  getPresentQueue();
		spSwapchain  getSwapchain();
	protected:
		void pickPhysicalDevice();
		bool isDeviceSuitable(const vk::PhysicalDevice& device);

		void createLogicalDevice();

		void print();

		vk::PhysicalDevice _pDevice;
		vk::Device         _device;
		vk::Instance       _instance;
		vk::Queue          _presentQueue;
		vk::Queue          _graphicsQueue;
		vk::SurfaceKHR     _surface;

		spSwapchain          _swapchain;

		glm::ivec2         _size;
};

typedef std::shared_ptr<r267::Device> spDevice;

};

