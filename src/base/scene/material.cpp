#include "material.hpp"

using namespace boost::property_tree;
using namespace r267;

Material::Material(){
	_data.albedo = 1.0f; 
	_data.roughness = 0.0f;
	_data.diffuseColor = glm::vec3(1.0f);
	_data.specularColor = glm::vec3(1.0f);
}
Material::~Material(){}

// Read json material from filename
void Material::read(const ptree& root,const std::string& object){
	//ptree root;
	//json_parser::read_json(filename,root);
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
	_diffuseFilename = tree.get<std::string>("diffuseTexture","");
}

void Material::save(ptree& root,const std::string& object){
	ptree material;
	save(material);
	root.add_child(object,material);
}

void Material::save(ptree& tree){
	tree.put<float>("albedo",_data.albedo);
	tree.put<float>("roughness",_data.roughness);
	ptree diffColor;
	for(int v = 0;v<3;++v){
		ptree value;
		value.put<float>("",_data.diffuseColor[v]);
		diffColor.push_back(std::make_pair("",value));
	}
	tree.add_child("diffuseColor",diffColor);
	ptree specularColor;
	for(int v = 0;v<3;++v){
		ptree value;
		value.put<float>("",_data.specularColor[v]);
		specularColor.push_back(std::make_pair("",value));
	}
	tree.add_child("specularColor",specularColor);
	tree.put("diffuseTexture",_diffuseFilename);
}

MaterialUBO Material::data(){
	return _data;
}

void Material::setPath(const std::string& path){
	_path = path;
}

void Material::setAlbedo(const float& albedo){
	_data.albedo = albedo;
}

void Material::setRoughness(const float& roughness){
	_data.roughness = roughness;
}

void Material::setDiffuseColor(const glm::vec3& color){
	_data.diffuseColor = color;
}

void Material::setSpecularColor(const glm::vec3& color){
	_data.specularColor = color;
}

void Material::setDiffuseTexture(const std::string& filename){
	_diffuseFilename = filename;
}

bool Material::equal(const std::shared_ptr<Material>& material){
	if(_data.albedo != material->_data.albedo)return false;
	if(_data.roughness != material->_data.roughness)return false;
	if(_data.diffuseColor != material->_data.diffuseColor)return false;
	if(_data.specularColor != material->_data.specularColor)return false;
	if(_diffuseFilename != material->_diffuseFilename)return false;
	return true;
}

void Material::create(spDevice device,std::unordered_map<std::string,spImage>& imagesBuffer){
	_uniform.create(device,sizeof(MaterialUBO),&_data);
	std::string filename = _path+_diffuseFilename;
	auto tmp = imagesBuffer.find(filename);
	if(tmp != imagesBuffer.end()){
		_diffTexture = tmp->second;
	} else {
		if(!_diffuseFilename.empty())_diffTexture = loadImage(device,_path+_diffuseFilename);
		else _diffTexture = whiteTexture(device,1,1); 
		imagesBuffer.insert(std::pair<std::string,spImage>(filename,_diffTexture));
	}

	_diffView = _diffTexture->createImageView();
	_sampler  = createSampler(device->getDevice(),linearSampler());

	_descSet  = device->create<DescSet>();
	_descSet->setUniformBuffer(_uniform,0,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(_diffView,_sampler,1,vk::ShaderStageFlagBits::eFragment);
	_descSet->create();
}

spDescSet Material::getDescSet(){
	return _descSet;
}

