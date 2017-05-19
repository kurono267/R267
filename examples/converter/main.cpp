#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/scene/mesh.hpp>

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

int main(){
	testMeshPackUnpack();
	return 0;
}