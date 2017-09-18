#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(tex,uv).rgb, 1.0);
}