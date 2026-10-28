#pragma once
#include <stdint.h>

namespace spire {
constexpr unsigned RetainedFilterContentVersion=36;
constexpr uint32_t FilteredRetainedGroupMagic=0x32524752u; // RGR2

namespace retained_filter_detail {
inline unsigned word(const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline uint32_t dword(const uint8_t*p){return uint32_t(word(p))|(uint32_t(word(p+2))<<16);}
template<unsigned Mode,unsigned Stride> inline void colors(uint8_t*data,unsigned count){
    for(unsigned i=0;i<count;++i,data+=Stride){
        const unsigned packed=word(data),green=packed&63u;
        unsigned red=(packed>>6)&31u,blue=packed>>11;
        if(Mode==2){red=(red>>1)^(0u-(red&1u));blue=(blue>>1)^(0u-(blue&1u));}
        const unsigned rgb=(((red+(green>>1))&31u)<<11)|(green<<5)|((blue+(green>>1))&31u);
        data[0]=uint8_t(rgb);data[1]=uint8_t(rgb>>8);
    }
}
template<unsigned Mode> inline bool restore(uint8_t*data,unsigned bytes,unsigned format){
    if(format==0){if(bytes&1u)return false;colors<Mode,2>(data,bytes/2);return true;}
    // Format 1 has a contiguous RGB565 plane followed by its alpha plane.
    // Only sparse format 4 interleaves alpha with the two color bytes.
    if(format==1){if(bytes%3u)return false;colors<Mode,2>(data,bytes/3);return true;}
    if(format<4){
        if(bytes<=512||(format==3&&((bytes-512)&1u)))return false;
        colors<Mode,2>(data,256);return true;
    }
    if(format!=4)return false;
    unsigned at=0,rows=0;
    while(at<bytes&&rows<240){
        if(bytes-at<2)return false;
        const unsigned spans=word(data+at);at+=2;
        if(spans>320)return false;
        unsigned previous=0;
        for(unsigned span=0;span<spans;++span){
            if(bytes-at<5)return false;
            const unsigned x=word(data+at),count=word(data+at+2),alpha=data[at+4];at+=5;
            if(!count||alpha>1||x<previous||x>320||count>320-x)return false;
            previous=x+count;
            const unsigned n=count*(2+alpha);
            if(n>bytes-at)return false;
            if(alpha)colors<Mode,3>(data+at,count);else colors<Mode,2>(data+at,count);
            at+=n;
        }
        ++rows;
    }
    return at==bytes;
}
}

// Runs once, in place, before the existing decoded group is published. The
// caller owns the encoded transfer lease containing the small slice table;
// neither the table nor the transformed colors need another resident pool.
inline bool restore_retained_colors(uint8_t*data,unsigned bytes,unsigned format,unsigned mode){
    if(!data||!bytes||bytes>512u*1024u||format>4||mode<1||mode>2)return false;
    return mode==2?retained_filter_detail::restore<2>(data,bytes,format):
                   retained_filter_detail::restore<1>(data,bytes,format);
}
inline bool valid_retained_slices(const uint8_t*table,unsigned count,unsigned bytes){
    if(!table||!count||count>bytes/2)return false;
    unsigned end=0;
    for(unsigned i=0;i<count;++i){
        const unsigned at=retained_filter_detail::dword(table+i*8);
        const unsigned shape=retained_filter_detail::dword(table+i*8+4),length=shape&0xffffffu;
        if(at!=end||!length||(shape>>24)>4||length>bytes-end)return false;
        end+=length;
    }
    return end==bytes;
}
inline bool restore_retained_group(uint8_t*data,unsigned bytes,const uint8_t*table,unsigned count,unsigned mode){
    if(!data||!bytes||bytes>512u*1024u||mode<1||mode>2||!valid_retained_slices(table,count,bytes))return false;
    for(unsigned i=0;i<count;++i){
        const unsigned at=retained_filter_detail::dword(table+i*8);
        const unsigned shape=retained_filter_detail::dword(table+i*8+4);
        if(!restore_retained_colors(data+at,shape&0xffffffu,shape>>24,mode))return false;
    }
    return true;
}
}
