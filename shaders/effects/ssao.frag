#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D posMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D colorMap;
layout(binding = 3) uniform sampler2D rotationSSAO;

layout(binding = 4) uniform UBO {
	mat4        proj;
	mat4        view;
	mat4        invmvp;
	vec4        kernels[64];
} ubo;

const vec2 noiseScale = vec2(1280.0/4.0f,720.0/4.0f);

const float biasSSAO   = 0.025f;
const int   kernelSize = 64;
const float radius = 0.5f;

vec3 viewPos(float proj_depth){
	vec4 result = ubo.invmvp*vec4(uv.x*2.0f-1.0f,uv.y*2.0f-1.0f,proj_depth,1.0f);
	return result.xyz/result.w;
}

float getZ(float proj_depth){
	const float zNear = 1.0f;const float zFar = 10000.0f;
	float z_n = 2.0 * proj_depth - 1.0;
    float z_e = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
    return z_e;
}

void main() {
	float depth = textureLod(posMap,uv,0).r;
	if(depth == 1.0f)discard;
	vec3 viewP = viewPos(depth);
	float pos_z    = viewP.z;
	vec3 normal = (textureLod(normalMap,uv,0)).rgb;
	vec3 random = textureLod(rotationSSAO,uv*noiseScale, 0).xyz;

	vec3 tangent = normalize(random-normal*dot(random,normal));
	vec3 bi      = cross(normal,tangent);
	mat3 TBN     = mat3(tangent,bi,normal);

	float occlusion = 0.0;

	for(int i = 0; i < kernelSize; ++i){
	    // get sample position
	    vec3 s = TBN * ubo.kernels[i].xyz; // From tangent to view-space
	    s = viewP + s * radius;

	    vec4 offset = vec4(s, 1.0);
		offset      = ubo.proj * offset;
		offset.xyz /= offset.w;
		offset.xyz  = offset.xyz * 0.5 + 0.5;

		float sampleDepth = viewPos(textureLod(posMap, offset.xy, 0).r).z;

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(pos_z - sampleDepth));
		occlusion       += (sampleDepth >= s.z + biasSSAO ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / kernelSize);

	outColor = vec4(vec3(occlusion),1.0f);
}