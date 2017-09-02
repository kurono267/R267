#include "image.hpp"

using namespace r267;

Image::Image(spDevice device,vk::Queue queue,vk::CommandPool pool) : _device(device),_queue(queue),_pool(pool),_imageView(nullptr) {}
Image::~Image(){}

void Image::create(const uint& width,const uint& height,
					const vk::Format& format,const uint& mipLevels,const vk::ImageTiling& tiling,
					const vk::ImageUsageFlags& usage,const vk::ImageLayout& layout,
					const vk::MemoryPropertyFlags& properties){
	_width = width;
	_height = height;
	_format = format;
	_mipLevels = mipLevels;
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
		vk::SharingMode::eExclusive, 0, nullptr, layout
	);

	auto vk_device = _device->getDevice();

	// Create Image
	_image = vk_device.createImage(imageInfo);
	auto memoryReq = vk_device.getImageMemoryRequirements(_image);

	vk::MemoryAllocateInfo allocInfo(memoryReq.size,findMemoryType(_device->getPhysicalDevice(),memoryReq.memoryTypeBits, properties));
	_memory = vk_device.allocateMemory(allocInfo);

	vk_device.bindImageMemory(_image,_memory,0);
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

	vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(format)) {
			aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}

	uint srcAccess;
	uint dstAccess;
	if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		srcAccess = (uint)vk::AccessFlagBits::eHostWrite;
		dstAccess = (uint)vk::AccessFlagBits::eTransferWrite;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		srcAccess = (uint)vk::AccessFlagBits::eTransferWrite;
		dstAccess = (uint)vk::AccessFlagBits::eShaderRead;
	} else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            srcAccess = 0;
            dstAccess = (uint)(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    } else if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eGeneral) {
		srcAccess = (uint)vk::AccessFlagBits::eHostWrite;
		dstAccess = (uint)(vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
	} else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vk::ImageSubresourceRange subRes(
		aspectMask,
		0, _mipLevels, /* Mip levels current and count*/ 0, 1 /* Layers current and count */ );

	vk::ImageMemoryBarrier barrier(
		(vk::AccessFlagBits)srcAccess,
		(vk::AccessFlagBits)dstAccess,
		oldLayout,
		newLayout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		_image,
		subRes
	);

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
		vk::DependencyFlagBits::eByRegion,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
}

void Image::setBuffer(const spBuffer& buffer){
	vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

	vk::ImageSubresourceLayers subRes(
		vk::ImageAspectFlagBits::eColor,
		0, /* Mip levels current*/ 0, 1 /* Layers current and count */ );

	vk::BufferImageCopy region(
		/* bufferOffset bufferRowLength bufferImageHeight */
		0, 0, 0,
		/* subRes imageOffest */ 
		subRes, {0, 0, 0},
		/* imageExtent */
		{_width,_height,1}
	);

	commandBuffer.copyBufferToImage(buffer->buffer,_image,vk::ImageLayout::eTransferDstOptimal, {region});

	endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
}

void Image::set(const spBuffer& buffer, const std::vector<uint>& offsets, const std::vector<glm::ivec2>& sizes){
	transition(_format,vk::ImageLayout::ePreinitialized,vk::ImageLayout::eTransferDstOptimal);

	for(uint l = 0;l<offsets.size();++l){
		vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

		vk::ImageSubresourceLayers subRes(
			vk::ImageAspectFlagBits::eColor,
			l, /* Mip levels current*/ 0, 1 /* Layers current and count */ );

		vk::BufferImageCopy region(
			/* bufferOffset bufferRowLength bufferImageHeight */
			offsets[l], 0, 0,
			/* subRes imageOffest */ 
			subRes, {0, 0, 0},
			/* imageExtent */
			{sizes[l].x,sizes[l].y,1}
		);

		commandBuffer.copyBufferToImage(buffer->buffer,_image,vk::ImageLayout::eTransferDstOptimal, {region});

		endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
	}

	transition(_format,vk::ImageLayout::eTransferDstOptimal,vk::ImageLayout::eShaderReadOnlyOptimal);
}

vk::ImageView Image::ImageView(){
	if(_imageView)return _imageView;
	else {
		_imageView = r267::createImageView(_device->getDevice(),_image,_format,vk::ImageAspectFlagBits::eColor, _mipLevels);
		return _imageView;
	}
}

uint Image::mipLevels(){
	return _mipLevels;
}

uint Image::width(){
	return _width;
}

uint Image::height(){
	return _height;
}

void Image::release(spDevice device){
	auto vk_device = device->getDevice();
	vk_device.destroyImageView(_imageView);
	vk_device.freeMemory(_memory);
	vk_device.destroyImage(_image);
}
