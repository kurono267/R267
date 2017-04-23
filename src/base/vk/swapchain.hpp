#pragma once

#include <base/default.hpp>
#include "utils.hpp"

namespace r267 {

class Swapchain {
	public:
		Swapchain(){}
		~Swapchain(){}

		void create(const vk::PhysicalDevice& pDevice,const vk::Device& device,const vk::SurfaceKHR& surface,const glm::ivec2& size);
		void release();

		vk::SwapchainKHR getSwapchain() const ;
		std::vector<vk::Image> getImages() const ;
		vk::Format    getFormat() const ;
		vk::Extent2D  getExtent() const ;
		std::vector<vk::ImageView> getImageViews() const ;
	protected:
		vk::SwapchainKHR      _swapchain;
		std::vector<vk::Image> _images;
		vk::Format         _imageFormat;
		vk::Extent2D       _extent; 

		void createImageViews(const vk::Device& device);
		std::vector<vk::ImageView> _imageViews;

		vk::Device _device;
};

typedef std::shared_ptr<Swapchain> spSwapchain;

};

