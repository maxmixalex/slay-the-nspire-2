#pragma once
#include <stdint.h>

namespace spire {
constexpr unsigned TileFilterContentVersion=33;
constexpr uint32_t FilteredTileGroupMagic=0x32524754u; // TGR2
constexpr unsigned LargeTileContentVersion=44,LargeTileItems=1360;

inline unsigned tile_filter_plane_bytes(unsigned items){return items*258u;}
inline unsigned tile_filter_forward_items(unsigned items){return (items*2u+257u)/258u;}
template<unsigned Mode>
inline unsigned tile_filter_color(unsigned packed){
    const unsigned green=packed&63u;unsigned red=(packed>>6)&31u,blue=packed>>11;
    if(Mode==2){red=(red>>1)^(0u-(red&1u));blue=(blue>>1)^(0u-(blue&1u));}
    return (((red+(green>>1))&31u)<<11)|(green<<5)|((blue+(green>>1))&31u);
}
// Two independent 16-bit lanes; masking before additions prevents a low
// lane's sign or carry from reaching the high lane. This is the same exact
// RGB565 inverse, including hidden colors under zero alpha.
template<unsigned Mode>
inline uint32_t tile_filter_color_pair(uint32_t packed){
    const uint32_t green=packed&0x003f003fu,half=(packed>>1)&0x001f001fu;
    uint32_t red=(packed>>6)&0x001f001fu,blue=(packed>>11)&0x001f001fu;
    if(Mode==2){red=((red>>1)&0x000f000fu)^((red&0x00010001u)*31u);
        blue=((blue>>1)&0x000f000fu)^((blue&0x00010001u)*31u);}
    return (((red+half)&0x001f001fu)<<11)|(green<<5)|((blue+half)&0x001f001fu);
}
template<unsigned Mode,bool Reverse>
inline void restore_tile_filter_colors(uint8_t*output,const uint8_t*input,unsigned pixels){
    unsigned paired=0;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // Plane starts may be 2 mod 4 and each 770-byte record alternates word
    // alignment. Explicit aligned halfwords avoid ARM926's rotated LDR;
    // memcpy keeps the byte-backed input valid under C++ aliasing rules.
    if(!((reinterpret_cast<uintptr_t>(output)|reinterpret_cast<uintptr_t>(input))&1u)){
        paired=pixels/2u;
        for(unsigned i=0;i<paired;++i){const unsigned pixel=Reverse?pixels-2u-i*2u:i*2u;
            uint16_t low,high;
            __builtin_memcpy(&low,__builtin_assume_aligned(input+pixel*2u,2),2);
            __builtin_memcpy(&high,__builtin_assume_aligned(input+pixel*2u+2u,2),2);
            const uint32_t rgb=tile_filter_color_pair<Mode>(uint32_t(low)|(uint32_t(high)<<16));
            low=uint16_t(rgb);high=uint16_t(rgb>>16);
            // Read both source pixels before either overlapping write.
            __builtin_memcpy(__builtin_assume_aligned(output+pixel*2u,2),&low,2);
            __builtin_memcpy(__builtin_assume_aligned(output+pixel*2u+2u,2),&high,2);
        }
    }
#endif
    for(unsigned i=paired*2u;i<pixels;++i){const unsigned pixel=Reverse?pixels-1u-i:i;
        const unsigned packed=unsigned(input[pixel*2u])|(unsigned(input[pixel*2u+1u])<<8);
        const unsigned rgb=tile_filter_color<Mode>(packed);
        output[pixel*2u]=uint8_t(rgb);output[pixel*2u+1u]=uint8_t(rgb>>8);
    }
}
// Header/alpha are retained separately. Colors can then be restored in their
// own output: move the overlapping high records backwards, then the small
// low prefix forwards. Include the two-byte record header in the split:
// header destination minus color source is 258*i-2*N. Splitting at the
// color destination would overwrite the preceding record at N=129*k+1.
template<unsigned Mode,bool Reverse>
inline void restore_tile_filter_in_place_record(uint8_t*out,const uint8_t*planes,unsigned items,unsigned id){
    uint8_t*record=out+id*770u;const uint8_t*colors=out+items*2u+id*512u;
    restore_tile_filter_colors<Mode,Reverse>(record+2,colors,256);
    record[0]=planes[id*2];record[1]=planes[id*2+1];
    for(unsigned i=0;i<256;++i)record[514+i]=planes[items*2u+id*256u+i];
}
inline bool restore_tile_filter_in_place_step(uint8_t*out,const uint8_t*planes,unsigned items,unsigned mode,unsigned index){
    if(!out||!planes||!items||items>LargeTileItems||mode<1||mode>2||index>=items)return false;
    const unsigned high=items-tile_filter_forward_items(items),id=index<high?items-1u-index:index-high;
    if(!planes[id*2]||planes[id*2]>16||!planes[id*2+1]||planes[id*2+1]>16)return false;
    if(index<high){if(mode==2)restore_tile_filter_in_place_record<2,true>(out,planes,items,id);
        else restore_tile_filter_in_place_record<1,true>(out,planes,items,id);}
    else {if(mode==2)restore_tile_filter_in_place_record<2,false>(out,planes,items,id);
        else restore_tile_filter_in_place_record<1,false>(out,planes,items,id);}
    return true;
}
inline bool restore_tile_filter_in_place(uint8_t*out,unsigned bytes,unsigned items,unsigned mode,uint8_t*scratch,unsigned capacity){
    if(!out||!scratch||!items||items>LargeTileItems||bytes!=items*770u||mode<1||mode>2||capacity<tile_filter_plane_bytes(items))return false;
    const uintptr_t a=reinterpret_cast<uintptr_t>(out),b=reinterpret_cast<uintptr_t>(scratch);
    if(a<=b?b-a<bytes:a-b<tile_filter_plane_bytes(items))return false;
    for(unsigned i=0;i<items*2u;++i)scratch[i]=out[i];
    for(unsigned i=0;i<items*256u;++i)scratch[items*2u+i]=out[items*514u+i];
    for(unsigned i=0;i<items;++i)if(!restore_tile_filter_in_place_step(out,scratch,items,mode,i))return false;
    return true;
}

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
        restore_tile_filter_colors<Mode,false>(record+2,source,256);
        for(unsigned pixel=0;pixel<256;++pixel)record[514+pixel]=alpha[tile*256u+pixel];
    }
    return true;
}
inline bool restore_tile_filter(uint8_t* output,const uint8_t* input,
    unsigned bytes,unsigned items,unsigned mode){
    if(!output||!input||!items||items>LargeTileItems||bytes!=items*770u||mode<1||mode>2)return false;
    // The ARM compiler otherwise compares this invariant inside every pixel loop.
    // Select the lossless inverse once per group; neither path needs storage.
    return mode==2?restore_tile_filter_records<2>(output,input,items):
                   restore_tile_filter_records<1>(output,input,items);
}
} // namespace spire
