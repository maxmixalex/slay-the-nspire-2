#pragma once
#include <stdint.h>

namespace spire {
// RichTextLabel owns its ItemFX elapsed time. Only rendered label instances
// retain an origin; a newly created/reopened label begins at zero. This small
// presentation table neither serializes nor consumes gameplay randomness.
struct RichLabelClocks {
    static constexpr unsigned Capacity=32;
    struct Label {uint32_t owner=0,text=0,origin=0,seen=0,last=0;};
    Label labels[Capacity]{};
    uint32_t frame=0;
    unsigned depth=0;
    static uint32_t hash(const char*text){uint32_t value=2166136261u;while(*text)value=(value^uint8_t(*text++))*16777619u;return value?value:1;}
    static bool contains_rainbow(const char*text){
        for(;*text;++text)if(*text=='['){unsigned i=0;constexpr char tag[]="[rainbow";
            while(tag[i]&&text[i]==tag[i])++i;if(!tag[i])return true;}
        return false;
    }
    void reset(){for(auto&label:labels)label={};}
    void begin(){if(depth++==0&&++frame==0){reset();frame=1;}}
    void end(){if(depth&&--depth==0)for(auto&label:labels)if(label.text&&label.seen!=frame)label={};}
    uint32_t observe(uint32_t owner,uint32_t text,uint32_t now){
        Label*oldest=&labels[0];
        for(auto&label:labels){
            if(label.text==text&&label.owner==owner){
                if(int32_t(now-label.last)<0)label.origin=now;
                label.seen=frame;label.last=now;return now-label.origin;
            }
            if(!label.text||uint32_t(frame-label.seen)>uint32_t(frame-oldest->seen))oldest=&label;
        }
        *oldest={owner,text,now,frame,now};return 0;
    }
    struct Frame {
        RichLabelClocks& clocks;
        explicit Frame(RichLabelClocks& value):clocks(value){clocks.begin();}
        ~Frame(){clocks.end();}
        Frame(const Frame&)=delete;Frame&operator=(const Frame&)=delete;
    };
};
static_assert(sizeof(RichLabelClocks)<=768,"Bounded native label clocks");
}
