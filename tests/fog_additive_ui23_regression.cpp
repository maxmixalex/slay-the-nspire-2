#ifdef NDEBUG
#undef NDEBUG
#endif
#include "nonu_fog_ui23.hpp"
#include "presentation_blend.hpp"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
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
 printf("Exact constant-color fog additive comparisons: %u\n",checks);
}
