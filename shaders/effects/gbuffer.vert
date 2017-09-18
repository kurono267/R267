#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec3 Tangent;
layout(location = 3) out vec3 Binormal;
layout(location = 4) out vec3 Normal;

void main() {
	// View space TBN
	//mat3 viewPart = mat3(ubo.viewMat);
	Tangent = inTangent;
    Binormal = inBinormal;
    Normal = inNormal;

    gl_Position = inPosition;//ubo.mvp*vec4(inPosition,1.0);
    //vec4 viewPos = ubo.viewMat*vec4(inPosition,1.0);
    outPos = inPosition;//viewPos.xyz/viewPos.w;
    outUV = inUV;
}