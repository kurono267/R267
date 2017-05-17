#include "image.hpp"

Image::Image(spDevice device,vk::Queue queue,vk::CommandPool pool) : _device(device),_queue(queue),_pool(pool) {}
Image::~Image(){}

void Image::create(const uint& width,const uint& height,
					const vk::Format& format,const uint& mipLevels = 1,const vk::ImageTiling& tiling = vk::ImageTiling::eOptimal,
					const vk::ImageUsageFlags& usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
					const vk::MemoryProperyFlags& properties = vk::MemoryPropertyFlagBits::eDeviceLocal){
	_width = width;
	_height = height;
	_format = format;
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
	_image = vk_device.createImage(imageInfo);
	auto memoryReq = vk_device.getImageMemoryRequirements();

	MemoryAllocateInfo allocInfo(memoryReq.size,findMemoryType(memoryReq.memoryTypeBits, properties));
	_memory = vk_device.allocateMemory(allocInfo);

	vk_device.bindImageMemory(result.image,result.memory,0);
}

void Image::set(const spBuffer& buffer){
	transition(_format,vk::ImageLayout::ePreinitialized,vk::ImageLayout::eTransferDstOptimal);
	setBuffer(buffer);
	transition(_format,vk::ImageLayout::eTransferDstOptimal,vk::ImageLayout::eShaderReadOnlyOptimal);
}

vk::Image Image::vk_image(){
	return _image;
}

void Image::transition(const vk::Format& format,const vk::ImageLayout& oldLayout,const vk::ImageLayout& newLayout) {
	vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;
	if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		srcAccess = vk::AccessFlagBits::HostWrite;
		dstAccess = vk::AccessFlagBits::eTransferWrite;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		srcAccess = vk::AccessFlagBits::eTransferWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
	} else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vk::ImageSubresourceRange subRes(
		vk::ImageAspectFlagBits::eColor,
		0, 1, /* Mip levels current and count*/ 0, 1 /* Layers current and count */ );

	vk::ImageMemoryBarrier barrier(
		srcAccess,
		dstAccess,
		oldLayout,
		newLayout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		_image,
		subRes
	);

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
}

void Image::setBuffer(const spBuffer& buffer){
	vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

	vk::ImageSubresourceRange subRes(
		vk::ImageAspectFlagBits::eColor,
		0, 1, /* Mip levels current and count*/ 0, 1 /* Layers current and count */ );

	vk::BufferImageCopy region(
		/* bufferOffset bufferRowLength bufferImageHeight */
		0, 0, 0,
		/* subRes imageOffest */ 
		subRes, {0, 0, 0},
		/* imageExtent */
		{_width,_height,1}
	);

	commandBuffer.copyBufferToImage(buffer->buffer,image,vk::ImageLayout::eTransferDstOptimal, {region});

	endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
}
