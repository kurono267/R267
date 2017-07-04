#include "bvh.hpp"

using namespace r267;

BVH::BVH() {}

BVH::~BVH(){}

std::ostream& r267::operator<<(std::ostream& os,const glm::vec3& v){
	os << v.x << "," << v.y << "," << v.z;
	return os;
}

std::ostream& r267::operator<<(std::ostream& os, const BVHNode& n){
	os << "NODE" << std::endl;
    glm::vec3 tmp = glm::vec3(n.min);
	os << "MIN:" << tmp << std::endl;
    tmp = glm::vec3(n.max);
	os << "MAX:" << tmp << std::endl;
	if(n.max.w == 1.0f)os << "DEPTH:" << n.data.w << std::endl;
	return os;
}

void BVH::operator()(const spMesh& mesh){
	run(mesh);
}

glm::vec3 min(const glm::vec3& a,const glm::vec3& b){
	return glm::vec3(fmin(a.x,b.x),fmin(a.y,b.y),fmin(a.z,b.z));
}

glm::vec3 max(const glm::vec3& a,const glm::vec3& b){
	return glm::vec3(fmax(a.x,b.x),fmax(a.y,b.y),fmax(a.z,b.z));
}

inline float SurfaceArea(const BVHNode& n){
	glm::vec3 sides(n.max-n.min);
	return 2*(sides.x*sides.y+sides.y*sides.z+sides.x*sides.z);
}

void BVH::run(const spMesh& mesh){
    auto vertexes  = mesh->vertexes();
    auto indexes = mesh->indexes();

    glm::vec3 minBox = vertexes[0].pos;
    glm::vec3 maxBox = vertexes[0].pos;

    uint32_t numTriangles = indexes.size()/3; // Num triangles is num indexes divided by 3

	std::vector<Prim> primitives;
	primitives.resize(numTriangles);
	for(int i = 0;i<numTriangles;++i){
        int startTriangle = i*3;
        uint32_t v0 = indexes[startTriangle];

		primitives[i].minBox = vertexes[v0].pos;
		primitives[i].maxBox = vertexes[v0].pos;
		primitives[i].id = i;
		for(int v = 1;v<3;++v){
			primitives[i].minBox = min(primitives[i].minBox,vertexes[indexes[startTriangle+v]].pos);
			primitives[i].maxBox = max(primitives[i].minBox,vertexes[indexes[startTriangle+v]].pos);
		}
        minBox = min(minBox,primitives[i].minBox);
        maxBox = max(maxBox,primitives[i].maxBox);
	}

    BVHNode root;
    root.min = glm::vec4(minBox.x,minBox.y,minBox.z,0.0f);
    root.max = glm::vec4(maxBox.x,maxBox.y,maxBox.z,0.0f);
    root.data = glm::ivec4(0);
    _maxDepth = 0;

	recursive(root,primitives,0);
	_nodes.push_back(root);
	rootId = _nodes.size()-1;
	std::cout << "BVH finish with " << _nodes.size() << " nodes" << std::endl;
	std::cout << "Max depth " << _maxDepth << std::endl;
}

inline bool check(const glm::vec3& pos,const glm::vec3& min,const glm::vec3 max){
	if(pos.x < min.x || pos.y < min.y || pos.z < min.z)return false;
	if(pos.x > max.x || pos.y > max.y || pos.z > max.z)return false;
	return true;
}

std::vector<BVHNode>& BVH::nodes(){
	return _nodes;
}

size_t BVH::rootID(){
	return rootId;
}

struct CompareMin {
	int axis;
	CompareMin(int a) : axis(a){}
	CompareMin(const CompareMin& a) : axis(a.axis){}

	bool operator()(const BVH::Prim &a, const BVH::Prim &b){
		if(a.minBox[axis] < b.minBox[axis])return true;
		return false;
	}
};

struct CompareMax {
	int axis;
	CompareMax(int a) : axis(a){}
	CompareMax(const CompareMax& a) : axis(a.axis){}

