#pragma once
#include <stdint.h>

namespace spire { namespace presentation {
// Ordered rest-actor planes use straight alpha, except additive planes whose
// RGB already contains the summed emission and whose nonzero alpha is 255.
// Blend after multiplying image and layer opacity, exactly once. The main
// stage is opaque; its integer path needs neither floats nor a lookup table.
enum Blend : unsigned { Normal=0, Add=1 };

constexpr unsigned divide255(unsigned value){
    value+=128;return (value+(value>>8))>>8;
}
// Neutral RGB modulation precedes compositing and never changes coverage.
// A white multiplier is exactly the existing RGB565 source, including when
// that source will be added to a partially covered destination.
constexpr uint16_t modulate_rgb(uint16_t source,unsigned brightness){
    return uint16_t((divide255((source>>11)*brightness)<<11)|
        (divide255(((source>>5)&63)*brightness)<<5)|
        divide255((source&31)*brightness));
}
// CanvasItem.Modulate may tint each source channel independently. Keep
// alpha separate; transparent texels and additive outlines retain coverage.
constexpr uint16_t tint_rgb(uint16_t source,unsigned red,unsigned green,unsigned blue){
    return uint16_t((divide255((source>>11)*red)<<11)|
        (divide255(((source>>5)&63)*green)<<5)|
        divide255((source&31)*blue));
}
// hsv.gdshader with its authored h=1: transform to YIQ, multiply chroma
// and value, transform back, then apply CanvasItem modulation. Keep values
// above white until AFTER modulation; an entering highlighted button must
// not clip its bright texture before its independent gray fade. Q12 color
// and the inverse matrix's first column avoid floating point on ARMv5.
inline uint16_t yiq_material(uint16_t source,unsigned saturation,unsigned value,
                             unsigned brightness=255,unsigned red=255,unsigned green=255,unsigned blue=255){
    const int r=int(((source>>11)*4096u+15)/31),g=int((((source>>5)&63)*4096u+31)/63),b=int(((source&31)*4096u+15)/31);
    const int luminance=(2989*r+5870*g+1140*b+5000)/10000;
    const auto channel=[&](int original,int inverse,unsigned tint,unsigned maximum){
        const int neutral=(luminance*inverse+32768)/65536;
        int component=neutral+(original-neutral)*int(saturation)/1000;
        if(component<0)component=0;
        unsigned q=unsigned(component)*value/1000;
        q=(q*brightness*tint+32512)/65025;
        if(q>4096)q=4096;
        return (q*maximum+2048)/4096;
    };
    return uint16_t((channel(r,65737,red,31)<<11)|(channel(g,65487,green,63)<<5)|channel(b,65318,blue,31));
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
