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

// Simple loading image with usage OpenImageIO
// Usage overheaded format
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING
spImage r267::loadImage(spDevice device,const std::string& filename){
	ImageInput *in = ImageInput::open (filename);
	if (! in){
		throw std::runtime_error("Image don't found");
	}
	const ImageSpec &spec = in->spec();
	int width = spec.width;
	int height = spec.height;
	int channels = spec.nchannels;
	std::vector<float> pixels (width*height*channels);
	in->read_image (TypeDesc::FLOAT, pixels.data());
	in->close ();
	ImageInput::destroy (in);

	vk::Format format = vk::Format::eR32G32B32A32Sfloat;
	size_t pixelSize = 4*sizeof(float);
	if(channels == 3){
		// Bad case emulate RGBA8
		std::vector<float> pixels4(width*height*4);
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

	vk::DeviceSize size = width*height*pixelSize;

	spBuffer cpu = device->create<Buffer>();
	cpu->create(size,vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | 
		vk::MemoryPropertyFlagBits::eHostCoherent);
	cpu->set(pixels.data(),size);

	spImage image = device->create<Image>();
	image->create(width,height,format);
	image->set(cpu);

	return image;
}
