#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D atlas;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(atlas,uv);
    float a = color.a*tex.a*3.0f;
    outColor = vec4(color.rgb*tex.rgb,a);
}