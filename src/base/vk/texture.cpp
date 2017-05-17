#include "texture.hpp"

Texture::Texture(spDevice device,vk::Queue queue,vk::CommandPool pool) : _device(device),_queue(queue),_pool(pool) {}
Texture::~Texture(){}

Image Texture::createImage(const uint& width,const uint& height,const uint& mipLevels,
					 const vk::Format& format,const vk::ImageTiling& tiling,
					 const vk::ImageUsageFlags& usage,const vk::MemoryProperyFlags& properties){
	vk::ImageCreateInfo imageInfo(
		vk::ImageCreateFlags(), // Basic
		vk::ImageType::e2D, // Type 1D,2D,3D
		format, // Format
		vk::Extent3D(width,height,1), // Width, Height and Depth
		mipLevels, // Mip Levels
		1, // Array Layers
		vk::SampleCountFlagBits::e1, // Samples
		tiling,
		usage,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::ePreinitialized
	);

	auto vk_device = _device->getDevice();

	// Create Image
	Image result;
	result.image = vk_device.createImage(imageInfo);
	auto memoryReq = vk_device.getImageMemoryRequirements();

	MemoryAllocateInfo allocInfo(memoryReq.size,findMemoryType(memoryReq.memoryTypeBits, properties));
	result.memory = vk_device.allocateMemory(allocInfo);

	vk_device.bindImageMemory(result.image,result.memory,0);
}

void Texture::createTexture(const uint& width,const uint& height,const vk::Format& format,const void* pixels,const size_t& pixelSize){
	vk::DeviceSize imageSize = width*height*pixelSize;

	_cpu = std::make_shared<Buffer>(_device,_queue,_pool);
	_cpu->create(imageSize,vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);

	if(pixels != nullptr){
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);
	}

	_image = createImage(texWidth, texHeight, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Texture::release(){

}
