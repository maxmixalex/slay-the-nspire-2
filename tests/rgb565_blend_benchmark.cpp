// Isolated native operation only; ordinary-OS measurements are separate.
#include "presentation_blend.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>

static uint16_t background[320*240],foreground[320*240];
static uint8_t alpha[320*240],coverage[320*240];
static volatile uint32_t sink;
static uint16_t previous(uint16_t a,uint16_t b,unsigned opacity){
    const unsigned inverse=255-opacity;
    const unsigned first=(a>>11)|(unsigned(a&31)<<16),second=(b>>11)|(unsigned(b&31)<<16);
    unsigned lanes=first*inverse+second*opacity+0x00800080u;
    lanes=(lanes+((lanes>>8)&0x00ff00ffu))>>8;lanes&=0x001f001fu;
    const unsigned green=spire::presentation::divide255(((a>>5)&63)*inverse+((b>>5)&63)*opacity);
    return uint16_t((lanes<<11)|(green<<5)|(lanes>>16));
}
template<bool Current,bool Covered> __attribute__((noinline)) static void draw(){
    for(unsigned i=0;i<320*240;++i)
        if constexpr(!Covered)
            background[i]=Current?spire::presentation::source_over_rgb565(background[i],foreground[i],alpha[i]):previous(background[i],foreground[i],alpha[i]);
        else{
            const unsigned a=alpha[i],remaining=(coverage[i]*(255-a)+127)/255;
            const unsigned total=a+remaining,round=total/2;
            const unsigned red=(foreground[i]>>11)*a+(background[i]>>11)*remaining+round;
            const unsigned green=((foreground[i]>>5)&63)*a+((background[i]>>5)&63)*remaining+round;
            const unsigned blue=(foreground[i]&31)*a+(background[i]&31)*remaining+round;
            if constexpr(Current){
                const unsigned reciprocal=spire::presentation::CoverageDivision.values[total];
                background[i]=uint16_t((spire::presentation::coverage_quotient(red,total,reciprocal)<<11)|
                    (spire::presentation::coverage_quotient(green,total,reciprocal)<<5)|
                    spire::presentation::coverage_quotient(blue,total,reciprocal));
            }else background[i]=uint16_t(((red/total)<<11)|((green/total)<<5)|(blue/total));
        }
}
struct Result{double milliseconds;uint32_t hash;};
template<bool Current,bool Covered> static Result measure(){
    for(unsigned i=0;i<320*240;++i){
        background[i]=uint16_t(i*4793u+0x185fu);
        foreground[i]=uint16_t(i*1327u+(i>>3));
        alpha[i]=Covered?uint8_t(2+(i*37u+(i>>4))%252):uint8_t(i*37u+(i>>4));
        coverage[i]=uint8_t(1+(i*71u+(i>>5))%253);
    }
    uint32_t hash=2166136261u;
    const auto begin=std::chrono::steady_clock::now();
    for(unsigned frame=0;frame<240;++frame){
        draw<Current,Covered>();
        for(unsigned i=frame;i<320*240;i+=997)hash=(hash^background[i])*16777619u;
    }
    const auto end=std::chrono::steady_clock::now();sink=hash;
    return {std::chrono::duration<double,std::milli>(end-begin).count(),hash};
}
template<bool Covered> static bool compare(const char*operation){
    double before[7],after[7];uint32_t hash=0;
    for(unsigned run=0;run<7;++run){
        Result a,b;if(run&1){b=measure<true,Covered>();a=measure<false,Covered>();}
        else{a=measure<false,Covered>();b=measure<true,Covered>();}
        if(a.hash!=b.hash){std::fprintf(stderr,"%s benchmark output differs\n",operation);return false;}
        before[run]=a.milliseconds;after[run]=b.milliseconds;hash=a.hash;
    }
    std::sort(before,before+7);std::sort(after,after+7);
    std::printf("{\"operation\":\"%s\",\"frames\":240,\"runs\":7,\"previous_median_ms\":%.6f,\"current_median_ms\":%.6f,\"hash\":\"%08x\"}\n",operation,before[3],after[3],hash);
    return true;
}
int main(){return compare<false>("opaque_source_over")&&compare<true>("cached_coverage")?0:1;}
