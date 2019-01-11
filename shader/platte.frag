#version 450 core

in vec3 fs_Position;
in vec3 fs_Normal;
in vec2 fs_TexCoord;

out vec4 FragColor;


layout(binding=0) uniform sampler2D g_ShadowMap;
layout(binding=1) uniform sampler2D g_DiffuseMap;
layout(binding=2) uniform sampler2D g_AlphaMap;

uniform int g_EnableAlphaToCoverage;
uniform vec3 g_Eye;
uniform float g_DepthOffset, g_MomentOffset;
uniform vec3 g_SunLightDir;
uniform mat4 g_LightViewProj;


float MSM_ComputeLitness(sampler2D shadowmap, mat4 light_view_proj, vec3 position);

void main()
{
    if (g_EnableAlphaToCoverage>0) {
        float alpha_mask = texture(g_AlphaMap,fs_TexCoord).r;
        if (alpha_mask < .5)
            discard;
    }

    vec3 sun_light_dir = normalize(g_SunLightDir);
    vec3 view_dir = normalize(g_Eye - fs_Position);
    vec3 normal = normalize(fs_Normal);
    float cosNL = dot(normal,sun_light_dir);
    float diffuse = max(cosNL,0.);

    vec3 diffuse_color = diffuse*texture(g_DiffuseMap,fs_TexCoord).rgb;

    float litness = MSM_ComputeLitness(g_ShadowMap, g_LightViewProj,fs_Position);

    FragColor = vec4(diffuse_color*litness,1.);
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
