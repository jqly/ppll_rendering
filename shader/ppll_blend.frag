#version 450 core

////
// Defines
////

#define PPLL_NULL 0xffffffff
#define KBUF_SIZE 32
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

uint PPLL_GetDepth(uint node_addr)
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


vec4 PPLL_Blend(uint node_addr)
{
    int num_nodes = 0;

    struct KBufElem {
        vec4 color;
        uint depth;
    };

    KBufElem kbuf[KBUF_SIZE];
    for (int kthnode = 0; kthnode < KBUF_SIZE; ++kthnode)
        kbuf[kthnode].depth = 0xffffffff;

    for (int kthnode = 0; kthnode < KBUF_SIZE; ++kthnode) {
        if (node_addr == PPLL_NULL)
            break;

        kbuf[kthnode].color = PPLL_GetColor(node_addr);
        kbuf[kthnode].depth = PPLL_GetDepth(node_addr);

        node_addr = PPLL_GetNext(node_addr);
    }

    for (;node_addr!=PPLL_NULL;node_addr = PPLL_GetNext(node_addr)) {

        vec4 node_color = PPLL_GetColor(node_addr);
        uint node_depth = PPLL_GetDepth(node_addr);

        for (int i = 0; i < KBUF_SIZE; ++i) {
            if (kbuf[i].depth > node_depth) {
                kbuf[i].depth = node_depth;
                kbuf[i].color = node_color;
                break;
            }
        }

    }

    vec4 color = vec4(0,0,0,1);

    for (int kth_blend = 0; kth_blend < KBUF_SIZE; ++kth_blend) {

        uint maxnode = 0;
        uint maxdepth = 0;

        for (int j = 0; j < KBUF_SIZE; ++j) {
            if (kbuf[j].depth > maxdepth) {
                maxdepth = kbuf[j].depth;
                maxnode = j;
            }
        }

        if (maxdepth==0)
            break;

        kbuf[maxnode].depth = 0;
        if (maxdepth!=0xffffffff) {
            vec4 node_color = kbuf[maxnode].color;

            color.rgb = (1.-node_color.a)*color.rgb+node_color.a*node_color.rgb;
            color.a = color.a*clamp(1.-node_color.a,0.,1.);
        }
    }
    return color;
}

void main()
{
    uint head_node_addr = PPLL_GetHeadNodeAddr(ivec2(gl_FragCoord.xy));
    if (head_node_addr!=PPLL_NULL)
        HairColor = PPLL_Blend(head_node_addr);
    else
        discard;
}
