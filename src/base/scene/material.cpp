#include "material.hpp"

using namespace boost::property_tree;
using namespace r267;

Material::Material(){}
Material::~Material(){}

// Read json material from filename
void Material::read(const std::string& filename,const std::string& object){
	ptree root;
	json_parser::read_json(filename,root);
	read(root.get_child(object));
}

void Material::read(const ptree& tree){
	_data.albedo = tree.get<float>("albedo",1.0f);
	_data.roughness = tree.get<float>("roughness",0.0f);
	auto diffColor = tree.get_child_optional("diffuseColor");
	auto specularColor = tree.get_child_optional("specularColor");
	_data.diffuseColor = glm::vec3(1.0f);
	_data.specularColor = glm::vec3(1.0f);
	uint i = 0;
	if(diffColor){
		for(auto& c : *diffColor){
			if(i >= 3)throw std::logic_error("Wrong array format for diffuseColor");
			_data.diffuseColor[i] = c.second.get_value<float>();
			++i;
		}
	}
	i = 0;
	if(specularColor){
		for(auto& c : *specularColor){
			if(i >= 3)throw std::logic_error("Wrong array format for specularColor");
			_data.specularColor[i] = c.second.get_value<float>();
			++i;
		}
	}
}

MaterialUBO Material::data(){
	return _data;
}

spImage Material::diffuseTexture(spDevice device){
	if(_diffuseFilename.empty())throw std::logic_error("Material::diffuseTexture Filename is empty");
	return loadImage(device,_diffuseFilename);
}

