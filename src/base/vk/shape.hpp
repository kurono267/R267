#pragma once
#include "buffer.hpp"

namespace r267 {

class Shape {
	public:
		Shape();
		virtual ~Shape();

		void create(spDevice device);
		void draw(vk::CommandBuffer& commandBuffer);
	protected:
		virtual void createShape() = 0;

		spBuffer _vb;
		spBuffer _ib;

		uint _numIndex;
		uint _numVertex;
};

class Quad : public Shape {
	public:
		Quad(){}
		virtual ~Quad(){}
	protected:
		virtual void createShape();
};

class Cube : public Shape {
	public:
		Cube(){}
		virtual ~Cube(){}
	protected:
		virtual void createShape();
};

typedef std::shared_ptr<Shape> spShape;

};
