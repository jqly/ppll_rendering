#version 450 core

layout(early_fragment_tests) in;

////
// Defines
////

#define PPLL_NULL 0x3fffffff

////
// Ins & Outs.
////

in vec3 fs_Position;
in vec3 fs_Tangent;
in float fs_Scale;
in vec3 fs_P0;
in vec3 fs_P1;

out vec4 ColorResult;

////
// PPLL buffers and functions.
////

// PPLL node type
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

void main()
{
    vec4 hair_color = vec4(1,0,0,1);
    uint res = LinkNewNode(ivec2(gl_FragCoord.xy),hair_color,gl_FragCoord.z);
    if (res == PPLL_NULL)
        ColorResult = vec4(1,0,0,1);
    else
        ColorResult = vec4(0,1,0,1);
}
