#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 rayDir;

layout(binding = 7) uniform UBO {
	vec4 view;
	mat4 viewMat;
	mat4 proj;
	mat4 invview;
    mat4 invproj;
} ubo;

void main() {
	vec4 reverseVec;

	/* inverse perspective projection */
	reverseVec = vec4(inPosition.xyz, 1.0);
	reverseVec = ubo.invproj * reverseVec;

	/* inverse modelview, without translation */
	reverseVec.w = 0.0;
	reverseVec = ubo.invview * reverseVec;

	rayDir = normalize(vec3(reverseVec));

    gl_Position = vec4(inPosition, 1.0);
    outUV = vec2(inUV.x,1.0f-inUV.y);
}
