#pragma once 

#include <base/default.hpp>
#include "swapchain.hpp"
#include "uniform.hpp"

namespace r267 {

// Pipeline pattern
class RenderPattern {
	friend class Pipeline;
	public:
		RenderPattern();
		RenderPattern(const RenderPattern& r);
		~RenderPattern();

		void inputAssembly(vk::PrimitiveTopology topology);
		void viewport(const float& x,const float& y,const float& width,const float& height,const float& minDepth = 0.0f,const float& maxDepth = 1.0f);

		void scissor(const glm::ivec2& offset,const glm::ivec2& size);
		void scissor(const vk::Offset2D& offset,const vk::Extent2D& extent);

		void rasterizer(const vk::PolygonMode& pmode = vk::PolygonMode::eFill,
						const vk::CullModeFlagBits& cmode = vk::CullModeFlagBits::eBack,
						const vk::FrontFace& face = vk::FrontFace::eClockwise,
						const float& lineWidth = 1.0f,
						const bool& depthClamp = false,
						const bool& discard = false,
						const bool& depthBias = false,const glm::vec3& depthBiasFactor = glm::vec3(0.0f)); // depthBiasFactor x - depthBiasConstantFactor, y - depthBiasClamp, z - depthBiasSlopeFactor

		void multisampling(const vk::SampleCountFlagBits& count = vk::SampleCountFlagBits::e1,
						   const bool& sampleShading = false,
						   const float& minSampleShading = 1.0f,
						   const bool& alphaToCoverageEnable = false,
						   const bool& alphaToOneEnable = false,
						   const vk::SampleMask* sampleMask = nullptr);

		void blend(const bool& enable = true,const vk::ColorComponentFlags& writeMask = RGBA,
					const vk::BlendFactor& srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,const vk::BlendFactor& dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
					const vk::BlendOp& colorBlendOp = vk::BlendOp::eAdd,
					const vk::BlendFactor& srcAlphaBlendFactor = vk::BlendFactor::eOne,const vk::BlendFactor& dstAlphaBlendFactor = vk::BlendFactor::eZero,
					const vk::BlendOp& alphaBlendOp = vk::BlendOp::eAdd);

		void createRenderPass(const vk::Format& swapchainFormat);

		static RenderPattern&& basic(const spSwapchain& swapchain){
			// Create Basic Render Pattern
			RenderPattern pattern;
			pattern.inputAssembly(vk::PrimitiveTopology::eTriangleList);
			auto extent = swapchain->getExtent();
			pattern.viewport(0,0,extent.width,extent.height);
			pattern.scissor(vk::Offset2D(),extent);
			pattern.rasterizer();
			pattern.multisampling();
			pattern.blend();

			pattern.createRenderPass(swapchain->getFormat());
			return std::move(pattern);
		}
	private:
		vk::PipelineInputAssemblyStateCreateInfo _assembly;
		vk::Viewport                             _viewport;
		vk::Rect2D                               _scissor;
		vk::PipelineRasterizationStateCreateInfo _rasterizer;
		vk::PipelineMultisampleStateCreateInfo   _multisampling;
		vk::PipelineColorBlendAttachmentState    _blendAttacment;
		vk::PipelineColorBlendStateCreateInfo    _blend;
		
		vk::AttachmentDescription _colorAttachment;
		vk::AttachmentReference   _colorAttachmentRef;
		vk::SubpassDescription    _subPass;
		vk::RenderPassCreateInfo  _renderPassInfo;
};

class Pipeline {
	public:
		Pipeline(const RenderPattern& rp,vk::Device device) : _device(device),_renderpattern(rp) {}
		~Pipeline(){}

		void setUniformBuffer(const Uniform& buffer,const size_t& binding,const vk::ShaderStageFlags& stage);
		void addShader(const vk::ShaderStageFlagBits& type,const std::string& filename);

		void create();

		vk::RenderPass getRenderPass();

		vk::DescriptorSet getDescriptorSet(){return _descSet;}
		vk::PipelineLayout      getPipelineLayout(){return _pLayout;}

		operator vk::Pipeline(){return _pipeline;}
	protected:
		RenderPattern _renderpattern;

		vk::Device _device; 

		std::vector<vk::PipelineShaderStageCreateInfo> _shaders;
		vk::DescriptorSetLayout _uboLayout;
		vk::DescriptorSet       _descSet;
		vk::PipelineViewportStateCreateInfo      _viewportState;

		vk::RenderPass              _renderPass;
		vk::Pipeline                _pipeline;
		vk::PipelineLayout          _pLayout;
};

typedef std::shared_ptr<Pipeline> spPipeline;

};


