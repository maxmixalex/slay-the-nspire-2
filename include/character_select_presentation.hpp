#pragma once
#include <stdint.h>
#include "health_bar.hpp"

namespace spire {
// NCharacterSelectButton: 1.1 focus scale, 0.5s EXPO_OUT on focus loss.
// NCharacterSelectScreen: information panel enters from 300px left in 0.5s.
// Fifty display pixels preserve that distance at 1920 -> 320 scaling.
// The background's 1.1 responsive scale is not an animation to invent here.
struct CharacterSelectPresentation {
    static constexpr unsigned Count=5,Scale=1024,FocusScale=1126;
    static constexpr uint32_t DurationMs=500;
    struct Portrait {uint16_t from=Scale;uint32_t start=0;};
    Portrait portraits[Count]{};
    uint32_t panel_start=0;
    unsigned selected=Count;
    int focused=-1;
    bool active=false;
    void reset(){*this=CharacterSelectPresentation{};}
    static unsigned ease(uint32_t elapsed){return health_bar::expo_out(elapsed>=DurationMs?health_bar::TweenMs:elapsed*2);}
    unsigned portrait_scale(unsigned index,uint32_t now)const{
        if(index>=Count)return Scale;
        if(int(index)==focused)return FocusScale;
        const auto&p=portraits[index];return unsigned(int(p.from)+(int(Scale)-p.from)*int(ease(now-p.start))/4096);
    }
    int panel_offset(uint32_t now)const{return active?-50+int((50*ease(now-panel_start)+2048)/4096):0;}
    static unsigned outline_alpha(uint32_t now){
        // Original 0.35..1 cosine pulse, 1.25s. A 32-step cycle is adequate
        // for a 1px outline and avoids per-frame trigonometry on the device.
        static constexpr uint8_t pulse[]={255,253,249,241,231,219,204,189,172,156,140,126,113,103,95,91,89};
        unsigned phase=(now%1250)*32/1250;return pulse[phase<=16?phase:32-phase];
    }
    void observe(bool visible,unsigned selection,int hover,uint32_t now){
        if(!visible){active=false;focused=-1;return;}
        if(selection>=Count)selection=0;
        if(!active){reset();active=true;selected=selection;panel_start=now;}
        if(selected!=selection){selected=selection;panel_start=now;}
        // -1 means keyboard focus; -2 is an active pointer outside the
        // portraits and must let the previously hovered button settle.
        const int next=hover>=0&&hover<int(Count)?hover:hover==-2?-1:int(selection);
        if(next==focused)return;
        if(focused>=0){auto&p=portraits[focused];p.from=uint16_t(portrait_scale(unsigned(focused),now));p.start=now;}
        focused=next;
    }
};
}
