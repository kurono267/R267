#include "camera.hpp"
#include <iostream>

using namespace glm;
using namespace r267;

Camera::Camera(const vec3& pos,const vec3& lookAt,const vec3& up) : _pos(pos),_lookAt(lookAt),_up(up),_tp(0.0f) {
	_view = ::lookAt(_pos,_lookAt,_up);
}
Camera::~Camera(){}

void Camera::move(const vec2& dmouse,const float& dt){
/*	const vec3 viewVec = normalize(_lookAt - _pos);
	float dist = length(_lookAt - _pos);

	_lookAt += vec3(0.0f,0.0f,1.0f)*dmouse.x*0.1f*dt;
	_lookAt += vec3(1.0f,0.0f,0.0f)*dmouse.y*0.1f*dt;

	_pos = _lookAt+dist*viewVec;
	_view = lookAt(_pos,_lookAt,_up);*/
}


inline mat4 rotateV(const vec3& p, const vec3& u, const float& r){
	return translate(p)*rotate(r,u)*translate(-p);
}

void Camera::rotate(const vec2& dmouse,const float& dt){
	float angularSpeed = 0.1f*dt;
	float theta = dmouse.x * angularSpeed;
	float phi = dmouse.y * angularSpeed;

	vec3 viewVec = normalize(_lookAt - _pos);
	float dist = length(_lookAt - _pos);

	// Rotate along Y
	const vec3 dY = normalize(cross(viewVec,_up));
	glm::quat qY(phi,dY);

	//_up = normalize(qY*_up);

	const vec3 dX = normalize(qY*_up);

	glm::quat qX(theta,dX);
	glm::quat r = normalize(qX*qY);

	viewVec = normalize(r*viewVec);
	_pos = _lookAt+dist*viewVec*vec3(-1.0f,1.0f,-1.0f);
/*
	const vec3 viewVec = normalize(_lookAt - _pos);
	float dist = length(_lookAt - _pos);

	const vec3 dX = normalize(cross(viewVec,_up));
	const vec3 dY = normalize(cross(viewVec,dX));

	mat4 rot_x = rotateV(_lookAt,dX,phi);

	_view = rot_x * _view; 
	_view = rotateV(_lookAt,dY,theta) * _view; 
	_pos  = _lookAt+vec3(dist*(vec4(0.0f,0.0f,1.0f,0.0f)*_view));
*/
	_view = lookAt(_pos,_lookAt,_up);
}

void Camera::scale(const float& dvalue,const float& dt){
	const float scaleSpeed = 3.0f;

	const vec3 viewVec = normalize(_lookAt - _pos);
	float dist = length(_lookAt - _pos);

	_pos += (dist*dvalue*dt*scaleSpeed)*viewVec;
	_view = lookAt(_pos,_lookAt,_up);
}

void Camera::setProj(const float& angle,const float& aspect,const float& near,const float& far){
	_aspect = aspect; _angle = angle; _near = near; _far = far;
	_proj = glm::perspective(_angle,_aspect,_near,_far);
}

mat4 Camera::getView(){
	return _view;
}

mat4 Camera::getProj(){
	return _proj;
}

mat4 Camera::getVP(){
	return _proj*_view;
}

vec3 Camera::getPos() const{
	return _pos;
}

vec3 Camera::getUp() const {
	return _up;
}

vec3 Camera::getRight() const {
	vec3 look(normalize(_lookAt-_pos));
	return normalize(cross(look,_up));
}

vec3 Camera::getLook() const {
	return normalize(_lookAt-_pos);
}

