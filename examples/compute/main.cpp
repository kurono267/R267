#include <base/wnd/MainApp.hpp>
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/vk/compute.hpp>

#include <chrono>

using namespace r267;

struct ComputeCamera {
	glm::vec4  pos; 
	glm::vec4  up;
	glm::vec4  right;
	glm::ivec4 viewport; 
};

class ComputeApp : public BaseApp {
	public:
		ComputeApp(spMainApp app) : BaseApp(app) {}
		virtual ~ComputeApp(){}

		bool init(){
			vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
			_commandPool = device->getCommandPool();

			glm::ivec2 wndSize = mainApp->wndSize();

			// Create Compute Shader
			_surface = device->create<Image>();
			_surface->create(wndSize.x,wndSize.y,vk::Format::eR8G8B8A8Unorm,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);

            _defaultSampler = createSampler(vk_device,linearSampler());

			_computeDescSet = device->create<DescSet>();
            _computeDescSet->setTexture(_surface->createImageView(),_defaultSampler,0,vk::ShaderStageFlagBits::eCompute,
                                        vk::DescriptorType::eStorageImage, vk::ImageLayout::eGeneral);
            _computeDescSet->create();

            _quadDescSet = device->create<DescSet>();
            _quadDescSet->setTexture(_surface->createImageView(),_defaultSampler,0,vk::ShaderStageFlagBits::eFragment);
            _quadDescSet->create();

			_compute = std::make_shared<Compute>(device);
			_compute->create("assets/compute/main_comp.spv",_computeDescSet);
			_compute->dispatch(wndSize);

			auto baseRP = RenderPattern::basic(device);
			_main = std::make_shared<Pipeline>(baseRP,vk_device);

			_main->addShader(vk::ShaderStageFlagBits::eVertex,"assets/compute/main_vert.spv");
			_main->addShader(vk::ShaderStageFlagBits::eFragment,"assets/compute/main_frag.spv");

			_main->descSet(_quadDescSet);
			_main->create();

			_quad = std::make_shared<Quad>();
			_quad->create(device);

			_framebuffers = createFrameBuffers(device,_main);

			vk::CommandBufferAllocateInfo allocInfo(_commandPool,vk::CommandBufferLevel::ePrimary, _framebuffers.size());
			_commandBuffers = vk_device.allocateCommandBuffers(allocInfo);

			for(int i = 0;i<_commandBuffers.size();++i){
				// Fill Render passes
				vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

				_commandBuffers[i].begin(&beginInfo);

				std::array<vk::ClearValue, 2> clearValues = {};
				clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.5f, 0.0f, 1.0f});
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

				vk::RenderPassBeginInfo renderPassInfo(
					_main->getRenderPass(),
					_framebuffers[i]->vk_framebuffer(),
					vk::Rect2D(vk::Offset2D(),swapchain->getExtent()),
					clearValues.size(), clearValues.data()
				);

				_commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
				_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

					vk::DescriptorSet descSets[] = {_quadDescSet->getDescriptorSet()};
					_commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _main->getPipelineLayout(), 0, 1, descSets, 0, nullptr);

					_quad->draw(_commandBuffers[i]);

				_commandBuffers[i].endRenderPass();
				_commandBuffers[i].end();
			}

			// Create semaphores
			_imageAvailable = device->createSemaphore(vk::SemaphoreCreateInfo());
			_renderFinish = device->createSemaphore(vk::SemaphoreCreateInfo());

			prev = std::chrono::steady_clock::now();

			return true;
		}
		bool draw(){
			unsigned int imageIndex = vk_device.acquireNextImageKHR(swapchain->getSwapchain(),std::numeric_limits<uint64_t>::max(),_imageAvailable,nullptr).value;

			auto next = std::chrono::steady_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::duration<double> >(next - prev).count();
			std::cout << "FPS " << 1.0f/dt << std::endl;
			prev = next;

			auto computeFinish = _compute->run(_imageAvailable);

			vk::Semaphore waitSemaphores[] = {computeFinish};
			vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eComputeShader};

			vk::Semaphore signalSemaphores[] = {_renderFinish};

			vk::SubmitInfo submitInfo(
				1, waitSemaphores,
				waitStages, 
				1, &_commandBuffers[imageIndex],
				1, signalSemaphores
			);

			device->getGraphicsQueue().submit(submitInfo, nullptr);

			vk::SwapchainKHR swapChains[] = {swapchain->getSwapchain()};

			vk::PresentInfoKHR presentInfo(
				1, signalSemaphores,
				1, swapChains,
				&imageIndex
			);

			device->getPresentQueue().presentKHR(presentInfo);

			return true;
		}
		bool update(){
			return true;
		}
		
		bool onKey(const GLFWKey& key){}
		bool onMouse(const GLFWMouse& mouse){}
		bool onExit(){
			vulkan = mainApp->vulkan();device = vulkan->device();vk_device = device->getDevice();
			vk_device.freeCommandBuffers(device->getCommandPool(),_commandBuffers);
			_main->release();
		}
	protected:
		spPipeline _main;

		vk::Sampler _defaultSampler;

		// Framebuffers
		std::vector<spFramebuffer> _framebuffers;
		// Command Buffers
		vk::CommandPool _commandPool;
		std::vector<vk::CommandBuffer> _commandBuffers;
		// Semaphores
		vk::Semaphore _imageAvailable;
		vk::Semaphore _renderFinish;

		spShape _quad;

		spCompute _compute;
		spDescSet _computeDescSet;
        spDescSet _quadDescSet;
		spImage   _surface;

		std::chrono::time_point<std::chrono::steady_clock> prev;
};

int main(){
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<ComputeApp>(main);

	main->create("Quad",1280,720);
	main->setBaseApp(app);

	main->run();

	return 0;
}