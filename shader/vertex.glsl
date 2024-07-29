#version 450

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

layout(binding = 0) uniform Transform {
    mat4 m;
    mat4 v;
    mat4 p;
} transform;

// out
layout(location = 1) out vec3 v_normal;

void main() {
    v_normal = normal;
    gl_Position = transform.p * transform.v * transform.m * vec4(vertex, 1.0f);
}