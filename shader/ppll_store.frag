#version 450 core

layout(early_fragment_tests) in;

////
// Defines
////

#define PPLL_NULL 0xffffffff

////
// Ins & Outs.
////

in vec3 fs_Position;
in vec4 fs_Tangent;
in vec4 fs_WinE0E1;

out vec4 ColorResult;

uniform vec3 g_Eye;
uniform float g_DepthOffset, g_MomentOffset;
uniform vec3 g_SunLightDir;
uniform mat4 g_LightViewProj;
layout(binding=0) uniform sampler2D g_ShadowMap;
layout(binding=1) uniform sampler2D g_HairBaseColorTex;
layout(binding=2) uniform sampler2D g_HairSpecOffsetTex;

struct PPLLNode {
    uint depth;
    uint data;
    uint color;
    uint next;
};

layout(binding=0,offset=0)
uniform atomic_uint g_Counter;

layout(binding=0,std430) 
buffer PPLL { PPLLNode g_PPLL[]; };

layout(binding=0,r32ui)
uniform uimage2D g_PPLLHeads;

uniform uint g_NumNodes;
uniform float g_HairTransparency;
uniform vec2 g_WinSize;

// PPLL helper functions.
uint PackVec4IntoUint(vec4 val)
{
    return (uint(val.x * 255) << 24) | (uint(val.y * 255) << 16) | (uint(val.z * 255) << 8) | uint(val.w * 255);
}
 
// Gather all fragments into PPLL.
uint LinkNewNode(ivec2 win_addr, vec4 color, float depth)
{
    uint node_addr = atomicCounterIncrement(g_Counter);
    if (node_addr >= g_NumNodes)
        return PPLL_NULL;

    uint prev_node_addr = imageAtomicExchange(g_PPLLHeads, win_addr, node_addr);

    g_PPLL[node_addr].depth = floatBitsToUint(depth);
    g_PPLL[node_addr].color = PackVec4IntoUint(color);
    g_PPLL[node_addr].next = prev_node_addr;
    
    return node_addr;
}

float ComputePixelCoverage(vec2 p0, vec2 p1, vec2 pixel_loc, vec2 win_size)
{
    p0 = (p0+1)*.5;
    p1 = (p1+1)*.5;
    p0 *= win_size;
    p1 *= win_size;

    float p0dist = length(p0 - pixel_loc);
    float p1dist = length(p1 - pixel_loc);
    float hairWidth = length(p0 - p1);

    // will be 1.f if pixel outside hair, 0.f if pixel inside hair
    float outside = max(step(hairWidth, p0dist), step(hairWidth, p1dist));

    // if outside, set sign to -1, else set sign to 1
    float sign = outside > 1e-3f ? -1.f : 1.f;

    // signed distance (positive if inside hair, negative if outside hair)
    float relDist = sign * clamp(min(p0dist, p1dist),0.,1.);

    // returns coverage based on the relative distance
    // 0, if completely outside hair edge
    // 1, if completely inside hair edge
    return (relDist + 1.f) * 0.5f;
}

float MSM_ComputeLitness(sampler2D shadowmap, mat4 light_view_proj, vec3 position);
vec3 HairShading(
    vec3 eye_dir, 
    vec3 light_dir, 
    vec3 tangent, 
    float scale,
    int hair_id,
    sampler2D base_color_tex,
    sampler2D spec_offset_tex);

void main()
{
    float cov = ComputePixelCoverage(fs_WinE0E1.xy,fs_WinE0E1.zw,gl_FragCoord.xy,g_WinSize);
    cov *= fract(fs_Tangent.w);

    float litness = MSM_ComputeLitness(g_ShadowMap,g_LightViewProj,fs_Position);

    vec3 hair_color = vec3(0.,0.,0.);

    if (litness > 1e-2)
        hair_color = HairShading(
            g_Eye - fs_Position,
            g_SunLightDir,
            fs_Tangent.xyz,
            fract(fs_Tangent.w),
            int(fs_Tangent.w),
            g_HairBaseColorTex,
            g_HairSpecOffsetTex
        );

    vec4 result = vec4(litness*hair_color,cov*g_HairTransparency);

    uint res = LinkNewNode(ivec2(gl_FragCoord.xy),result,gl_FragCoord.z);

    ColorResult = result;
}


vec4 MSM_ConvertOptimizedMoments(vec4 opt_moments)
{
    opt_moments[0] -= 0.035955884801f;
    mat4 T = mat4(
        0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
        0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f,
        0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
        0.0319417555f,-0.1722823173f,-0.2758014811f,-0.3376131734f
    );

    vec4 moments = T*opt_moments;

    return clamp(moments,0,1);
}

