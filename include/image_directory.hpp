#pragma once
#include <stdint.h>

namespace spire {
// Private resident representation. The immutable NSP2 stream and all virtual
// offsets remain unchanged; only a streamed Content's owned directory uses
// twelve-byte rows instead of the sixteen-byte on-disk image records.
struct ImageDescriptor {
    uint32_t offset=0,shape=0,format=0,length=0;
    unsigned width()const{return shape&65535u;}
    unsigned height()const{return shape>>16;}
};
struct ImageDirectoryView {
    const uint8_t* bytes=nullptr;
    unsigned stride=16;
    static uint32_t word(const uint8_t*p){
        return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
    }
    static void word(uint8_t*p,uint32_t value){for(unsigned i=0;i<4;++i)p[i]=uint8_t(value>>(i*8));}
    ImageDescriptor operator[](unsigned id)const{
        const auto*p=bytes+id*stride;const auto flags=word(p+8);
        return {word(p),word(p+4),stride==12?flags>>29:flags,stride==12?flags&0x1fffffffu:word(p+12)};
    }
    bool equal(unsigned first,unsigned second)const{
        const auto*a=bytes+first*stride;const auto*b=bytes+second*stride;
        return word(a)==word(b)&&word(a+4)==word(b+4)&&word(a+8)==word(b+8)&&
            (stride==12||word(a+12)==word(b+12));
    }
    static void compact(uint8_t*to,const uint8_t*from){
        const uint32_t format=word(from+8),length=word(from+12);
        word(to,word(from));word(to+4,word(from+4));
        // Malformed images still fail on use, as before. Never silently
        // truncate an invalid format/length into a valid resident record.
        word(to+8,format>7||length>0x1fffffffu?0xffffffffu:(format<<29)|length);
    }
};
}
