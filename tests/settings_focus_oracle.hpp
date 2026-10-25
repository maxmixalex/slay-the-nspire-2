#pragma once
#include "settings_focus.hpp"
#include <cmath>

// Independent source timeline: Godot's default linear TweenProperty from
// modulate:a0.2 to0 over0.1s, quantized once to the host's8-bit coverage.
inline bool source_settings_focus_timeline(){
    using namespace spire;ButtonMotion row;
    constexpr uint32_t first=0x738b5000u,second=first+1;
    if(settings_focus::observe(row,first,false,1000)!=0)return false;
    if(settings_focus::observe(row,first,true,1001)!=51)return false;
    if(settings_focus::observe(row,first,false,1100)!=51)return false;
    for(unsigned ms=0;ms<=125;++ms){const double remaining=ms>=100?0.0:1.0-double(ms)/100.0;
        const unsigned expected=unsigned(std::lround(255.0*0.2*remaining));
        if(settings_focus::observe(row,first,false,1100+ms)!=expected)return false;
    }
    // Refocus kills the interrupted fade immediately. Changing the slot's
    // command identity cannot inherit its previous owner's background.
    if(settings_focus::observe(row,first,true,1300)!=51)return false;
    settings_focus::observe(row,first,false,1400);
    if(settings_focus::observe(row,first,false,1450)!=26)return false;
    if(settings_focus::observe(row,first,true,1451)!=51)return false;
    if(settings_focus::observe(row,second,false,1452)!=0)return false;
    ButtonMotion slots[3]{};
    settings_focus::observe(slots[0],first,true,1500);settings_focus::observe(slots[1],second,true,1500);
    slots[2].identity=0x1234;slots[2].state=ButtonState::Focus;slots[2].from.hover=2048;
    settings_focus::blur_rows(slots,3,1600,second);
    if(slots[0].state!=ButtonState::Rest||slots[1].state!=ButtonState::Focus||slots[2].state!=ButtonState::Focus||slots[2].from.hover!=2048)return false;
    if(settings_focus::opacity(slots[0],1650)!=26||settings_focus::opacity(slots[1],1650)!=51)return false;
    settings_focus::blur_rows(slots,3,1700);
    if(settings_focus::opacity(slots[1],1800)!=0)return false;
    settings_focus::observe(row,first,true,UINT32_MAX-30u);settings_focus::observe(row,first,false,UINT32_MAX-20u);
    return settings_focus::opacity(row,29)==26&&settings_focus::opacity(row,79)==0;
}
