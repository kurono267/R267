#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 matrices[6];
} ubo;

layout(location = 0)out vec4 outPos;

void main() {
    for(int face = 0; face < 6; ++face) {
        gl_Layer = face;
        for(int i = 0; i < 3; ++i) {
            outPos = gl_in[i].gl_Position;
            gl_Position = ubo.matrices[face] * outPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}