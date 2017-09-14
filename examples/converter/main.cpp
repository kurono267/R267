#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>
#include <base/scene/scene.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags

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
		material->setDiffuseColor(glm::vec3(0.1f,0.2f,0.3f));
		material->setSpecularColor(glm::vec3(0.4f,0.5f,0.6f));
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

spScene assimp_import(const std::string& filename){
	// Assimp import
	Assimp::Importer importer;

	std::cout << filename << std::endl;

	const aiScene* scene = importer.ReadFile( filename,
											aiProcess_Triangulate | 
											aiProcess_GenSmoothNormals |
											aiProcess_CalcTangentSpace |
											aiProcess_SortByPType);

	if( !scene)
		throw std::runtime_error("Assimp!: import failed");

	std::cout << "NumMeshes " << scene->mNumMeshes << std::endl;

	if(!scene->HasMeshes())
		throw std::runtime_error("Assimp!: imported scene hasn't mashes");

	spScene result_scene = std::make_shared<Scene>();

	for(int i = 0;i<scene->mNumMeshes;++i){
		const aiMesh* i_mesh = scene->mMeshes[i];

		if(!i_mesh)continue;

		if(!i_mesh->HasPositions())continue;
		if(!i_mesh->HasFaces())continue;

		std::string name = i_mesh->mName.data;

		spMaterial s_material;

		if(scene->HasMaterials()){
			if(result_scene->materials().find(name) == result_scene->materials().end()){
				uint materialId = i_mesh->mMaterialIndex;
				aiMaterial* aMat = scene->mMaterials[materialId];
				if(aMat){
					aiColor3D aKd;aiColor3D aKs;

					aMat->Get(AI_MATKEY_COLOR_DIFFUSE,aKd);
					aMat->Get(AI_MATKEY_COLOR_SPECULAR,aKs);

					float shininess;
					aMat->Get(AI_MATKEY_SHININESS,shininess);

					aiString map_Kd;
					aMat->GetTexture(aiTextureType_DIFFUSE,0,&map_Kd);
					aiString map_Bump;
					aMat->GetTexture(aiTextureType_NORMALS,0,&map_Bump);
					std::cout << "Normals " << map_Bump.data << std::endl;
					aiString map_Height;
					aMat->GetTexture(aiTextureType_HEIGHT,0,&map_Height);
					std::cout << "Normals " << map_Height.data << std::endl;

					s_material = std::make_shared<Material>();
					s_material->setDiffuseColor(glm::vec3(aKd.r,aKd.g,aKd.b));
					s_material->setSpecularColor(glm::vec3(aKs.r,aKs.g,aKs.b));
					s_material->setDiffuseTexture(map_Kd.data);
					s_material->setNormalTexture(map_Bump.data);
					s_material->setHeightmapTexture(map_Height.data);
					s_material->setRoughness(1.0f-shininess);
					result_scene->materials().insert(std::pair<std::string,spMaterial>(name,s_material));
				}
			} else {
				s_material = result_scene->materials()[name];
			}
		}

		std::vector<sVertex>  vertexes;
		std::vector<uint32_t> indexes;

		for(int f = 0;f<i_mesh->mNumFaces;++f){
			const aiFace i_face = i_mesh->mFaces[f];

			for(int v = 0;v<3;++v){
				aiVector3D i_pos;
				if(i_face.mIndices[v] >= i_mesh->mNumVertices){
					break;
				}
				i_pos = i_mesh->mVertices[i_face.mIndices[v]];

				sVertex vertex;
				vertex.pos = glm::vec3(i_pos.x,i_pos.y,i_pos.z);
				if(i_mesh->mNormals != nullptr){
					const aiVector3D i_no = i_mesh->mNormals[i_face.mIndices[v]];
					vertex.normal  = glm::vec3(i_no.x,i_no.y,i_no.z);
				}
				if(i_mesh->mTextureCoords[0] != nullptr){
					const aiVector3D i_uv = i_mesh->mTextureCoords[0][i_face.mIndices[v]];
					vertex.uv = glm::vec2(i_uv.x,i_uv.y);
				}
				if(i_mesh->mTangents != nullptr){
					const aiVector3D i_tangents = i_mesh->mTangents[i_face.mIndices[v]];
					vertex.tangent = glm::vec3(i_tangents.x,i_tangents.y,i_tangents.z);
				}
				if(i_mesh->mBitangents != nullptr){
					const aiVector3D i_bitangents = i_mesh->mBitangents[i_face.mIndices[v]];
					vertex.binormal = glm::vec3(i_bitangents.x,i_bitangents.y,i_bitangents.z);
				}
				vertexes.push_back(vertex);
				indexes.push_back(vertexes.size()-1);
			}
		}

		spMesh s_mesh = std::make_shared<Mesh>();
		s_mesh->setData(vertexes,indexes,name);

		spModel model = std::make_shared<Model>(name);
		model->setMesh(s_mesh);
		model->setMaterial(s_material);

		result_scene->add(model);
	}
	importer.FreeScene();
	return result_scene;
}

