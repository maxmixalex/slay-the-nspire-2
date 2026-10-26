#pragma once
#include <stdint.h>

namespace spire {
// Stable language IDs. English/Simplified Chinese keep their shipped NSL1 IDs.
// The remaining languages are the pinned LocManager language/culture catalog.
constexpr unsigned LocaleLanguageCount=16;
struct LocaleLanguageInfo {const char*code;const char*name;const char*file;const char*preference;bool substitute,word_wrap;};
inline const LocaleLanguageInfo& locale_language_info(unsigned id){
    static const LocaleLanguageInfo values[]={
        {"eng","English","nSpire2.eng.dat.tns","NSL1eng\n",false,true},
        {"zhs","简体中文","nSpire2.zhs.dat.tns","NSL1zhs\n",true,false},
        {"zht","繁體中文","nSpire2.zht.dat.tns","NSL1zht\n",true,false},
        {"deu","Deutsch","nSpire2.deu.dat.tns","NSL1deu\n",false,true},
        {"esp","Español (Latinoamérica)","nSpire2.esp.dat.tns","NSL1esp\n",false,true},
        {"fra","Français","nSpire2.fra.dat.tns","NSL1fra\n",false,true},
        {"ind","Bahasa Indonesia","nSpire2.ind.dat.tns","NSL1ind\n",false,true},
        {"ita","Italiano","nSpire2.ita.dat.tns","NSL1ita\n",false,true},
        {"jpn","日本語","nSpire2.jpn.dat.tns","NSL1jpn\n",true,false},
        {"kor","한국어","nSpire2.kor.dat.tns","NSL1kor\n",true,true},
        {"pol","Polski","nSpire2.pol.dat.tns","NSL1pol\n",true,true},
        {"ptb","Português (Brasil)","nSpire2.ptb.dat.tns","NSL1ptb\n",false,true},
        {"rus","Русский","nSpire2.rus.dat.tns","NSL1rus\n",true,true},
        {"spa","Español (España)","nSpire2.spa.dat.tns","NSL1spa\n",false,true},
        {"tha","ภาษาไทย","nSpire2.tha.dat.tns","NSL1tha\n",true,false},
        {"tur","Türkçe","nSpire2.tur.dat.tns","NSL1tur\n",false,true}
    };
    return values[id>=1&&id<=LocaleLanguageCount?id-1:0];
}
constexpr bool locale_language_valid(unsigned id){return id>=1&&id<=LocaleLanguageCount;}
constexpr uint32_t locale_language_mask(unsigned id){return locale_language_valid(id)?1u<<(id-1):0;}
inline unsigned locale_language_id(const char*code){
    if(!code||!code[0]||!code[1]||!code[2]||code[3])return 0;
    for(unsigned id=1;id<=LocaleLanguageCount;++id){const char*p=locale_language_info(id).code;
        if(code[0]==p[0]&&code[1]==p[1]&&code[2]==p[2]&&!code[3])return id;}
    return 0;
}
inline unsigned locale_language_at(uint32_t mask,unsigned ordinal){
    for(unsigned id=1;id<=LocaleLanguageCount;++id)if(mask&locale_language_mask(id)){if(!ordinal--)return id;}
    return 0;
}
inline unsigned locale_language_count(uint32_t mask){unsigned count=0;for(unsigned id=1;id<=LocaleLanguageCount;++id)if(mask&locale_language_mask(id))++count;return count;}
}
