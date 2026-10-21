#include "presentation_blend.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

static unsigned checks=0;
static void check(bool value){
    ++checks;if(!value){std::fprintf(stderr,"FAIL presentation RGB565 check %u\n",checks);std::exit(1);}
}
static unsigned rounded(double value,unsigned maximum){
    const auto result=unsigned(std::floor(value+.5));return result>maximum?maximum:result;
}
int main(){
    using namespace spire::presentation;
    // Modulate the source before normal or additive composition. Neutral
    // white is byte-identical for every RGB565 color; black never implies
    // transparent coverage. Channel expectations use an independent oracle.
    for(unsigned source=0;source<=65535;++source){
        check(modulate_rgb(uint16_t(source),255)==source);
        check(modulate_rgb(uint16_t(source),0)==0);
        check(tint_rgb(uint16_t(source),255,255,255)==source);
        check(tint_rgb(uint16_t(source),0,0,0)==0);
    }
    for(unsigned level=0;level<=255;++level)for(unsigned value=0;value<=63;++value){
        const unsigned r=value&31,g=value,b=(value*7)&31;
        const uint16_t source=uint16_t((r<<11)|(g<<5)|b);
        const uint16_t expected=uint16_t((rounded(r*double(level)/255.,31)<<11)|
            (rounded(g*double(level)/255.,63)<<5)|rounded(b*double(level)/255.,31));
        check(modulate_rgb(source,level)==expected);
        check(tint_rgb(source,level,level,level)==expected);
    }
    // Independent per-channel oracle, including the original dark ancient
    // button tints and the non-gray additive focus outline. This detects
    // accidental channel swaps and applying one brightness to all channels.
    for(unsigned source:{0u,1u,0x39e7u,0xf800u,0x07e0u,0x001fu,0xffffu})
        for(unsigned red:{0u,13u,120u,255u})for(unsigned green:{0u,26u,191u,255u})
            for(unsigned blue:{0u,51u,254u,255u}){
                uint16_t expected=uint16_t((rounded((source>>11)*double(red)/255.,31)<<11)|
                    (rounded(((source>>5)&63)*double(green)/255.,63)<<5)|
                    rounded((source&31)*double(blue)/255.,31));
                check(tint_rgb(uint16_t(source),red,green,blue)==expected);
            }
    for(unsigned level:{0u,1u,127u,128u,254u,255u})for(unsigned source:{0u,0x39e7u,0xf81fu,0xffffu})
        for(unsigned destination:{0u,0x18c3u,0xffffu})for(unsigned alpha:{0u,1u,63u,128u,254u,255u}){
            const uint16_t color=modulate_rgb(uint16_t(source),level);
            // Source alpha is applied once after RGB modulation. A zero
            // source alpha preserves the opaque destination, even at black.
            for(unsigned previous:{0u,1u,63u,128u,254u,255u}){
                const double a=alpha/255.,b=previous/255.,total=a+b*(1-a);
                uint16_t expected=0;
                for(unsigned shift:{0u,5u,11u}){
                    const unsigned maximum=shift==5?63:31;
                    const unsigned src=(color>>shift)&maximum,dst=(destination>>shift)&maximum;
                    const unsigned value=total?rounded((src*a+dst*b)/total,maximum):0;
                    expected=uint16_t(expected|(value<<shift));
                }
                check(add_covered(uint16_t(destination),color,alpha,previous)==expected);
            }
            uint16_t expected=0;
            for(unsigned shift:{0u,5u,11u}){
                const unsigned maximum=shift==5?63:31;
                const unsigned src=(color>>shift)&maximum,dst=(destination>>shift)&maximum;
                expected=uint16_t(expected|(rounded(dst+src*(alpha/255.),maximum)<<shift));
            }
            check(add_opaque(uint16_t(destination),color,alpha)==expected);
            if(!alpha)check(add_opaque(uint16_t(destination),color,alpha)==destination);
        }
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
    std::printf("%u presentation RGB565 checks passed\n",checks);
}
