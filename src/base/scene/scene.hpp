#pragma once

#include <experimental/filesystem>

#include "model.hpp"
#include <boost/property_tree/json_parser.hpp>

namespace r267 {

class Scene {
	const std::string binExt = "r267b";
	const std::string mtlExt = "json";
	public:
		Scene();
		virtual ~Scene();

		void add(const spModel& model);
		
		// Filename without resolution
		// Save and load 2 files 
		// filename.r267b -- meshes
		// filename.json -- materials
		void load(const std::string& filename);
		void save(const std::string& filename);

		std::vector<spModel> models();

		bool equal(const std::shared_ptr<Scene>& scene);
	protected:
		std::vector<spModel> _models;
};

typedef std::shared_ptr<Scene> spScene;

} // r267