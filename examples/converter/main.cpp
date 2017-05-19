#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/scene/mesh.hpp>
#include <base/scene/material.hpp>

using namespace r267;

void testMeshPackUnpack(){
	try {
		spMesh mesh = std::make_shared<Mesh>();
		// Create data for cube
		std::vector<sVertex> vdata = createCubeVB();
		std::vector<uint32_t> idata = createCubeIB();
		mesh->setData(vdata,idata,"Cube");

		// Pack data
		uint64_t size;
		uint8_t* data = mesh->packed(size);

		// Test unpack data
		spMesh other_mesh = std::make_shared<Mesh>();
		other_mesh->setData(data,size);

		if(mesh->equal(other_mesh))std::cout << "Mesh Pack Unpack Successfull" << std::endl;
		else std::cout << "Mesh Pack Unpack Failed" << std::endl;
	} catch(const std::exception& e){
		std::cout << e.what() << std::endl;
	}
}

void testMaterialRead(){
	try {
		spMaterial material = std::make_shared<Material>();
		material->read("assets/models/test.json","Cube");
		auto ubo = material->data();
		std::cout << "albedo " <<  ubo.albedo << std::endl;
		std::cout << "roughness " << ubo.roughness << std::endl;
		std::cout << "diffuseColor " << ubo.diffuseColor.x << "," << ubo.diffuseColor.y << "," << ubo.diffuseColor.z << std::endl; 
		std::cout << "specularColor " << ubo.specularColor.x << "," << ubo.specularColor.y << "," << ubo.specularColor.z << std::endl; 
	} catch(const std::exception& e){
		std::cout << e.what() << std::endl;
	}
}

int main(){
	testMeshPackUnpack();
	testMaterialRead();
	return 0;
}