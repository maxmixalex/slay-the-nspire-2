#pragma once
#include <stdint.h>
namespace spire { namespace crystal_shader {
// Both source shader rectangles have fixed projected dimensions. Their UV,
// grid and edge coordinates depend on no clock, cell or save state; evaluate
// the exact integer mapping at compile time instead of dividing per frame.
struct AxisSample {int8_t cell;uint8_t sample,low,high;};
template<unsigned Size> struct Axis {AxisSample pixels[Size]{};};
template<unsigned Size> constexpr Axis<Size> make_axis(){
 Axis<Size> out{};
 for(unsigned i=0;i<Size;++i){
  const int value=int((2*i+1)*1419)-int(348*Size),denom=200*Size;
  const int cell=value>=0?value/denom:-((-value+denom-1)/denom),fraction=(value-cell*denom)*1000/denom;
  const int low=fraction>=50?0:(50-fraction)*255/30,high=fraction<=950?0:(fraction-950)*255/30;
  out.pixels[i]={int8_t(cell),uint8_t((2*i+1)*160/(2*Size)),uint8_t(low<255?low:255),uint8_t(high<255?high:255)};
 }return out;
}
inline constexpr auto OuterX=make_axis<158>();
inline constexpr auto OuterY=make_axis<157>();
inline constexpr auto FogX=make_axis<159>();
inline constexpr auto FogY=make_axis<160>();
struct Phase {uint32_t one=0,two=0;};
constexpr Phase phase(uint32_t now){return {uint32_t(uint64_t(now)*425984/1000000),uint32_t(uint64_t(now)*720896/10000000)};}
static_assert(sizeof(AxisSample)==4&&sizeof(OuterX)+sizeof(OuterY)+sizeof(FogX)+sizeof(FogY)==2536,"Exact bounded crystal coordinate storage");
}}
