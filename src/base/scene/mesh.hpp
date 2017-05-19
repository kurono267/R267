#pragma once

#include "base/vk/shape.hpp"
#include "format.hpp"

namespace r267 {

class Mesh : public Shape {
	public:
		Mesh(const bool storeCPU = false);
		virtual ~Mesh();

		void setData(const uint8_t* data,const uint64_t& size); // Set Packed data
		void setData(const std::vector<sVertex>& vertexes,const std::vector<uint32_t>& indexes,const std::string& name); // Set Unpacked data

		bool equal(const std::shared_ptr<r267::Mesh> other);

		uint8_t* packed(uint64_t& size);
		bool is(); 
	protected:
		void createShape();
		// Our data at CPU
		std::string          _name;
		std::vector<sVertex> _vertexes;
		std::vector<uint32_t>    _indexes;

		bool _isData;
		bool _storeCPU;

		void unpack(const uint8_t* data,const uint64_t& size);
		uint8_t* pack(uint64_t& size);
};

typedef std::shared_ptr<r267::Mesh> spMesh;

} // r267
