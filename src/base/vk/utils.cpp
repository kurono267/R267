#include "utils.hpp"

using namespace r267;

SwapchainSupportDetails r267::swapchainSupport(vk::PhysicalDevice device,vk::SurfaceKHR surface){
	SwapchainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats      = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

QueueFamilyIndices r267::queueFamilies(const vk::PhysicalDevice& device,const vk::SurfaceKHR surface) {
	QueueFamilyIndices indices;

	auto queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i,surface);

		if(queueFamily.queueCount > 0 && presentSupport){
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

std::vector<char> r267::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

uint32_t r267::findMemoryType(vk::PhysicalDevice pDevice,uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties memProperties = pDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

vk::CommandBuffer r267::beginSingle(vk::Device device,vk::CommandPool pool){
	vk::CommandBufferAllocateInfo allocInfo(pool,vk::CommandBufferLevel::ePrimary,1);
	auto cmdBuffers = device.allocateCommandBuffers(allocInfo);

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	cmdBuffers[0].begin(beginInfo);
	return cmdBuffers[0];
}

void r267::endSingle(vk::Device device,vk::Queue queue,vk::CommandPool pool,vk::CommandBuffer commands){
	commands.end();

	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBufferCount(1);
	submitInfo.setPCommandBuffers(&commands);

	queue.submit(submitInfo,vk::Fence());
	queue.waitIdle();

	device.freeCommandBuffers(pool,{commands});
}

vk::ImageView r267::createImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, const uint& mipLevels){
	vk::ImageViewCreateInfo createInfo(
		vk::ImageViewCreateFlags(),
		image,
		vk::ImageViewType::e2D,
		format,
		vk::ComponentMapping(),
		vk::ImageSubresourceRange(
			aspectFlags,
			0, mipLevels, 0, 1)
	);

	return device.createImageView(createInfo);
}

vk::Sampler   r267::createSampler(vk::Device device, vk::SamplerCreateInfo samplerInfo){
	return device.createSampler(samplerInfo);
}

vk::SamplerCreateInfo r267::linearSampler(const uint& mipLevels){
	return vk::SamplerCreateInfo(
		vk::SamplerCreateFlags(),
		vk::Filter::eLinear, // Mag Filter
		vk::Filter::eLinear, // Min Filter
		vk::SamplerMipmapMode::eLinear, // MipMap Mode
		vk::SamplerAddressMode::eRepeat, // U Address mode
		vk::SamplerAddressMode::eRepeat, // V Address mode
		vk::SamplerAddressMode::eRepeat, // W Address mode
		0, // Mip Lod bias
		1, // Anisotropic enabled
		16, // Max anisotropy
		0, // Compare enabled
		vk::CompareOp::eAlways, // Compare Operator
		0, // Min lod
		mipLevels-1, // Max lod
		vk::BorderColor::eFloatTransparentBlack, // Border color
		0 // Unnormalized coordiante
	);
}
