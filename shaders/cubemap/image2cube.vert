#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 pos;
layout(location = 1) out vec2 uv;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 matrices[6];
} ubo;

layout(push_constant) uniform PushConsts {
	int face;
} pushConsts;

void main() {
	pos         = inPosition;
	uv = inUV;
    gl_Position = ubo.matrices[pushConsts.face]*vec4(pos,1.0);
}