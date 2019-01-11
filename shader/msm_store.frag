#version 450 core

out vec4 FragOutput;

vec4 MSM_OptimizedMoments(float depth);

void main() {
    vec4 moments = (MSM_OptimizedMoments(gl_FragCoord.z));
    FragOutput = moments;    
}

vec4 MSM_OptimizedMoments(float depth)
{
    float depth_sq = depth*depth;
    vec4 moments = vec4(depth,depth_sq,depth_sq*depth,depth_sq*depth_sq);
    // return moments;
    mat4 T = mat4(
        -2.07224649f,    13.7948857237f,  0.105877704f,   9.7924062118f,
        32.23703778f,  -59.4683975703f, -1.9077466311f, -33.7652110555f,
        -68.571074599f,  82.0359750338f,  9.3496555107f,  47.9456096605f,
        39.3703274134f,-35.364903257f,  -6.6543490743f, -23.9728048165f
    );
    vec4 opt_moments = T*moments;
    opt_moments[0] += 0.035955884801f;

    return clamp(opt_moments,0.,1.);
}
