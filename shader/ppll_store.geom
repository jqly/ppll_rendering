#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=4) out;

////
// Defines
////

#define HAIR_RADIUS_SCALE 1E-4

////
// Ins & Outs.
////

in vec3 gs_Position[];
in vec3 gs_Tangent[];
in float gs_Scale[];

out vec3 fs_Position;
out vec3 fs_Tangent;
out float fs_Scale;
out vec3 fs_P0;
out vec3 fs_P1;

////
// Uniforms.
////

uniform float g_HairRadius;
uniform vec3 g_Eye;
uniform mat4 g_ViewProj;
uniform vec2 g_WinSize;

////
// Fns.
////

void ExpandeFibers(vec3 p0, vec3 p1, vec3 t0, vec3 t1, float s0, float s1);

void main()
{
    ExpandeFibers(
        gs_Position[0],gs_Position[1],
        gs_Tangent[0],gs_Tangent[1],
        gs_Scale[0],gs_Scale[1]);
}

void ExpandeFibers(vec3 p0, vec3 p1, vec3 t0, vec3 t1, float s0, float s1)
{
    float ratio0 = s0, ratio1 = s1;

    // the thickness of the strand vary from root to tip using a function below

    float radius0 = g_HairRadius * ratio0 * HAIR_RADIUS_SCALE;
    float radius1 = g_HairRadius * ratio1 * HAIR_RADIUS_SCALE;

    t0 = normalize(t0);
    t1 = normalize(t1);

    vec3 v0 = p0;
    vec3 v1 = p1;

    vec3 viewDir0 = normalize(v0 - g_Eye);
    vec3 right0 = normalize(cross(t0, viewDir0));
    vec2 proj_right0 = normalize((g_ViewProj*vec4(right0, 0)).xy);
    
    vec3 viewDir1 = normalize(v1 - g_Eye);
    vec3 right1 = normalize(cross(t1, viewDir1));
    vec2 proj_right1 = normalize((g_ViewProj*vec4(right1, 0)).xy);
    

    float expandPixels = 0.71; //sqrt(2)/2

    vec2 winSizeRT = g_WinSize;

    vec3 tmp1;
    vec4 tmp2;

    tmp1 = p0 - right0 * radius0;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    gl_Position = vec4(tmp2.xyz/tmp2.w,1) - vec4(proj_right0*expandPixels/winSizeRT.y,0,0);
    fs_Position = v0;
    fs_P0 = v0;
    fs_P1 = v1;
    fs_Tangent = t0;
    fs_Scale = s0;
    EmitVertex();

    tmp1 = p1 - right1*radius1;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    gl_Position = vec4(tmp2.xyz/tmp2.w,1) - vec4(proj_right1*expandPixels/winSizeRT.y,0,0);
    fs_Position = v1;
    fs_P0 = v0;
    fs_P1 = v1;
    fs_Tangent = t1;
    fs_Scale = s1;
    EmitVertex();


    tmp1 = p0 + right0*radius0;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    gl_Position = vec4(tmp2.xyz/tmp2.w,1) + vec4(proj_right0*expandPixels/winSizeRT.y,0,0);
    fs_Position = v0;
    fs_P0 = v0;
    fs_P1 = v1;
    fs_Tangent  = t0;
    fs_Scale = s0;
    EmitVertex();

    tmp1 = p1 + right1*radius1;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    gl_Position = vec4(tmp2.xyz/tmp2.w,1) + vec4(proj_right1*expandPixels/winSizeRT.y,0,0);
    fs_Position = v1;
    fs_P0 = v0;
    fs_P1 = v1;
    fs_Tangent = t1;
    fs_Scale = s1;
    EmitVertex();

}
