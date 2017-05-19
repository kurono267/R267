#include "shape.hpp"

using namespace r267;

Shape::Shape(){}
Shape::~Shape(){}

void Shape::create(spDevice device){
	_vb = device->create<Buffer>();
	_ib = device->create<Buffer>();

	createShape();
}

void Shape::draw(vk::CommandBuffer& commandBuffer){
	vk::DeviceSize buffer = 0;
	commandBuffer.bindVertexBuffers(0,1,&_vb->buffer,&buffer);
	commandBuffer.bindIndexBuffer(_ib->buffer,0,vk::IndexType::eUint32);

	commandBuffer.drawIndexed(_numIndex,1,0,0,0);
}

void Quad::createShape(){
	std::vector<sVertex> vdata = {
				sVertex(-1.0, 1.0, 0.0f, // Pos
						0.0f, 0.0f), 	 // Texcoord
				sVertex(-1.0, -1.0, 0.0f,// Pos
						0.0f, 1.0f),     // Texcoord
				sVertex(1.0, -1.0, 0.0f, // Pos
						1.0, 1.0),       // Texcoord
				sVertex(1.0, 1.0, 0.0f,  // Pos
						1.0, 0.0f)};     // Texcoord

	std::vector<uint32_t> idata = {0,1,2,/*First*/2,3,0/*Second triangle*/};
	_numIndex = idata.size();
	_numVertex = vdata.size();

	_vb->createVB(vdata);
	_ib->createIB(idata);
}

std::vector<sVertex> r267::createCubeVB(){
	std::vector<sVertex> vdata(24);

	// front
	vdata[0] = sVertex(glm::vec3(-1.0, -1.0,  1.0),glm::vec2(0.0f, 1.0f));
	vdata[1] = sVertex(glm::vec3(1.0, -1.0,  1.0),glm::vec2(1.0f, 1.0f));
	vdata[2] = sVertex(glm::vec3(1.0,  1.0,  1.0),glm::vec2(1.0f, 0.0f));
	vdata[3] = sVertex(glm::vec3(-1.0,  1.0,  1.0),glm::vec2(0.0f, 0.0f));
	// top
	vdata[4] = sVertex(glm::vec3(-1.0,  1.0,  1.0),glm::vec2(0.0f, 1.0f));
	vdata[5] = sVertex(glm::vec3(1.0,  1.0,  1.0),glm::vec2(1.0f, 1.0f));
	vdata[6] = sVertex(glm::vec3(1.0,  1.0, -1.0),glm::vec2(1.0f, 0.0f));
	vdata[7] = sVertex(glm::vec3(-1.0,  1.0, -1.0),glm::vec2(0.0f, 0.0f));
	// back
	vdata[8] = sVertex(glm::vec3(1.0, -1.0, -1.0),glm::vec2(0.0f, 1.0f));
	vdata[9] = sVertex(glm::vec3(-1.0, -1.0, -1.0),glm::vec2(1.0f, 1.0f));
	vdata[10] = sVertex(glm::vec3(-1.0,  1.0, -1.0),glm::vec2(1.0f, 0.0f));
	vdata[11] = sVertex(glm::vec3(1.0,  1.0, -1.0),glm::vec2(0.0f, 0.0f));
	// bottom
	vdata[12] = sVertex(glm::vec3(-1.0, -1.0, -1.0),glm::vec2(0.0f, 1.0f));
	vdata[13] = sVertex(glm::vec3( 1.0, -1.0, -1.0),glm::vec2(1.0f, 1.0f));
	vdata[14] = sVertex(glm::vec3( 1.0, -1.0,  1.0),glm::vec2(1.0f, 0.0f));
	vdata[15] = sVertex(glm::vec3(-1.0, -1.0,  1.0),glm::vec2(0.0f, 0.0f));
	// left
	vdata[16] = sVertex(glm::vec3(-1.0, -1.0, -1.0),glm::vec2(0.0f, 1.0f));
	vdata[17] = sVertex(glm::vec3(-1.0, -1.0,  1.0),glm::vec2(1.0f, 1.0f));
	vdata[18] = sVertex(glm::vec3(-1.0,  1.0,  1.0),glm::vec2(1.0f, 0.0f));
	vdata[19] = sVertex(glm::vec3(-1.0,  1.0, -1.0),glm::vec2(0.0f, 0.0f));
	// right
	vdata[20] = sVertex(glm::vec3( 1.0, -1.0,  1.0),glm::vec2(0.0f, 1.0f));
	vdata[21] = sVertex(glm::vec3( 1.0, -1.0, -1.0),glm::vec2(1.0f, 1.0f));
	vdata[22] = sVertex(glm::vec3( 1.0,  1.0, -1.0),glm::vec2(1.0f, 0.0f));
	vdata[23] = sVertex(glm::vec3( 1.0,  1.0,  1.0),glm::vec2(0.0f, 0.0f));

	return vdata;
}

std::vector<uint32_t> r267::createCubeIB(){
	std::vector<uint32_t> idata = {
		0,  1,  2,
		2,  3,  0,
		4,  5,  6,
		6,  7,  4,
		8,  9, 10,
		10, 11,  8,
		12, 13, 14,
		14, 15, 12,
		16, 17, 18,
		18, 19, 16,
		20, 21, 22,
		22, 23, 20
	};
	return idata;
}

void Cube::createShape(){
	std::vector<sVertex> vdata = createCubeVB();
	std::vector<uint32_t> idata = createCubeIB();

	_numIndex = idata.size();
	_numVertex = vdata.size();

	_vb->createVB(vdata);
	_ib->createIB(idata);
}