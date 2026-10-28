#pragma once
#include <stddef.h>
#include <stdint.h>
namespace spire {
// Bounded LZ4 block decoder. No frame header, dictionary or heap allocation.
// Format: https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md
inline bool decode_lz4(const uint8_t* source,size_t size,uint8_t* dest,size_t capacity) {
    if(!source||!dest)return false;
    size_t in=0,out=0;
    while(in<size){
        unsigned token=source[in++];size_t literals=token>>4;
        if(literals==15){unsigned extra;do{if(in==size)return false;extra=source[in++];if(literals>capacity||extra>capacity-literals)return false;literals+=extra;}while(extra==255);}
        if(literals>size-in||literals>capacity-out)return false;
        for(size_t i=0;i<literals;++i)dest[out++]=source[in++];
        if(in==size)return out==capacity;
        if(size-in<2)return false;
        size_t offset=source[in]|(size_t(source[in+1])<<8);in+=2;
        if(!offset||offset>out)return false;
        size_t match=(token&15)+4;
        if((token&15)==15){unsigned extra;do{if(in==size)return false;extra=source[in++];if(match>capacity||extra>capacity-match)return false;match+=extra;}while(extra==255);}
        if(match>capacity-out)return false;
        for(size_t i=0;i<match;++i){dest[out]=dest[out-offset];++out;}
    }
    return false;
}
}
