#pragma once
#include "ancient_fire_ui23.hpp"
namespace spire::ancient_fire_grid {
// Only the procedural flame material is sampled on this UV grid. Node
// geometry, body frames, particles and the material clock retain their pace.
// Q15 channels include both exact endpoints. Q8 interpolation keeps the
// per-pixel products in 32 bits on ARM926 and uses the existing scratch lease.
// A 33x33 trial still spent 117.8ms/render on the formal 64MiB guest. The
// projected flame quads are small; sixteen cells preserve their smooth UV
// contour while reducing full material evaluations from3267 to867/frame.
#ifndef SPIRE_ANCIENT_FIRE_GRID_BITS
#define SPIRE_ANCIENT_FIRE_GRID_BITS 4
#endif
constexpr unsigned Bits=SPIRE_ANCIENT_FIRE_GRID_BITS,Shift=20-Bits,Cells=1u<<Bits,Side=Cells+1,Pixels=Side*Side,Models=3;
struct Texel {uint16_t inner,alpha;};
static_assert(Bits>=3&&Bits<=5&&sizeof(Texel)*Pixels*Models<=13068,"Flame lookup is a bounded transient, not another image pool");
inline Texel quantize(ancient_fire_ui23::Pixel p){return {uint16_t((p.inner+16)>>5),uint16_t((p.alpha+16)>>5)};}
inline unsigned channel(unsigned a,unsigned b,unsigned c,unsigned d,unsigned fx,unsigned fy){
 const int top=int(a)*256+(int(b)-int(a))*int(fx),bottom=int(c)*256+(int(d)-int(c))*int(fx);
 const uint32_t weighted=(uint32_t(top)<<8)+uint32_t(bottom-top)*fy;
 return ((weighted+32768u)>>16)<<5;
}
inline ancient_fire_ui23::Pixel sample(const Texel*grid,unsigned u,unsigned v){
 const unsigned x=u>>Shift,y=v>>Shift,fx=(u&((1u<<Shift)-1))>>(Shift-8),fy=(v&((1u<<Shift)-1))>>(Shift-8);
 const auto&a=grid[y*Side+x];const auto&b=grid[y*Side+x+1];
 const auto&c=grid[(y+1)*Side+x];const auto&d=grid[(y+1)*Side+x+1];
 return {channel(a.inner,b.inner,c.inner,d.inner,fx,fy),channel(a.alpha,b.alpha,c.alpha,d.alpha,fx,fy)};
}
}
