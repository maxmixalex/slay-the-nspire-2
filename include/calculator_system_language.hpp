#pragma once
#include "locale_languages.hpp"
#include <stdint.h>

namespace spire {
// The OS formatter returns the BASIC string value of getLangInfo(). Keep the
// OS allocation behind this interface, so startup uses no resident ZIP parser
// or language table derived from a particular calculator OS image.
using CalculatorLocaleEvaluate=int(*)(void*,void*,const uint16_t*,void*,void*);
using CalculatorLocaleFormat=int(*)(void*,void*,uint16_t**);
using CalculatorLocaleFree=void(*)(void*);
inline unsigned calculator_language_result(const uint16_t*text){
    if(!text)return 0;
    char tag[32]{};unsigned count=0;
    const bool quoted=*text=='"';if(quoted)++text;
    while(*text&&(!quoted||*text!='"')){
        if(count+1>=sizeof tag||*text>127)return 0;
        tag[count++]=char(*text++);
    }
    if(quoted&&(*text!='"'||text[1]))return 0;
    return locale_system_language_id(tag);
}
inline unsigned calculator_system_language(CalculatorLocaleEvaluate evaluate,
                                           CalculatorLocaleFormat format,
                                           CalculatorLocaleFree release){
    if(!evaluate||!format||!release)return 0;
    static const uint16_t query[]={'g','e','t','L','a','n','g','I','n','f','o','(',')',0};
    void*expression=nullptr;uint16_t*text=nullptr;int offset=0;unsigned language=0;
    const int evaluated=evaluate(nullptr,nullptr,query,&expression,&offset);
    if(!evaluated&&expression&&!format(expression,nullptr,&text))language=calculator_language_result(text);
    if(text&&static_cast<void*>(text)!=expression)release(text);
    if(expression)release(expression);
    return language;
}

struct CalculatorSettingsEntry {
    const uint8_t*data=nullptr;
    unsigned compressed=0,plain=0;
    bool deflated=false;
};
inline unsigned calculator_language_xml(const uint8_t*data,unsigned size){
    if(!data)return 0;
    static const char open[]="<lang>",close[]="</lang>";
    for(unsigned i=0;i+sizeof(open)-1<size;++i){
        unsigned k=0;while(k<sizeof(open)-1&&data[i+k]==uint8_t(open[k]))++k;
        if(k!=sizeof(open)-1)continue;
        char tag[32]{};unsigned n=0,at=i+k;
        while(at<size&&data[at]!='<'&&n+1<sizeof tag){
            if(data[at]>127)return 0;tag[n++]=char(data[at++]);
        }
        if(at+sizeof(close)-1>size)return 0;
        for(k=0;k<sizeof(close)-1;++k)if(data[at+k]!=uint8_t(close[k]))return 0;
        return locale_system_language_id(tag);
    }
    return 0;
}
inline bool calculator_settings_entry(const uint8_t*data,unsigned size,CalculatorSettingsEntry&entry){
    entry={};if(!data)return false;
    const auto u16=[](const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);};
    const auto u32=[&](const uint8_t*p){return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);};
    unsigned at=0;
    // Read only the bounded settings entry. CRC/hash scans are unnecessary
    // for obtaining an OS preference; bounds and exact inflate remain needed.
    while(at<=size&&size-at>=30&&u32(data+at)==0x04034b50u){
        const uint8_t*p=data+at;const unsigned flags=u16(p+6),method=u16(p+8);
        const uint32_t compressed=u32(p+18),plain=u32(p+22);
        const unsigned name=u16(p+26),extra=u16(p+28);
        if(flags&9u||name>size-at-30||extra>size-at-30-name)return false;
        const unsigned payload=at+30+name+extra;
        if(compressed>size-payload)return false;
        static const char wanted[]="settings.xml";
        bool matched=name==sizeof(wanted)-1;
        for(unsigned i=0;matched&&i<name;++i)matched=p[30+i]==uint8_t(wanted[i]);
        if(matched){
            if(!plain||plain>8192||(method!=0&&method!=8)||(method==0&&compressed!=plain))return false;
            entry={data+payload,compressed,plain,method==8};return true;
        }
        at=payload+compressed;
    }
    return false;
}
}
