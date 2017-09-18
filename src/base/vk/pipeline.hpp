#pragma once 

#include <base/default.hpp>
#include "swapchain.hpp"
#include "uniform.hpp"
#include "framebuffer.hpp"

#include <unordered_map>

namespace r267 {

// Pipeline pattern
// By default scissor is dynamic, if that doesn't set
class RenderPattern {
	friend class Pipeline;
	public:
		struct Attachment {
			vk::AttachmentDescription desc;
			vk::AttachmentReference   ref;
		};

		RenderPattern();
		RenderPattern(const RenderPattern& r);
		~RenderPattern();

		void inputAssembly(vk::PrimitiveTopology topology);
		void viewport(const float& x,const float& y,const float& width,const float& height,const float& minDepth = 0.0f,const float& maxDepth = 1.0f);

		void scissor(const glm::ivec2& offset,const glm::ivec2& size);
		void scissor(const vk::Offset2D& offset,const vk::Extent2D& extent);

		void dynamicScissor();
		void dynamicViewport();

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

		void blend(const uint& attachments = 1,const bool& enable = true,const vk::ColorComponentFlags& writeMask = RGBA,
					const vk::BlendFactor& srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,const vk::BlendFactor& dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
					const vk::BlendOp& colorBlendOp = vk::BlendOp::eAdd,
					const vk::BlendFactor& srcAlphaBlendFactor = vk::BlendFactor::eOne,const vk::BlendFactor& dstAlphaBlendFactor = vk::BlendFactor::eZero,
					const vk::BlendOp& alphaBlendOp = vk::BlendOp::eAdd);
		void depth(const bool& enable = true, const bool& write = true,const vk::CompareOp& comp = vk::CompareOp::eLess);
		void constants(const size_t& offset, const size_t& size,const vk::ShaderStageFlagBits& stage);

		void createRenderPass(const vk::Format& swapchainFormat,const vk::Format& depthFormat,const uint& attachments = 1);
		void createRenderPass(const std::vector<Attachment>& attachments,const Attachment& depth);

		static RenderPattern basic(const spDevice& device){
			// Create Basic Render Pattern
			RenderPattern pattern;
			pattern.inputAssembly(vk::PrimitiveTopology::eTriangleList);
			auto extent = device->getSwapchain()->getExtent();
			pattern.viewport(0,0,extent.width,extent.height);
			pattern.scissor(vk::Offset2D(),extent);
			pattern.rasterizer();
			pattern.multisampling();
			pattern.blend();
			pattern.depth();

			pattern.createRenderPass(device->getSwapchain()->getFormat(),device->depthFormat());
			return pattern;
		}
	private:
		vk::PipelineInputAssemblyStateCreateInfo _assembly;
		vk::Viewport                             _viewport;
		vk::Rect2D                               _scissor;
		vk::PipelineRasterizationStateCreateInfo _rasterizer;
		vk::PipelineMultisampleStateCreateInfo   _multisampling;
		std::vector<vk::PipelineColorBlendAttachmentState> _blendAttachments;
		vk::PipelineColorBlendStateCreateInfo    _blend;
		vk::PipelineDepthStencilStateCreateInfo  _depthStencil;

		bool _dynamicScissor;
		bool _dynamicViewport;

		std::vector<vk::AttachmentDescription>   _attachmentsDesc;
		std::vector<vk::AttachmentReference>     _attachmentsRef;
		vk::SubpassDescription    _subPass;
		vk::SubpassDependency     _subPassDep[2];
		vk::RenderPassCreateInfo  _renderPassInfo;

		std::vector<vk::PushConstantRange> _pushConsts;
};
RenderPattern::Attachment createAttachment(const vk::Format& format, const bool& depth, const int index);

class DescSet {
	public:
		DescSet(spDevice device,vk::Queue queue,vk::CommandPool pool);
		virtual ~DescSet();

		void create();

		void setUniformBuffer(const Uniform& buffer,const size_t& binding,const vk::ShaderStageFlags& stage,
							  const vk::DescriptorType& descType = vk::DescriptorType::eUniformBuffer);
		void setTexture(const vk::ImageView& imageView,const vk::Sampler& sampler, const size_t& binding, const vk::ShaderStageFlags& stage,
						const vk::DescriptorType& descType = vk::DescriptorType::eCombinedImageSampler,
						const vk::ImageLayout& imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal);

		void release(spDevice device);

		vk::DescriptorSet getDescriptorSet(){return _descSet;}
		vk::DescriptorSetLayout getLayout(){return _descLayout;}
	protected:
		spDevice _device;

		vk::DescriptorSetLayout _descLayout;
		vk::DescriptorSet       _descSet;
		vk::DescriptorPool      _descPool;

		std::vector<vk::DescriptorImageInfo>  _descImageInfos;
		std::vector<vk::DescriptorBufferInfo> _descBufferInfos;

		struct UBOBinding {
			Uniform buffer;
			size_t    binding;
			vk::ShaderStageFlags stage;
			vk::DescriptorType   descType;
		};
		struct SamplerBinding {
			vk::ImageView imageView;
			vk::Sampler   sampler;
			size_t    binding;
			vk::ShaderStageFlags stage;
			vk::DescriptorType   descType;
			vk::ImageLayout      layout;
		};
		std::vector<UBOBinding>     _uboBinds;
		std::vector<SamplerBinding> _samplerBinds;
};

typedef std::shared_ptr<DescSet> spDescSet;

class Pipeline {
	public:
		Pipeline(const RenderPattern& rp,vk::Device device) : _device(device),_renderpattern(rp) {}
		~Pipeline(){}

		void addShader(const vk::ShaderStageFlagBits& type,const std::string& filename);

		void create(const vk::VertexInputBindingDescription& vertexBinding = sVertex::bindingDesc(), const std::vector<vk::VertexInputAttributeDescription>& vertexAttrib = sVertex::attributes());
		void release();

		void descSets(const std::vector<spDescSet>& descSets);
		void descSet(const spDescSet& descSet);

		vk::RenderPass getRenderPass();

		operator vk::Pipeline(){return _pipeline;}
		vk::PipelineLayout      getPipelineLayout(){return _pLayout;}
	protected:
		RenderPattern _renderpattern;

		vk::Device _device; 

		std::vector<vk::PipelineShaderStageCreateInfo> _shaders;
		vk::PipelineViewportStateCreateInfo      _viewportState;

		std::vector<vk::DescriptorSetLayout> _descLayouts;
		vk::PipelineLayoutCreateInfo _pipelineLayoutInfo;

		vk::PushConstantRange _pushConstRange;

		vk::RenderPass              _renderPass;
		vk::Pipeline                _pipeline;
		vk::PipelineLayout          _pLayout;

		std::vector<vk::ShaderModule> _shaderModules;
};

typedef std::shared_ptr<Pipeline> spPipeline;

};