	bool operator()(const BVH::Prim &a, const BVH::Prim &b){
		if(a.maxBox[axis] < b.maxBox[axis])return true;
		return false;
	}
};

#define EMPTY_COST 10.0f

BVH::SAH BVH::sah(BVHNode& root,std::vector<Prim>& primitives){
	SAH msah;
	msah.axis = 0;
	msah.sah = std::numeric_limits<float>::infinity();
	for(int a = 0;a<3;++a){
		CompareMin comp(a);
		std::sort(primitives.begin(),primitives.end(),comp);

		for(int i=1;i<primitives.size();i++) {
			int onLeftSide = i;
			int onRightSide = primitives.size()-i;

			float split = primitives[i].minBox[a];
			if(split == root.min[a])
				continue;

			BVHNode box_left = root;
			BVHNode box_right = root;

			box_left.max[a] = split;
			box_right.min[a] = split;

			float tsah = EMPTY_COST + onLeftSide*SurfaceArea(box_left) + onRightSide*SurfaceArea(box_right);

			if(tsah < msah.sah) {
				msah.sah = tsah;
				msah.split = split;
				msah.axis = a;
				msah.left_size = onLeftSide;
			}
		}

		CompareMax comp2(a);
		std::sort(primitives.begin(), primitives.end(), comp2);

		for(int i=primitives.size()-2;i>=0;i--){
			int onLeftSide = i+1;
			int onRightSide = primitives.size()-i-1;

			float split = primitives[i].maxBox[a];
			if(split == root.max[a])
				continue;

			BVHNode box_left = root;
			BVHNode box_right = root;

			box_left.max[a] = split;
			box_right.min[a] = split;

			float tsah = EMPTY_COST + onLeftSide*SurfaceArea(box_left) + onRightSide*SurfaceArea(box_right);
			if(tsah < msah.sah) {
				msah.sah = tsah;
				msah.split = split;
				msah.axis = a;
				msah.left_size = onLeftSide;
			}
		}
	}
	return msah;
}

void BVH::recursive(BVHNode& root,
						std::vector<Prim>& primitives,
						const int depth){
	if(depth != 0 && primitives.size() <= NODE_MAX_TRIANGLE){
        root.max.w = 1.0f;
		for(size_t t = 0;t<primitives.size();++t){
			size_t tIdx = primitives[t].id;
			root.data[t] = tIdx;
		}
		for(size_t t = primitives.size();t<NODE_MAX_TRIANGLE;++t){
			root.data[t] = -1;
		}
		return;
	}
	if(depth > _maxDepth)_maxDepth = depth;

	SAH rsah = sah(root,primitives);

	// Init leafs
	BVHNode left = root;
	BVHNode right = root;
	left.max[rsah.axis] = rsah.split;
	right.min[rsah.axis] = rsah.split;

	//std::cout << rsah.axis << std::endl;

	CompareMax comp2(rsah.axis);
	std::sort(primitives.begin(), primitives.end(), comp2);
	int left_size = primitives.size()-1;

	for (int i = 0; i < primitives.size(); ++i){
		if(primitives[i].maxBox[rsah.axis] > rsah.split){
			left_size = i;
			break;
		}
	}

	int right_size = primitives.size()-left_size;

	if(right_size == 0){
        right.max.w = 1.0f;
		for(size_t t = 0;t<NODE_MAX_TRIANGLE;++t){
			right.data[t] = -1;
		}
	} else {
		std::vector<Prim> right_prim(primitives.begin()+left_size+1,primitives.end());
		recursive(right,right_prim,depth+1);
	}

	std::vector<Prim> left_prim(primitives.begin(),primitives.begin()+left_size);

	recursive(left,left_prim,depth+1);

	_nodes.push_back(left);
	root.data.x = _nodes.size()-1;

	_nodes.push_back(right);
	root.data.y = _nodes.size()-1;
	root.min.w = rsah.split;
	root.data.z = rsah.axis;
    root.data.w = depth;
    root.max.w = 0.0f;
}
