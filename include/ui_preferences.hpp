#pragma once
#include <stddef.h>
#include <stdint.h>

namespace spire {
// Presentation preferences are separate from a run and its historical saves.
// Only explicitly supported settings are persisted. No checksum or resource
// scan is involved in reading this fixed eight-byte record.
struct UiPreferences {
    bool fast_mode=false;
    bool show_card_indices=false;
    bool text_effects=true;
    bool keyboard_only=false;
    bool phobia_mode=false;
    uint8_t screen_shake=2;
    bool bestiary_actions_preferred=true;
};
constexpr unsigned UiPreferencesBytes=8;
inline unsigned ui_preference_flags(const UiPreferences& p){
    return unsigned(p.fast_mode)|(unsigned(p.show_card_indices)<<1)|
        (unsigned(p.text_effects)<<2)|(unsigned(p.keyboard_only)<<3)|
        (unsigned(p.phobia_mode)<<4)|(unsigned(!p.bestiary_actions_preferred)<<5)|(unsigned(p.screen_shake)<<8);
}
inline bool ui_preferences_from_flags(unsigned flags,UiPreferences& out){
    if((flags&~0x73fu)||(flags>>8)>4)return false;
    UiPreferences p{};p.fast_mode=flags&1;p.show_card_indices=flags&2;
    p.text_effects=flags&4;p.keyboard_only=flags&8;p.phobia_mode=flags&16;
    p.screen_shake=uint8_t(flags>>8);p.bestiary_actions_preferred=!(flags&32);out=p;return true;
}
inline void encode_ui_preferences(const UiPreferences& prefs,uint8_t* out){
    const unsigned flags=ui_preference_flags(prefs);
    out[0]='N';out[1]='S';out[2]='P';out[3]='F';out[4]=2;
    out[5]=uint8_t(flags);out[6]=uint8_t(flags>>8);out[7]=0;
}
inline bool decode_ui_preferences(const uint8_t* bytes,size_t count,UiPreferences& out){
    if(!bytes||count!=UiPreferencesBytes||bytes[0]!='N'||bytes[1]!='S'||
       bytes[2]!='P'||bytes[3]!='F'||bytes[7])return false;
    if(bytes[4]==1){
        if(bytes[5]>1||bytes[6])return false;
        UiPreferences p{};p.fast_mode=bytes[5]!=0;out=p;return true;
    }
    return bytes[4]==2&&ui_preferences_from_flags(unsigned(bytes[5])|(unsigned(bytes[6])<<8),out);
}
}
