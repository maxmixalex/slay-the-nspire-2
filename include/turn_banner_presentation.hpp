#pragma once
#include <stdint.h>
#include "health_bar.hpp"
#include "room_ui7_presentation.hpp"

namespace spire {
// NPlayerTurnBanner / NEnemyTurnBanner, pinned v0.111.0. All clocks are
// presentation ticks: no event, random stream or saved turn is modified.
struct TurnBannerPresentation {
    enum Kind : uint8_t {None,Player,Enemy};
    struct Frame {Kind kind=None;unsigned alpha=0,scale=4096,red=0;int offset=0,number=0;bool extra=false;};
    uint32_t player_start=0,enemy_start=0;
    int number=0;
    bool player_active=false,enemy_active=false,in_enemy=false,extra=false;
    static constexpr uint32_t PlayerMs=2200,EnemyMs=2300,EnemyTicks=(EnemyMs+32)/33;
    // CombatManager.StartTurn waits 0.8s after creating the banner, then
    // NCreature.PerformIntent waits 0.4s before TakeTurn in normal speed.
    static constexpr uint32_t EnemyLeadTicks=(800+32)/33,IntentLeadTicks=(400+32)/33;
    // Cmd.CustomScaledWait selects independent real-time durations. Fast
    // changes these waits, not Engine time or the banner/animation clock.
    static constexpr uint32_t wait_ticks(unsigned normal_ms,unsigned fast_ms,bool fast){return ((fast?fast_ms:normal_ms)+32u)/33u;}
    static constexpr uint32_t enemy_lead_ticks(bool fast){return wait_ticks(800,500,fast);}
    static constexpr uint32_t intent_lead_ticks(bool fast){return wait_ticks(400,250,fast);}
    void reset(){*this=TurnBannerPresentation{};}
    static unsigned ease(uint32_t elapsed,uint32_t duration){return health_bar::expo_out(elapsed>=duration?1000:elapsed*1000/duration);}
    void begin_enemy(uint32_t now){
        if(in_enemy)return;enemy_start=now;enemy_active=in_enemy=true;
    }
    void begin_player(int turn,uint32_t earliest,bool extra_turn=false){
        player_start=earliest;number=turn;extra=extra_turn;player_active=true;in_enemy=false;
    }
    Frame sample_enemy(uint32_t now)const{
        Frame f;
        if(enemy_active&&int32_t(now-enemy_start)>=0&&now-enemy_start<EnemyTicks){
            const uint32_t ms=(now-enemy_start)*33;f.kind=Enemy;
            f.scale=8192-ease(ms,750);f.alpha=ease(ms,1300)*255/4096;
            if(ms>1300){const unsigned t=ms-1300;f.red=ease(t,1000)*255/4096;
                // CUBIC_OUT fade to zero, unlike the EXPO_OUT color tween.
                const unsigned left=t>=1000?0:(1000-t)*1024/1000;
                f.alpha=(((left*left)>>10)*left*255)>>20;
            }return f;
        }
        return f;
    }
    Frame sample_player(uint32_t now)const{
        Frame f;
        if(!player_active||int32_t(now-player_start)<0||now-player_start>=(PlayerMs+32)/33)return f;
        const uint32_t ms=(now-player_start)*33;f.kind=Player;f.number=number;f.extra=extra;
        f.alpha=ms<1900?ease(ms,1000)*255/4096:
            255-255*room_curve::sine_out(ms-1900,300)/room_curve::Q;
        // Opposite50px scene tweens project to10px at the real1680px UI.
        f.offset=int((ease(ms,1500)*10+2048)/4096);return f;
    }
    Frame sample(uint32_t now)const{const auto enemy=sample_enemy(now);return enemy.kind!=None?enemy:sample_player(now);}
};
}
