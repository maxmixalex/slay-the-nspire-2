#pragma once
#include <stdint.h>
#include "health_bar.hpp"

namespace spire {
// NCardRewardSelectionScreen / NGridCardHolder at the original 4:3 viewport.
// This state belongs to the overlay, never to gameplay or its random streams.
struct CardRewardPresentation {
    struct Face {uint32_t released=0;bool focused=false,seen=false;};
    Face faces[8]{};
    uint32_t key=0,start=0,opened=0;
    uint16_t count=0;
    int16_t focus=-1;
    bool active=false;
    bool observe(bool visible,uint32_t identity,unsigned total,uint32_t now){
        if(!visible){active=false;return false;}
        if(active&&key==identity&&count==total)return false;
        if(!active)opened=now;
        active=true;key=identity;count=uint16_t(total);start=now;focus=-1;
        for(auto&face:faces)face={};
        return true;
    }
    bool clickable(uint32_t now)const{return active&&uint32_t(now-opened)>=350;}
    static unsigned expo(uint32_t age,unsigned duration){return age>=duration?4096:health_bar::expo_out(age*1000/duration);}
    static unsigned cubic(uint32_t age,unsigned duration){
        if(age>=duration)return 4096;const unsigned v=4096-age*4096/duration;
        return 4096-unsigned(uint64_t(v)*v*v/16777216);
    }
    static int back(uint32_t age,unsigned duration){
        if(age>=duration)return 4096;const int u=int(age*4096/duration)-4096;
        return 4096+(u*u/4096)*((11066*u/4096)+6970)/4096;
    }
    static int rounded(int value,int divisor){return value>=0?(value+divisor/2)/divisor:-((-value+divisor/2)/divisor);}
    int x(unsigned index,uint32_t now)const{
        // Normal source rewards have up to four offers, spaced at 350px.
        // A bounded extension can contain more; keep the same single row and
        // fit only those exceptional >5-card sets inside the 4:3 viewport.
        const int unit=int(index)*2-int(count)+1;
        const int offset=count>5?rounded(unit*262,int(count-1)*2):rounded(unit*100,3);
        return 160+rounded(offset*int(expo(now-start,500)),4096);
    }
    unsigned shade(uint32_t now)const{return cubic(now-start,1000)*255/4096;}
    int scale(unsigned index,uint32_t now)const{
        if(index>=8)return int(index)==focus?1000:800;
        const auto&face=faces[index];if(face.focused)return 1000;
        return face.seen?800+200*int(4096-expo(now-face.released,500))/4096:800;
    }
    void set_focus(int selected,uint32_t now){
        focus=int16_t(selected);
        for(unsigned i=0;i<8;++i){auto&face=faces[i];const bool on=int(i)==selected;
            if(face.focused!=on){face.focused=on;if(on)face.seen=true;else face.released=now;}}
    }
    int banner_y(uint32_t now)const{return 56+rounded(10*(4096-back(now-opened,400)),4096);}
    unsigned banner_alpha(uint32_t now)const{return expo(now-opened,400)*255/4096;}
    int alternative_y(uint32_t now)const{return 186-rounded(10*(4096-back(now-opened,500)),4096);}
};
}
