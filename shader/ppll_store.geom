#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=4) out;


////
// Ins & Outs.
////

in vec3 gs_Position[];
in vec4 gs_Tangent[];

out vec3 fs_Position;
out vec4 fs_Tangent;
out vec4 fs_WinE0E1;

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

void ExpandeFibers(vec3 p0, vec3 p1, vec4 t0, vec4 t1);

void main()
{
    ExpandeFibers(
        gs_Position[0],gs_Position[1],
        gs_Tangent[0],gs_Tangent[1]);
}

void ExpandeFibers(vec3 p0, vec3 p1, vec4 t0, vec4 t1)
{

    // the thickness of the strand vary from root to tip using a function below

    float ratio0 = fract(t0.w), ratio1 = fract(t1.w);

    float radius_scale = 1./max(g_WinSize.x,g_WinSize.y);

    float radius0 = g_HairRadius * ratio0 * radius_scale;
    float radius1 = g_HairRadius * ratio1 * radius_scale;

    vec3 nt0 = normalize(t0.xyz);
    vec3 nt1 = normalize(t1.xyz);

    vec3 viewDir0 = normalize(p0 - g_Eye);
    vec3 right0 = normalize(cross(nt0, viewDir0));
    vec2 proj_right0 = normalize((g_ViewProj*vec4(right0, 0)).xy);
    
    vec3 viewDir1 = normalize(p1 - g_Eye);
    vec3 right1 = normalize(cross(nt1, viewDir1));
    vec2 proj_right1 = normalize((g_ViewProj*vec4(right1, 0)).xy);

    float expandPixels = 0.71; //sqrt(2)/2

    vec3 tmp1;
    vec4 tmp2;

    tmp1 = p0 - right0 * radius0;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    vec4 e0_root = vec4(tmp2.xyz/tmp2.w,1) - vec4(proj_right0*expandPixels/g_WinSize.y,0,0);

    tmp1 = p1 - right1*radius1;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    vec4 e0_tip = vec4(tmp2.xyz/tmp2.w,1) - vec4(proj_right1*expandPixels/g_WinSize.y,0,0);

    tmp1 = p0 + right0*radius0;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    vec4 e1_root = vec4(tmp2.xyz/tmp2.w,1) + vec4(proj_right0*expandPixels/g_WinSize.y,0,0);

    tmp1 = p1 + right1*radius1;
    tmp2 = g_ViewProj*vec4(tmp1,1);
    vec4 e1_tip = vec4(tmp2.xyz/tmp2.w,1) + vec4(proj_right1*expandPixels/g_WinSize.y,0,0);

    // Fixed: Quad may be rendered as a butterfly-shape.
    if (dot(proj_right0,proj_right1)<0) {
        vec4 tmp = e1_tip;
        e1_tip = e0_tip;
        e0_tip = tmp;
    }

    ////
    // Emit verts.
    ////

    fs_Position = p0;
    fs_Tangent = t0;
    fs_WinE0E1 = vec4(e0_root.xy,e1_root.xy);
    gl_Position = e0_root;
    EmitVertex();

    fs_Position = p1;
    fs_Tangent = t1;
    fs_WinE0E1 = vec4(e0_tip.xy,e1_tip.xy);
    gl_Position = e0_tip;
    EmitVertex();

    fs_Position = p0;
    fs_Tangent  = t0;
    fs_WinE0E1 = vec4(e0_root.xy,e1_root.xy);
    gl_Position = e1_root;
    EmitVertex();

    fs_Position = p1;
    fs_Tangent = t1;
    fs_WinE0E1 = vec4(e0_tip.xy,e1_tip.xy);
    gl_Position = e1_tip;
    EmitVertex();


}
