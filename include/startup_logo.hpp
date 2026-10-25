#pragma once
#include <stdint.h>
#include "room_ui7_presentation.hpp"

namespace spire {
// NGame's first LaunchMainMenu and NLogoAnimation share one host-lifetime
// presentation. It never modifies a run, consumes gameplay RNG or allocates
// a pixel surface. Reopening the main menu or replacing its locale is not a
// new process launch and must not replay it.
struct StartupLogoPresentation {
    static constexpr uint32_t InitialFade=800,Delay=1000,Clip=3000,Color=2000,Hold=1000;
    static constexpr uint32_t ClipStart=InitialFade+Delay,ClipEnd=ClipStart+Clip;
    static constexpr uint32_t NaturalFade=ClipEnd+Color+Hold,ExitFade=800,MenuFade=2000;
    uint32_t start=0,fade_at=NaturalFade,canceled_at=UINT32_MAX;
    bool attempted=false,running=false,skipped=false;
    struct Frame {bool menu=false;unsigned image=0,alpha=0,background=0,black=0;int y=0;};
    bool begin(uint32_t now,bool assets,bool skip){
        if(attempted)return false;attempted=true;
        if(!assets)return false;
        start=now;fade_at=NaturalFade;canceled_at=UINT32_MAX;running=true;skipped=skip;return true;
    }
    uint32_t menu_at()const{return skipped?0:fade_at+ExitFade;}
    bool active()const{return running;}
    void advance(uint32_t now){if(running&&now-start>=menu_at()+MenuFade)running=false;}
    void cancel(uint32_t now){
        const uint32_t age=now-start;
        // Source only observes cancel before or while the Spine track plays;
        // its final color tween and hold finish normally.
        if(!running||skipped||canceled_at!=UINT32_MAX||age>=ClipEnd)return;
        canceled_at=age;
        fade_at=age<InitialFade?age:age<ClipStart?ClipStart:age+250;
    }
    Frame sample(uint32_t now)const{
        Frame frame{};const uint32_t age=now-start,menu=menu_at();
        if(age>=menu){
            frame.menu=true;
            const unsigned q=room_curve::linear(age-menu,MenuFade);
            frame.black=255*(4096-room_curve::multiply(room_curve::multiply(q,q),q))/4096;
            return frame;
        }
        if(age<ClipStart)return frame;
        const uint32_t clip_age=age-ClipStart;
        frame.image=clip_age>=Clip?36:clip_age*12/1000;
        const uint32_t entry=canceled_at==UINT32_MAX?clip_age:canceled_at<ClipStart?0:canceled_at-ClipStart;
        frame.y=-152*(4096-room_curve::back_out(room_curve::linear(entry,500)))/4096;
        frame.alpha=255*room_curve::linear(entry,500)/4096;
        if(canceled_at!=UINT32_MAX){
            if(canceled_at<ClipStart)frame.alpha=0;
            else frame.alpha=frame.alpha*(4096-room_curve::expo_out(age-canceled_at,250))/4096;
        }else if(age>=ClipEnd)frame.background=room_curve::cubic_out(age-ClipEnd,Color);
        if(age>=fade_at)frame.black=255*room_curve::linear(age-fade_at,ExitFade)/4096;
        return frame;
    }
};
static_assert(sizeof(StartupLogoPresentation)<=16,"Startup stores only bounded presentation clocks");
}
