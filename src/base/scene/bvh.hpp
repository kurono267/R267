#pragma once

#include "mesh.hpp"
#include <algorithm> 

#define NODE_MAX_TRIANGLE 4 // Max triangle in node

namespace r267 {

struct BVHNode {
	BVHNode(){}
	BVHNode(const BVHNode& n) : min(n.min),max(n.max),data(n.data) {}

	// Alligned BVH Node
    glm::vec4 min; // xyz min, w split
    glm::vec4 max; // xyz max, w 1 if leaf

    // Data changes by leaf or not current node
    // If Leaf x,y,z,w index of triangle
    // If Not leaf x,y r leaf, l leaf, z - axis, w - depth
    glm::ivec4 data;
};

std::ostream& operator<<(std::ostream& os,const glm::vec3& v);
std::ostream& operator<<(std::ostream& os, const BVHNode& n);

class BVH {
	struct SAH {
		float split;
		int axis;
		float sah;

		int left_size;
	};
	public:
		struct Prim {
			glm::vec3 minBox;
			glm::vec3 maxBox;

			size_t id;
		};

		BVH();
		virtual ~BVH();

		void operator()(const spMesh& mesh);
		void run(const spMesh& mesh);

		std::vector<BVHNode>& nodes();
		size_t rootID();
	protected:
		void recursive(BVHNode& root, std::vector<Prim>& primitives, const int depth);
		SAH  sah(BVHNode& root, std::vector<Prim>& primitives);

		size_t rootId;
		std::vector<BVHNode> _nodes;
		size_t			  _maxDepth;
};

}
