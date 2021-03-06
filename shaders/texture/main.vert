#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UniformBufferObject { 
	vec4 color;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    outUV = inUV;
    outColor = ubo.color.rgb;
}