float MSM_ComputeLitness(
    vec4 moments, 
    float fragment_depth, 
    float depth_bias, 
    float moment_bias)
{
    vec4 b = mix(moments,vec4(.5,.5,.5,.5),moment_bias);

    vec3 z;
    z[0]=fragment_depth - depth_bias;

    // OpenGL 4 only - fma has higher precision:
    float l32_d22 = fma(-b.x, b.y, b.z); // a * b + c
    float d22 = fma(-b.x, b.x, b.y);     // a * b + c
    float squared_depth_variance = fma(-b.y, b.y, b.w); // a * b + c
    
    float d33_d22 = dot(
        vec2(squared_depth_variance, -l32_d22), vec2(d22, l32_d22));
    float inv_d22 = 1. - d22;
    float l32 = l32_d22 * inv_d22;


    vec3 c = vec3(1., z[0], z[0] * z[0]);
    c.y -= b.x;
    c.z -= b.y + l32 * c.y;
    c.y *= inv_d22;
    c.z *= d22 / d33_d22;
    c.y -= l32 * c.z;
    c.x -= dot(c.yz, b.xy);
    
    float inv_c2 = 1. / c.z;
    float p = c.y * inv_c2;
    float q = c.x * inv_c2;
    float r = sqrt((p * p * .25) - q);

    z[1] = -p * .5 - r;
    z[2] = -p * .5 + r;

    vec4 tmp=
        (z[2]<z[0])?vec4(z[1],z[0],1.,1.):
        ((z[1]<z[0])?vec4(z[0],z[1],0.,1.):
        vec4(0.,0.,0.,0.));
    float quotient=(
        tmp[0]*z[2]-b[0]*(tmp[0]+z[2])+b[1])/((z[2]-tmp[1])*(z[0]-z[1]));
    
    // Divide to reduce light leaking (a little).
    float shadowness =  (tmp[2]+tmp[3]*quotient)/.98;
    shadowness = clamp(shadowness,0.,1.);
    return 1. - shadowness;
}

float MSM_ComputeLitness(sampler2D shadowmap, mat4 light_view_proj, vec3 position)
{
    vec4 tmp = light_view_proj * vec4(position,1.);
    vec3 light_view_position = tmp.xyz / tmp.w;
    light_view_position = .5*light_view_position+.5;

    vec4 moments = MSM_ConvertOptimizedMoments(
        texture(shadowmap,light_view_position.xy));

    // Make shadow less noise.
    vec4 moments_ = mix(moments,vec4(0,.63,0,.63),3e-5);

    return MSM_ComputeLitness(
        moments_,
        light_view_position.z,
        g_DepthOffset*.01,
        g_MomentOffset*.01);
}

vec3 HairShading(
    vec3 eye_dir, 
    vec3 light_dir, 
    vec3 tangent, 
    float scale,
    int hair_id,
    sampler2D base_color_tex,
    sampler2D spec_offset_tex)
{

    vec2 hair_tex_idx = vec2(float(hair_id%100)/100., scale);
    hair_tex_idx = clamp(hair_tex_idx,.1,.9);

    vec3 hair_base_color = texture(base_color_tex,hair_tex_idx).rgb;

    float randn = texture(spec_offset_tex,hair_tex_idx).r;
    vec3 randv = vec3(.99,1.03,.97)*vec3(randn,randn,randn);

    // define baseColor and Ka Kd Ks coefficient for hair
    float Ka = 0.5, Kd = .5, Ks1 = .12, Ex1 = 24, Ks2 = .16, Ex2 = 6.;
    // float Ka = 0., Kd = .4, Ks1 = .4, Ex1 = 80, Ks2 = .5, Ex2 = 8.;

    light_dir = normalize(light_dir);
    eye_dir = normalize(eye_dir);
    tangent = normalize(tangent);// + In.Tangent*randv*4);

    // in Kajiya's model: diffuse component: sin(t, l)
    float cosTL = (dot(tangent, light_dir));
    float sinTL = sqrt(1 - cosTL*cosTL);
    float vDiffuse = sinTL; // here sinTL is apparently larger than 0

    float alpha = (randn*10.)*3.1415926/180.; // tiled angle (5-10 dgree)

    // in Kajiya's model: specular component: cos(t, rl)*cos(t, e) + sin(t, rl)sin(t, e)
    float cosTRL = -cosTL;
    float sinTRL = sinTL;
    float cosTE = (dot(tangent, eye_dir));
    float sinTE = sqrt(1- cosTE*cosTE);

    // primary highlight: reflected direction shift towards root (2*Alpha)
    float cosTRL_r = cosTRL*cos(2*alpha) - sinTRL*sin(2*alpha);
    float sinTRL_r = sqrt(1 - cosTRL_r*cosTRL_r);
    float vSpecular_r = max(0, cosTRL_r*cosTE + sinTRL_r*sinTE);

    // secondary highlight: reflected direction shifted toward tip (3*Alpha)
    float cosTRL_trt = cosTRL*cos(-3*alpha) - sinTRL*sin(-3*alpha);
    float sinTRL_trt = sqrt(1 - cosTRL_trt*cosTRL_trt);
    float vSpecular_trt = max(0, cosTRL_trt*cosTE + sinTRL_trt*sinTE);
    
    vec3 vColor = Ka * hair_base_color + // ambient
                    1. * vec3(1,1,1) * (
                    Kd * vDiffuse* hair_base_color + // diffuse
                    Ks1 * pow(vSpecular_r, Ex1)  + // primary hightlight r
                    Ks2 * pow(vSpecular_trt, Ex2) * hair_base_color); // secondary highlight rtr 
    
    return vColor;
}
