#include "base/wnd/MainApp.hpp"
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>

#include <chrono>

using namespace r267;

struct UBO {
	glm::vec4 color;
};

class TriangleApp : public BaseApp {
	public:
		TriangleApp(spMainApp app) : BaseApp(app) {}
		virtual ~TriangleApp(){}

		bool init(){
			vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
			_commandPool = device->getCommandPool();

			auto baseRP = RenderPattern::basic(device);
			_main = std::make_shared<Pipeline>(baseRP,vk_device);

			_main->addShader(vk::ShaderStageFlagBits::eVertex,"assets/triangle/main_vert.spv");
			_main->addShader(vk::ShaderStageFlagBits::eFragment,"assets/triangle/main_frag.spv");

			_main->create();

			_vb = device->create<Buffer>();
			_vb->createVB({sVertex(0.0f, -1.0f, 0.0f),sVertex(1.0f, 1.0f, 0.0f),sVertex(-1.0f, 1.0f, 0.0f)});

			_ib = device->create<Buffer>();
			_ib->createIB({0,1,2});

			auto extent = swapchain->getExtent();
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
					vk::Rect2D(vk::Offset2D(),extent),
					clearValues.size(), clearValues.data()
				);

				_commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
				_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

				vk::DeviceSize offsets[] = {0};
					_commandBuffers[i].bindVertexBuffers(0,1,&_vb->buffer,offsets);
					_commandBuffers[i].bindIndexBuffer(_ib->buffer,0,vk::IndexType::eUint32);

					_commandBuffers[i].drawIndexed(3, 1, 0, 0 , 0);

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

			vk::Semaphore waitSemaphores[] = {_imageAvailable};
			vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

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

		// Framebuffers
		std::vector<spFramebuffer> _framebuffers;
		// Command Buffers
		vk::CommandPool _commandPool;
		std::vector<vk::CommandBuffer> _commandBuffers;
		// Semaphores
		vk::Semaphore _imageAvailable;
		vk::Semaphore _renderFinish;

		spBuffer _vb;
		spBuffer _ib;

		std::chrono::time_point<std::chrono::steady_clock> prev;
};

int main(){
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<TriangleApp>(main);

	main->create("Triangle",1280,720);
	main->setBaseApp(app);

	main->run();

	return 0;
}