#pragma once
#include <stdint.h>

namespace spire {
// NEndTurnLongPressBar owns only presentation/input time. Neither the run's
// clock nor its RNG participates, and a captured press survives only its own
// release. Mouse, physical hotkeys and host command buttons share this timer.
struct EndTurnHold {
    static constexpr uint16_t Mouse=0xfffe,Semantic=0xffff;
    static constexpr unsigned Duration=500,FadeDuration=250,Unit=4096;
    uint32_t context=0;
    int32_t turn=0;
    uint16_t owner=0,timer=0,completion=0,fade_age=0;
    bool pressed=false,fired=false,dispatching=false;
    bool start(uint16_t input,uint32_t captured_context,int captured_turn){
        if(!input||(fired&&fade_age<FadeDuration)||owner)return false;
        if(fired){fired=false;timer=0;}
        owner=input;context=captured_context;turn=captured_turn;pressed=true;return true;
    }
    void cancel(uint16_t input,bool release=true){
        if(owner!=input)return;
        pressed=false;if(release)owner=0;
    }
    void unfocus(){pressed=false;}
    bool advance(unsigned ms){
        if(fired){fade_age=uint16_t(ms>=FadeDuration-fade_age?FadeDuration:fade_age+ms);return false;}
        if(pressed){
            // The source comparison is strictly greater than half a second.
            if(ms>Duration-timer){completion=timer;timer=0;fade_age=0;fired=true;pressed=false;return true;}
            timer=uint16_t(timer+ms);
        }else timer=uint16_t(ms>=timer?0:timer-ms);
        return false;
    }
    struct Visual {unsigned width_q12=0,alpha_q12=0;uint16_t red=0,green=0,blue=153;};
    Visual visual()const{
        Visual result{};const unsigned time=fired?completion:timer;
        if(!time||(fired&&fade_age>=FadeDuration))return result;
        result.width_q12=fired?204*Unit:204*Unit*time/Duration;
        // Ease.CubicOut(0.75*q), retaining the last recalculated color and
        // alpha when completion sets only Size before its linear fade.
        const unsigned inverse=Unit-3*time*Unit/2000;
        const unsigned cube=unsigned((uint64_t(inverse)*inverse*inverse+(uint64_t(1)<<23))>>24);
        result.alpha_q12=Unit-cube;
        if(fired)result.alpha_q12=result.alpha_q12*(FadeDuration-fade_age)/FadeDuration;
        result.red=uint16_t(time*255*5/1000);result.green=uint16_t(153+time*255/Duration);
        return result;
    }
};
static_assert(sizeof(EndTurnHold)<=24,"Long-press confirmation must remain a small input timer");
}
