#pragma once
#include <stdint.h>
namespace spire {
// The first thirty IDs preserve historical bit values. New IDs are ordinal
// tokens. Save codecs explicitly encode words; this is never an object dump.
inline constexpr unsigned RelicOrdinal(uint32_t id){
    if(id&0x80000000u)return id&0x7fffffffu;
    if(!id)return 320;
    unsigned n=0;while(!(id&1u)){id>>=1;++n;}return n;
}
struct RelicSet {
    static constexpr unsigned Words=10;
    uint32_t words[Words]{};
    constexpr RelicSet()=default;
    constexpr RelicSet(uint32_t legacy):words{legacy}{}
    RelicSet& operator=(uint32_t legacy){for(auto&w:words)w=0;words[0]=legacy;return *this;}
    RelicSet& operator|=(uint32_t id){
        if(id&0x80000000u){unsigned i=id&0x7fffffffu;if(i<Words*32)words[i/32]|=1u<<(i%32);}
        else words[0]|=id;
        return *this;
    }
    bool operator&(uint32_t id)const{
        if(id&0x80000000u){unsigned i=id&0x7fffffffu;return i<Words*32&&(words[i/32]&(1u<<(i%32)));}
        return (words[0]&id)!=0;
    }
    bool operator==(const RelicSet&b)const{for(unsigned i=0;i<Words;i++)if(words[i]!=b.words[i])return false;return true;}
    bool operator!=(const RelicSet&b)const{return !(*this==b);}
    explicit operator bool()const{for(auto w:words)if(w)return true;return false;}
    uint32_t low()const{return words[0];}
    bool valid(unsigned count)const{
        if(count>Words*32)return false;
        for(unsigned i=0;i<Words;i++){
            unsigned used=count>i*32?count-i*32:0;
            if(used<32&&(words[i]&~(used?((1u<<used)-1):0u)))return false;
        }
        return true;
    }
};
}
