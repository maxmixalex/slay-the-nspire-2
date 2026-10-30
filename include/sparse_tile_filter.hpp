#pragma once
#include <stdint.h>
#include "tile_filter.hpp"

namespace spire {
constexpr unsigned SparseTileContentVersion=45;
constexpr uint32_t SparseTileGroupMagic=0x33524754u; // TGR3

inline bool sparse_tile_shape(unsigned items,unsigned bytes,unsigned&pixels){
    if(!items||items>LargeTileItems||bytes<items*34u||(bytes-items*34u)%3u)return false;
    pixels=(bytes-items*34u)/3u;
    // Header, occupancy and compact alpha fit the old header/alpha reservation.
    return pixels<=items*224u;
}
inline unsigned sparse_tile_popcount(unsigned value){
    value=value-((value>>1)&0x55u);
    value=(value&0x33u)+((value>>2)&0x33u);
    return (value+(value>>4))&15u;
}
inline unsigned sparse_tile_count(const uint8_t*planes,unsigned items,unsigned id){
    const uint8_t*mask=planes+items*2u+id*32u;unsigned count=0;
    for(unsigned i=0;i<32;++i)count+=sparse_tile_popcount(mask[i]);
    return count;
}
// Scan one tile per call. Finding the overlap boundary and validating mask
// population are functional decoding work, bounded inside this one group.
inline bool sparse_tile_scan(const uint8_t*planes,unsigned items,unsigned pixels,
    unsigned index,unsigned&prefix,unsigned&low){
    if(!planes||!items||items>LargeTileItems||index>=items||low>items||prefix>pixels)return false;
    if(!planes[index*2]||planes[index*2]>16||!planes[index*2+1]||planes[index*2+1]>16)return false;
    if(low==items&&index*770u>=items*34u+prefix*2u)low=index;
    const unsigned count=sparse_tile_count(planes,items,index);
    if(count>pixels-prefix)return false;
    prefix+=count;return true;
}
// High records move backwards, then the low prefix moves forwards. A single
// tile's at-most-512-byte colors are copied before expansion: this covers the
// one record that straddles the source/destination overlap boundary.
inline bool restore_sparse_tile_step(uint8_t*out,const uint8_t*planes,unsigned items,
    unsigned pixels,unsigned mode,unsigned index,unsigned low,unsigned&cursor){
    if(!out||!planes||!items||items>LargeTileItems||pixels>items*224u||
       mode<1||mode>2||index>=items||low>items||cursor>pixels)return false;
    const unsigned high=items-low,id=index<high?items-1u-index:index-high;
    const unsigned count=sparse_tile_count(planes,items,id);
    unsigned start=0;
    if(index<high){if(count>cursor)return false;start=cursor-count;cursor=start;}
    else {if(index==high)cursor=0;if(count>pixels-cursor)return false;start=cursor;cursor+=count;}
    const uint8_t*source=out+items*34u+start*2u;
    uint16_t colors[256];for(unsigned i=0;i<count;++i)colors[i]=uint16_t(source[i*2]|unsigned(source[i*2+1])<<8);
    const uint8_t*mask=planes+items*2u+id*32u,*alpha=planes+items*34u+start;
    uint8_t*record=out+id*770u;record[0]=planes[id*2];record[1]=planes[id*2+1];
    unsigned used=0;
    for(unsigned i=0;i<256;++i){
        unsigned rgb=0,opacity=0;
        if(mask[i/8]&(1u<<(i&7u))){
            if(used>=count)return false;
            const unsigned packed=colors[used],green=packed&63u;
            unsigned red=(packed>>6)&31u,blue=packed>>11;
            if(mode==2){red=(red>>1)^(0u-(red&1u));blue=(blue>>1)^(0u-(blue&1u));}
            rgb=(((red+(green>>1))&31u)<<11)|(green<<5)|((blue+(green>>1))&31u);
            opacity=alpha[used++];
        }
        record[2+i*2]=uint8_t(rgb);record[3+i*2]=uint8_t(rgb>>8);record[514+i]=uint8_t(opacity);
    }
    return used==count;
}
inline bool restore_sparse_tiles_in_place(uint8_t*out,unsigned bytes,unsigned items,unsigned mode,
    uint8_t*scratch,unsigned capacity){
    unsigned pixels=0;if(!out||!scratch||mode<1||mode>2||!sparse_tile_shape(items,bytes,pixels))return false;
    const unsigned planes=items*34u+pixels;
    if(capacity<planes)return false;
    const uintptr_t a=reinterpret_cast<uintptr_t>(out),b=reinterpret_cast<uintptr_t>(scratch);
    if(a<=b?b-a<items*770u:a-b<planes)return false;
    for(unsigned i=0;i<items*34u;++i)scratch[i]=out[i];
    for(unsigned i=0;i<pixels;++i)scratch[items*34u+i]=out[items*34u+pixels*2u+i];
    unsigned prefix=0,low=items;
    for(unsigned i=0;i<items;++i)if(!sparse_tile_scan(scratch,items,pixels,i,prefix,low))return false;
    if(prefix!=pixels)return false;
    unsigned cursor=pixels;
    for(unsigned i=0;i<items;++i)if(!restore_sparse_tile_step(out,scratch,items,pixels,mode,i,low,cursor))return false;
    return true;
}
} // namespace spire