// TODO convert objects to const
// TODO Add materials
#if 0
void assimp_export(spScene scene,const std::string& filename){
	Assimp::Exporter exporter;
	int formats = exporter.GetExportFormatCount();
	
	size_t pos = filename.find_first_of(".");
	std::string res = filename.substr(pos+1,filename.size()-pos-1);
	std::string fileId;
	for(int i = 0;i<formats;++i){
		if(res == exporter.GetExportFormatDescription(i)->fileExtension){
			fileId = exporter.GetExportFormatDescription(i)->id;
			break;
		}
	}

	if(fileId.empty()){
		std::stringstream error_message;
		error_message << "This resolution " << res << "not supported for export";
		throw std::runtime_error(error_message.str());
	}

	const auto models = scene->models();

	aiScene* e_scene = new aiScene();

	e_scene->mMaterials = new aiMaterial*[ 1 ];
	e_scene->mNumMaterials = 1;

	e_scene->mMaterials[ 0 ] = new aiMaterial(); 

	e_scene->mRootNode = new aiNode();
	e_scene->mRootNode->mMeshes = new unsigned int[models.size()];
	e_scene->mRootNode->mNumMeshes = models.size();

	e_scene->mMeshes = new aiMesh*[models.size()];
	e_scene->mNumMeshes = models.size();
	for(int i = 0;i<mesh.size();++i){
		e_scene->mRootNode->mMeshes[i] = i;
		aiMesh* e_mesh = new aiMesh();

		const auto& vertexes = models[i]->mesh()->vertexes();
		const auto& indexes = models[i]->mesh()->indexes();

		// Compute size of model
		Index e_size = vertexes.size();

		e_mesh->mVertices = new aiVector3D[e_size];
		e_mesh->mNormals  = new aiVector3D[e_size];
		e_mesh->mTextureCoords[0] = new aiVector3D[e_size];
		e_mesh->mNumUVComponents[0] = 2;

		e_mesh->mNumVertices = e_size;

		const size_t faces_num = indexes.size()/3;

		e_mesh->mFaces = new aiFace[faces_num];
		e_mesh->mNumFaces = faces_num;
		for(int f = 0;f<faces_num;++f){
			aiFace e_face;
			e_face.mIndices = new unsigned int[3];
			e_face.mNumIndices = 3;

			for(int v = 0;v<3;++v){
				size_t index_id = f*3+v;
				if(index_id >= indexes.size())break;
				uint32_t index = indexes[f*3+v];

				e_face.mIndices[v] = index;

				sVertex vertex = vertexes[index];

				e_mesh->mVertices[index] = aiVector3D(vertex.pos.x,vertex.pos.y,vertex.pos.z);
				e_mesh->mNormals[index] = aiVector3D(vertex.no.x,vertex.no.y,vertex.no.z);
				e_mesh->mTextureCoords[0][index] = aiVector3D(vertex.uv.x,vertex.uv.y,0.0f);
			}

			e_mesh->mFaces[f] = aiFace(e_face);
		}

		e_scene->mMeshes[i] = e_mesh;
	}

	if(exporter.Export(e_scene,fileId,filename,aiProcess_JoinIdenticalVertices) == AI_SUCCESS)return true;
	return false;
}
#endif

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


int main(int argc, char const *argv[]) {
	// Basic tests
	testMeshPackUnpack();
	testMaterialRead();
	testSceneSaveLoad();
	if(argc < 3){
		std::cout << "Wrong arguments number" << std::endl;
		std::cout << "converter input output" << std::endl;
		return 0;
	}
	std::string input = argv[1];
	std::string output = argv[2];
	spScene scene = assimp_import(input);
	scene->save(output);
	return 0;
}