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

glm::vec3 vmin(const glm::vec3& a,const glm::vec3& b){
	return glm::vec3(fmin(a.x,b.x),fmin(a.y,b.y),fmin(a.z,b.z));
}

glm::vec3 vmax(const glm::vec3& a,const glm::vec3& b){
	return glm::vec3(fmax(a.x,b.x),fmax(a.y,b.y),fmax(a.z,b.z));
}

inline float SurfaceArea(const BVHNode& n){
	glm::vec3 sides(n.max-n.min);
	return 6*2*(sides.x*sides.y+sides.y*sides.z+sides.x*sides.z);
}

struct BuildEntry {
    uint32_t parent;
    uint32_t start; uint32_t end;
};

void BVH::run(const spMesh& mesh,const uint& mesh_id){
    auto vertexes  = mesh->vertexes();
    auto indexes = mesh->indexes();

    uint32_t numTriangles = indexes.size()/3; // Num triangles is num indexes divided by 3

	std::vector<Prim> primitives;
	primitives.resize(numTriangles);
	for(int i = 0;i<numTriangles;++i){
        int startTriangle = i*3;
        uint32_t v0 = indexes[startTriangle];

        Prim prim;

        glm::vec3 v0pos = vertexes[v0].pos;
        prim.minBox = v0pos;
        prim.maxBox = v0pos;
        prim.center = v0pos;
        prim.id = startTriangle;
		for(int v = 1;v<3;++v){
            glm::vec3 pos = vertexes[indexes[startTriangle+v]].pos;
            prim.minBox = min(prim.minBox,pos);
            prim.maxBox = max(prim.maxBox,pos);
            prim.center += pos;
		}
        prim.center /= 3.0f;
        primitives[i] = prim;
	}

    BVHNode root;
    root.data = glm::ivec4(0);
	_nodes.push_back(root);
	rootId = _nodes.size()-1;
    _maxDepth = 0;

    recursive(root,primitives,0,primitives.size(),0,mesh_id);

	_nodes[rootId] = root;
	std::cout << "BVH finish with " << _nodes.size() << " nodes" << std::endl;
	std::cout << "Max depth " << _maxDepth << std::endl;
}

void BVH::run(const spScene& scene){
    // Create BVHs for meshes in scene
    _meshes.resize(scene->models().size());
    _meshIDs.resize(_meshes.size());
    std::vector<Prim> scene_prims(_meshes.size());
    uint32_t ibOffset = 0;
    uint32_t vbOffset = 0;
    for(uint32_t i = 0;i<_meshes.size();++i){
        spModel model = scene->models()[i];
        spMesh  mesh = model->mesh();

        _meshes[i].run(mesh,i);

        _meshIDs[i].vbOffset = vbOffset;
        _meshIDs[i].ibOffset = ibOffset;

        vbOffset += mesh->vertexes().size();
        ibOffset += mesh->indexes().size();

        BVHNode root = _meshes[i]._nodes[_meshes[i].rootID()];
        Prim prim;
        prim.minBox = glm::vec3(root.min.x,root.min.y,root.min.z);
        prim.maxBox = glm::vec3(root.max.x,root.max.y,root.max.z);
        prim.id     = i;
        prim.center = prim.minBox+(prim.maxBox-prim.minBox)*0.5f; // As center use center of BBox
        scene_prims.push_back(prim);
    }

    BVHNode root;
    root.data = glm::ivec4(0);
    _nodes.push_back(root);
    rootId = _nodes.size()-1;
    _maxDepth = 0;

    recursiveScene(root,scene_prims,0,scene_prims.size(),0);

    _nodes[rootId] = root;
}

struct AABB {
    AABB() : min(std::numeric_limits<float>::infinity()), max(-std::numeric_limits<float>::infinity()) {}
    AABB(const BVHNode& node) : min(node.min.x,node.min.y,node.min.z), max(node.max.x,node.max.y,node.max.z) {}
    AABB(const AABB& a) : min(a.min), max(a.max) {}

    void expand(const glm::vec3& _min,const glm::vec3& _max){
        min = vmin(_min,min);
        max = vmax(_max,max);
    }
    void expand(const glm::vec3& _p){
        min = vmin(_p,min);
        max = vmax(_p,max);
    }

    uint32_t maxDim(){
        glm::vec3 axisSize = max-min;
        if(axisSize.x <= axisSize.y){
            if(axisSize.y >= axisSize.z)return 1; // Y axis
        } else {
            if(axisSize.x >= axisSize.z)return 0; // X axis
        }
        return 2; // Z axis Z more then other
    }

