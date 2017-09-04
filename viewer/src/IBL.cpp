#include "IBL.hpp"

void IBL::init(spDevice device,spImage image){

}

vk::ImageView IBL::irradiance(){
	return _irrImage->ImageView();
}

void IBL::irradianceCompute(spDevice device,spImage image){


	_irrCompute = std::make_shared<Compute>(device);
	_irrCompute->create("assets/effects/IBL/irradiance.comp",_irrDescSets);
}