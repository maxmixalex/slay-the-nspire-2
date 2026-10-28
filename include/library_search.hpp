#pragma once
#include <stdint.h>

namespace spire { namespace library_query {
// NSearchBar.Normalize applies to the joined card title/description only.
// Its query is merely ToLowerInvariant: spaces around a nonblank query stay.
inline uint32_t next(const char*&p){
    uint32_t cp=uint8_t(*p++);if(cp<128)return cp;
    unsigned count=cp<224?1:cp<240?2:3;cp&=(1u<<(6-count))-1;
    while(count--){const unsigned byte=uint8_t(*p);if((byte&192)!=128)return 0xfffd;++p;cp=(cp<<6)|(byte&63);}return cp;
}
inline bool whitespace(uint32_t cp){
    return (cp>=9&&cp<=13)||cp==32||cp==0x85||cp==0xa0||cp==0x1680||
        (cp>=0x2000&&cp<=0x200a)||cp==0x2028||cp==0x2029||cp==0x202f||cp==0x205f||cp==0x3000;
}
inline uint32_t lower(uint32_t cp){
    if((cp>='A'&&cp<='Z')||(cp>=0xc0&&cp<=0xde&&cp!=0xd7))return cp+32;
    // These are the additional cased letters in the pinned English text;
    // Chinese remains unchanged. No locale-sensitive Turkish-I conversion.
    if(cp==0x11e||cp==0x141)return cp+1;return cp;
}
template<class Emit>inline void utf8(uint32_t cp,Emit emit){
    if(cp<128)emit(uint8_t(cp));
    else if(cp<0x800){emit(uint8_t(0xc0|(cp>>6)));emit(uint8_t(0x80|(cp&63)));}
    else if(cp<0x10000){emit(uint8_t(0xe0|(cp>>12)));emit(uint8_t(0x80|((cp>>6)&63)));emit(uint8_t(0x80|(cp&63)));}
    else{emit(uint8_t(0xf0|(cp>>18)));emit(uint8_t(0x80|((cp>>12)&63)));emit(uint8_t(0x80|((cp>>6)&63)));emit(uint8_t(0x80|(cp&63)));}
}
inline bool empty(const char*text){while(*text)if(!whitespace(next(text)))return false;return true;}
inline void fold(char*out,unsigned capacity,const char*text){
    unsigned size=0;if(!capacity)return;
    while(*text){const auto cp=lower(next(text));const unsigned count=cp<128?1:cp<0x800?2:cp<0x10000?3:4;
        if(size+count>=capacity)break;utf8(cp,[&](uint8_t byte){out[size++]=char(byte);});}
    out[size]=0;
}
// One128-byte prefix table is prepared per edited query. Each card is streamed
// once, with no combined title/description buffer or per-card allocation.
struct Matcher {
    const char*query;uint8_t prefix[128]{};unsigned length=0;
    explicit Matcher(const char*value):query(value){
        while(query[length]&&length<127)++length;
        for(unsigned i=1,matched=0;i<length;++i){while(matched&&query[i]!=query[matched])matched=prefix[matched-1];
            if(query[i]==query[matched])++matched;prefix[i]=uint8_t(matched);}
    }
    bool matches(const char*title,const char*description)const{
        if(!length)return true;unsigned matched=0,pending=0;uint32_t pending_cp=0;bool started=false,found=false;
        const auto byte=[&](uint8_t value){if(found)return;
            while(matched&&value!=uint8_t(query[matched]))matched=prefix[matched-1];
            if(value==uint8_t(query[matched])&&++matched==length)found=true;};
        const auto character=[&](uint32_t cp){
            if(cp=='\t'||cp=='\r'||cp=='\n')cp=' ';
            if(whitespace(cp)){if(pending<2)++pending;pending_cp=cp;return;}
            if(started&&pending)utf8(pending>1?' ':pending_cp,byte);pending=0;started=true;utf8(lower(cp),byte);
        };
        const auto part=[&](const char*text,bool tags){
            while(*text&&!found){
                if(tags&&(*text=='['||*text=='<')){const char close=*text=='['?']':'>';const char*end=text+1;
                    while(*end&&*end!=close&&*end!='\n')++end;
                    if(*end==close){text=end+1;continue;}}
                character(next(text));
            }
        };
        part(title,false);if(found)return true;character(' ');part(description,true);return found;
    }
};
} }
