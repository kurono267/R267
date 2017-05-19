#pragma once

#include "material.hpp"
#include "mesh.hpp"

namespace r267 {

// Our Representation of Model
class Model {
	public:
		Model(const std::string& name);
		virtual ~Model();
		
		spMaterial  material();
		spMesh      mesh();
		std::string name();

		void        setMaterial(const spMaterial& material);
		void        setMesh(const spMesh& mesh);

		bool equal(const std::shared_ptr<Model>& model);
	protected:
		std::string _name;
		spMaterial  _material;
		spMesh      _mesh;
};

typedef std::shared_ptr<Model> spModel;

} // r267
