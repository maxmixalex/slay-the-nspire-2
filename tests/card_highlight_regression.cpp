#include "card_highlight.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static unsigned long long checks;
static void check(bool value){++checks;if(!value){std::fprintf(stderr,"card highlight exact check %llu failed\n",checks);std::exit(1);}}
static unsigned scalar_smooth(unsigned value,unsigned maximum){
 if(!maximum||value>=maximum)return 65536;
 const auto q=uint64_t(value)*65536/maximum,q2=q*q/65536;
 return unsigned(q2*(3*65536-2*q)/65536);
}
static unsigned scalar_alpha(uint16_t alpha,unsigned width,uint32_t ms){
 if(!width)return 0;if(width>4915)width=4915;
 const unsigned source=unsigned(uint64_t(alpha)*980000/65535),ripple=((ms%2000)*30+source)%20000;
 const int brightness=int(source)-int(ripple)+int(uint64_t(scalar_smooth(ripple,5000))*20000/65536);
 const unsigned thickness=unsigned(uint64_t(width)*1000000/65536);
 const int distance=brightness-int(1000000-thickness);
 return distance<=0?0:scalar_smooth(unsigned(distance),thickness)*255/65536;
}
int main(){
 using namespace spire::card_highlight;
 for(unsigned alpha=0;alpha<65536;++alpha)check(source_alpha(uint16_t(alpha))==uint64_t(alpha)*980000/65535);
 for(unsigned denominator:{15u,30u,259u,5000u,65535u,65536u,74981u,75000u}){
  const Fraction fraction(denominator);
  for(unsigned value=0;value<=denominator+1;++value)check(fraction.smooth(value)==scalar_smooth(value,denominator));
 }
 // Every reachable width and all 16-bit out-of-range widths preserve the
 // exact inclusive boundary of the original early return. Its successor
 // must remain eligible, even when animation time will ultimately hide it.
 for(unsigned width=0;width<=65535;++width){const Context shader(width,0);
  const unsigned through=shader.transparent_through();
  const unsigned reference=unsigned((uint64_t(980000-shader.edge.divisor+1)*65535-1)/980000);
  check(through==reference);
  check(source_alpha(uint16_t(through))+20000u<=1000000u-shader.edge.divisor);
  if(through<65535)check(source_alpha(uint16_t(through+1))+20000u>1000000u-shader.edge.divisor);
 }
 for(unsigned width:{0u,1u,2u,17u,997u,4914u,4915u,65535u,0xffffffffu})
  for(uint32_t time:{0u,1u,37u,166u,333u,666u,1000u,1999u,2000u,0xffffffffu}){
   const Context shader(width,time);
   const unsigned transparent=shader.transparent_through();
   for(unsigned alpha=0;alpha<65536;++alpha){const unsigned reference=scalar_alpha(uint16_t(alpha),width,time);
    check(shader.alpha(uint16_t(alpha))==reference);
    check((alpha<=transparent?0:shader.alpha(uint16_t(alpha)))==reference);
   }
  }
 // Full visible-axis range, including left/top clipping, up/down sampling
 // and a partial terminal row. Compare against the old integer map exactly.
 for(unsigned source:{145u,181u})for(unsigned extent=1;extent<=384;++extent)
  for(unsigned start:{0u,extent/3,extent-1}){
   const unsigned step=source/extent,advance=source%extent;
   unsigned index=start*source/extent,remainder=start*source%extent;
   for(unsigned position=start;position<extent;++position){check(index==position*source/extent);index+=step;remainder+=advance;if(remainder>=extent){remainder-=extent;++index;}}
  }
 std::printf("card highlight: %llu exact scalar and clipping checks passed\n",checks);
}
