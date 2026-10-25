#pragma once
#include <stddef.h>
#include <stdint.h>
#include "input_bindings.hpp"

namespace spire {
// Presentation preferences are separate from a run and its historical saves.
// Only explicitly supported settings are persisted. No checksum or resource
// scan is involved in reading this bounded record. Versions 1/2 retain their
// eight-byte flags and migrate to the host's visible default key table.
// Versions 4/5/6 assign the run-timer/long-press/skip-logo bits without growing the record;
// earlier versions retain their exact flag meanings and default to off.
struct UiPreferences {
    bool fast_mode=false;
    bool show_card_indices=false;
    bool text_effects=true;
    bool keyboard_only=false;
    bool phobia_mode=false;
    uint8_t screen_shake=2;
    bool bestiary_actions_preferred=true;
    InputBindings bindings=default_input_bindings();
    bool bindings_saved=false;
    bool show_run_timer=false;
    bool long_press=false;
    bool skip_intro_logo=false;
};
constexpr unsigned UiPreferencesBytes=8+4*InputCommandCount;
inline unsigned ui_preference_flags(const UiPreferences& p){
    return unsigned(p.fast_mode)|(unsigned(p.show_card_indices)<<1)|
        (unsigned(p.text_effects)<<2)|(unsigned(p.keyboard_only)<<3)|
        (unsigned(p.phobia_mode)<<4)|(unsigned(!p.bestiary_actions_preferred)<<5)|
        (unsigned(p.show_run_timer)<<6)|(unsigned(p.long_press)<<7)|(unsigned(p.screen_shake)<<8)|
        (unsigned(p.skip_intro_logo)<<11);
}
inline bool ui_preferences_from_flags(unsigned flags,UiPreferences& out){
    if((flags&~0xfffu)||((flags>>8)&7u)>4)return false;
    UiPreferences p{};p.fast_mode=flags&1;p.show_card_indices=flags&2;
    p.text_effects=flags&4;p.keyboard_only=flags&8;p.phobia_mode=flags&16;
    p.screen_shake=uint8_t((flags>>8)&7u);p.bestiary_actions_preferred=!(flags&32);p.show_run_timer=flags&64;p.long_press=flags&128;p.skip_intro_logo=flags&2048;out=p;return true;
}
inline void encode_ui_preferences(const UiPreferences& prefs,uint8_t* out){
    const unsigned flags=ui_preference_flags(prefs);
    out[0]='N';out[1]='S';out[2]='P';out[3]='F';out[4]=6;
    out[5]=uint8_t(flags);out[6]=uint8_t(flags>>8);out[7]=0;
    unsigned at=8;for(const auto&bank:prefs.bindings.keys)for(uint16_t key:bank){out[at++]=uint8_t(key);out[at++]=uint8_t(key>>8);}
}
inline bool decode_ui_preferences(const uint8_t* bytes,size_t count,UiPreferences& out){
    if(!bytes||(count!=8&&count!=UiPreferencesBytes)||bytes[0]!='N'||bytes[1]!='S'||
       bytes[2]!='P'||bytes[3]!='F'||bytes[7])return false;
    if(bytes[4]==1){
        if(count!=8||bytes[5]>1||bytes[6])return false;
        UiPreferences p{};p.fast_mode=bytes[5]!=0;out=p;return true;
    }
    if(bytes[4]<2||bytes[4]>6)return false;
    if(count!=(bytes[4]==2?8u:UiPreferencesBytes))return false;
    const unsigned flags=unsigned(bytes[5])|(unsigned(bytes[6])<<8);
    if(bytes[4]<4&&(flags&64))return false;
    if(bytes[4]<5&&(flags&128))return false;
    if(bytes[4]<6&&(flags&2048))return false;
    UiPreferences p{};if(!ui_preferences_from_flags(flags,p))return false;
    if(bytes[4]>=3){unsigned at=8;for(auto&bank:p.bindings.keys)for(auto&key:bank){key=uint16_t(bytes[at]|unsigned(bytes[at+1])<<8);at+=2;}
        if(!valid_input_bindings(p.bindings))return false;p.bindings_saved=true;}
    out=p;return true;
}
}
