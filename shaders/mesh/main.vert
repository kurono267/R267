#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UniformBufferObject { 
	mat4 mvp;
	vec4 view;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 view;

void main() {
    gl_Position = ubo.mvp*vec4(inPosition,1.0);
    outPos = inPosition;
    outUV = inUV;
    outNormal = inNormal;
    view = ubo.view.xyz;
}
