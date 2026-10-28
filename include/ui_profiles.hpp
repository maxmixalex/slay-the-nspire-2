#pragma once
#include <stddef.h>
#include <stdint.h>
#include "health_bar.hpp"
#include "reference_viewport.hpp"
namespace spire {
constexpr unsigned UiProfileCount=3,UiProfileSelectorBytes=8;
namespace profile_layout {
constexpr int Width=92,Height=145,Top=48;
constexpr int center(unsigned i){return 62+int(i)*98;}
constexpr int left(unsigned i){return center(i)-Width/2;}
constexpr int DeleteTop=198;
}
struct UiProfileLabelMotion {
    uint32_t changed=0;unsigned from=0;bool focused=false;
    unsigned alpha(uint32_t now)const{
        const unsigned age=now-changed,duration=focused?200:50,t=age>=duration?4096:age*4096/duration;
        return unsigned(int(from)+((focused?255:0)-int(from))*int(t)/4096);
    }
    unsigned observe(bool next,uint32_t now){const unsigned current=alpha(now);if(next!=focused){focused=next;from=current;changed=now;}return current;}
};
struct UiProfileMotion {
    uint32_t changed=0;int from=0;bool focused=false;
    int value(uint32_t now,bool menu_button=false)const{
        const unsigned age=now-changed,duration=focused?50:menu_button?300:500;
        const int t=age>=duration?4096:focused&&!menu_button?int(age*4096/duration):int(health_bar::expo_out(age*1000/duration));
        return from+((focused?4096:0)-from)*t/4096;
    }
    int observe(bool next,uint32_t now,bool menu_button=false){const int current=value(now,menu_button);if(next!=focused){focused=next;from=current;changed=now;}return current;}
};
inline unsigned decode_profile_selector(const uint8_t*data,size_t size){
    return data&&size==UiProfileSelectorBytes&&data[0]=='N'&&data[1]=='S'&&data[2]=='L'&&data[3]=='T'&&
        data[4]==1&&data[5]>=1&&data[5]<=UiProfileCount&&!data[6]&&!data[7]?data[5]:1;
}
inline void encode_profile_selector(unsigned profile,uint8_t*out){
    out[0]='N';out[1]='S';out[2]='L';out[3]='T';out[4]=1;
    out[5]=uint8_t(profile>=1&&profile<=UiProfileCount?profile:1);out[6]=out[7]=0;
}
// Slot one keeps the historical filename. Each optional profile has its own
// run/history; its filename retains .tns on the calculator.
inline bool profile_scoped_path(char*out,size_t capacity,const char*base,unsigned profile){
    if(!out||!capacity||!base||profile<1||profile>UiProfileCount)return false;
    size_t n=0;while(base[n])++n;const bool tns=n>=4&&base[n-4]=='.'&&base[n-3]=='t'&&base[n-2]=='n'&&base[n-1]=='s';
    const size_t prefix=n-(tns?4:0),extra=profile==1?0:3;
    if(n+extra>=capacity){out[0]=0;return false;}
    size_t at=0;for(;at<prefix;++at)out[at]=base[at];
    if(extra){out[at++]='.';out[at++]='p';out[at++]=char('0'+profile);}
    for(size_t i=prefix;i<=n;++i)out[at++]=base[i];return true;
}
}
