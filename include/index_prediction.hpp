#pragma once
#include <stdint.h>

namespace spire {
constexpr unsigned PredictiveIndexContentVersion=41;
// NSP42 keeps the direct group divisor/IDs but permits fewer stored tile
// records after offline compaction of unreferenced local slots.
constexpr unsigned CompactTileContentVersion=42;
constexpr unsigned MaximumContentVersion=43;
constexpr uint32_t PredictiveIndexGroupMagic=0x32494754u; // TGI2

namespace index_prediction_detail {
inline uint32_t word(const uint8_t* p){
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
inline void word(uint8_t* p,uint32_t value){
    p[0]=uint8_t(value);p[1]=uint8_t(value>>8);p[2]=uint8_t(value>>16);p[3]=uint8_t(value>>24);
}
}

// TGI2 keeps every original frame header, record length and geometry byte.
// Only tile IDs are predicted. Bit0 chooses the prior frame's corresponding
// ID (when its grid length matches) or this frame's preceding ID. The remaining
// bits are a zigzag signed difference. A missing predictor is zero. Decoding
// proceeds in place, so no extra buffer, group read or cross-group dependency
// is introduced. Unaligned records deliberately use byte loads/stores.
inline bool restore_index_prediction(uint8_t* data,unsigned bytes,unsigned items,unsigned tiles){
    using index_prediction_detail::word;
    if(!data||!bytes||bytes>524288u||!items||items>64u||!tiles||tiles>65535u*300u)return false;
    unsigned cursor=0,previous_count=0;
    uint8_t* previous=nullptr;
    for(unsigned frame=0;frame<items;++frame){
        if(cursor>bytes||bytes-cursor<12u)return false;
        uint8_t* record=data+cursor;
        const unsigned decoded=word(record),index_bytes=word(record+4),geometry=word(record+8);
        if(!decoded||decoded>614880u||!index_bytes||index_bytes>1200u||(index_bytes&3u)||
           !geometry||geometry>decoded||index_bytes>bytes-cursor-12u||
           geometry>bytes-cursor-12u-index_bytes)return false;
        uint8_t* indices=record+12;
        const unsigned count=index_bytes/4u;
        uint32_t spatial=0;
        for(unsigned i=0;i<count;++i){
            uint8_t* at=indices+i*4u;
            const uint32_t encoded=word(at),zigzag=encoded>>1;
            if(zigzag>=tiles*2u)return false;
            const uint32_t base=(encoded&1u)?(previous_count==count?word(previous+i*4u):0u):spatial;
            const uint32_t delta=(zigzag>>1)^(0u-(zigzag&1u));
            const uint32_t id=base+delta;
            if(id>=tiles)return false;
            word(at,id);spatial=id;
        }
        previous=indices;previous_count=count;
        cursor+=12u+index_bytes+geometry;
    }
    return cursor==bytes;
}
} // namespace spire
