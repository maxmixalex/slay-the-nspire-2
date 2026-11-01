#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nonu_fog_ui23.hpp"
#include "presentation_blend.hpp"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include <array>
int main(){
 unsigned checks=0;
 for(unsigned bits:{5u,6u}){
  const unsigned maximum=(1u<<bits)-1,shift=bits==5?11:5;
  for(unsigned src=0;src<=maximum;++src)for(unsigned dst=0;dst<=maximum;++dst)
   for(unsigned alpha=0;alpha<256;++alpha){
    const auto source=uint16_t(src<<shift),destination=uint16_t(dst<<shift);
    const auto amount=spire::presentation::add_opaque(0,source,alpha);
    assert(spire::nonu_fog_ui23::add_weighted(destination,amount)==spire::presentation::add_opaque(destination,source,alpha));++checks;
   }
 }
 uint32_t state=0x1234fedcu;
 for(unsigned i=0;i<65536;++i){state=state*1664525u+1013904223u;
  const uint16_t source=uint16_t(i),destination=uint16_t(state>>16);const unsigned alpha=state&255u;
  assert(spire::nonu_fog_ui23::add_weighted(destination,spire::presentation::add_opaque(0,source,alpha))==
    spire::presentation::add_opaque(destination,source,alpha));++checks;
 }
 // Row recurrence must match signed division exactly across both numerator
 // and determinant signs, zero crossings and the full240-row viewport.
 for(int64_t denominator:{-321987654321ll,-4294967296ll,-1234567ll,-1ll,1ll,1234567ll,4294967296ll,321987654321ll})
  for(int64_t initial:{-900000000000000ll,-4294967297ll,-1ll,0ll,1ll,4294967297ll,900000000000000ll})
   for(int64_t step:{-1432109876543ll,-4294967296ll,-1ll,0ll,1ll,4294967296ll,1432109876543ll}){
    spire::nonu_fog_ui23::AffineRows rows(initial,step,denominator);
    for(unsigned y=0;y<240;++y){assert(rows.value()==int((initial+int64_t(y)*step)/denominator));rows.next();++checks;}
   }
 std::array<uint8_t,40000> pixels{};for(unsigned i=0;i<pixels.size();++i)pixels[i]=uint8_t(i*67+i/199);
 for(unsigned v=0;v<16777216u;v+=7919)for(unsigned u=0;u<16777216u;u+=123457){
  const int x=int((int64_t(u)*200)>>16)-128,y=int((int64_t(v)*200)>>16)-128;const int ix=x>>8,iy=y>>8;const unsigned fx=unsigned(x)&255u,fy=unsigned(y)&255u;
  auto clip=[](int p){return p<0?0:p>199?199:p;};
  unsigned top=pixels[clip(iy)*200+clip(ix)]*(256-fx)+pixels[clip(iy)*200+clip(ix+1)]*fx;
  unsigned bottom=pixels[clip(iy+1)*200+clip(ix)]*(256-fx)+pixels[clip(iy+1)*200+clip(ix+1)]*fx;
  assert(spire::nonu_fog_ui23::alpha(pixels.data(),int(u),int(v))==(top*(256-fy)+bottom*fy+32768)>>16);++checks;
 }
 printf("Exact constant-color fog additive comparisons: %u\n",checks);
}
