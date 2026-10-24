#pragma once
#include "health_bar.hpp"

namespace spire {
enum class ButtonFamily : uint8_t { Action, Pause, EndTurn, Back };
enum class ButtonState : uint8_t { Rest, Focus, Press, Disabled };
struct ButtonVisual {int scale=4096,y=0,hover=0,shade=255;};
inline ButtonVisual button_target(ButtonFamily family,ButtonState state){
    const bool focus=state==ButtonState::Focus,press=state==ButtonState::Press;
    ButtonVisual v;
    if(family==ButtonFamily::Pause){v.scale=focus?3891:press?3482:3686;v.y=press?8192:0;}
    else if(family==ButtonFamily::Action||family==ButtonFamily::Back){v.scale=focus?4301:press&&family==ButtonFamily::Action?3891:4096;v.shade=press?128:255;}
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
    bool interacted=false;
    ButtonVisual sample(uint32_t now)const{
        auto to=button_target(family,state);const uint32_t elapsed=now-start;
        if(family==ButtonFamily::Back){
            // NBackButton: focus and outline rise linearly over 50 ms;
            // press returns to scale 1 over 250 ms (Expo.Out), while its
            // shade/outline use Cubic.Out. Unfocus keeps authored 1.05.
            if(state==ButtonState::Rest&&interacted)to.scale=4301;
            if(state==ButtonState::Disabled)return to;
            const unsigned duration=state==ButtonState::Focus?50:state==ButtonState::Press?250:500;
            if(elapsed>=duration)return to;
            const unsigned linear=elapsed*4096/duration;
            const unsigned scale_q=state==ButtonState::Focus?linear:health_bar::expo_out(elapsed*1000/duration);
            unsigned color_q=linear;
            if(state==ButtonState::Press){const unsigned inverse=4096-linear;color_q=4096-(inverse*inverse/4096)*inverse/4096;}
            return {from.scale+(to.scale-from.scale)*int(scale_q)/4096,0,
                from.hover+(to.hover-from.hover)*int(color_q)/4096,
                from.shade+(to.shade-from.shade)*int(color_q)/4096};
        }
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
        if(identity!=key||family!=kind){identity=key;family=kind;state=ButtonState::Rest;interacted=false;from=button_target(kind,state);start=now;}
        if(state!=mode){from=sample(now);state=mode;start=now;if(mode==ButtonState::Focus||mode==ButtonState::Press)interacted=true;}
        seen=now;return sample(now);
    }
};
constexpr unsigned MaxButtonMotions=32;

// NEndTurnButton + end_turn_button.tscn, pinned v0.111.0. Scales are
// relative to the authored half-scale Image; alpha and scale use Q12.
struct EndTurnGlowVisual {
    int base_alpha=0,base_scale=4096,pulse_alpha=0,pulse_scale=4096;
};
struct EndTurnGlowIdentity {
    uint32_t seed=0;
    uint16_t floor=0,combat=0,encounter=0;
    bool operator==(const EndTurnGlowIdentity&v)const{
        return seed==v.seed&&floor==v.floor&&combat==v.combat&&encounter==v.encounter;
    }
};
struct EndTurnGlowMotion {
    EndTurnGlowIdentity identity{};
    EndTurnGlowVisual from{};
    uint32_t start=0;
    bool bound=false,shining=false;
    EndTurnGlowVisual sample(uint32_t now)const{
        const uint32_t elapsed=now-start;
        if(!shining){
            auto v=from;
            const int q=elapsed>=500?4096:int(health_bar::expo_out(elapsed*2));
            v.base_alpha=from.base_alpha*(4096-q)/4096;
            v.pulse_alpha=from.pulse_alpha*(4096-q)/4096;
            return v;
        }
        // Godot Back::out uses s=1.70158. Preserve its small overshoot;
        // clamping the easing coefficient would change the source tween.
        int q=4096;
        if(elapsed<800){const int u=int(elapsed*4096/800)-4096;
            q=4096+(u*u/4096)*((11066*u/4096)+6970)/4096;}
        EndTurnGlowVisual v;
        v.base_alpha=from.base_alpha+(3072-from.base_alpha)*q/4096;
        v.base_scale=3686+410*q/4096;
        const uint32_t pulse=elapsed%1500;
        const int inverse=4096-int(pulse*4096/1500),square=inverse*inverse/4096;
        const int quart=4096-square*square/4096;
        v.pulse_scale=4096+1638*quart/4096;
        v.pulse_alpha=int((1500-pulse)*1638/1500);
        return v;
    }
    EndTurnGlowVisual observe(EndTurnGlowIdentity key,bool active,uint32_t now){
        if(!bound||!(identity==key)){
            identity=key;from={};start=now;bound=true;shining=false;
        }
        if(shining!=active){from=sample(now);start=now;shining=active;}
        return sample(now);
    }
};
}
