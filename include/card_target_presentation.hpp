#pragma once
#include "core.hpp"
#include "health_bar.hpp"
namespace spire {
enum class CardTargetType : uint8_t { None,Self,AnyEnemy,AllEnemies,RandomEnemy,AnyPlayer,AnyAlly,AllAllies,TargetedNoCreature,Osty };
inline CardTargetType card_target_type(const State&s,Card card,int instance=-1,int deck=-1){
    static constexpr uint8_t targets[CardKinds]={
#include "../src/card_target_types.inc"
    };
    if(card.kind>=CardKinds)return CardTargetType::None;
    // The pinned models contain these four property overrides. Malaise is
    // constantly AnyEnemy; the other three follow the existing live rules.
    if(card.kind==Shiv||card.kind==SovereignBlade)return card_targets_enemy(s,card,instance,deck)?CardTargetType::AnyEnemy:CardTargetType::AllEnemies;
    if(card.kind==EventCardMadScience)return card_type(s,card,instance,deck)==CardType::Attack?CardTargetType::AnyEnemy:CardTargetType::Self;
    return CardTargetType(targets[card.kind]);
}
struct TargetReticleMotion {
    uint32_t start=0;int16_t left=0,top=0,right=0,bottom=0,from_scale=4096;
    bool selected=false,active=false;
    int scale(uint32_t now)const{
        if(!active)return 4096;const unsigned age=now-start;
        if(selected){
            if(!age||age>=500)return 4096;
            return 4096-410*int(4096-health_bar::expo_out(age*2))/4096;
        }
        return from_scale; // The drawing layer applies the 200ms sine fade.
    }
    void observe(bool next,uint32_t now){
        if(next==selected)return;
        from_scale=int16_t(scale(now));selected=next;active=true;start=now;
    }
    void bounds(int x,int y,int w,int h){left=int16_t(x);top=int16_t(y);right=int16_t(x+w);bottom=int16_t(y+h);}
};
}
