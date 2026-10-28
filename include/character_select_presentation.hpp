#pragma once
#include <stdint.h>
#include "health_bar.hpp"
#include "reference_viewport.hpp"
#include "random_character_presentation.hpp"

namespace spire {
// NCharacterSelectButton: 1.1 focus scale, 0.5s EXPO_OUT on focus loss.
// NCharacterSelectScreen: information panel enters from 300px left in 0.5s.
// Fifty-seven display pixels preserve that distance at 1680 -> 320 scaling.
// The background's 1.1 responsive scale is not an animation to invent here.
struct CharacterSelectPresentation {
    static constexpr unsigned Count=6,Scale=1024,FocusScale=1126;
    static constexpr uint32_t DurationMs=500;
    static constexpr int PanelDistance=reference_viewport::project(300);
    struct Portrait {uint16_t from=Scale,focus_scale=Scale,saturation=200,value=400;uint32_t start=0,color_start=0;bool color_tween=false;};
    Portrait portraits[Count]{};
    RandomCharacterPresentation random_background{};
    uint32_t opened=0,panel_start=0;
    unsigned selected=Count;
    int focused=-1;
    bool active=false;
    void reset(){*this=CharacterSelectPresentation{};}
    static unsigned ease(uint32_t elapsed){return health_bar::expo_out(elapsed>=DurationMs?health_bar::TweenMs:elapsed*2);}
    unsigned portrait_scale(unsigned index,uint32_t now)const{
        if(index>=Count)return Scale;
        if(int(index)==focused)return portraits[index].focus_scale;
        const auto&p=portraits[index];return unsigned(int(p.from)+(int(Scale)-p.from)*int(ease(now-p.start))/4096);
    }
    int panel_offset(uint32_t now)const{return active?-PanelDistance+int((PanelDistance*ease(now-panel_start)+2048)/4096):0;}
    unsigned saturation(unsigned index,uint32_t now)const{
        if(index>=Count)return 1000;if(index==selected||int(index)==focused)return 1000;
        const auto&p=portraits[index];return p.color_tween?unsigned(int(p.saturation)+(200-int(p.saturation))*int(ease(now-p.color_start))/4096):p.saturation;
    }
    unsigned value(unsigned index,uint32_t now)const{
        if(index>=Count)return 1000;if(index==selected||int(index)==focused)return 1100;
        // _Ready starts untouched icons at V=.4; the runtime getter used by
        // OnUnfocus/Deselect returns .8. These are distinct source states.
        const auto&p=portraits[index];return p.color_tween?unsigned(int(p.value)+(800-int(p.value))*int(ease(now-p.color_start))/4096):p.value;
    }
    static unsigned outline_alpha(uint32_t now){
        // Original 0.35..1 cosine pulse, 1.25s. A 32-step cycle is adequate
        // for a 1px outline and avoids per-frame trigonometry on the device.
        static constexpr uint8_t pulse[]={255,253,249,241,231,219,204,189,172,156,140,126,113,103,95,91,89};
        unsigned phase=(now%1250)*32/1250;return pulse[phase<=16?phase:32-phase];
    }
    void observe(bool visible,unsigned selection,int hover,uint32_t now){
        if(!visible){active=false;focused=-1;return;}
        if(selection>=Count)selection=0;
        if(!active){reset();active=true;selected=selection;opened=panel_start=now;}
        if(selected!=selection){auto&p=portraits[selected];p.saturation=uint16_t(saturation(selected,now));p.value=uint16_t(value(selected,now));p.color_start=now;p.color_tween=true;selected=selection;panel_start=now;}
        // -1 means keyboard focus; -2 is an active pointer outside the
        // portraits and must let the previously hovered button settle.
        const int next=hover>=0&&hover<int(Count)?hover:hover==-2?-1:int(selection);
        if(next==focused)return;
        const unsigned next_scale=next>=0?(unsigned(next)==selected?portrait_scale(unsigned(next),now):FocusScale):Scale;
        if(focused>=0){auto&p=portraits[focused];p.from=uint16_t(portrait_scale(unsigned(focused),now));p.start=now;
            p.saturation=uint16_t(saturation(unsigned(focused),now));p.value=uint16_t(value(unsigned(focused),now));p.color_start=now;p.color_tween=true;}
        focused=next;
        if(focused>=0)portraits[focused].focus_scale=uint16_t(next_scale);
    }
};
struct CharacterDeparturePresentation {
    static constexpr unsigned FadeMs=800,RandomRevealMs=1000;
    uint32_t started=0,reveal_ms=0;
    uint8_t phase=0,character=0; // 1: source texture wipe; 2: room fade-in.
    bool animated_ftue=false,continuing=false;
    bool active()const{return phase!=0;}
    unsigned progress(uint32_t now)const{
        const uint32_t elapsed=now-started;
        const uint32_t age=phase==1?(elapsed>reveal_ms?elapsed-reveal_ms:0):elapsed;
        return age>=FadeMs?4096:unsigned(age*4096/FadeMs);
    }
};
}
