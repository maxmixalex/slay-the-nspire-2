// Isolated native operation only; ordinary-OS measurements are separate.
#include "presentation_blend.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>

static uint16_t background[320*240],foreground[320*240];
static uint8_t alpha[320*240];
static volatile uint32_t sink;
static uint16_t previous(uint16_t a,uint16_t b,unsigned opacity){
    const unsigned inverse=255-opacity;
    const unsigned first=(a>>11)|(unsigned(a&31)<<16),second=(b>>11)|(unsigned(b&31)<<16);
    unsigned lanes=first*inverse+second*opacity+0x00800080u;
    lanes=(lanes+((lanes>>8)&0x00ff00ffu))>>8;lanes&=0x001f001fu;
    const unsigned green=spire::presentation::divide255(((a>>5)&63)*inverse+((b>>5)&63)*opacity);
    return uint16_t((lanes<<11)|(green<<5)|(lanes>>16));
}
template<bool Current> __attribute__((noinline)) static void draw(){
    for(unsigned i=0;i<320*240;++i)
        background[i]=Current?spire::presentation::source_over_rgb565(background[i],foreground[i],alpha[i]):previous(background[i],foreground[i],alpha[i]);
}
struct Result{double milliseconds;uint32_t hash;};
template<bool Current> static Result measure(){
    for(unsigned i=0;i<320*240;++i){
        background[i]=uint16_t(i*4793u+0x185fu);
        foreground[i]=uint16_t(i*1327u+(i>>3));alpha[i]=uint8_t(i*37u+(i>>4));
    }
    uint32_t hash=2166136261u;
    const auto begin=std::chrono::steady_clock::now();
    for(unsigned frame=0;frame<240;++frame){
        draw<Current>();
        for(unsigned i=frame;i<320*240;i+=997)hash=(hash^background[i])*16777619u;
    }
    const auto end=std::chrono::steady_clock::now();sink=hash;
    return {std::chrono::duration<double,std::milli>(end-begin).count(),hash};
}
int main(){
    double before[7],after[7];uint32_t hash=0;
    for(unsigned run=0;run<7;++run){
        Result a,b;if(run&1){b=measure<true>();a=measure<false>();}
        else{a=measure<false>();b=measure<true>();}
        if(a.hash!=b.hash){std::fprintf(stderr,"blend benchmark output differs\n");return 1;}
        before[run]=a.milliseconds;after[run]=b.milliseconds;hash=a.hash;
    }
    std::sort(before,before+7);std::sort(after,after+7);
    std::printf("{\"frames\":240,\"runs\":7,\"previous_median_ms\":%.6f,\"current_median_ms\":%.6f,\"hash\":\"%08x\"}\n",before[3],after[3],hash);
}
