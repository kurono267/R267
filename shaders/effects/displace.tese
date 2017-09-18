#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 mvp;
	mat4 viewMat;
	mat4 proj;
	vec4 view;
} ubo;

layout(triangles, equal_spacing, cw) in;

layout(set = 1, binding = 1) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;

layout(location = 0) in vec2 inUV[];
layout(location = 1) in vec4 inPos[];
layout(location = 2) in vec3 inTangent[];
layout(location = 3) in vec3 inBinormal[];
layout(location = 4) in vec3 inNormal[];

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBinormal;
layout(location = 4) out vec3 outNormal;

layout(location = 5) out vec3 tangentView;
layout(location = 6) out vec3 tangentPos;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2){
  return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2){
  return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

vec4 interpolate4D(vec4 v0, vec4 v1, vec4 v2){
  return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

void main(){
	vec4 pos = interpolate4D(inPos[0],inPos[1],inPos[2]);
	outUV    = interpolate2D(inUV[0],inUV[1],inUV[2]);

	vec3 normal = normalize(interpolate3D(inNormal[0],inNormal[1],inNormal[2]));

	// Compute position
	//const float scale = 0.1f;
	//float height = length(texture(diffuseTexture,outUV).xyz);
	//pos          = pos+normal*height*scale;
	vec4  viewPos = ubo.viewMat*pos;
	vec4  screenPos = ubo.mvp*pos;
	outPos       = screenPos;//viewPos.xyz;

	gl_Position = screenPos;

	vec3 tangent = normalize(interpolate3D(inTangent[0],inTangent[1],inTangent[2]));
	vec3 binormal = normalize(cross(normal,tangent));//normalize(interpolate3D(inBinormal[0],inBinormal[1],inBinormal[2]));

	mat3 tbn = mat3(tangent,binormal,normal);
	tbn = transpose(tbn);

	tangentView     = tbn*ubo.view.xyz; // View-Pos in tangent space
	tangentPos      = tbn*(pos.xyz/pos.w);

	mat3 viewPart = mat3(ubo.viewMat);
	normal = viewPart*normal;
	binormal = viewPart*binormal;
	tangent = viewPart*tangent;

	outNormal = normal;
	outTangent = tangent;
	outBinormal = binormal;
}