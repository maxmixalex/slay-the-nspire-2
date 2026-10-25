#pragma once
#include "end_turn_hold.hpp"
#include <cmath>

// Independent, continuous source equations. Timelines contain exact threshold,
// interrupted decay, restart, completion and release; no production easing or
// rendering helper computes the expected values.
inline bool source_end_turn_hold_timeline(){
    using spire::EndTurnHold;
    EndTurnHold hold;
    double timer=0,alpha=0,width=0,red=0,green=.6,fade=0,from_alpha=0;
    bool pressed=false,fired=false;
    unsigned emitted=0;
    const auto sample=[&](){
        const auto value=hold.visual();
        return (!alpha||std::abs(double(value.width_q12)/4096-width)<.001)&&
            std::abs(double(value.alpha_q12)/4096-alpha)<.001&&
            (!alpha||(std::abs(double(value.red)/255-red)<.0041&&std::abs(double(value.green)/255-green)<.0041));
    };
    const auto step=[&](unsigned milliseconds){
        const double delta=milliseconds/1000.0;bool trigger=false;
        if(fired){fade+=delta;alpha=from_alpha*std::fmax(0.,1.-fade/.25);}
        else{
            timer=pressed?timer+delta:std::fmax(0.,timer-delta);
            if(pressed&&timer>.5+1e-10){trigger=true;fired=true;width=204;timer=0;from_alpha=alpha;}
            else{const double q=timer/.5;width=q*204;alpha=1-std::pow(1-.75*q,3);red=q*2.5;green=.6+q;}
        }
        const bool actual=hold.advance(milliseconds);emitted+=actual;
        return actual==trigger&&sample();
    };
    if(!hold.start('E',4,1))return false;pressed=true;
    if(!step(125)||!step(175))return false;
    hold.cancel('E');pressed=false;
    if(!step(75))return false;
    if(!hold.start('E',4,1))return false;pressed=true;
    if(!step(250)||!step(25)||emitted||hold.timer!=500)return false;
    if(!step(1)||emitted!=1||!hold.fired)return false;
    if(hold.start('E',4,2))return false; // OS repeat/new turn keeps old owner.
    if(!step(125))return false;
    hold.cancel('E');pressed=false;
    if(hold.start('F',4,2))return false; // Source disables the bar during fade.
    if(!step(125)||!step(100)||emitted!=1)return false;
    if(!hold.start('F',4,2)||!hold.pressed||hold.timer)return false;
    hold.advance(333);hold.unfocus();
    if(hold.pressed||hold.owner!='F'||hold.advance(100)||hold.timer!=233)return false;
    if(hold.start('F',4,2)||hold.start('E',4,2))return false;
    hold.cancel('E');if(hold.owner!='F')return false;
    hold.cancel('F');hold.advance(234);
    return !hold.timer&&!hold.visual().alpha_q12&&!hold.owner;
}
