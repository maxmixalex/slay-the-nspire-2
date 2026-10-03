#include "core.hpp"
#include "assets.hpp"
namespace spire {
// Values and trigger phases inspected in the supplied 41cef1ea assembly.
// IDs 1, 2, 4 keep their original meaning; order and counters are now explicit.
const RelicDefinition relic_definitions[RelicKinds]={
 {Vajra,"VAJRA","金刚杵","战斗开始时获得 [gold]1[/gold] 点力量。",A_Vajra},
 {Anchor,"ANCHOR","锚","战斗开始时获得 [blue]10[/blue] 点格挡。",A_Anchor},
 {BagOfPreparation,"BAG_OF_PREPARATION","准备背包","首回合额外抽 [gold]2[/gold] 张牌。",A_Bag},
 {BurningBlood,"BURNING_BLOOD","燃烧之血","战斗胜利后回复 [green]6[/green] 点生命。",A_BurningBlood},
 {Lantern,"LANTERN","灯笼","每场战斗的首回合获得 [gold]1[/gold] 点能量。",A_Lantern},
 {OddlySmoothStone,"ODDLY_SMOOTH_STONE","意外光滑的石头","战斗开始时获得 [blue]1[/blue] 点敏捷。",A_SmoothStone},
 {BloodVial,"BLOOD_VIAL","小血瓶","每场战斗的首回合回复 [green]2[/green] 点生命。",A_BloodVial},
 {HappyFlower,"HAPPY_FLOWER","开心小花","每经过 [gold]3[/gold] 个回合获得 [gold]1[/gold] 点能量。计数跨战斗保留。",A_HappyFlower},
 {Orichalcum,"ORICHALCUM","奥利哈钢","结束回合时若没有格挡，获得 [blue]6[/blue] 点格挡。",A_Orichalcum},
 {PenNib,"PEN_NIB","钢笔尖","每打出 [gold]10[/gold] 张攻击牌，第十张造成双倍伤害。计数跨战斗保留。",A_PenNib}
};
const RelicDefinition* relic_definition(unsigned id){for(const auto&d:relic_definitions)if(d.id==id)return &d;return nullptr;}
static void flash(State&s,unsigned id){s.relic_flash|=(uint16_t)id;++s.relic_revision;}
bool gain_relic(State&s,unsigned id){
 const auto*d=relic_definition(id);if(!d||(s.relics&id)||s.relic_count>=RelicKinds)return false;
 s.relic_order[s.relic_count++]=(uint8_t)(d-relic_definitions);s.relics|=(uint16_t)id;
 flash(s,id);record(s,LogKind::Relic,(int)id);return true;
}
unsigned random_relic(State&s){
 unsigned ids[RelicKinds],n=0;for(const auto&d:relic_definitions)if(d.id!=BurningBlood&&!(s.relics&d.id))ids[n++]=d.id;
 return n?ids[random_below(s.rng_world,n)]:0;
}
unsigned relic_counter(const State&s,unsigned id){const auto*d=relic_definition(id);return d?s.relic_counters[d-relic_definitions]:0;}
void relic_hook(State&s,RelicHook hook){
 s.relic_flash=0;
 for(unsigned i=0;i<s.relic_count;i++){
  unsigned index=s.relic_order[i],id=relic_definitions[index].id;bool triggered=false;
  if(hook==RelicHook::CombatStart){
   if(id==Vajra){s.strength+=1;triggered=true;}
   else if(id==Anchor){s.block+=10;triggered=true;}
   else if(id==OddlySmoothStone){s.dexterity+=1;triggered=true;}
   else if(id==BagOfPreparation)triggered=true; // draw count is applied by combat entry.
  }else if(hook==RelicHook::TurnStart){
   if(id==HappyFlower){auto&n=s.relic_counters[index];if(++n==3){n=0;++s.energy;triggered=true;}}
   else if(id==Lantern&&s.turn==1){++s.energy;triggered=true;}
   else if(id==BloodVial&&s.turn==1){heal(s,2);triggered=true;}
  }else if(hook==RelicHook::TurnEnd){
   if(id==Orichalcum&&s.block==0){s.block=6;triggered=true;}
  }else if(hook==RelicHook::Victory&&id==BurningBlood){heal(s,6);triggered=true;}
  if(triggered)flash(s,id);
 }
}
void relic_attack_begin(State&s){
 s.double_attack=0;
 if(!(s.relics&PenNib))return;
 auto&n=s.relic_counters[9];if(++n==10){n=0;s.double_attack=1;flash(s,PenNib);}
}
void relic_attack_end(State&s){s.double_attack=0;}
int relic_attack_multiplier(const State&s,bool preview){
 if(s.double_attack)return 2;
 return preview&&(s.relics&PenNib)&&relic_counter(s,PenNib)==9?2:1;
}
int card_block(const State&s,Card c){const auto&d=definitions[c.kind];int b=d.block+(c.upgraded?d.block_up:0);return b>0?b+s.dexterity:0;}
}
