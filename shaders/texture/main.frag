#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(tex,vec2(uv.x,1.0f-uv.y)).rgb, 1.0);
}