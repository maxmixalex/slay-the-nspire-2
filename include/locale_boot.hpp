#pragma once
#include "resource_source.hpp"
#include "hud_ui7_assets.hpp"

namespace spire {
// Built-in load-error text remains independent of resource packs.
// Unknown is deliberately bilingual, including missing/invalid-pack errors.
enum class BootLanguage : uint8_t { Unknown=0,English=1,Chinese=2 };
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
    static const char*const chinese[]={
        "打开资源","校验资源数据","读取目录","读取图像","读取动画索引","读取字形","读取文本","准备就绪",
        "准备战斗","未找到资源包","资源读取失败","选择语言",
        "ENTER 确认","ESC 返回","4 中文 / 6 ENGLISH","未找到语言包","正在启动","中文"
    };
    static const char*const bilingual[]={
        "打开资源 / OPENING","校验资源 / VERIFYING DATA","读取目录 / DIRECTORY","读取图像 / IMAGES",
        "动画索引 / ANIMATION INDEX","读取字形 / FONTS","读取文本 / TEXT","准备就绪 / READY",
        "准备战斗 / PREPARING BATTLE","未找到资源 / RESOURCES NOT FOUND",
        "资源读取失败 / LOAD FAILED","选择语言 / LANGUAGE",
        "ENTER 确认 / CONFIRM","ESC 返回 / BACK","4 中文 / 6 ENGLISH",
        "未找到语言包 / MISSING LANGUAGE","正在启动 / STARTING","中文 / ENGLISH"
    };
    static_assert(sizeof(english)/sizeof(*english)==unsigned(BootText::Count)&&
        sizeof(chinese)/sizeof(*chinese)==unsigned(BootText::Count)&&
        sizeof(bilingual)/sizeof(*bilingual)==unsigned(BootText::Count),"Every boot message needs all display languages");
    unsigned n=unsigned(id);if(n>=unsigned(BootText::Count))n=0;
    return language==BootLanguage::English?english[n]:language==BootLanguage::Chinese?chinese[n]:bilingual[n];
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
    if(version>HudUi7ContentVersion)return BootLanguage::Unknown;
    const uint32_t images=u32(h+16),glyphs=u32(h+20),strings=u32(h+24);
    if(!images||images>65535||!glyphs||glyphs>32768||strings<=6110||strings>16384)return BootLanguage::Unknown;
    uint8_t row[8],marker[4];if(!read(40+images*16+glyphs*16+6110*8,row,8)||u32(row+4)!=3||!read(u32(row),marker,4)||marker[3])return BootLanguage::Unknown;
    if(marker[0]=='e'&&marker[1]=='n'&&marker[2]=='g')return BootLanguage::English;
    if(marker[0]=='z'&&marker[1]=='h'&&marker[2]=='s')return BootLanguage::Chinese;
    return BootLanguage::Unknown;
}
inline BootLanguage boot_preference(const uint8_t*bytes,uint32_t length){
    if(!bytes||length!=8||bytes[0]!='N'||bytes[1]!='S'||bytes[2]!='L'||bytes[3]!='1'||bytes[7]!='\n')return BootLanguage::Unknown;
    if(bytes[4]=='e'&&bytes[5]=='n'&&bytes[6]=='g')return BootLanguage::English;
    if(bytes[4]=='z'&&bytes[5]=='h'&&bytes[6]=='s')return BootLanguage::Chinese;
    return BootLanguage::Unknown;
}
inline const char* boot_preference_bytes(BootLanguage language){return language==BootLanguage::English?"NSL1eng\n":"NSL1zhs\n";}
} // namespace spire
