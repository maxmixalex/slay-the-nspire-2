#pragma once
#include <stdint.h>
#include "health_bar.hpp"

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
    void reset(){*this=TurnBannerPresentation{};}
    static unsigned ease(uint32_t elapsed,uint32_t duration){return health_bar::expo_out(elapsed>=duration?1000:elapsed*1000/duration);}
    void begin_enemy(uint32_t now){
        if(in_enemy)return;enemy_start=now;enemy_active=in_enemy=true;player_active=false;
    }
    void begin_player(int turn,uint32_t earliest,bool extra_turn=false){
        if(enemy_active&&int32_t(enemy_start+EnemyTicks-earliest)>0)earliest=enemy_start+EnemyTicks;
        player_start=earliest;number=turn;extra=extra_turn;player_active=true;in_enemy=false;
    }
    Frame sample(uint32_t now)const{
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
        if(!player_active||int32_t(now-player_start)<0||now-player_start>=(PlayerMs+32)/33)return f;
        const uint32_t ms=(now-player_start)*33;f.kind=Player;f.number=number;f.extra=extra;
        f.alpha=ms<1900?ease(ms,1000)*255/4096:(ms>=PlayerMs?0:(PlayerMs-ms)*255/300);
        f.offset=int((ease(ms,1500)*8+2048)/4096);return f;
    }
};
}
