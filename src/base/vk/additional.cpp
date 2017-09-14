#include "additional.hpp"

using namespace r267;

std::vector<spFramebuffer> r267::createFrameBuffers(spDevice device,spPipeline pipeline){
	std::vector<spFramebuffer> framebuffers;

	auto swapchain = device->getSwapchain();
	auto imageViews = swapchain->getImageViews();
	auto extent = swapchain->getExtent();
	auto vk_device = device->getDevice();

	for(int i = 0;i<imageViews.size();++i){
		spFramebuffer framebuffer = device->create<Framebuffer>();

		framebuffer->attachment(imageViews[i]);
		framebuffer->depth(extent.width,extent.height);
		framebuffer->create(extent.width,extent.height,pipeline->getRenderPass());

		framebuffers.push_back(framebuffer);
	}
	return framebuffers;
}

spImage r267::checkboardTexture(spDevice device,const uint& width, const uint& height, const uint& step){
	// Create data
	glm::vec4* pixels = new glm::vec4[width*height];
	for(uint y = 0;y<height;++y){
		uint yStep = (y/step);
		bool isLine = yStep%2;
		for(uint x = 0;x<width;++x){
			uint xStep = (x/step);
			bool isX = (xStep+isLine)%2;
			if(isX){
				pixels[y*width+x] = glm::vec4(1.0f,1.0f,1.0f,1.0f);
			} else pixels[y*width+x] = glm::vec4(0,0,0,1.0f);
		}
	}

	size_t pixelSize = sizeof(glm::vec4);
	vk::DeviceSize size = width*height*pixelSize;

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(pixels,size);

	spImage image = device->create<Image>();
	image->create(width,height,vk::Format::eR32G32B32A32Sfloat);
	image->transition(vk::ImageLayout::eTransferDstOptimal);
	image->set(cpu);
	image->transition(vk::ImageLayout::eShaderReadOnlyOptimal);

	return image;
}

spImage r267::defaultCubemap(spDevice device,const uint& width,const uint& height){
	// Create data
	const int step = std::max(width,height)/20;
	glm::vec4* pixels = new glm::vec4[width*height];
	for(uint y = 0;y<height;++y){
		uint yStep = (y/step);
		bool isLine = yStep%2;
		for(uint x = 0;x<width;++x){
			uint xStep = (x/step);
			bool isX = (xStep+isLine)%2;
			if(isX){
				pixels[y*width+x] = glm::vec4(1.0f,1.0f,1.0f,1.0f);
			} else pixels[y*width+x] = glm::vec4(0,0,0,1.0f);
		}
	}

	size_t pixelSize = sizeof(glm::vec4);
	vk::DeviceSize size = width*height*pixelSize;

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(pixels,size);

	spImage image = device->create<Image>();
	image->createCubemap(width,height,vk::Format::eR32G32B32A32Sfloat);
	image->transition(vk::ImageLayout::eTransferDstOptimal);
	for(int i = 0;i<6;++i){
		image->set(cpu,glm::ivec2(width,height),0,i,0);
	}
	image->transition(vk::ImageLayout::eShaderReadOnlyOptimal);

	return image;
}

spImage r267::whiteTexture(spDevice device,const uint& width, const uint& height){
	// Create data
	glm::vec4* pixels = new glm::vec4[width*height];
	for(uint y = 0;y<height;++y){
		for(uint x = 0;x<width;++x){
			pixels[y*width+x] = glm::vec4(1.0f,1.0f,1.0f,1.0f);
		}
	}

	size_t pixelSize = sizeof(glm::vec4);
	vk::DeviceSize size = width*height*pixelSize;

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(pixels,size);

	spImage image = device->create<Image>();
	image->create(width,height,vk::Format::eR32G32B32A32Sfloat);
	image->transition(vk::ImageLayout::eTransferDstOptimal);
	image->set(cpu);
	image->transition(vk::ImageLayout::eShaderReadOnlyOptimal);

	return image;
}

