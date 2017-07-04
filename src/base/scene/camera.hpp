#pragma once

#define GLM_FORCE_RADIANS
#define GLM_GTC_constants
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtc/matrix_transform.hpp> // glm::perspective
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>

using namespace glm;

namespace r267 {

class Camera {
	public:
		Camera(const vec3& pos,const vec3& lookAt = vec3(0.0f),const vec3& up = vec3(0.f,1.f,0.f));
		virtual ~Camera();

		void move(const vec2& dmouse,const float& dt);
		void rotate(const vec2& dmouse,const float& dt);
		void scale(const float& dvalue,const float& dt);

		void setProj(const float& angle,const float& aspect,const float& near,const float& far);

		mat4 getView();
		mat4 getProj();
		mat4 getVP();

		vec3 getPos() const;
		vec3 getUp() const;
		vec3 getRight() const;
		vec3 getLook() const;
	protected:
		float _angle;
		float _aspect;
		float _near;
		float _far;

		vec3  _pos;
		vec3  _up;
		vec3  _lookAt;

		vec2  _tp;

		mat4 _view;
		mat4 _viewRay;

		mat4 _proj;
};

typedef std::shared_ptr<Camera> spCamera;

}
