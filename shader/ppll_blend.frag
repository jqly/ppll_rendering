#version 450 core

////
// Defines
////

#define PPLL_NULL 0x3fffffff
#define SORT_PRE_K 8
////
// Ins & Outs.
////

out vec4 HairColor;

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

layout(binding=0,std430) 
buffer PPLL { PPLLNode g_PPLL[]; };

layout(binding=0,r32ui)
uniform uimage2D g_PPLLHeads;

uniform vec2 g_WinSize;
uniform uint g_NumNodes;
 
// PPLL helper functions.
vec4 UnpackUintIntoVec4(uint val)
{
    return vec4(
        float((val & 0xFF000000) >> 24) / 255.0, 
        float((val & 0x00FF0000) >> 16) / 255.0, 
        float((val & 0x0000FF00) >> 8) / 255.0, 
        float((val & 0x000000FF)) / 255.0);
}

uint PPLL_GetHeadNodeAddr(ivec2 win_addr)
{
    return imageLoad(g_PPLLHeads,win_addr).r;
}

uint PPLL_GetUDepth(uint node_addr)
{
    return g_PPLL[node_addr].depth;
}

vec4 PPLL_GetColor(uint node_addr)
{
    return UnpackUintIntoVec4(g_PPLL[node_addr].color);
}

uint PPLL_GetNext(uint node_addr)
{
    return g_PPLL[node_addr].next;
}

void main()
{
    uint node_addr = PPLL_GetHeadNodeAddr(ivec2(gl_FragCoord.xy));
    if (node_addr!=PPLL_NULL)
        HairColor = vec4(1,0,0,1);
    else
        HairColor = vec4(0,0,1,1);
}
