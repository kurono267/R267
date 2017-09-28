//
// Created by kurono267 on 17.09.17.
//

#include "differed.hpp"

using namespace r267;

void Differed::init(spDevice device,soCamera camera,GBuffer& gbuffer,IBL& ibl){
	auto vk_device = device->getDevice();

	auto baseRP = RenderPattern::basic(device);
    baseRP.blend(1,false);
    baseRP.depth(false,false);
    _pipeline = std::make_shared<Pipeline>(baseRP,vk_device);

    _pipeline->addShader(vk::ShaderStageFlagBits::eVertex,"../shaders/effects/differed_vert.spv");
    _pipeline->addShader(vk::ShaderStageFlagBits::eFragment,"../shaders/effects/differed_frag.spv");

	_ubo.view = glm::vec4(camera->getPos(),1.0f);
    _ubo.viewMat = camera->getView();
    _ubo.proj = camera->getProj();
    _ubo.invview  = inverse(camera->getView());
    _ubo.invproj  = inverse(camera->getProj());
    _uniform.create(device,sizeof(UBO),&_ubo);

    auto sampler = createSampler(vk_device,linearSampler());

	_descSet = device->create<DescSet>();
	_descSet->setTexture(gbuffer.posMap(),sampler,0,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(gbuffer.normalMap(),sampler,1,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(gbuffer.colorMap(),sampler,2,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(ibl.cubemap(),ibl.cubemapSampler(),3,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(ibl.irradiance(),sampler,4,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(ibl.brdf(),sampler,5,vk::ShaderStageFlagBits::eFragment);
	_descSet->setUniformBuffer(_uniform,7,vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eVertex);
	_descSet->create();

	_pipeline->descSet(_descSet);
    _pipeline->create();

	_quad = std::make_shared<Quad>();
	_quad->create(device);
}

vk::ImageView Differed::result(){

}

vk::Semaphore Differed::run(vk::Semaphore wait){

}