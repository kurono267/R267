#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject { 
	mat4 ortho;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

void main() {
    gl_Position = ubo.ortho*vec4(inPosition, 0.0, 1.0);
    outUV = inUV;
    outColor = inColor;
}
