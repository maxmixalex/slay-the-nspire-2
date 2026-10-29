#pragma once
#include "resource_source.hpp"
#include "game_over_transition_assets.hpp"
#include "grouped_glyphs.hpp"
#include "grouped_strings.hpp"
#include "bestiary_kaiser_assets.hpp"
#include "scene_font_assets.hpp"
#include "retained_filter.hpp"
#include "speech_bubble_assets.hpp"
#include "history_body_ui10_assets.hpp"
#include "ui12_assets.hpp"

namespace spire {
// Built-in load-error text remains independent of resource packs.
// These recovery messages deliberately use English for every locale, including
// an unavailable or malformed language pack; they never depend on game fonts.
enum class BootLanguage : uint8_t { Unknown=0,English=1,Chinese=2,TraditionalChinese=3,German=4,LatinAmericanSpanish=5,French=6,Indonesian=7,Italian=8,Japanese=9,Korean=10,Polish=11,Portuguese=12,Russian=13,Spanish=14,Thai=15,Turkish=16 };
enum class BootText : uint8_t {
    Opening,Integrity,Directory,Images,AnimationIndex,Glyphs,Strings,Ready,
    PreparingBattle,MissingResources,InvalidResources,ChooseLanguage,
    Confirm,Back,ChooseKeys,MissingLanguage,Starting,NativeLanguage,Count
};
inline BootLanguage boot_language(bool english){return english?BootLanguage::English:BootLanguage::Chinese;}
inline const char* boot_text(BootText id,BootLanguage language){
    static const char*const english[]={
        "OPENING","VERIFYING DATA","DIRECTORY","IMAGES","ANIMATION INDEX","FONTS","TEXT","READY",
        "PREPARING BATTLE","RESOURCES NOT FOUND","RESOURCE LOADING FAILED","CHOOSE LANGUAGE",
        "ENTER CONFIRM","ESC BACK","4 CHINESE / 6 ENGLISH","LANGUAGE PACK NOT FOUND","STARTING","ENGLISH"
    };
    static_assert(sizeof(english)/sizeof(*english)==unsigned(BootText::Count),
        "Every boot message needs a built-in English fallback");
    (void)language;
    unsigned n=unsigned(id);if(n>=unsigned(BootText::Count))n=0;
    return english[n];
}

// A bounded metadata hint selects boot text before Content opens the directory.
inline BootLanguage boot_probe_language(ResourceSource source){
    auto read=[&](uint32_t offset,void*dst,uint32_t n){
        if(offset>source.length||n>source.length-offset)return false;
        if(source.direct){auto*out=static_cast<uint8_t*>(dst);for(uint32_t i=0;i<n;++i)out[i]=source.direct[offset+i];return true;}
        return source.read_at&&source.read_at(source.context,offset,dst,n);
    };
    auto u32=[](const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);};
    uint8_t h[40];if(!read(0,h,sizeof h)||h[0]!='N'||h[1]!='S'||h[2]!='P'||h[3]!='2'||
        u32(h+8)!=0x41cef1eau||u32(h+12)!=source.length)return BootLanguage::Unknown;
    const uint32_t version=u32(h+4);if(version<17)return version>=1?BootLanguage::Chinese:BootLanguage::Unknown;
    if(version>Ui12ContentVersion)return BootLanguage::Unknown;
    const uint32_t images=u32(h+16),glyphs=u32(h+20),strings=u32(h+24);
    if(!images||images>65535||!glyphs||glyphs>scene_font_glyph_limit(version)||strings<=6110||strings>16384)return BootLanguage::Unknown;
    uint8_t row[8],marker[4];if(!read(40+images*16+glyphs*16+6110*8,row,8)||u32(row+4)!=3||!read(u32(row),marker,4)||marker[3])return BootLanguage::Unknown;
    return static_cast<BootLanguage>(locale_language_id(reinterpret_cast<const char*>(marker)));
}
inline BootLanguage boot_preference(const uint8_t*bytes,uint32_t length){
    if(!bytes||length!=8||bytes[0]!='N'||bytes[1]!='S'||bytes[2]!='L'||bytes[3]!='1'||bytes[7]!='\n')return BootLanguage::Unknown;
    const char code[]={char(bytes[4]),char(bytes[5]),char(bytes[6]),0};
    return static_cast<BootLanguage>(locale_language_id(code));
}
inline const char* boot_preference_bytes(BootLanguage language){return locale_language_info(unsigned(language)).preference;}
} // namespace spire
