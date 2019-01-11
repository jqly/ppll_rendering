#version 450 core

layout (location = 0) in vec3 vs_Position;

uniform mat4 g_LightModelViewProj;

void main() {
	gl_Position = g_LightModelViewProj*vec4(vs_Position,1);
}
