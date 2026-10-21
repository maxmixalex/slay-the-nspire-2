#pragma once
#include <cstdint>

namespace spire { namespace presentation {
// Ordered rest-actor planes use straight alpha, except additive planes whose
// RGB already contains the summed emission and whose nonzero alpha is 255.
// Blend after multiplying image and layer opacity, exactly once. The main
// stage is opaque; its integer path needs neither floats nor a lookup table.
enum Blend : unsigned { Normal=0, Add=1 };

constexpr unsigned divide255(unsigned value){
    value+=128;return (value+(value>>8))>>8;
}
constexpr unsigned add_channel(unsigned destination,unsigned source,unsigned alpha,unsigned maximum){
    const unsigned result=destination+divide255(source*alpha);
    return result>maximum?maximum:result;
}
constexpr uint16_t add_opaque(uint16_t destination,uint16_t source,unsigned alpha){
    return uint16_t((add_channel(destination>>11,source>>11,alpha,31)<<11)|
        (add_channel((destination>>5)&63,(source>>5)&63,alpha,63)<<5)|
        add_channel(destination&31,source&31,alpha,31));
}

// Cached cards currently use normal blending exclusively. Preserve sensible
// source-over coverage if a future bounded intermediate canvas uses a plane:
// blend premultiplied contributions, then recover its straight-alpha color.
// Do not reuse that tile as a substitute for blending against a different
// background: additive light depends on the destination color.
constexpr unsigned add_covered_channel(unsigned destination,unsigned source,unsigned alpha,
                                       unsigned previous,unsigned maximum){
    const unsigned total=alpha*255+previous*(255-alpha);
    if(!total)return 0;
    const unsigned value=((source*alpha+destination*previous)*255+total/2)/total;
    return value>maximum?maximum:value;
}
constexpr uint16_t add_covered(uint16_t destination,uint16_t source,unsigned alpha,unsigned previous){
    return uint16_t((add_covered_channel(destination>>11,source>>11,alpha,previous,31)<<11)|
        (add_covered_channel((destination>>5)&63,(source>>5)&63,alpha,previous,63)<<5)|
        add_covered_channel(destination&31,source&31,alpha,previous,31));
}
}}
