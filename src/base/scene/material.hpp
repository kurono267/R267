#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include "base/vk/additional.hpp"

#include <unordered_map>

using namespace boost::property_tree;
// Our simple material
namespace r267 {

struct MaterialUBO {
	// Material data
	float       albedo;
	float       roughness;
	glm::vec3   diffuseColor;
	glm::vec3   specularColor;
};

class Material {
	public:
		Material();
		virtual ~Material();

		void setPath(const std::string& path);
		
		void read(const ptree& root,const std::string& object); // Read json material from filename

		void save(ptree& root,const std::string& object);

		void setAlbedo(const float& albedo);
		void setRoughness(const float& roughness);
		void setDiffuseColor(const glm::vec3& color);
		void setSpecularColor(const glm::vec3& color);

		void setDiffuseTexture(const std::string& filename);

		bool equal(const std::shared_ptr<Material>& material);

		MaterialUBO data();

		void        create(spDevice device,std::unordered_map<std::string,spImage>& imagesBuffer);

		spDescSet   getDescSet();
	protected:
		void read(const ptree& tree);           // Read material from ptree
		void save(ptree& tree);
		// Material data
		MaterialUBO _data;
		// Material Texture
		std::string _diffuseFilename;

		Uniform       _uniform;
		spImage       _diffTexture;
		vk::ImageView _diffView;
		vk::Sampler   _sampler;
		spDescSet     _descSet;

		std::string   _path;
};

typedef std::shared_ptr<Material> spMaterial;

} // r267