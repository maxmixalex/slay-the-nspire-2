#pragma once
#include <stdint.h>

namespace spire {
constexpr unsigned TileFilterContentVersion=33;
constexpr uint32_t FilteredTileGroupMagic=0x32524754u; // TGR2

// The encoded scratch is no longer live after LZMA2 finishes. The caller copies
// its decoded bytes there, then restores the original 770-byte tile records in
// the already-reserved group pages. There is no additional resident buffer.
// Modes preserve every RGB565 bit (including fully transparent pixels):
// 1: green, red-minus-green/2, blue-minus-green/2; 2: zigzag the two differences.
template<unsigned Mode>
inline bool restore_tile_filter_records(uint8_t* output,const uint8_t* input,
    unsigned items){
    const uint8_t* colors=input+items*2u;
    const uint8_t* alpha=input+items*514u;
    for(unsigned tile=0;tile<items;++tile){
        const unsigned w=input[tile*2],h=input[tile*2+1];
        if(!w||w>16||!h||h>16)return false;
        uint8_t* record=output+tile*770u;record[0]=uint8_t(w);record[1]=uint8_t(h);
        const uint8_t* source=colors+tile*512u;
        for(unsigned pixel=0;pixel<256;++pixel){
            const unsigned packed=unsigned(source[pixel*2])|(unsigned(source[pixel*2+1])<<8);
            const unsigned green=packed&63u;
            unsigned red=(packed>>6)&31u,blue=packed>>11;
            if(Mode==2){red=(red>>1)^(0u-(red&1u));blue=(blue>>1)^(0u-(blue&1u));}
            const unsigned rgb=(((red+(green>>1))&31u)<<11)|(green<<5)|((blue+(green>>1))&31u);
            record[2+pixel*2]=uint8_t(rgb);record[3+pixel*2]=uint8_t(rgb>>8);
            record[514+pixel]=alpha[tile*256u+pixel];
        }
    }
    return true;
}
inline bool restore_tile_filter(uint8_t* output,const uint8_t* input,
    unsigned bytes,unsigned items,unsigned mode){
    if(!output||!input||!items||items>680||bytes!=items*770u||mode<1||mode>2)return false;
    // The ARM compiler otherwise compares this invariant inside every pixel loop.
    // Select the lossless inverse once per group; neither path needs storage.
    return mode==2?restore_tile_filter_records<2>(output,input,items):
                   restore_tile_filter_records<1>(output,input,items);
}
} // namespace spire
