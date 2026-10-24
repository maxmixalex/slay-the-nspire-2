#pragma once
#include <stdint.h>
#include <stddef.h>

namespace spire {
// Host text events may contain a complete IME commit, not one ASCII byte.
// Invalid or incomplete UTF-8 is ignored rather than inserted into UI text.
template<class Emit> void host_text_utf8(const char*text,size_t length,Emit emit){
    for(size_t i=0;i<length;){
        const unsigned lead=uint8_t(text[i]);unsigned code=lead,count=1,minimum=0;
        if(lead>=0xc2&&lead<=0xdf){code=lead&31;count=2;minimum=0x80;}
        else if(lead>=0xe0&&lead<=0xef){code=lead&15;count=3;minimum=0x800;}
        else if(lead>=0xf0&&lead<=0xf4){code=lead&7;count=4;minimum=0x10000;}
        else if(lead>=0x80){++i;continue;}
        if(i+count>length)break;
        bool valid=true;
        for(unsigned n=1;n<count;++n){const unsigned next=uint8_t(text[i+n]);if((next&0xc0)!=0x80){valid=false;break;}code=(code<<6)|(next&63);}
        if(!valid||code<minimum||code>0x10ffff||(code>=0xd800&&code<=0xdfff)){++i;continue;}
        i+=count;if(code>=32)emit(int(code));
    }
}

// TI-Nspire CX / touchpad keyboard matrix, as documented by Ndless keys.h.
// This is only a physical-key -> text adapter; it never dispatches gameplay.
struct CalculatorTextKey {uint8_t row;uint16_t mask;uint8_t character;};
static constexpr CalculatorTextKey CalculatorTextKeys[]={
    {4,0x40,'a'},{4,0x20,'b'},{4,0x10,'c'},{4,0x04,'d'},{4,0x02,'e'},{4,0x01,'f'},
    {3,0x40,'g'},{3,0x20,'h'},{3,0x10,'i'},{3,0x04,'j'},{3,0x02,'k'},{3,0x01,'l'},
    {2,0x40,'m'},{2,0x20,'n'},{2,0x10,'o'},{2,0x04,'p'},{2,0x02,'q'},{2,0x01,'r'},
    {1,0x40,'s'},{1,0x20,'t'},{1,0x10,'u'},{1,0x04,'v'},{1,0x02,'w'},{1,0x01,'x'},
    {0,0x40,'y'},{0,0x20,'z'},{0,0x10,' '},
    {0,0x80,'0'},{1,0x80,'1'},{6,0x10,'2'},{1,0x08,'3'},{2,0x80,'4'},
    {5,0x40,'5'},{2,0x08,'6'},{3,0x80,'7'},{6,0x40,'8'},{3,0x08,'9'},
    {5,0x10,'.'},{5,0x04,'-'},{6,0x04,'+'},{5,0x01,'\''},{7,0x400,','},
    {5,0x200,8},{0,0x02,13},{6,0x80,27}
};
template<class Emit> void calculator_text_edges(const uint16_t*current,const uint16_t*previous,Emit emit){
    const bool shift=current[7]&0x100;
    for(const auto&key:CalculatorTextKeys)if((current[key.row]&key.mask)&&!(previous[key.row]&key.mask)){
        int code=key.character;if(shift&&code>='a'&&code<='z')code-=32;emit(code);
    }
}
} // namespace spire
