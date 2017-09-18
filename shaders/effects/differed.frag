#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 rayDir;

layout(binding = 0) uniform sampler2D posMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D colorMap;
layout(binding = 3) uniform sampler2D ssaoMap;
layout(binding = 4) uniform samplerCube background;
layout(binding = 5) uniform samplerCube irradianceMap;
layout(binding = 6) uniform sampler2D brdf;

layout(binding = 7) uniform UBO {
	vec4 view;
	mat4 viewMat;
	mat4 proj;
	mat4 invview;
    mat4 invproj;
} ubo;

#include "light.glsl"

const vec3 lightPos = vec3(0.0,1000.0,0.0);
const vec3 lightPoses[4] = {
	vec3(10.0,10.0f,-10.0f),
	vec3(10.0,10.0f,10.0f),
	vec3(-10.0,10.0f,10.0f),
	vec3(-10.0,10.0f,-10.0f)
};


struct TraceResult {
	vec2 uv; // UV coords
	float L; // Length between find point and ray pos
	float Lp; // Length between find point and start pos
};

const int maxStep = 16;

vec3 toScreen(vec3 pos){
	 vec4 pVP = ubo.proj*vec4(pos,1.0f);
	 pVP.xyz /= pVP.w;
	 pVP.xy = vec2(0.5f, 0.5f) + vec2(0.5f, -0.5f) * pVP.xy;
	 pVP.y = 1.0f-pVP.y;
	 return pVP.xyz;
 }

float pixelDiff(){
	return length(texture(posMap,uv).xyz-texture(posMap,uv+1.0f/textureSize(posMap,0)).xyz);
}

TraceResult trace(vec3 org,vec3 dir,float eps,bool reflection){
	vec3 rayPos = org+dir*eps;
	TraceResult result;
	result.uv = uv;
	result.L = 10000.0f; // Init result with max length to ray
	for(int step = 1;step<=maxStep;++step){
		vec3 rayScreen = toScreen(rayPos);
		if(rayScreen.z < 0.0f)break;
		if(rayScreen.x < 0.0f || rayScreen.y < 0.0f || rayScreen.x > 1.0f || rayScreen.y > 1.0f)break;
		vec3 pos     = texture(posMap,rayScreen.xy).xyz;

		float cL = length(pos-rayPos);
		float cP = length(pos-org);
		if(cL < result.L || reflection){
			result.uv = rayScreen.xy;
			result.L = cL;
			result.Lp = cP;
		}

		//rayPos = org+dir*eps*(step+1);
		rayPos = org+dir*cP;
	}
	return result;
}

float shadow(vec3 pos,vec3 lightPos){
	float pixelEps = 0.5f;
	TraceResult test = trace(pos,normalize(lightPos-pos),pixelEps,false);

	return mix(clamp(test.L,0.0f,1.0f),1.0f,1.0f-clamp(length(test.uv-uv)*20.0f,0.0f,1.0f));
}

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 sample3D(uint i, uint N) {
	vec2 Xi   = Hammersley(i,N);
    float phi = 2.0 * pi * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    return H;
}

vec3 reflection(){
	vec4 posData = texture(posMap,uv);
	vec3 pos = posData.xyz;
	const float r = posData.w;
	vec3 normal = texture(normalMap,uv).xyz;
	vec3 viewDir    = normalize(pos);
	vec3 reflectDir = normalize(reflect(viewDir,normal));
	float pixelEps = 0.05f;
	TraceResult test = trace(pos,reflectDir,pixelEps,true);

	return texture(colorMap,test.uv).xyz;
	//return texture(colorMap,test.uv).xyz;
}

vec3 lightCompute(){
	vec3 ssao = vec3(0.0f);
	vec2 texelSize = 1.0 / vec2(textureSize(ssaoMap, 0));
	for(int y = -2;y<2;++y){
		for(int x = -2;x<2;++x){
			ssao += texture(ssaoMap,uv+vec2(float(x)*texelSize.x,float(y)*texelSize.y)).xyz;
		}
	}
	ssao /= 16.0f;

	vec4 posData = texture(posMap,uv);
	vec4 normalData = texture(normalMap,uv);

	vec4 viewPosView  = ubo.viewMat*ubo.view;

	const vec4 colorData = texture(colorMap,uv);
	if(colorData.a == 0.0f){
		// Background
		return textureLod(background,rayDir,0).rgb;
	}
	const float metallic = normalData.a;

	const float r = posData.w;
	const vec3  specColor = vec3(1.0f);//reflection(posData.xyz,normalData.xyz);

	vec3 lightColor = vec3(0.0f);
	const float lightPower = 500.0f;
	float iblShadow = 0.0f;
	for(int s = 0;s<5;++s){
		iblShadow += shadow(posData.xyz,(sample3D(s,5)+normalData.xyz)*vec3(1000.0f));
	}
	iblShadow /= 5;
	//vec3 localReflection = reflection();
	for(int i = 0;i<4;++i){
		vec4 lightPosView = ubo.viewMat*vec4(lightPoses[i],1.0f);
		const vec3 lightPos3 = lightPosView.xyz/lightPosView.w;

		float distance    = length(lightPos3 - posData.xyz);
        float attenuation = lightPower / (distance * distance);

		lightColor += light(lightPos3,normalData.xyz,viewPosView.xyz/viewPosView.w,posData.xyz,metallic,r,colorData.rgb,specColor)*attenuation;
	}
	lightColor += ibl(normalData.xyz,viewPosView.xyz,posData.xyz,colorData.rgb,metallic,r,1.0f);
	lightColor = iblShadow*ssao*lightColor;//*colorData.a;
	lightColor /= lightColor+vec3(1.0f);
	lightColor = pow(lightColor,vec3(1.0f/2.2f));

	return lightColor;
}

void main() {
	//vec3 localReflection = reflection();
	outColor = vec4(texture(ssaoMap,uv).rgb,1.0f);
}