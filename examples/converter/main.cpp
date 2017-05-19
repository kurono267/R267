#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/scene/scene.hpp>
#include <boost/property_tree/json_parser.hpp>

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

		delete[] data;
	} catch(const std::exception& e){
		std::cout << e.what() << std::endl;
	}
}

void testMaterialRead(){
	try {
		spMaterial material = std::make_shared<Material>();
		// Create simple material
		material->setAlbedo(0.5f);
		material->setRoughness(0.4f);
		material->setDiffuseColor(glm::vec3(0.66f));
		material->setSpecularColor(glm::vec3(0.33f));
		ptree out;
		material->save(out,"Cube");
		json_parser::write_json("assets/models/test.json",out);

		ptree root;
		json_parser::read_json("assets/models/test.json",root);
		spMaterial other_material = std::make_shared<Material>();
		other_material->read(root,"Cube");

		if(material->equal(other_material))std::cout << "Material Save Load Successfull" << std::endl;
		else std::cout << "Material Save Load Failed" << std::endl;
	} catch(const std::exception& e){
		std::cout << e.what() << std::endl;
	}
}

void testSceneSaveLoad(){
	try {
		spScene scene = std::make_shared<Scene>();
		// Create simple model
		spMesh mesh = std::make_shared<Mesh>();
		// Create data for cube
		std::vector<sVertex> vdata = createCubeVB();
		std::vector<uint32_t> idata = createCubeIB();
		mesh->setData(vdata,idata,"Cube");
		// Create simple material
		spMaterial material = std::make_shared<Material>();
		// Create simple material
		material->setAlbedo(0.5f);
		material->setRoughness(0.4f);
		material->setDiffuseColor(glm::vec3(0.66f));
		material->setSpecularColor(glm::vec3(0.33f));
		spModel model = std::make_shared<Model>("Cube");
		model->setMesh(mesh);
		model->setMaterial(material);
		scene->add(model);
		scene->save("assets/models/scene");

		auto other_scene = std::make_shared<Scene>();
		other_scene->load("assets/models/scene");

		if(scene->equal(other_scene))std::cout << "Scene Save Load Successfull" << std::endl;
		else std::cout << "Scene Save Load Failed" << std::endl;
	} catch(const std::exception& e){
		std::cout << e.what() << std::endl;
	}
}

int main(){
	testMeshPackUnpack();
	testMaterialRead();
	testSceneSaveLoad();
	return 0;
}