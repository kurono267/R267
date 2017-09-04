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

void Image::transition(const vk::ImageLayout& newLayout){
	transition(newLayout,_currLayout);
}

void flagsFromLayout(const vk::ImageLayout& layout,vk::AccessFlags& accessFlag,vk::PipelineStageFlags& stage){
	switch(layout){
		case vk::ImageLayout::ePreinitialized:
			accessFlag = vk::AccessFlagBits::eHostWrite;
			stage      = vk::PipelineStageFlagBits::eHost;
		break;
		case vk::ImageLayout::eTransferDstOptimal:
			accessFlag = vk::AccessFlagBits::eTransferWrite;
			stage      = vk::PipelineStageFlagBits::eTransfer;
		break;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			accessFlag = vk::AccessFlagBits::eShaderRead;
			stage      = vk::PipelineStageFlagBits::eFragmentShader;
		break;
		case vk::ImageLayout::eUndefined:
			accessFlag = (vk::AccessFlagBits)0;
			stage      = vk::PipelineStageFlagBits::eTopOfPipe;
		break;
		case vk::ImageLayout::eGeneral:
			accessFlag = vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eShaderWrite;
			stage      = vk::PipelineStageFlagBits::eFragmentShader;
		break;
		case vk::ImageLayout::eTransferSrcOptimal:
			accessFlag = vk::AccessFlagBits::eTransferRead;
			stage      = vk::PipelineStageFlagBits::eTransfer;
		break;
		case vk::ImageLayout::eColorAttachmentOptimal:
			accessFlag = vk::AccessFlagBits::eColorAttachmentWrite;
			stage      = vk::PipelineStageFlagBits::eFragmentShader;
		break;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			accessFlag = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			stage      = vk::PipelineStageFlagBits::eFragmentShader;
		break;
	}
}

void Image::transition(vk::CommandBuffer& cmd,const vk::ImageLayout& newLayout, const vk::ImageLayout& oldLayout, const int level, const int numLevels, const int layer, const int numLayers){
	vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(_format)) {
			aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;
	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;
	flagsFromLayout(oldLayout,srcAccess,srcStage);
	flagsFromLayout(newLayout,dstAccess,dstStage);

	vk::ImageSubresourceRange subRes(
		aspectMask,
		level, numLevels==-1?_mipLevels:numLevels, /* Mip levels current and count*/ layer, numLayers==-1?_layers:numLayers /* Layers current and count */ );

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

	cmd.pipelineBarrier(
		srcStage, dstStage,
		vk::DependencyFlagBits::eByRegion,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void Image::transition(const vk::ImageLayout& newLayout, const vk::ImageLayout& oldLayout, const int level, const int numLevels, const int layer, const int numLayers){
	vk::CommandBuffer commandBuffer = beginSingle(_device->getDevice(),_pool);

	transition(commandBuffer,newLayout,oldLayout,level,numLevels,layer,numLayers);

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
