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

class Device : public std::enable_shared_from_this<Device> {
	typedef std::shared_ptr<Device> ptr;
	typedef std::function<void(ptr)> releaseFunc;
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
		vk::Queue  getComputeQueue();
		spSwapchain  getSwapchain();

		vk::Format supportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
		vk::Format depthFormat();

		vk::CommandPool getCommandPool(const bool isCompute = false);

		vk::Semaphore createSemaphore(vk::SemaphoreCreateInfo info);

		template<typename M>
		std::shared_ptr<M> create(){
			std::shared_ptr<M> result = std::make_shared<M>(shared_from_this(),_graphicsQueue,_pool);
			_release.push_back([result](ptr device){
				result->release(device);
			});
			return result;
		}
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
		vk::Queue          _computeQueue;
		vk::SurfaceKHR     _surface;

		vk::CommandPool    _pool;
		vk::CommandPool    _poolCompute;

		spSwapchain          _swapchain;

		glm::ivec2         _size;

		std::vector<vk::Semaphore> _semaphores;
		std::vector<releaseFunc>       _release;
};

typedef std::shared_ptr<r267::Device> spDevice;

};

