#version 450 core

layout(location=0) in vec3 vs_Position;
layout(location=1) in vec3 vs_Tangent;
layout(location=2) in float vs_Scale;

out vec3 gs_Position;
out vec4 gs_Tangent;

uniform mat4 g_Model;

void main()
{
    gs_Position = (g_Model*vec4(vs_Position,1.)).xyz;

    gs_Tangent.xyz = mat3(transpose(inverse(g_Model)))*vs_Tangent;
    gs_Tangent.w = vs_Scale;
}
