#pragma once
#include <stdint.h>
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
}
