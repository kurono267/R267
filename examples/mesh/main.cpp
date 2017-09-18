#include "base/wnd/MainApp.hpp"
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/vk/image.hpp>
#include <base/scene/scene.hpp>
#include <base/scene/camera.hpp>

#include <chrono>

using namespace r267;

struct UBO {
	glm::mat4 mvp;
	glm::vec4 view;
};

std::string scene_filename;

class MeshApp : public BaseApp {
	public:
		MeshApp(spMainApp app) : BaseApp(app) {}
		virtual ~MeshApp(){}

		bool init(){
			vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
			_commandPool = device->getCommandPool();
			glm::ivec2 wnd = mainApp->wndSize();

			// Load scene 
			_scene = std::make_shared<Scene>();
			_scene->load(scene_filename);
			spDescSet matDesc;
			// Create Vulkan meshes
			for(auto& m : _scene->models()){
				m->mesh()->create(device);
				m->material()->create(device,_imagesBuffer);
				if(!matDesc)matDesc = m->material()->getDescSet();
			}

			_cube = std::make_shared<Cube>();
			_cube->create(device);

			auto baseRP = RenderPattern::basic(device);
			_main = std::make_shared<Pipeline>(baseRP,vk_device);

			_sceneDesc = device->create<DescSet>();

			_main->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/mesh/main_vert.spv");
			_main->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/mesh/main_frag.spv");

			_camera = std::make_shared<Camera>(vec3(0.0f, 0.0f, -15.0f),vec3(0.0f,0.0f,0.0f),vec3(0.0,-1.0f,0.0f));
			_camera->setProj(glm::radians(45.0f),(float)(wnd.x)/(float)(wnd.y),0.1f,10000.0f);

			_mvpData.mvp = _camera->getVP();
			_mvpData.view = glm::vec4(_camera->getPos(),1.0f);
			_mvp.create(device,sizeof(UBO),&_mvpData);

			_sceneDesc->setUniformBuffer(_mvp,0,vk::ShaderStageFlagBits::eVertex);
			_sceneDesc->create();

			_main->descSets({_sceneDesc,matDesc});
			_main->create();

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

					// Draw scene
					for(auto& model : _scene->models()){
						vk::DescriptorSet descSets[] = {_sceneDesc->getDescriptorSet(),model->material()->getDescSet()->getDescriptorSet()};

						_commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _main->getPipelineLayout(), 0, 2, descSets, 0, nullptr);

						model->mesh()->draw(_commandBuffers[i]);
					}

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
			_mvp.set(sizeof(UBO),&_mvpData);

			auto next = std::chrono::steady_clock::now();
			_dt = std::chrono::duration_cast<std::chrono::duration<double> >(next - prev).count();
			std::cout << "FPS " << 1.0f/_dt << std::endl;
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
		bool onMouse(const GLFWMouse& mouse){
			if(mouse.callState == GLFWMouse::onMouseButton){
				if(mouse.button == GLFW_MOUSE_BUTTON_1){
					if(mouse.action == GLFW_PRESS){
						_isPressed = true;
						_isFirst = true;
					} else _isPressed = false;
				}
			} else if (mouse.callState == GLFWMouse::onMousePosition){
				if(_isPressed){
					if(!_isFirst){
						glm::vec2 dp = glm::vec2(mouse.x,mouse.y)-_prev_mouse;
						_camera->rotate(dp,_dt);
					}
					_prev_mouse = glm::vec2(mouse.x,mouse.y);
					_isFirst = false;
				}
			}
			_mvpData.view = glm::vec4(_camera->getPos(),1.0f);
			_mvpData.mvp = _camera->getVP();
		}
		bool onScroll(const glm::vec2& offset){
			_camera->scale(offset.y,_dt);
			_mvpData.view = glm::vec4(_camera->getPos(),1.0f);
			_mvpData.mvp = _camera->getVP();
			return true;
		}
		bool onExit(){
			vulkan = mainApp->vulkan();device = vulkan->device();vk_device = device->getDevice();
			vk_device.freeCommandBuffers(device->getCommandPool(),_commandBuffers);
			_main->release();
		}
	protected:
		spDescSet  _sceneDesc;
		spPipeline _main;
		UBO        _mvpData;
		Uniform    _mvp;

		// Framebuffers
		std::vector<spFramebuffer> _framebuffers;
		// Command Buffers
		vk::CommandPool _commandPool;
		std::vector<vk::CommandBuffer> _commandBuffers;
		// Semaphores
		vk::Semaphore _imageAvailable;
		vk::Semaphore _renderFinish;

		std::unordered_map<std::string,spImage> _imagesBuffer;

		spScene  _scene;
		spShape  _cube;
		spCamera _camera;

		// For camera rotate
		bool _isPressed;
		bool _isFirst;
		glm::vec2 _prev_mouse;
		float _dt;

		std::chrono::time_point<std::chrono::steady_clock> prev;
};

int main(int argc, char const *argv[]){
	if(argc < 2){
		std::cout << "Wrong arguments" << std::endl;
		return -1;
	}
	scene_filename = argv[1];
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<MeshApp>(main);

	main->create("Mesh",1280,720);
	main->setBaseApp(app);

	main->run();
	return 0;
}
