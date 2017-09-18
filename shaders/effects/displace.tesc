#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(vertices = 3) out;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 mvp;
	mat4 viewMat;
	mat4 proj;
	vec4 view;
} ubo;

layout(location = 0) in vec2 inUV[];
layout(location = 1) in vec4 inPos[];
layout(location = 2) in vec3 inTangent[];
layout(location = 3) in vec3 inBinormal[];
layout(location = 4) in vec3 inNormal[];

layout(location = 0) out vec2 outUV[];
layout(location = 1) out vec4 outPos[];
layout(location = 2) out vec3 outTangent[];
layout(location = 3) out vec3 outBinormal[];
layout(location = 4) out vec3 outNormal[];

const float edgeSize = 2000.0f;
const float maxTessFactor = 64.0f;
const vec2  viewport = vec2(1280.0,720.0f);

float screenSpaceTessFactor(vec4 p0, vec4 p1){
	// Calculate edge mid point
	vec4 midPoint = 0.5 * (p0 + p1);
	// Sphere radius as distance between the control points
	float radius = distance(p0, p1) / 2.0;

	// View space
	vec4 v0 = ubo.viewMat  * midPoint;

	// Project into clip space
	vec4 clip0 = (ubo.proj * (v0 - vec4(radius, vec3(0.0))));
	vec4 clip1 = (ubo.proj * (v0 + vec4(radius, vec3(0.0))));

	// Get normalized device coordinates
	clip0 /= clip0.w;
	clip1 /= clip1.w;

	// Convert to viewport coordinates
	clip0.xy *= viewport;
	clip1.xy *= viewport;

	// Return the tessellation factor based on the screen size
	// given by the distance of the two edge control points in screen space
	// and a reference (min.) tessellation size for the edge set by the application
	return clamp(distance(clip0, clip1) / edgeSize * maxTessFactor, 1.0, maxTessFactor);
}

void main(){
	outUV[gl_InvocationID] = inUV[gl_InvocationID];
	outPos[gl_InvocationID] = inPos[gl_InvocationID];
	outTangent[gl_InvocationID] = inTangent[gl_InvocationID];
	outBinormal[gl_InvocationID] = inBinormal[gl_InvocationID];
	outNormal[gl_InvocationID] = inNormal[gl_InvocationID];

	if (gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = 1.0f;//screenSpaceTessFactor(gl_in[2].gl_Position,gl_in[0].gl_Position);
		gl_TessLevelOuter[1] = 1.0f;//screenSpaceTessFactor(gl_in[1].gl_Position,gl_in[2].gl_Position);
		gl_TessLevelOuter[2] = 1.0f;//screenSpaceTessFactor(gl_in[2].gl_Position,gl_in[0].gl_Position);
		gl_TessLevelInner[0] = gl_TessLevelOuter[0];
		gl_TessLevelInner[1] = gl_TessLevelOuter[1];
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}