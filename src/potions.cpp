#include "core.hpp"
namespace spire {
// Values and use/target scopes inspected in the pinned 41cef1ea assembly.
const PotionDefinition potion_definitions[PotionKinds]={
 {"FIRE_POTION","火焰药水","对一个敌人造成 [blue]20[/blue] 点伤害。",1,true,true},
 {"BLOCK_POTION","格挡药水","获得 [blue]12[/blue] 点格挡。",1,true,false},
 {"BLOOD_POTION","鲜血药水","回复最大生命值的 [green]20%[/green]。",1,false,false},
 {"ENERGY_POTION","能量药水","获得 [gold]2[/gold] 点能量。",1,true,false},
 {"STRENGTH_POTION","力量药水","获得 [gold]2[/gold] 点力量。",1,true,false},
 {"DEXTERITY_POTION","敏捷药水","获得 [blue]2[/blue] 点敏捷。",1,true,false},
 {"SWIFT_POTION","迅捷药水","抽 [blue]3[/blue] 张牌。",1,true,false},
 {"EXPLOSIVE_AMPOULE","爆炸安瓿","对所有敌人造成 [blue]10[/blue] 点伤害。",1,true,false},
 {"VULNERABLE_POTION","易伤药水","给予一个敌人 [blue]3[/blue] 层易伤。",1,true,true},
 {"WEAK_POTION","虚弱药水","给予一个敌人 [blue]3[/blue] 层虚弱。",1,true,true},
 {"FRUIT_JUICE","果汁","获得 [green]5[/green] 点最大生命值。",3,false,false},
 {"BLESSING_OF_THE_FORGE","熔炉的祝福","在本场战斗中 [green]升级[/green] 手牌中的所有牌。",2,true,false},
 {"FLEX_POTION","肌肉药水","获得 [gold]5[/gold] 点力量。在本回合结束时失去这部分力量。",1,true,false},
 {"SPEED_POTION","速度药水","获得 [blue]5[/blue] 点敏捷。在本回合结束时失去这部分敏捷。",1,true,false},
 {"LIQUID_BRONZE","流动铜液","获得 [gold]3[/gold] 点荆棘。受到攻击时对攻击者造成荆棘伤害。",2,true,false},
 {"FORTIFIER","固化药水","将当前的 [blue]格挡[/blue] 变为三倍。",2,true,false},
#include "bulk_potion_definitions.inc"
};
const PotionDefinition* potion_definition(Potion p){
    unsigned i=(unsigned)p;return i&&i<=PotionKinds?&potion_definitions[i-1]:nullptr;
}
static int cap(int a,int b){return a<b?a:b;}
static void grant(int16_t& stat,int amount,int16_t* temporary=nullptr){
    int n=cap(amount,9999-stat);stat=(int16_t)(stat+n);
    if(temporary)*temporary=(int16_t)(*temporary+n);
}
bool can_use_potion(const State&s,int slot,int target){
    if(slot<0||slot>=s.potion_slots||s.hp<=0||choice_phase(s.phase)||s.phase==Phase::Dead||
       s.phase==Phase::Complete||s.phase==Phase::Error||s.effect_count||s.playing>=0)return false;
    const auto*d=potion_definition(s.potions[slot]);if(!d||d->automatic)return false;
    if(s.potions[slot]==Potion::FoulPotion&&s.phase!=Phase::Combat&&s.phase!=Phase::Shop)return false;
    if(d->combat_only&&s.phase!=Phase::Combat)return false;
    if(d->targeted&&(target<0||target>=s.enemy_count||s.enemies[target].hp<=0))return false;
    return true;
}
bool use_potion(State&s,int slot,int target){
    if(!can_use_potion(s,slot,target))return false;
    Potion p=s.potions[slot];
    // Consume first so the same bottle cannot be reused during any later hook.
    s.potions[slot]=Potion::Empty;record(s,LogKind::Potion,(int)p);
    switch(p){
    case Potion::Fire:damage_unpowered(s,target,20);break;
    case Potion::Block:s.block=(int16_t)cap(9999,s.block+12);break;
    case Potion::Blood:heal(s,s.max_hp/5);break;
    case Potion::Energy:s.energy=(int16_t)cap(999,s.energy+2);break;
    case Potion::Strength:bulk_add_strength(s,2);break;
    case Potion::Dexterity:bulk_add_dexterity(s,2);break;
    case Potion::Swift:draw_cards(s,3);break;
    case Potion::Explosive:for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,10);break;
    case Potion::Vulnerable:grant(s.enemies[target].vulnerable,3);break;
    case Potion::Weak:grant(s.enemies[target].weak,3);break;
    case Potion::Fruit:increase_max_hp(s,5);break;
    case Potion::Forge:for(int i=0;i<s.hand.count;i++)s.cards[s.hand.ids[i]].upgraded=1;break;
    case Potion::Flex:{int old=s.strength;bulk_add_strength(s,5);s.temporary_strength+=(int16_t)(s.strength-old);break;}
    case Potion::Speed:bulk_add_dexterity(s,5,true);break;
    case Potion::Bronze:grant(s.thorns,3);break;
    case Potion::Fortifier:s.block=(int16_t)cap(9999,s.block*3);break;
    default:bulk_use_potion(s,p,target);break;
    }
    if((s.relics&ReptileTrinket)&&combat_phase(s.phase)){int old=s.strength;bulk_add_strength(s,3);s.temporary_strength+=(int16_t)(s.strength-old);}
    bulk_resolve(s);s.revision++;return true;
}
bool discard_potion(State&s,int slot){
    if(slot<0||slot>=s.potion_slots||!potion_definition(s.potions[slot])||choice_phase(s.phase)||
       s.phase==Phase::Dead||s.phase==Phase::Error||s.phase==Phase::Complete)return false;
    s.potions[slot]=Potion::Empty;s.revision++;return true;
}
}
