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
	image->set(cpu);

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
	image->set(cpu);

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

// Simple loading image with usage OpenImageIO
// Usage overheaded format
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING

template<typename T>
spImage readImage(spDevice device,ImageInput* in,TypeDesc inputFormat,vk::Format format,int width,int height,int channels){
	std::vector<T> pixels (width*height*channels);
	in->read_image (inputFormat, pixels.data());
	in->close ();
	ImageInput::destroy (in);

	size_t pixelSize = 4*sizeof(T);
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
	} else if(channels != 4)throw std::runtime_error("Unsupported format");

	uint mipLevels = 1;std::vector<uint> offsets;
	std::vector<glm::ivec2> sizes;
	auto mipMaps = genMipmaps<T>(pixels,width,height,mipLevels,offsets,sizes);

	vk::DeviceSize size = mipMaps.size()*sizeof(T);

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(mipMaps.data(),size);

	spImage image = device->create<Image>();
	image->create(width,height,format,mipLevels);
	image->set(cpu,offsets,sizes);

	return image;
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

	std::cout << format << std::endl;

	switch(format.basetype){
		case TypeDesc::UINT8:
		return readImage<uint8_t>(device,in,format,vk::Format::eR8G8B8A8Unorm,width,height,channels);
		break;
		case TypeDesc::FLOAT:
		return readImage<float>(device,in,format,vk::Format::eR32G32B32A32Sfloat,width,height,channels);
		break;
		case TypeDesc::HALF:
		return readImage<float>(device,in,format,vk::Format::eR16G16B16A16Unorm,width,height,channels);
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
