#include "model.hpp"

using namespace r267;

Model::Model(const std::string& name) : _name(name){}
Model::~Model(){}

spMaterial  Model::material(){
	return _material;
}
spMesh      Model::mesh(){
	return _mesh;
}
std::string Model::name(){
	return _name;
}

void        Model::setMaterial(const spMaterial& material){
	_material = material;
}
void        Model::setMesh(const spMesh& mesh){
	_mesh     = mesh;
}

bool Model::equal(const std::shared_ptr<Model>& model){
	if(!_mesh->equal(model->_mesh))return false;
	if(!_material->equal(model->_material))return false;
	return true;
}
