#include "scene.hpp"

using namespace r267;

namespace fs = std::experimental::filesystem;

Scene::Scene(){}
Scene::~Scene(){}

void Scene::add(const spModel& model){
	_models.push_back(model);
}

void Scene::load(const std::string& filename){
	// Read binary meshes
	// Check filesize
	std::string binFilename = filename + "." + binExt;
	fs::path path = fs::canonical(binFilename);

    uint64_t size = fs::file_size(path);
    // Create main file buffer
    uint8_t* data = new uint8_t[size];

    const size_t ui32 = sizeof(uint32_t);

    std::ifstream file(binFilename,std::ios::in | std::ios::binary);
    file.read((char*)data,size);
    file.close();

    uint32_t numMeshes = 0;

    size_t offset = 0;
    uint32_t tmp;
	std::memcpy(&tmp,data+offset,ui32); offset += ui32;
	if(tmp != format::R267Magic)throw std::logic_error("Wrong magic");
	std::memcpy(&numMeshes,data+offset,ui32); offset += ui32;

	for(uint32_t m = 0;m<numMeshes;++m){
		std::memcpy(&tmp,data+offset,ui32); offset += ui32;
		if(tmp != format::R267Mesh)throw std::logic_error("Wrong mesh");

		uint64_t pack_size;
		std::memcpy(&pack_size,data+offset,sizeof(uint64_t)); offset += sizeof(uint64_t);
		
		spMesh mesh = std::make_shared<Mesh>();
		mesh->setData(data+offset,pack_size);
		offset += pack_size;

		spModel model = std::make_shared<Model>(mesh->name());
		model->setMesh(mesh);
		_models.push_back(model);
	}
	if(offset != size){
		throw std::logic_error("Checksum failed");
	}
	delete[] data;

	ptree root;
	json_parser::read_json(filename + "." + mtlExt,root);
	// Read material
	for(auto m : _models){
		spMaterial material = std::make_shared<Material>();
		material->read(root,m->name());
		m->setMaterial(material);
	}
}

void Scene::save(const std::string& filename){
	// Save meshes
	// Packed meshes
	std::vector<std::pair<uint64_t,uint8_t*> > packed_models;
	uint64_t size = 0;
	const size_t ui32 = sizeof(uint32_t);
	// Format description size
	size += 2*ui32;
	for(auto m : _models){
		size += ui32;
		size += sizeof(uint64_t);
		std::pair<uint64_t,uint8_t*> pack;
		pack.second = m->mesh()->packed(pack.first);
		packed_models.push_back(pack);
		size += pack.first;
	}
	// Create file data
	uint8_t* data = new uint8_t[size];
	size_t offset = 0;
	uint32_t tmp = format::R267Magic;
	std::memcpy(data+offset,&tmp,ui32); offset += ui32;
	tmp = packed_models.size();
	std::memcpy(data+offset,&tmp,ui32); offset += ui32;

	for(auto &pack : packed_models){
		tmp = format::R267Mesh;

		std::memcpy(data+offset,&tmp,ui32); offset += ui32;
		std::memcpy(data+offset,&pack.first,sizeof(uint64_t)); offset += sizeof(uint64_t);
		std::memcpy(data+offset,pack.second,pack.first); offset += pack.first;
		delete[] pack.second;
	}

	if(offset != size){
		throw std::logic_error("Checksum failed");
	}

	std::ofstream file (filename + "." + binExt, std::ios::out | std::ios::binary);
    file.write ((const char*)data, size);
    file.close();

    delete[] data;

    // Save materials
    ptree root;
    for(auto m : _models){
    	m->material()->save(root,m->name());
    }
    json_parser::write_json(filename + "." + mtlExt,root);
}

std::vector<spModel> Scene::models(){
	return _models;
}

bool Scene::equal(const std::shared_ptr<Scene>& scene){
	if(_models.size() != scene->_models.size())return false;
	for(int i = 0;i<_models.size();++i){
		auto m = _models[i];
		if(!m->equal(scene->_models[i]))return false;
	}
	return true;
}