// Downscale to 2x
template<typename T>
std::vector<T> downscale(const std::vector<T>& pixels,const uint& width,const uint& height){
	uint width2 = width/2;
	uint height2 = height/2;
	std::vector<T> result;
	for(uint y2 = 0;y2<height2;++y2){
		for(uint x2 = 0;x2<width2;++x2){
			float pixel[4] = {0};
			float num = 0.0f;
			// Compute middle color
			for(uint ys = 0;ys<2;++ys){
				uint y = y2*2+ys;
				if(y >= height)continue;
				for(uint xs = 0;xs<2;++xs){
					uint x = x2*2+xs;
					if(x >= width)continue;
					for(uint ch = 0;ch<4;++ch){
						pixel[ch] += pixels[(y*width+x)*4+ch];
					}
					num+=1.0f;
				}
			}
			for(uint ch = 0;ch<4;++ch){
				if(num != 0.0f)pixel[ch] = pixel[ch]/num;
				result.push_back(pixel[ch]);
			}
		}
	}
	return result;
}

// Very simple gen mipmaps
template<typename T>
std::vector<T> genMipmaps(const std::vector<T>& pixels,const uint& width,const uint& height, uint& mipLevels, std::vector<uint>& offsets, std::vector<glm::ivec2>& sizes){
	uint w = width;
	uint h = height;
	mipLevels = 0;
	std::vector<T> result;
	while(w > 1 && h > 1){
		std::cout << "w " << w << ", h " << h << std::endl;
		sizes.push_back(glm::ivec2(w,h));
		w /= 2;
		h /= 2;
		mipLevels++;
	}
	result.insert(result.end(), pixels.begin(), pixels.end());
	auto prevLevel = pixels;
	w = width; h = height;
	offsets.push_back(0);
	for(int m = 1;m<mipLevels;++m){
		auto level = downscale<T>(prevLevel,w,h);

		offsets.push_back(result.size()*sizeof(T));
		result.insert(result.end(), level.begin(), level.end());
		prevLevel = level;

		w/=2;h/=2;
	}
	return result;
}

void genMipmapsGPU(spDevice device,spImage image){

}

// Simple loading image with usage OpenImageIO
// Usage overheaded format
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
OIIO_NAMESPACE_USING

template<typename T>
spImage readImage(spDevice device,ImageInput* in,TypeDesc inputFormat,vk::Format format,int width,int height,int channels){
	std::vector<T> pixels (width*height*channels);
	in->read_image (inputFormat, pixels.data());
	in->close ();
	ImageInput::destroy (in);

	std::cout << channels << std::endl;

	if(channels == 3){
		// Bad case emulate RGBA8
		std::vector<T> pixels4(width*height*4);
		for(int y = 0;y<height;++y){
			for(int x = 0;x<width;++x){
				pixels4[(y*width+x)*4+0] = pixels[(y*width+x)*3];
				pixels4[(y*width+x)*4+1] = pixels[(y*width+x)*3+1];
				pixels4[(y*width+x)*4+2] = pixels[(y*width+x)*3+2];
				pixels4[(y*width+x)*4+3] = 1.0f;
			}
		}
		pixels = pixels4;
		channels = 4;
	}
	size_t pixelSize = channels*sizeof(T);

	vk::DeviceSize size = pixels.size()*sizeof(T);

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(pixels.data(),size);

	int mipLevels = floor(log2(std::max(width, height))) + 1;

	spImage image = device->create<Image>();
	image->create(width,height,format,mipLevels,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eSampled);
	image->transition(vk::ImageLayout::eTransferDstOptimal,vk::ImageLayout::ePreinitialized,0,1,0,1);
	image->set(cpu);
	image->transition(vk::ImageLayout::eTransferSrcOptimal,vk::ImageLayout::eTransferDstOptimal,0,1,0,1);

	auto vk_device = device->getDevice();
	vk::CommandBufferAllocateInfo allocInfo(device->getCommandPool(),vk::CommandBufferLevel::ePrimary, 1);

	vk::CommandBuffer blitCmd = vk_device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
	blitCmd.begin(&beginInfo);

	for(int i = 1;i<mipLevels;++i){
		vk::ImageBlit imageBlit;

		imageBlit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor,i-1,0,1);
		imageBlit.srcOffsets[1].x = width >> (i-1);
		imageBlit.srcOffsets[1].y = height >> (i-1);
		imageBlit.srcOffsets[1].z = 1;

		imageBlit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor,i,0,1);
		imageBlit.dstOffsets[1].x = width >> (i);
		imageBlit.dstOffsets[1].y = height >> (i);
		imageBlit.dstOffsets[1].z = 1;

		image->transition(blitCmd,vk::ImageLayout::eTransferDstOptimal,vk::ImageLayout::ePreinitialized,i,1,0,1);

		blitCmd.blitImage(image->vk_image(),vk::ImageLayout::eTransferSrcOptimal,image->vk_image(),vk::ImageLayout::eTransferDstOptimal,imageBlit,vk::Filter::eLinear);

		image->transition(blitCmd,vk::ImageLayout::eTransferSrcOptimal,vk::ImageLayout::eTransferDstOptimal,i,1,0,1);
	}
	blitCmd.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &blitCmd;

	// Create fence to ensure that the command buffer has finished executing
	vk::FenceCreateInfo fenceInfo;
	vk::Fence fence;
	fence = vk_device.createFence(fenceInfo);

	device->getGraphicsQueue().submit(submitInfo,fence);
	vk_device.waitForFences(fence,true,100000000000);

	// Transition image to shader read optimal
	image->transition(vk::ImageLayout::eShaderReadOnlyOptimal,vk::ImageLayout::eTransferSrcOptimal);

	return image;
}

