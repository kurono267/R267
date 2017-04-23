#include "base/wnd/MainApp.hpp"
#include <base/vk/pipeline.hpp>

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

			auto baseRP = RenderPattern::basic(swapchain);
			_main = std::make_shared<Pipeline>(baseRP,vk_device);

			_main->addShader(vk::ShaderStageFlagBits::eVertex,"assets/triangle/main_vert.spv");
			_main->addShader(vk::ShaderStageFlagBits::eFragment,"assets/triangle/main_frag.spv");

			vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlags(),device->queueFamiliesIndices().graphicsFamily);
			_commandPool = vk_device.createCommandPool(poolInfo); 

			_colorData.color = glm::vec4(0.5f,0.5f,0.0f,1.0f);
			_color.set(sizeof(UBO),&_colorData);
			_color.create(device,device->getGraphicsQueue(),_commandPool);

			_main->setUniformBuffer(_color,0,vk::ShaderStageFlagBits::eVertex);
			_main->create();

			auto imageViews = swapchain->getImageViews();
			auto extent = swapchain->getExtent();
			for(int i = 0;i<imageViews.size();++i){
				vk::ImageView attachments[] = {imageViews[i]};

				vk::FramebufferCreateInfo framebufferInfo(
					vk::FramebufferCreateFlags(), // Default
					_main->getRenderPass(), // Current render pass
					1, attachments, // Attachments
					extent.width, // Width
					extent.height, // Height
					1 // Layers
				);
				_framebuffers.push_back(vk_device.createFramebuffer(framebufferInfo));
			}

			vk::CommandBufferAllocateInfo allocInfo(_commandPool,vk::CommandBufferLevel::ePrimary, _framebuffers.size());
			_commandBuffers = vk_device.allocateCommandBuffers(allocInfo);

			for(int i = 0;i<_commandBuffers.size();++i){
				// Fill Render passes
				vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

				_commandBuffers[i].begin(&beginInfo);

				vk::ClearValue clearColor = vk::ClearColorValue(std::array<float,4>{0.0f, 0.5f, 0.0f, 1.0f});
				vk::RenderPassBeginInfo renderPassInfo(
					_main->getRenderPass(),
					_framebuffers[i],
					vk::Rect2D(vk::Offset2D(),extent),
					1, &clearColor
				);

				_commandBuffers[i].beginRenderPass(&renderPassInfo,vk::SubpassContents::eInline);
				_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *_main);

					_commandBuffers[i].draw(3, 1, 0, 0);

				_commandBuffers[i].endRenderPass();
				_commandBuffers[i].end();
			}

			// Create semaphores
			vk::SemaphoreCreateInfo semaphoreInfo;
			_imageAvailable = vk_device.createSemaphore(semaphoreInfo);
			_renderFinish = vk_device.createSemaphore(semaphoreInfo);

			return true;
		}
		bool draw(){
			unsigned int imageIndex = vk_device.acquireNextImageKHR(swapchain->getSwapchain(),std::numeric_limits<uint64_t>::max(),_imageAvailable,nullptr).value;

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
		bool onExit(){}
	protected:
		spPipeline _main;
		UBO        _colorData;
		Uniform    _color;

		// Framebuffers
		std::vector<vk::Framebuffer> _framebuffers;
		// Command Buffers
		vk::CommandPool _commandPool;
		std::vector<vk::CommandBuffer> _commandBuffers;
		// Semaphores
		vk::Semaphore _imageAvailable;
		vk::Semaphore _renderFinish;
};

int main(){
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<TriangleApp>(main);

	main->create("Triangle",1280,720);
	main->setBaseApp(app);

	main->run();

	return 0;
}