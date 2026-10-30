#pragma once
#include <stdint.h>

namespace spire {
constexpr unsigned RetainedPlanesContentVersion=43;
constexpr uint32_t PlanarRetainedGroupMagic=0x33524752u; // RGR3
enum class RetainedPlanesResult:uint8_t {Pending,Ready,Failed};
struct RetainedPlanesCursor {
    uint32_t scan=0,total=0,pixels=0,alphas=0,rows=0,spans=0,previous=0;
    uint32_t geometry=0,color_at=0,alpha_at=0,output=0,colors=0;
    uint8_t stride=0,phase=0;
};
inline unsigned retained_planes_word(const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline unsigned retained_planes_rgb(unsigned packed,unsigned mode){
    if(!mode)return packed;
    const unsigned green=packed&63u;
    unsigned red=(packed>>6)&31u,blue=packed>>11;
    if(mode==2){red=(red>>1)^(0u-(red&1u));blue=(blue>>1)^(0u-(blue&1u));}
    return (((red+(green>>1))&31u)<<11)|(green<<5)|((blue+(green>>1))&31u);
}
// Source and destination are distinct existing cache/transfer reservations.
// Each call scans or writes at most budget bytes, apart from one 5-byte header.
inline RetainedPlanesResult step_retained_planes(RetainedPlanesCursor&c,uint8_t*out,
    const uint8_t*in,unsigned bytes,unsigned mode,unsigned budget){
    if(!out||!in||out==in||!bytes||bytes>512u*1024u||mode>2||budget<5)
        return RetainedPlanesResult::Failed;
    if(c.phase==2)return RetainedPlanesResult::Ready;
    while(budget>=5){
        if(c.phase==0){
            if(c.spans){
                if(c.scan>bytes||bytes-c.scan<5)return RetainedPlanesResult::Failed;
                const unsigned x=retained_planes_word(in+c.scan),n=retained_planes_word(in+c.scan+2),alpha=in[c.scan+4];
                if(!n||alpha>1||x<c.previous||x>320||n>320-x||c.total>bytes||5+n*(2+alpha)>bytes-c.total)
                    return RetainedPlanesResult::Failed;
                c.scan+=5;c.total+=5+n*(2+alpha);c.pixels+=n;c.alphas+=n*alpha;
                c.previous=x+n;--c.spans;budget-=5;continue;
            }
            if(c.total==bytes){
                if(c.scan+c.pixels*2+c.alphas!=bytes)return RetainedPlanesResult::Failed;
                c.geometry=c.scan;c.color_at=c.scan;c.alpha_at=c.scan+c.pixels*2;
                c.scan=c.rows=c.previous=0;c.phase=1;continue;
            }
            if(c.total>bytes||bytes-c.total<2||c.scan>bytes||bytes-c.scan<2||c.rows>=240)
                return RetainedPlanesResult::Failed;
            c.spans=retained_planes_word(in+c.scan);if(c.spans>320)return RetainedPlanesResult::Failed;
            c.scan+=2;c.total+=2;++c.rows;c.previous=0;budget-=2;continue;
        }
        if(c.colors){
            unsigned n=c.colors;if(n>budget/c.stride)n=budget/c.stride;
            if(c.color_at>bytes||n>(bytes-c.color_at)/2||c.output>bytes||n>(bytes-c.output)/c.stride||
               (c.stride==3&&(c.alpha_at>bytes||n>bytes-c.alpha_at)))return RetainedPlanesResult::Failed;
            for(unsigned i=0;i<n;++i){
                const unsigned rgb=retained_planes_rgb(retained_planes_word(in+c.color_at),mode);c.color_at+=2;
                out[c.output++]=uint8_t(rgb);out[c.output++]=uint8_t(rgb>>8);
                if(c.stride==3)out[c.output++]=in[c.alpha_at++];
            }
            c.colors-=n;budget-=n*c.stride;if(c.colors)return RetainedPlanesResult::Pending;continue;
        }
        if(c.spans){
            if(c.scan>c.geometry||c.geometry-c.scan<5||c.output>bytes||bytes-c.output<5)
                return RetainedPlanesResult::Failed;
            c.colors=retained_planes_word(in+c.scan+2);c.stride=uint8_t(2+in[c.scan+4]);
            for(unsigned i=0;i<5;++i)out[c.output++]=in[c.scan++];
            --c.spans;budget-=5;continue;
        }
        if(c.scan==c.geometry){
            if(c.output!=bytes||c.color_at!=c.geometry+c.pixels*2||c.alpha_at!=bytes)return RetainedPlanesResult::Failed;
            c.phase=2;return RetainedPlanesResult::Ready;
        }
        if(c.scan>c.geometry||c.geometry-c.scan<2||c.output>bytes||bytes-c.output<2)
            return RetainedPlanesResult::Failed;
        c.spans=retained_planes_word(in+c.scan);
        out[c.output++]=in[c.scan++];out[c.output++]=in[c.scan++];budget-=2;
    }
    return RetainedPlanesResult::Pending;
}
inline bool restore_retained_planes(uint8_t*out,const uint8_t*in,unsigned bytes,unsigned mode){
    RetainedPlanesCursor cursor{};RetainedPlanesResult result;
    do{result=step_retained_planes(cursor,out,in,bytes,mode,4096);}while(result==RetainedPlanesResult::Pending);
    return result==RetainedPlanesResult::Ready;
}
}
