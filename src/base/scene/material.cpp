#include "material.hpp"

using namespace boost::property_tree;
using namespace r267;

Material::Material(){
	_data.diffuseColor = glm::vec4(1.0f);
	_data.data         = glm::vec4(DefaultSurface, 0.5f, 0.5f, 0.0f);
}
Material::~Material(){}

// Read json material from filename
void Material::read(const ptree& root,const std::string& object){
	//ptree root;
	//json_parser::read_json(filename,root);
	read(root.get_child(object));
}

void Material::read(const ptree& tree){
	float albedo = tree.get<float>("albedo",1.0f);
	float roughness = tree.get<float>("roughness",0.0f);
	auto diffColor = tree.get_child_optional("diffuseColor");
	auto specularColor = tree.get_child_optional("specularColor");

	_data.data.y = roughness;
	_data.data.z = albedo;

	_data.diffuseColor = glm::vec4(1.0f,1.0f,1.0f,1.0f);

	uint i = 0;
	if(diffColor){
		for(auto& c : *diffColor){
			if(i >= 3)throw std::logic_error("Wrong array format for diffuseColor");
			_data.diffuseColor[i] = c.second.get_value<float>();
			++i;
		}
	}
	i = 0;

	//std::cout << _data.diffuseColor.x << ", " << _data.diffuseColor.y << ", " << _data.diffuseColor.z << std::endl;
	//std::cout << _data.specularColor.x << ", " << _data.specularColor.y << ", " << _data.specularColor.z << std::endl;

	_diffuseFilename = tree.get<std::string>("diffuseTexture","");
	_normalFilename  = tree.get<std::string>("normalTexture","");
	_heightmapFilename = tree.get<std::string>("heightmapTexture","");
}

void Material::save(ptree& root,const std::string& object){
	ptree material;
	save(material);
	root.add_child(object,material);
}

void Material::save(ptree& tree){
	tree.put<float>("albedo",getMetallic());
	tree.put<float>("roughness",getRoughness());
	ptree diffColor;
	for(int v = 0;v<3;++v){
		ptree value;
		value.put<float>("",_data.diffuseColor[v]);
		diffColor.push_back(std::make_pair("",value));
	}
	tree.add_child("diffuseColor",diffColor);
	tree.put("diffuseTexture",_diffuseFilename);
	tree.put("normalTexture",_normalFilename);
	tree.put("heightmapTexture",_heightmapFilename);
}

MaterialUBO Material::data(){
	return _data;
}

void Material::setPath(const std::string& path){
	_path = path;
}

void Material::setMetallic(const float& metallic){
	_data.data.z = metallic;
	if(_uniform)_uniform.set(sizeof(MaterialUBO),&_data);
}

void Material::setRoughness(const float& roughness){
	_data.data.y = roughness;
	if(_uniform)_uniform.set(sizeof(MaterialUBO),&_data);
}

void Material::setDiffuseColor(const glm::vec3& color){
	float a = _data.diffuseColor.w;
	_data.diffuseColor = glm::vec4(color,a);
	if(_uniform)_uniform.set(sizeof(MaterialUBO),&_data);
}

float Material::getMetallic(){
	return _data.data.z;
}

float Material::getRoughness(){
	return _data.data.y;
}

glm::vec3 Material::getDiffuseColor(){
	return glm::vec3(_data.diffuseColor);
}

void Material::setDiffuseTexture(const std::string& filename){
	_diffuseFilename = filename;
}

void Material::setNormalTexture(const std::string& filename){
	_normalFilename = filename;
}

bool Material::equal(const std::shared_ptr<Material>& material){
	if(_data.diffuseColor != material->_data.diffuseColor)return false;
	if(_diffuseFilename != material->_diffuseFilename)return false;
	return true;
}

void Material::create(spDevice device,std::unordered_map<std::string,spImage>& imagesBuffer){
	//std::cout << sizeof(MaterialUBO) << std::endl;
	float type = DefaultSurface;
	if(!_normalFilename.empty()){
		if(!_heightmapFilename.empty())type = ParallaxSurface;
		else type = NormalSurface;
	}
	_data.data.x = type;

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
	filename = _path+_normalFilename;
	tmp = imagesBuffer.find(filename);
	if(tmp != imagesBuffer.end()){
		_normalTexture = tmp->second;
	} else {
		if(!_normalFilename.empty())_normalTexture = loadImage(device,filename);
		else _normalTexture = whiteTexture(device,1,1);
		imagesBuffer.insert(std::pair<std::string,spImage>(filename,_normalTexture));
	}
	filename = _path+_heightmapFilename;
	tmp = imagesBuffer.find(filename);
	if(tmp != imagesBuffer.end()){
		_heightmapTexture = tmp->second;
	} else {
		if(!_heightmapFilename.empty())_heightmapTexture = loadImage(device,filename);
		else _heightmapTexture = whiteTexture(device,1,1);
		imagesBuffer.insert(std::pair<std::string,spImage>(filename,_heightmapTexture));
	}

	_diffView = _diffTexture->ImageView();
	auto diffSamp  = createSampler(device->getDevice(),anisoSampler(_diffTexture->mipLevels())); // Color sampler with anisotropic filter

	_descSet  = device->create<DescSet>();
	_descSet->setUniformBuffer(_uniform,0,vk::ShaderStageFlagBits::eFragment);
	_descSet->setTexture(_diffView,diffSamp,1,vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eTessellationEvaluation);
	_descSet->setTexture(_normalTexture->ImageView(),_normalTexture->linearSampler(),2,vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eTessellationEvaluation);
	_descSet->setTexture(_heightmapTexture->ImageView(),_heightmapTexture->linearSampler(),3,vk::ShaderStageFlagBits::eFragment|vk::ShaderStageFlagBits::eTessellationEvaluation);
	_descSet->create();
}

void Material::setHeightmapTexture(const std::string& filename){
	_heightmapFilename = filename;
}

spDescSet Material::getDescSet(){
	return _descSet;
}

