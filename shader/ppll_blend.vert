#version 450 core

layout (location = 0) in vec3 vs_Position;


void main() {
    gl_Position = vec4(vs_Position, 1.f);
}
