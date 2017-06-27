#include "base/wnd/MainApp.hpp"
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/gui/gui.hpp>

#include <chrono>

using namespace r267;

struct UBO {
	glm::vec4 color;
};

class GUIApp : public BaseApp {
	public:
		GUIApp(spMainApp app) : BaseApp(app) {}
		virtual ~GUIApp(){}

		bool init(){
			vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
			_commandPool = device->getCommandPool();

			_gui = std::make_shared<GUI>(device);
			_gui->create(mainApp->wndSize());

			auto baseRP = RenderPattern::basic(device);
			baseRP.blend(true,RGBA,
					vk::BlendFactor::eSrcAlpha,vk::BlendFactor::eOneMinusSrcAlpha,
					vk::BlendOp::eAdd,
					vk::BlendFactor::eSrcAlpha,vk::BlendFactor::eOneMinusSrcAlpha,
					vk::BlendOp::eAdd);
			_main = std::make_shared<Pipeline>(baseRP,vk_device);
			_texDesc = device->create<DescSet>();

			_main->addShader(vk::ShaderStageFlagBits::eVertex,"assets/gui/render_vert.spv");
			_main->addShader(vk::ShaderStageFlagBits::eFragment,"assets/gui/render_frag.spv"); // Shaders for render quad with gui

			_texDesc->setTexture(_gui->getImageView(),createSampler(vk_device,linearSampler()),0,vk::ShaderStageFlagBits::eFragment);
			_texDesc->create();

			_main->descSet(_texDesc);
			_main->create();

			_quad = std::make_shared<Quad>();
			_quad->create(device);

			_framebuffers = createFrameBuffers(device,_main);

			vk::CommandBufferAllocateInfo allocInfo(_commandPool,vk::CommandBufferLevel::ePrimary, (uint32_t)_framebuffers.size());
			_commandBuffers = vk_device.allocateCommandBuffers(allocInfo);

			for(int i = 0;i<_commandBuffers.size();++i){
				// Fill Render passes
				vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

				_commandBuffers[i].begin(&beginInfo);

				std::array<vk::ClearValue, 2> clearValues = {};
				clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.0f, 0.5f, 0.0f, 0.0f});
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

				vk::RenderPassBeginInfo renderPassInfo(
					_main->getRenderPass(),
					_framebuffers[i]->vk_framebuffer(),
					vk::Rect2D(vk::Offset2D(),swapchain->getExtent()),
					(uint32_t)clearValues.size(), clearValues.data()
				);

				_commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
				_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

					vk::DescriptorSet descSets[] = {_texDesc->getDescriptorSet()};

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

			vk::Semaphore waitSemaphores[] = {_gui->render(_imageAvailable)};
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
		
		bool onKey(const GLFWKey& key){return true;}
		bool onMouse(const GLFWMouse& mouse){return true;}
		bool onExit(){
			vulkan = mainApp->vulkan();device = vulkan->device();vk_device = device->getDevice();
			vk_device.freeCommandBuffers(device->getCommandPool(),_commandBuffers);
			_main->release();
			return true;
		}
	protected:
		spDescSet  _texDesc;
		spPipeline _main;
		spGUI      _gui;

		// Framebuffers
		std::vector<spFramebuffer> _framebuffers;
		// Command Buffers
		vk::CommandPool _commandPool;
		std::vector<vk::CommandBuffer> _commandBuffers;
		// Semaphores
		vk::Semaphore _imageAvailable;
		vk::Semaphore _renderFinish;

		spShape _quad;

		std::chrono::time_point<std::chrono::steady_clock> prev;
};

int main(){
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<GUIApp>(main);

	main->create("Texture",1280,720);
	main->setBaseApp(app);

	main->run();

	return 0;
}