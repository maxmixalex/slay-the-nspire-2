#pragma once
#include "health_bar.hpp"

namespace spire {
enum class ButtonFamily : uint8_t { Action, Pause, EndTurn };
enum class ButtonState : uint8_t { Rest, Focus, Press, Disabled };
struct ButtonVisual {int scale=4096,y=0,hover=0,shade=255;};
inline ButtonVisual button_target(ButtonFamily family,ButtonState state){
    const bool focus=state==ButtonState::Focus,press=state==ButtonState::Press;
    ButtonVisual v;
    if(family==ButtonFamily::Pause){v.scale=focus?3891:press?3482:3686;v.y=press?8192:0;}
    else if(family==ButtonFamily::Action){v.scale=focus?4301:press?3891:4096;v.shade=press?128:255;}
    else v.y=press?8192:focus?-2048:0;
    v.hover=focus?4096:0;if(state==ButtonState::Disabled)v.shade=128;
    return v;
}
// One bounded, interruptible presentation per visible control. Restoring a
// save never serializes a pointer state or a partially completed UI tween.
struct ButtonMotion {
    uint32_t identity=0,start=0,seen=0;
    ButtonVisual from{};
    ButtonFamily family=ButtonFamily::Action;
    ButtonState state=ButtonState::Rest;
    ButtonVisual sample(uint32_t now)const{
        const auto to=button_target(family,state);const uint32_t elapsed=now-start;
        unsigned duration=500;
        if(state==ButtonState::Disabled)return to;
        if(state==ButtonState::Focus){if(family!=ButtonFamily::Pause)return to;duration=50;}
        if(state==ButtonState::Press&&family==ButtonFamily::Action)duration=250;
        if(elapsed>=duration)return to;
        unsigned q=elapsed*4096/duration;
        if(state==ButtonState::Press&&family==ButtonFamily::Action){unsigned inverse=4096-q;q=4096-(inverse*inverse/4096)*inverse/4096;}
        else if(state!=ButtonState::Focus)q=health_bar::expo_out(elapsed*1000/duration);
        auto mix=[&](int a,int b){return a+(b-a)*int(q)/4096;};
        ButtonVisual out{mix(from.scale,to.scale),mix(from.y,to.y),mix(from.hover,to.hover),mix(from.shade,to.shade)};
        if(state==ButtonState::Focus)out.hover=4096; // reference shader changes immediately
        return out;
    }
    ButtonVisual observe(uint32_t key,ButtonFamily kind,ButtonState mode,uint32_t now){
        if(identity!=key||family!=kind){identity=key;family=kind;state=ButtonState::Rest;from=button_target(kind,state);start=now;}
        if(state!=mode){from=sample(now);state=mode;start=now;}
        seen=now;return sample(now);
    }
};
constexpr unsigned MaxButtonMotions=32;
}
