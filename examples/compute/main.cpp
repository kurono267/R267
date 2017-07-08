#include <base/wnd/MainApp.hpp>
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/vk/compute.hpp>
#include <base/scene/bvh.hpp>

#include <chrono>
#include <base/scene/scene.hpp>
#include <base/scene/camera.hpp>

using namespace r267;

struct ComputeCamera {
	glm::vec4  pos;
	glm::vec4  look;
	glm::vec4  up;
	glm::vec4  right;
	glm::ivec4 data; // x,y wnd size z, num of nodes
};

glm::vec4 convert(const glm::vec3& v){
	return glm::vec4(v.x,v.y,v.z,0);
}

#define COMPUTE_LOCAL_SIZE 32

class ComputeApp : public BaseApp {
	public:
		ComputeApp(spMainApp app) : BaseApp(app) {}
		virtual ~ComputeApp(){}

		void updateCameraData(){
			glm::ivec2 wndSize = mainApp->wndSize();

			glm::vec3 look(_camera->getLook());
			glm::vec3 up(normalize(cross(vec3(0.0,1.0,0.0),look)));
			glm::vec3 right(normalize(cross(look,up)));
			up = normalize(cross(right,look));

			_cameraCompute.pos  = convert(_camera->getPos());
			_cameraCompute.look = convert(look);
			_cameraCompute.up  = convert(up);
			_cameraCompute.right = convert(right);
			_cameraCompute.data = glm::ivec4(wndSize.x,wndSize.y,_bvh.nodes().size(),_bvh.rootID());
		}

