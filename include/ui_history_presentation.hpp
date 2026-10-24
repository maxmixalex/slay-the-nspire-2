#pragma once
#include <stdint.h>
namespace spire {
// NScrollableContainer's bounded UI-only target/current positions.
struct UiHistoryScroll {
    int position=0,target=0,maximum=0;
    bool dragging=false,thumb=false;
    int last_y=0;
    static int clamp(int v,int low,int high){return v<low?low:v>high?high:v;}
    void reset(){position=target=maximum=0;dragging=thumb=false;}
    void configure(int height){maximum=height>174?(height-174)*256:0;target=clamp(target,0,maximum);position=clamp(position,0,maximum);}
    int offset()const{return position/256;}
    void wheel(int rows){target=clamp(target+rows*80*256/9,-174*256,maximum+174*256);}
    void set_thumb(int y){target=maximum*clamp(y-61,0,128)/128;position=target;}
    int thumb_y()const{return 61+(maximum?128*clamp(position,0,maximum)/maximum:0);}
    void step(unsigned ms){
        if(!ms)return;const int dt=ms>66?66:int(ms);
        if(!dragging){const int legal=clamp(target,0,maximum);target+=(legal-target)*dt*12/1000;if(target<0&&target>-128)target=0;if(target>maximum&&target<maximum+128)target=maximum;}
        const int delta=target-position;
        if(delta>-128&&delta<128)position=target;
        else{const int change=delta*dt*15/1000;position+=change?change:(delta>0?1:-1);}
    }
};
struct UiMenuTransition {
    uint32_t start=0;uint8_t state=0,previous=0;bool bound=false;
    unsigned from_scale=1000;
    unsigned observe(unsigned next,uint32_t now,bool bottom){
        if(!bound){bound=true;state=previous=0;start=now;}
        const unsigned duration=state==1?50:state==2?200:300;
        unsigned age=now-start,t=age>=duration?1000:age*1000/duration;
        if(state!=1){const unsigned q=1000-t;t=1000-q*q/1000*q/1000;}
        const unsigned goal=state==1?(bottom?1050:1020):state==2?950:1000;
        const unsigned scale=unsigned(int(from_scale)+(int(goal)-int(from_scale))*int(t)/1000);
        if(next!=state){previous=state;state=uint8_t(next);start=now;from_scale=scale;}
        return scale;
    }
    unsigned alpha(uint32_t now)const{const unsigned duration=state==1?50:state==2?200:300;const unsigned age=now-start;return age>=duration?255:age*255/duration;}
};
}
