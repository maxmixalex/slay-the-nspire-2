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

// OS locale tags use ISO language names, optional script/region subtags and,
// on desktop hosts, an encoding suffix. The installed pack names stay the
// original game's three-letter catalog. Unknown system locales fall through
// to English rather than silently selecting an unrelated installed language.
inline unsigned locale_system_language_id(const char*tag){
    if(!tag)return 0;
    char language[4]{};unsigned n=0;
    while(*tag&&*tag!='-'&&*tag!='_'&&*tag!='.'&&*tag!='@'){
        if(n==3)return 0;
        const char ch=*tag++;language[n++]=ch>='A'&&ch<='Z'?char(ch-'A'+'a'):ch;
    }
    if(n==3)return locale_language_id(language);
    if(n!=2)return 0;
    const unsigned key=(unsigned(uint8_t(language[0]))<<8)|unsigned(uint8_t(language[1]));
    if(key==(('z'<<8)|'h')){
        bool traditional=false,simplified=false;
        while(*tag=='-'||*tag=='_'){
            ++tag;char part[8]{};unsigned count=0;
            while(*tag&&*tag!='-'&&*tag!='_'&&*tag!='.'&&*tag!='@'){
                const char ch=*tag++;if(count<7)part[count++]=ch>='A'&&ch<='Z'?char(ch-'A'+'a'):ch;
            }
            traditional|=(count==2&&((part[0]=='t'&&part[1]=='w')||(part[0]=='h'&&part[1]=='k')||(part[0]=='m'&&part[1]=='o')))||
                (count==4&&part[0]=='h'&&part[1]=='a'&&part[2]=='n'&&part[3]=='t');
            simplified|=count==4&&part[0]=='h'&&part[1]=='a'&&part[2]=='n'&&part[3]=='s';
        }
        return traditional&&!simplified?3:2;
    }
    if(key==(('e'<<8)|'s')){
        while(*tag=='-'||*tag=='_'){
            ++tag;char part[4]{};unsigned count=0;
            while(*tag&&*tag!='-'&&*tag!='_'&&*tag!='.'&&*tag!='@'){
                const char ch=*tag++;if(count<4)part[count]=ch>='A'&&ch<='Z'?char(ch-'A'+'a'):ch;++count;
            }
            // Four-letter script subtags do not choose a region. Bare
            // Spanish and an explicit Spain region use the Spain edition.
            if(count==2)return part[0]=='e'&&part[1]=='s'?14:5;
            if(count==3&&part[0]>='0'&&part[0]<='9')return 5;
        }
        return 14;
    }
    switch(key){
    case ('e'<<8)|'n':return 1;case ('d'<<8)|'e':return 4;case ('f'<<8)|'r':return 6;
    case ('i'<<8)|'d':return 7;case ('i'<<8)|'t':return 8;case ('j'<<8)|'a':return 9;
    case ('k'<<8)|'o':return 10;case ('p'<<8)|'l':return 11;case ('p'<<8)|'t':return 12;
    case ('r'<<8)|'u':return 13;case ('t'<<8)|'h':return 15;case ('t'<<8)|'r':return 16;
    default:return 0;
    }
}
inline unsigned locale_default_language(uint32_t installed,unsigned saved,unsigned system){
    if(installed&locale_language_mask(saved))return saved;
    if(installed&locale_language_mask(system))return system;
    if(installed&locale_language_mask(1))return 1;
    return locale_language_at(installed,0);
}
}
