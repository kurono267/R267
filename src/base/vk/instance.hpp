#pragma once

#include <base/default.hpp>
#include "device.hpp"
#include <iostream>

// Instance class 

namespace r267 {

class Instance {
	public:
		Instance(){}
		Instance(const Instance& i) = delete;
		~Instance(){}

		void init(const std::string& title, GLFWwindow* window,const glm::ivec2& size);
		void release();

		r267::Device& device();
	protected:
		void initVulkan();
		void createInstance();

		// Validation layers
		std::vector<const char*> getRequiredExtensions();
		void setupDebugCallback();
		bool checkValidationLayerSupport();

		// Create surface
		void createSurface(GLFWwindow* window);

		vk::Instance _instance;
		vk::DebugReportCallbackEXT _callback;
		vk::SurfaceKHR _surface;

		r267::Device _device;
		
		glm::ivec2 _size;
		std::string _title;
};

};
