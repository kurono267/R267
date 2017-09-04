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
	_currLayout    = layout;
	_layers   = 1;

	vk::ImageCreateInfo imageInfo(
		vk::ImageCreateFlags(), // Basic
		vk::ImageType::e2D, // Type 1D,2D,3D
		format, // Format
		vk::Extent3D(width,height,1), // Width, Height and Depth
		mipLevels, // Mip Levels
		_layers, // Array Layers
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
	setBuffer(buffer,glm::ivec2(_width,_height),0,0,0);
}

vk::Image Image::vk_image(){
	return _image;
}

void Image::transition(const vk::ImageLayout& newLayout) {
	const vk::ImageLayout oldLayout = _currLayout;
	vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

	vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(_format)) {
			aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}

	uint srcAccess;
	uint dstAccess;
	vk::PipelineStageFlagBits srcStage;
	vk::PipelineStageFlagBits dstStage;
	if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		srcAccess = (uint)vk::AccessFlagBits::eHostWrite;
		dstAccess = (uint)vk::AccessFlagBits::eTransferWrite;
		srcStage  = vk::PipelineStageFlagBits::eHost;
		dstStage  = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		srcAccess = (uint)vk::AccessFlagBits::eTransferWrite;
		dstAccess = (uint)vk::AccessFlagBits::eShaderRead;
		srcStage  = vk::PipelineStageFlagBits::eTransfer;
		dstStage  = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            srcAccess = 0;
            dstAccess = (uint)(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
            srcStage  = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage  = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eGeneral) {
		srcAccess = (uint)vk::AccessFlagBits::eHostWrite;
		dstAccess = (uint)(vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite);
		srcStage  = vk::PipelineStageFlagBits::eHost;
		dstStage  = vk::PipelineStageFlagBits::eFragmentShader;
	} else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vk::ImageSubresourceRange subRes(
		aspectMask,
		0, _mipLevels, /* Mip levels current and count*/ 0, _layers /* Layers current and count */ );

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
		srcStage, dstStage,
		vk::DependencyFlagBits::eByRegion,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
	_currLayout = newLayout;
}

void Image::setBuffer(const spBuffer& buffer, const glm::ivec2& size, const uint& mipLevel, const uint& layer, const uint& offsetBuffer){
	vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

	vk::ImageSubresourceLayers subRes(
		vk::ImageAspectFlagBits::eColor,
		mipLevel, /* Mip levels current*/ layer, 1 /* Layers current and count */ );

	vk::BufferImageCopy region(
		/* bufferOffset bufferRowLength bufferImageHeight */
		offsetBuffer, 0, 0,
		/* subRes imageOffest */ 
		subRes, {0, 0, 0},
		/* imageExtent */
		{(uint)size.x,(uint)size.y,1}
	);

	commandBuffer.copyBufferToImage(buffer->buffer,_image,vk::ImageLayout::eTransferDstOptimal, {region});

	endSingle(_device->getDevice(),_queue,_pool,commandBuffer);
}

void Image::setMipmaps(const spBuffer& buffer, const std::vector<uint>& offsets, const std::vector<glm::ivec2>& sizes){
	for(uint l = 0;l<offsets.size();++l){
		setBuffer(buffer,sizes[l],l,0,offsets[l]);
	}
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
