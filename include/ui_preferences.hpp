#pragma once
#include <stddef.h>
#include <stdint.h>

namespace spire {
// Presentation preferences are separate from a run and its historical saves.
// Only explicitly supported settings are persisted. No checksum or resource
// scan is involved in reading this fixed eight-byte record.
struct UiPreferences { bool fast_mode=false; };
constexpr unsigned UiPreferencesBytes=8;
inline void encode_ui_preferences(const UiPreferences& prefs,uint8_t* out){
    out[0]='N';out[1]='S';out[2]='P';out[3]='F';out[4]=1;
    out[5]=prefs.fast_mode?1:0;out[6]=out[7]=0;
}
inline bool decode_ui_preferences(const uint8_t* bytes,size_t count,UiPreferences& out){
    if(!bytes||count!=UiPreferencesBytes||bytes[0]!='N'||bytes[1]!='S'||
       bytes[2]!='P'||bytes[3]!='F'||bytes[4]!=1||bytes[5]>1||bytes[6]||bytes[7])return false;
    out.fast_mode=bytes[5]!=0;return true;
}
}
