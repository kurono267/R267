#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include "base/vk/additional.hpp"

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
		
		void read(const ptree& root,const std::string& object); // Read json material from filename

		void save(ptree& root,const std::string& object);

		void setAlbedo(const float& albedo);
		void setRoughness(const float& roughness);
		void setDiffuseColor(const glm::vec3& color);
		void setSpecularColor(const glm::vec3& color);

		void setDiffuseTexture(const std::string& filename);

		bool equal(const std::shared_ptr<Material>& material);

		MaterialUBO data();
		spImage     diffuseTexture(spDevice device);
	protected:
		void read(const ptree& tree);           // Read material from ptree
		void save(ptree& tree);
		// Material data
		MaterialUBO _data;
		// Material Texture
		std::string _diffuseFilename;
};

typedef std::shared_ptr<Material> spMaterial;

} // r267