vk::Format getFormat(TypeDesc format,int channels){
	switch(channels){
		case 1:
		switch(format.basetype){
			case TypeDesc::UINT8:
			return vk::Format::eR8Unorm;
			case TypeDesc::FLOAT:
			return vk::Format::eR32Sfloat;
			case TypeDesc::HALF:
			return vk::Format::eR16Sfloat;
		}
		case 2:
		switch(format.basetype){
			case TypeDesc::UINT8:
			return vk::Format::eR8G8Unorm;
			case TypeDesc::FLOAT:
			return vk::Format::eR32G32Sfloat;
			case TypeDesc::HALF:
			return vk::Format::eR16G16Sfloat;
		}
		case 3:
		case 4:
		switch(format.basetype){
			case TypeDesc::UINT8:
			return vk::Format::eR8G8B8A8Unorm;
			case TypeDesc::FLOAT:
			return vk::Format::eR32G32B32A32Sfloat;
			case TypeDesc::HALF:
			return vk::Format::eR16G16B16A16Sfloat;
		}
	}
}

spImage r267::loadImage(spDevice device,const std::string& filename){
	ImageInput *in = ImageInput::open (filename);
	if (! in){
		std::stringstream str;
		str << "Image " << filename << " don't found";
		throw std::runtime_error(str.str());
	}

	const ImageSpec &spec = in->spec();
	uint width = spec.width;
	uint height = spec.height;
	uint channels = spec.nchannels;
	TypeDesc format = spec.channelformat(0);
	for(int i = 1;i<channels;++i){
		if(format != spec.channelformat(i))throw std::runtime_error("Image channels has different channel");
	}

	switch(format.basetype){
		case TypeDesc::UINT8:
		return readImage<uint8_t>(device,in,format,getFormat(format,channels),width,height,channels);
		break;
		case TypeDesc::FLOAT:
		return readImage<float>(device,in,format,getFormat(format,channels),width,height,channels);
		break;
		case TypeDesc::HALF:
		return readImage<float>(device,in,format,getFormat(format,channels),width,height,channels);
		break;
		default:
		throw std::runtime_error("Unsupported channels format");
	}
}

vk::ImageMemoryBarrier r267::imageBarrier(spImage image, AccessTransfer access, ImageLayoutTransfer layout){
	vk::ImageMemoryBarrier memoryBarrier(
			access.src,access.dst,
			layout.src,layout.dst,
			0,0, // Queue src,dst zero by default
			image->vk_image(),
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,0,image->mipLevels(),0,1)
	);
	return memoryBarrier;
}
