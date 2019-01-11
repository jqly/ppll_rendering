#version 450 core

layout (location=0) in vec3 vs_Position;
layout (location=1) in vec3 vs_Normal;
layout (location=2) in vec2 vs_TexCoord;

out vec3 fs_Position;
out vec3 fs_Normal;
out vec2 fs_TexCoord;

uniform mat4 g_Model,g_ViewProj;

void main() 
{
    vec4 position = g_Model * vec4(vs_Position,1);
    fs_Position = position.xyz;
    fs_Normal = mat3(transpose(inverse(g_Model))) * vs_Normal;
    gl_Position = g_ViewProj*position;

    fs_TexCoord = vs_TexCoord;
}
