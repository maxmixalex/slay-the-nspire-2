#include "presentation_blend.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

static unsigned checks=0;
static void check(bool value){
    ++checks;if(!value){std::fprintf(stderr,"FAIL additive RGB565 check %u\n",checks);std::exit(1);}
}
static unsigned rounded(double value,unsigned maximum){
    const auto result=unsigned(std::floor(value+.5));return result>maximum?maximum:result;
}
int main(){
    using namespace spire::presentation;
    // Exhaust both RGB565 channel ranges and every alpha. This independent
    // normalized-color oracle catches early saturation and doubled alpha.
    for(unsigned maximum:{31u,63u})for(unsigned source=0;source<=maximum;++source)
        for(unsigned destination=0;destination<=maximum;++destination)
            for(unsigned alpha=0;alpha<=255;++alpha){
                const unsigned expected=rounded(destination+source*(double(alpha)/255.),maximum);
                check(add_channel(destination,source,alpha,maximum)==expected);
                check(add_covered_channel(destination,source,alpha,255,maximum)==expected);
            }
    // Coverage is a separate source-over intermediate; exercise empty and
    // partially transparent destinations rather than treating them as black.
    for(unsigned maximum:{31u,63u})for(unsigned source:{0u,1u,15u,31u})
        for(unsigned destination:{0u,1u,15u,31u})for(unsigned alpha:{0u,1u,63u,128u,254u,255u})
            for(unsigned previous:{0u,1u,63u,128u,254u,255u}){
                const double a=alpha/255.,b=previous/255.,total=a+b*(1-a);
                const unsigned expected=total?rounded((source*a+destination*b)/total,maximum):0;
                check(add_covered_channel(destination,source,alpha,previous,maximum)==expected);
            }
    check(add_opaque(0xf800,0x07e0,255)==0xffe0);
    check(add_opaque(0xffff,0xffff,255)==0xffff);
    check(add_opaque(0x1234,0xffff,0)==0x1234);
    check(add_opaque(0,0xffff,255)==0xffff);
    check(add_covered(0,0x07e0,128,0)==0x07e0);
    std::printf("%u additive RGB565 checks passed\n",checks);
}
