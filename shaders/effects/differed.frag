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

vec3 viewPos(float proj_depth){
	vec4 result = ubo.invproj*vec4(uv.x*2.0f-1.0f,uv.y*2.0f-1.0f,proj_depth,1.0f);
	return result.xyz/result.w;
}

vec3 lightCompute(){
	const vec4 colorData = texture(colorMap,uv);
	if(colorData.a == 0.0f){
		// Background
		return textureLod(background,rayDir,0).rgb;
	}
	vec3 ssao = vec3(0.0f);
	vec2 texelSize = 1.0 / vec2(textureSize(ssaoMap, 0));
	for(int y = -2;y<2;++y){
		for(int x = -2;x<2;++x){
			ssao += texture(ssaoMap,uv+vec2(float(x)*texelSize.x,float(y)*texelSize.y)).xyz;
		}
	}
	ssao /= 16.0f;

	vec3 pos = viewPos(texture(posMap,uv).r);
	vec4 normalData = texture(normalMap,uv);

	vec4 viewPosView  = ubo.viewMat*ubo.view;

	const float metallic = normalData.a;

	const float r = colorData.w;
	const vec3  specColor = vec3(1.0f);//reflection(posData.xyz,normalData.xyz);

	vec3 lightColor = vec3(0.0f);
	const float lightPower = 500.0f;
	//vec3 localReflection = reflection();
	for(int i = 0;i<4;++i){
		vec4 lightPosView = ubo.viewMat*vec4(lightPoses[i],1.0f);
		const vec3 lightPos3 = lightPosView.xyz/lightPosView.w;

		float distance    = length(lightPos3 - pos);
        float attenuation = lightPower / (distance * distance);

		lightColor += light(lightPos3,normalData.xyz,viewPosView.xyz/viewPosView.w,pos,metallic,r,colorData.rgb,specColor)*attenuation;
	}
	lightColor += ibl(normalData.xyz,viewPosView.xyz,pos,colorData.rgb,metallic,r,1.0f);
	lightColor /= lightColor+vec3(1.0f);
	lightColor = pow(lightColor,vec3(1.0f/2.2f));

	return ssao*lightColor;
}

void main() {
	//vec3 localReflection = reflection();
	outColor = vec4(lightCompute(),1.0f);
}