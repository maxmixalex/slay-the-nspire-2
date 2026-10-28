#pragma once
#include <stdint.h>

namespace spire {
// Original clip-directory identities only; sampled geometry and pixels stay
// in the private resource pack. A shared row serves both body-scale endpoints.
// Layer words contain absolute image IDs, including reused whole-frame slots.
constexpr unsigned VantomUi15BaseAuxiliary=5288;
constexpr unsigned VantomUi15ParameterCount=116;
constexpr unsigned VantomUi15AuxiliaryCount=VantomUi15BaseAuxiliary+VantomUi15ParameterCount;
constexpr unsigned VantomUi15FirstClip=1383,VantomUi15ClipCount=232;
constexpr uint16_t VantomUi15Magic=0x5654;
constexpr unsigned VantomUi15HeaderWords=12,VantomUi15FrameWords=5,VantomUi15LayerWords=7,VantomUi15EmitterWords=14;
constexpr unsigned VantomUi15MaxLayers=64;
// Maximum individual topology in every original compound frame. The source
// converter rejects larger meshes; triangle/frame totals are separate limits.
constexpr unsigned VantomUi15MaxMeshVertices=114,VantomUi15MaxMeshTriangles=112;
constexpr unsigned VantomUi15TexturePagePixels=256*240;
static constexpr uint8_t VantomUi15ClipRows[]={
 0,128,1,129,2,130,3,131,4,132,5,133,6,134,7,135,
 8,9,136,137,10,11,138,139,12,13,140,141,14,15,142,143,
 16,144,17,145,18,146,19,147,20,21,22,23,148,149,150,151,
 24,25,26,27,152,153,154,155,28,29,30,31,156,157,158,159,
 32,33,34,35,160,161,162,163,36,164,37,165,38,166,39,167,
 40,168,41,169,42,170,43,171,44,172,45,173,46,174,47,175,
 48,176,49,177,50,178,51,179,52,53,180,181,54,55,182,183,
 56,57,184,185,58,59,186,187,60,61,188,189,62,63,190,191,
 64,65,192,193,66,67,194,195,68,69,196,197,70,71,198,199,
 72,73,200,201,74,75,202,203,76,77,204,205,78,79,206,207,
 80,81,208,209,82,83,210,211,84,85,212,213,86,87,214,215,
 88,89,216,217,90,91,218,219,92,93,220,221,94,95,222,223,
 96,97,224,225,98,99,226,227,100,101,228,229,102,103,230,231,
 104,105,232,233,106,107,234,235,108,109,236,237,110,111,238,239,
 112,113,240,241,114,115,242,243,
};
static_assert(sizeof(VantomUi15ClipRows)==VantomUi15ClipCount,"Complete original Vantom clip mapping");
inline int vantom_ui15_row(int clip){
 return clip>=int(VantomUi15FirstClip)&&clip<int(VantomUi15FirstClip+VantomUi15ClipCount)?VantomUi15ClipRows[clip-VantomUi15FirstClip]:-1;
}
}
