#pragma once
#include "button_presentation.hpp"
#include "input_bindings.hpp"

namespace spire::settings_focus {
constexpr uint32_t FirstRow=0x738b5000u;
constexpr uint32_t identity(unsigned command){return FirstRow+command;}
inline bool owns(uint32_t key){return key>=FirstRow&&key<FirstRow+InputCommandCount;}
// NInputSettingsEntry sets alpha0.2 immediately on focus, kills any previous
// tween, and uses the default linear TweenProperty to reach zero in100ms.
// Reuse ButtonMotion's unused hover component; no new per-row allocation.
inline unsigned opacity(const ButtonMotion&motion,uint32_t now){
    if(motion.state==ButtonState::Focus)return 51;
    const uint32_t age=now-motion.start;
    return age>=100?0:(unsigned(motion.from.hover)*(100-age)+50)/100;
}
inline void blur(ButtonMotion&motion,uint32_t now){
    if(motion.state!=ButtonState::Focus)return;
    motion.state=ButtonState::Rest;motion.from.hover=51;motion.start=motion.seen=now;
}
inline unsigned observe(ButtonMotion&motion,uint32_t key,bool focused,uint32_t now){
    if(motion.identity!=key){motion=ButtonMotion{};motion.identity=key;motion.start=now;}
    if(focused){motion.state=ButtonState::Focus;motion.from.hover=51;}
    else blur(motion,now);
    motion.seen=now;return opacity(motion,now);
}
inline void blur_rows(ButtonMotion*motions,unsigned count,uint32_t now,uint32_t keep=0){
    for(unsigned i=0;i<count;++i)if(owns(motions[i].identity)&&motions[i].identity!=keep)blur(motions[i],now);
}
}
