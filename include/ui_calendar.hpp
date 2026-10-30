#pragma once
#include <stdint.h>
#include <stddef.h>
namespace spire {
struct UiCalendar {unsigned year,month,day,weekday,hour,minute;};
inline UiCalendar ui_calendar(uint32_t timestamp){
    const unsigned total_days=timestamp/86400;unsigned days=total_days,year=1970;
    const auto leap=[](unsigned y){return y%4==0&&(y%100!=0||y%400==0);};
    while(days>=(leap(year)?366u:365u)){days-=leap(year)?366:365;++year;}
    const unsigned lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};unsigned month=0;
    while(month<11&&days>=lengths[month]+unsigned(month==1&&leap(year))){days-=lengths[month]+unsigned(month==1&&leap(year));++month;}
    return {year,month+1,days+1,(total_days+4)%7,timestamp/3600%24,timestamp/60%60};
}

namespace calendar_detail {
struct Locale {
    const char* months;
    const char* genitive;
    const char* am;
    const char* pm;
    const char* date_separator;
    unsigned year_offset;
    char time_separator;
};
#include "ui_calendar_locale_data.inc"

struct Writer {
    char* out;size_t capacity,used=0;bool good=true;
    void append(const char* text,size_t count){
        if(!good)return;
        if(!out||used>=capacity||count>=capacity-used){good=false;return;}
        for(size_t i=0;i<count;++i)out[used++]=text[i];
        out[used]=0;
    }
    void text(const char* value){size_t n=0;while(value[n])++n;append(value,n);}
    void ch(char value){append(&value,1);}
    void number(unsigned value,unsigned minimum){
        char reversed[10];unsigned count=0;
        do{reversed[count++]=char('0'+value%10);value/=10;}while(value);
        while(count<minimum){ch('0');--minimum;}
        while(count)ch(reversed[--count]);
    }
    bool finish(){if(!good&&out&&capacity)out[0]=0;return good;}
};
inline const char* month_name(const char* names,unsigned month){
    while(--month){while(*names)++names;++names;}return names;
}
// DateTime's full month name uses the genitive form when a numeric day
// token occurs elsewhere in the custom pattern. Quoted/escaped 'd' is text.
inline bool numeric_day(const char* p){
    char quote=0;
    while(*p){
        const char ch=*p++;
        if(ch=='\\'){if(*p)++p;continue;}
        if(quote){if(ch==quote)quote=0;continue;}
        if(ch=='\''||ch=='"'){quote=ch;continue;}
        if(ch=='d'){unsigned n=1;while(*p=='d'){++p;++n;}if(n<=2)return true;}
    }
    return false;
}
}

// The custom-pattern subset used by the pinned history/daily resources.
// Language 0 selects InvariantCulture; IDs 1..16 match the resource contract.
// This formats already-selected calendar fields: it never changes timezone,
// UTC daily boundaries, saved timestamps or gameplay seeds. No locale/ICU
// runtime, allocation or writable static table is needed on the calculator.
inline bool ui_calendar_format(const UiCalendar& value,unsigned language,
                               const char* pattern,char* out,size_t capacity){
    if(out&&capacity)out[0]=0;
    if(!out||!capacity||!pattern||!value.month||value.month>12||
       !value.day||value.day>31||value.hour>23||value.minute>59)return false;
    const auto& locale=calendar_detail::Locales[language>=1&&language<=16?language-1:0];
    calendar_detail::Writer writer{out,capacity};
    const bool genitive=calendar_detail::numeric_day(pattern);
    while(*pattern&&writer.good){
        const char ch=*pattern++;
        if(ch=='\''||ch=='"'){
            while(*pattern&&*pattern!=ch){
                if(*pattern=='\\'){++pattern;if(!*pattern)break;}
                writer.ch(*pattern++);
            }
            if(*pattern!=ch){writer.good=false;break;}++pattern;continue;
        }
        if(ch=='\\'){
            if(!*pattern){writer.good=false;break;}
            writer.ch(*pattern++);continue;
        }
        unsigned count=1;
        while(*pattern==ch){++count;++pattern;}
        switch(ch){
        case 'd':
            if(count>2)writer.good=false;else writer.number(value.day,count);break;
        case 'M':
            if(count<=2)writer.number(value.month,count);
            else if(count==4)writer.text(calendar_detail::month_name(genitive&&locale.genitive?locale.genitive:locale.months,value.month));
            else writer.good=false;
            break;
        case 'y':{
            const unsigned year=value.year+locale.year_offset;
            if(count>10)writer.good=false;else writer.number(count<=2?year%100:year,count);break;
        }
        case 'H':case 'h':case 'm':
            if(count>2)writer.good=false;
            else writer.number(ch=='m'?value.minute:ch=='H'?value.hour:value.hour%12?value.hour%12:12,count);
            break;
        case 't':{
            const char* text=value.hour<12?locale.am:locale.pm;
            if(count==1&&*text){const unsigned first=uint8_t(*text);writer.append(text,first<0x80?1:first<0xe0?2:first<0xf0?3:4);}
            else if(count==2)writer.text(text);else writer.good=false;
            break;
        }
        case ':':while(count--)writer.ch(locale.time_separator);break;
        case '/':while(count--)writer.text(locale.date_separator);break;
        // Standard DateTime patterns/other fields are deliberately outside
        // this bounded custom format contract, rather than printed as text.
        case '%':case 'g':case 'f':case 'F':case 's':case 'z':case 'K':writer.good=false;break;
        default:while(count--)writer.ch(ch);break;
        }
    }
    return writer.finish();
}

// The existing Thai edition pre-bakes graphemes instead of carrying a font
// shaper in RAM. Calendar words are dynamic, so they have eleven stable
// display-only keys outside the old pack-local PUA and ASCII-kerning ranges.
// Every replacement is shorter than its original UTF-8 cluster, allowing an
// in-place conversion. Callers check exact glyph availability before drawing.
inline void ui_calendar_shape_thai(char* text){
    if(!text)return;
    char* out=text;const char* in=text;
    while(*in){
        uint32_t codepoint=0;size_t length=0;
        if(uint8_t(*in)>=0xe0)for(const auto& cluster:calendar_detail::ThaiMonthClusters){
            size_t n=0;while(cluster.text[n]&&in[n]==cluster.text[n])++n;
            if(!cluster.text[n]){codepoint=cluster.codepoint;length=n;break;}
        }
        if(!length){*out++=*in++;continue;}
        in+=length;
        *out++=char(0xf0|(codepoint>>18));*out++=char(0x80|((codepoint>>12)&63));
        *out++=char(0x80|((codepoint>>6)&63));*out++=char(0x80|(codepoint&63));
    }
    *out=0;
}
}
