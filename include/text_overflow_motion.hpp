#pragma once
#include <stdint.h>

namespace spire {
// A label pauses at both readable ends and moves in LCD-pixel increments.
// Its clock belongs to the visible label, so reopening starts at the beginning.
inline int text_overflow_offset(uint32_t elapsed,int overflow,unsigned pixels_per_second){
    if(overflow<=0||!pixels_per_second)return 0;
    constexpr uint32_t pause=1600;
    const uint32_t travel=(uint32_t(overflow)*1000+pixels_per_second-1)/pixels_per_second;
    const uint32_t clock=elapsed%(pause*2+travel*2);
    if(clock<pause)return 0;
    if(clock<pause+travel)return int((clock-pause)*pixels_per_second/1000);
    if(clock<pause*2+travel)return overflow;
    const unsigned returning=(clock-pause*2-travel)*pixels_per_second/1000;
    return returning<unsigned(overflow)?overflow-int(returning):0;
}
}
