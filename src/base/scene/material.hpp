#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
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
		
		void read(const std::string& filename,const std::string& object); // Read json material from filename
		void read(const ptree& tree);           // Read material from ptree

		MaterialUBO data();
		spImage     diffuseTexture(spDevice device);
	protected:
		// Material data
		MaterialUBO _data;
		// Material Texture
		std::string _diffuseFilename;
};

typedef std::shared_ptr<Material> spMaterial;

} // r267