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

static nk_color background = nk_rgb(28,48,62);
class GUIApp : public BaseApp {
	public:
		GUIApp(spMainApp app) : BaseApp(app) {}
		virtual ~GUIApp(){}

		bool updateGUI(nk_context* ctx){
			if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
						 NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
						 NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
			{
				enum {EASY, HARD};
				static int op = EASY;
				static int property = 20;
				nk_layout_row_static(ctx, 30, 80, 1);
				if (nk_button_label(ctx, "button"))
					fprintf(stdout, "button pressed\n");

				nk_layout_row_dynamic(ctx, 30, 2);
				if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
				if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

				nk_layout_row_dynamic(ctx, 25, 1);
				nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

				nk_layout_row_dynamic(ctx, 20, 1);
				nk_label(ctx, "background:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(ctx, 25, 1);
				if (nk_combo_begin_color(ctx, background, nk_vec2(nk_widget_width(ctx),400))) {
					nk_layout_row_dynamic(ctx, 120, 1);
					background = nk_color_picker(ctx, background, NK_RGBA);
					nk_layout_row_dynamic(ctx, 25, 1);
					background.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, background.r, 255, 1,1);
					background.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, background.g, 255, 1,1);
					background.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, background.b, 255, 1,1);
					background.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, background.a, 255, 1,1);
					nk_combo_end(ctx);
				}
			}
			nk_end(ctx);
		}

		bool init(){
			vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
			_commandPool = device->getCommandPool();

			auto baseRP = RenderPattern::basic(device);
            baseRP.blend();
			_main = std::make_shared<Pipeline>(baseRP,vk_device);
			_texDesc = device->create<DescSet>();

			_main->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/gui/render_vert.spv");
			_main->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/gui/render_frag.spv"); // Shaders for render quad with gui

			//_main->descSet(_texDesc);
			_main->create();

			_gui = std::make_shared<GUI>(device);
			_gui->create(mainApp->wndSize(),vk::CommandBufferInheritanceInfo(_main->getRenderPass()));
			_guiFunc = std::bind(&GUIApp::updateGUI,this,std::placeholders::_1);
			_gui->update(_guiFunc);

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
				clearValues[0].color = vk::ClearColorValue(std::array<float,4>{(float)background.r/255.0f,(float)background.g/255.0f,(float)background.b/255.0f, 1.0f});
				clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

				vk::RenderPassBeginInfo renderPassInfo(
					_main->getRenderPass(),
					_framebuffers[i]->vk_framebuffer(),
					vk::Rect2D(vk::Offset2D(),swapchain->getExtent()),
					(uint32_t)clearValues.size(), clearValues.data()
				);

				_commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
				_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

					//_commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _main->getPipelineLayout(), 0, 0, nullptr, 0, nullptr);

					_quad->draw(_commandBuffers[i]);

				_commandBuffers[i].executeCommands(_gui->commandBuffer());

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
			_gui->actionUpdate(mainApp->window());
			_gui->update(_guiFunc);
			return true;
		}
		
		bool onKey(const GLFWKey& key){return true;}

		double last_button_click;
		bool onMouse(const GLFWMouse& mouse){
			return true;
		}
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

		r267::updateGUI  _guiFunc;

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