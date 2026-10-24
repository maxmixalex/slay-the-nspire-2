#include "core.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
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
#include "potions_extra_definitions.inc"
};
const PotionDefinition* potion_definition(Potion p){
    unsigned i=(unsigned)p;return i&&i<=PotionKinds?&potion_definitions[i-1]:nullptr;
}
static int cap(int a,int b){return a<b?a:b;}
static void grant(int16_t& stat,int amount,int16_t* temporary=nullptr){
    int n=cap(amount,9999-stat);stat=(int16_t)(stat+n);
    if(temporary)*temporary=(int16_t)(*temporary+n);
}
static bool potion_choice_blocked(const State&s){
    if(s.phase==Phase::WorldRelicChoice)return s.world_relics.choice!=WorldRelicChoice::OfferReward&&s.world_relics.choice!=WorldRelicChoice::Potion;
    if(s.phase==Phase::OriginalEventChoice)return s.original_events.selection!=OriginalEventSelection::Rewards&&s.original_events.selection!=OriginalEventSelection::Potion;
    return choice_phase(s.phase);
}
bool can_use_potion(const State&s,int slot,int target){
    if(slot<0||slot>=(int)potion_slot_count(s)||s.hp<=0||potion_choice_blocked(s)||s.phase==Phase::Dead||
       s.phase==Phase::Complete||s.phase==Phase::Error||s.effect_count||s.playing>=0)return false;
    if(original_event_blocks_potions(s))return false;
    const auto*d=potion_definition(s.potions[slot]);if(!d||d->automatic)return false;
    if(d->combat_only&&s.phase!=Phase::Combat)return false;
    if(s.effect_count||s.playing>=0)return false;
    if(s.potions[slot]==Potion::FoulPotion&&s.phase!=Phase::Combat&&s.phase!=Phase::Shop&&!original_event_can_use_foul(s))return false;
    if(d->targeted&&!enemy_allow_target(s,target))return false;
    return true;
}
bool use_potion(State&s,int slot,int target){
    if(!can_use_potion(s,slot,target))return false;
    Potion p=s.potions[slot];
    const auto* definition=potion_definition(p);if(!definition->targeted)target=-1;
    // Consume first so the same bottle cannot be reused during any later hook.
    s.potions[slot]=Potion::Empty;record(s,LogKind::Potion,(int)p);
    const bool fighting=s.phase==Phase::Combat;
    if(fighting)s.expansion.potion_active=1;
    before_potion_use(s,p,target);
    switch(p){
    case Potion::Fire:damage_unpowered(s,target,20);break;
    case Potion::Block:gain_unpowered_block(s,12);break;
    case Potion::Blood:heal(s,s.max_hp/5);break;
    case Potion::Energy:s.energy=(int16_t)cap(999,s.energy+2);break;
    case Potion::Strength:world_relic_gain_strength(s,2);break;
    case Potion::Dexterity:world_relic_gain_dexterity(s,2);break;
    case Potion::Swift:queue_extension(s,XOp::Draw,3);break;
    case Potion::Explosive:for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,10);break;
    case Potion::Vulnerable:apply_vulnerable(s,target,3);break;
    case Potion::Weak:apply_weak(s,target,3);break;
    case Potion::Fruit:increase_max_hp(s,5);break;
    case Potion::Forge:for(int i=0;i<s.hand.count;i++)if(card_upgradeable(s.cards[s.hand.ids[i]]))s.cards[s.hand.ids[i]].upgraded=1;break;
    case Potion::Flex:{int amount=cap(5,9999-s.strength);world_relic_gain_strength(s,amount);s.temporary_strength=(int16_t)cap(9999,s.temporary_strength+amount);break;}
    case Potion::Speed:{int amount=cap(5,9999-s.dexterity);world_relic_gain_dexterity(s,amount);s.temporary_dexterity=(int16_t)cap(9999,s.temporary_dexterity+amount);break;}
    case Potion::Bronze:grant(s.thorns,3);break;
    case Potion::Fortifier:gain_unpowered_block(s,s.block*2);break;
    default:execute_potion(s,p,target);break;
    }
    if(fighting&&combat_phase(s.phase))queue_extension(s,XOp::PotionDone,(int)p,target);
    else if(s.hp>0)world_relic_hook(s,WorldRelicHook::AfterPotionUsed,(int)p,target);
    resolve_pending(s);s.revision++;return true;
}
bool can_discard_potion(const State&s,int slot){
    if(slot<0||slot>=(int)potion_slot_count(s)||!potion_definition(s.potions[slot])||potion_choice_blocked(s)||
       s.phase==Phase::Dead||s.phase==Phase::Error||s.phase==Phase::Complete)return false;
    return !original_event_blocks_potions(s);
}
bool discard_potion(State&s,int slot){
    if(!can_discard_potion(s,slot))return false;
    Potion old=s.potions[slot];s.potions[slot]=Potion::Empty;world_relic_hook(s,WorldRelicHook::AfterPotionDiscarded,(int)old,slot);s.revision++;return true;
}
}