		bool init(){
			vulkan = mainApp->vulkan();device = vulkan->device(); swapchain = device->getSwapchain();vk_device = device->getDevice();
			_commandPool = device->getCommandPool();

			glm::ivec2 wndSize = mainApp->wndSize();

            _scene = std::make_shared<Scene>();
            _scene->load("assets/models/monkey/monkey");

            //spModel monkey = _scene->models()[0];
            //spMesh  mesh = monkey->mesh();
            _bvh.run(_scene);

			// Pack scene to VB and IB
			std::vector<sVertex>   vertexes;
			std::vector<uint32_t > indexes;
			uint ibOffset = 0;
			for(uint i = 0;i<_scene->models().size();++i){
				std::vector<sVertex>   meshVertexes = _scene->models()[i]->mesh()->vertexes();
				std::vector<uint32_t > meshIndexes  = _scene->models()[i]->mesh()->indexes();
				for(auto index  : meshIndexes)indexes.push_back(index+ibOffset);
				for(auto vertex : meshVertexes)vertexes.push_back(vertex);
				ibOffset += meshIndexes.size();
			}

			std::cout << "NumVertexes " << vertexes.size() << std::endl;
			std::cout << "NumIndexes " << indexes.size() << std::endl;

			_camera = std::make_shared<Camera>(vec3(0.0f, 0.0f, -15.0f),vec3(0.0f,0.0f,0.0f),vec3(0.0,-1.0f,0.0f));
			_camera->setProj(glm::radians(45.0f),(float)(wndSize.x)/(float)(wndSize.y),0.1f,10000.0f);

			updateCameraData();

            auto& nodes = _bvh.nodes();

			_cameraUniform.create(device,sizeof(ComputeCamera),&_cameraCompute,vk::BufferUsageFlagBits::eUniformBuffer);
			_bvhUniform.create(device,sizeof(BVHNode)*nodes.size(),nodes.data(),vk::BufferUsageFlagBits::eStorageBuffer);

            _vb.create(device,sizeof(sVertex)*vertexes.size(),vertexes.data(),vk::BufferUsageFlagBits::eStorageBuffer);
            _ib.create(device,sizeof(uint32_t)*indexes.size(),indexes.data(),vk::BufferUsageFlagBits::eStorageBuffer);


			// Create Compute Shader
			_surface = device->create<Image>();
			_surface->create(wndSize.x,wndSize.y,vk::Format::eR8G8B8A8Unorm,1,vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled);

            _defaultSampler = createSampler(vk_device,linearSampler());

            _computeDescSets.resize(4);
            _computeDescSets[0] = device->create<DescSet>(); // Descriptor set for result image and camera settings
            _computeDescSets[0]->setTexture(_surface->createImageView(),_defaultSampler,0,vk::ShaderStageFlagBits::eCompute,
                                        vk::DescriptorType::eStorageImage, vk::ImageLayout::eGeneral);
            _computeDescSets[0]->setUniformBuffer(_cameraUniform,1,vk::ShaderStageFlagBits::eCompute,vk::DescriptorType::eUniformBuffer);
            _computeDescSets[1] = device->create<DescSet>();
            _computeDescSets[1]->setUniformBuffer(_bvhUniform,0,vk::ShaderStageFlagBits::eCompute,vk::DescriptorType::eStorageBuffer);
            _computeDescSets[2] = device->create<DescSet>();
            _computeDescSets[2]->setUniformBuffer(_vb,0,vk::ShaderStageFlagBits::eCompute,vk::DescriptorType::eStorageBuffer);
            _computeDescSets[3] = device->create<DescSet>();
            _computeDescSets[3]->setUniformBuffer(_ib,0,vk::ShaderStageFlagBits::eCompute,vk::DescriptorType::eStorageBuffer);

            for(auto c : _computeDescSets)c->create();

            _quadDescSet = device->create<DescSet>();
            _quadDescSet->setTexture(_surface->createImageView(),_defaultSampler,0,vk::ShaderStageFlagBits::eFragment);
            _quadDescSet->create();

			_compute = std::make_shared<Compute>(device);
			_compute->create("assets/compute/main_comp.spv",_computeDescSets);
			_compute->dispatch(glm::ivec2((wndSize.x/COMPUTE_LOCAL_SIZE)+1,(wndSize.y/COMPUTE_LOCAL_SIZE)+1));

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

			auto surfaceBarrier = imageBarrier(_surface,AccessTransfer(vk::AccessFlagBits::eShaderWrite,vk::AccessFlagBits::eShaderRead),ImageLayoutTransfer(vk::ImageLayout::eGeneral));

			for(int i = 0;i<_commandBuffers.size();++i){
				// Fill Render passes
				vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

				_commandBuffers[i].begin(&beginInfo);

				_commandBuffers[i].pipelineBarrier(
						vk::PipelineStageFlagBits::eComputeShader,
						vk::PipelineStageFlagBits::eFragmentShader,
						vk::DependencyFlagBits::eViewLocalKHX,
						0, nullptr,
						0, nullptr,
						1, &surfaceBarrier
				);

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
			device->getGraphicsQueue().waitIdle();

			return true;
		}
		bool update(){
			auto prev_t = std::chrono::steady_clock::now();
			_compute->wait();
			_compute->run();
			float dt = std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::steady_clock::now() - prev).count();
			//std::cout << "FPS " << 1.0f/dt << std::endl;
			_cameraUniform.set(sizeof(ComputeCamera),&_cameraCompute);
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
			updateCameraData();
		}
		bool onScroll(const glm::vec2& offset){
			_camera->scale(offset.y,_dt);
			updateCameraData();
			return true;
		}
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
        std::vector<spDescSet> _computeDescSets;
        spDescSet _quadDescSet;
		spImage   _surface;

        spScene  _scene;
		spCamera _camera;
		// For camera rotate
		bool _isPressed;
		bool _isFirst;
		glm::vec2 _prev_mouse;
		float _dt;

        BVH      _bvh; // BVH for monkey
		Uniform  _bvhUniform;
		Uniform  _cameraUniform;
        Uniform  _vb;
        Uniform  _ib;
		Uniform  _mesh_data;
		ComputeCamera _cameraCompute;

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