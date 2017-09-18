#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out float posMap;
layout(location = 1) out vec4 normalMap;
layout(location = 2) out vec4 colorMap;

layout(set = 1, binding = 1) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D heightmap;

layout(set = 1, binding = 0) uniform Material {
	vec4        diffuseColor;
	vec4        data;
} mat;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 pos;

layout(location = 2) in vec3 Tangent;
layout(location = 3) in vec3 Binormal;
layout(location = 4) in vec3 Normal;

layout(location = 5) in vec3 tangentView;
layout(location = 6) in vec3 tangentPos;

const float height_scale = 0.05f;

vec2 parallax(){
	vec3 viewDir = normalize(tangentView-tangentPos);
	// number of depth layers
	const float minLayers = 16.0;
	const float maxLayers = 64.0;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
	vec2  currentTexCoords     = uv;
	float currentDepthMapValue = 1.0f-texture(heightmap, currentTexCoords).r;

	while(currentLayerDepth < currentDepthMapValue)
	{
	    // shift texture coordinates along direction of P
	    currentTexCoords -= deltaTexCoords;
	    // get depthmap value at current texture coordinates
	    currentDepthMapValue = 1.0f-texture(heightmap, currentTexCoords).r;
	    // get depth of next layer
	    currentLayerDepth += layerDepth;
	}

	// get texture coordinates before collision (reverse operations)
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = (1.0f-texture(heightmap, prevTexCoords).r) - currentLayerDepth + layerDepth;

	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
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

const float pi = 3.1415926535;

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

void main() {
	vec2 parallaxUV = mat.data.x==2.0f?parallax():uv;

    colorMap  = vec4(texture(diffuseTexture,parallaxUV).rgb*mat.diffuseColor.xyz,mat.data.y); // Simple gamma

    vec3 normalTex = texture(normalTexture, parallaxUV).rgb;
    if(mat.data.x != 0.0f){
        normalTex = normalize(normalTex-0.5f);
        mat3 TBN  = mat3(Tangent,Binormal,Normal);
        normalTex = normalize(TBN*normalTex);
    } else normalTex = Normal;

	vec3 posOffset = vec3(0.0f);
	if(mat.data.x==2.0f)posOffset = height_scale*(1.0f-texture(heightmap,parallaxUV).r)*Normal;
    posMap    = pos.z/pos.w;

    normalMap = vec4(normalTex,mat.data.z);
}