    glm::vec3 min;
    glm::vec3 max;
};

void BVH::recursive(BVHNode& root, std::vector<Prim>& primitives, const uint32_t start, const uint32_t end, const int depth, const uint& mesh_id){
    if(_maxDepth < depth)_maxDepth = depth;
    AABB aabb;
    AABB centroid;
    // Recompute BBox
    for(int i = start;i<end;++i){
        const Prim& p = primitives[i];
        aabb.expand(p.minBox,p.maxBox);
        centroid.expand(p.center);
    }
    root.min = glm::vec4(aabb.min,0.0f);
    root.max = glm::vec4(aabb.max,0.0f);

    // Check leaf
    uint32_t size = end-start;
    if(size <= NODE_MAX_TRIANGLE){
        root.max.w = 1.0f;
        for(uint32_t i = 0;i<NODE_MAX_TRIANGLE;++i){
            if(i < size){
                root.data[i] = primitives[start+i].id;
            }
            else root.data[i] = -1;
        }
        return;
    }

    // Split axis, max axis in AABB
    uint32_t axis  = aabb.maxDim();

    float    split = 0.5f*(aabb.max[axis]-aabb.min[axis]);

    // Partly sort
    uint32_t  mid = start;
    for(uint32_t i = start;i<end;++i){
        if(primitives[i].center[axis] < split){
            std::iter_swap(primitives.begin()+i,primitives.begin()+mid);
            ++mid;
        }
    }

    if(mid == start || mid == end){
        mid = start + (end-start)/2;
    }

    // Right node
    BVHNode right;
    recursive(right,primitives,mid,end,depth+1,mesh_id);
    _nodes.push_back(right);
    root.data.y = _nodes.size()-1;

    BVHNode left;
    recursive(left,primitives,start,mid,depth+1,mesh_id);
    _nodes.push_back(left);
    root.data.x = _nodes.size()-1;

    // Fill additional data to node
    root.data.z = axis; // Axis
    root.data.w = depth;
    root.min.w  = split;
}

void BVH::recursiveScene(BVHNode& root, std::vector<Prim>& primitives,const uint32_t start, const uint32_t end, const int depth){
    if(_maxDepth < depth)_maxDepth = depth;
    AABB aabb;
    AABB centroid;
    // Recompute BBox
    for(int i = start;i<end;++i){
        const Prim& p = primitives[i];
        aabb.expand(p.minBox,p.maxBox);
        centroid.expand(p.center);
    }
    root.min = glm::vec4(aabb.min,0.0f);
    root.max = glm::vec4(aabb.max,0.0f);

    // Check leaf
    uint32_t size = end-start;
    if(size <= 1){
        // Add node of that mesh to current BVH
        BVH& meshBVH = _meshes[primitives[start].id];
        MeshID meshID = _meshIDs[primitives[start].id];
        BVHNode rootMesh = meshBVH._nodes[0];
        uint32_t nodeOffset = _nodes.size()-1; // Start of current meshes node
        for(int i = 1;i<meshBVH._nodes.size();++i){
            BVHNode node = meshBVH._nodes[i];
            if(node.max.w == 0){ // Modify node positions for not leaf
                node.data.x = node.data.x+nodeOffset;
                node.data.y = node.data.y+nodeOffset;
            } else if(node.max.w == 1){
                for(int i = 0;i<4;++i){
                    if(node.data[i] >= 0)node.data[i] += meshID.ibOffset;
                }
            }
            _nodes.push_back(node);
        }
        root.data.x = rootMesh.data.x+nodeOffset;
        root.data.y = rootMesh.data.y+nodeOffset;
        return;
    }

    // Split axis, max axis in AABB
    uint32_t axis  = aabb.maxDim();

    float    split = 0.5f*(aabb.max[axis]-aabb.min[axis]);

    // Partly sort
    uint32_t  mid = start;
    for(uint32_t i = start;i<end;++i){
        if(primitives[i].center[axis] < split){
            std::iter_swap(primitives.begin()+i,primitives.begin()+mid);
            ++mid;
        }
    }

    if(mid == start || mid == end){
        mid = start + (end-start)/2;
    }

    // Right node
    BVHNode right;
    recursiveScene(right,primitives,mid,end,depth+1);
    _nodes.push_back(right);
    root.data.y = _nodes.size()-1;

    BVHNode left;
    recursiveScene(left,primitives,start,mid,depth+1);
    _nodes.push_back(left);
    root.data.x = _nodes.size()-1;

    // Fill additional data to node
    root.data.z = axis; // Axis
    root.data.w = depth;
    root.min.w  = split;
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

