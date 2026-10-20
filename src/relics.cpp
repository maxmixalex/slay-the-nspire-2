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
 {PenNib,"PEN_NIB","钢笔尖","每打出 [gold]10[/gold] 张攻击牌，第十张造成双倍伤害。计数跨战斗保留。",A_PenNib},
 {Strawberry,"STRAWBERRY","草莓","拾取时获得 [green]7[/green] 点最大生命值。",-1},
 {Pear,"PEAR","梨","拾取时获得 [green]10[/green] 点最大生命值。",-1},
 {Mango,"MANGO","芒果","拾取时获得 [green]14[/green] 点最大生命值。",-1},
 {BagOfMarbles,"BAG_OF_MARBLES","弹珠袋","每场战斗开始时给予所有敌人 [blue]1[/blue] 层易伤。",-1},
 {Akabeko,"AKABEKO","赤牛","每场战斗的第一张攻击额外造成 [gold]8[/gold] 点伤害。",-1},
 {BronzeScales,"BRONZE_SCALES","铜制鳞片","每场战斗获得 [gold]3[/gold] 点荆棘。",-1},
 {RingOfTheSnake,"RING_OF_THE_SNAKE","蛇之戒","每场战斗的首回合额外抽 [gold]2[/gold] 张牌。",-1},
 {HornCleat,"HORN_CLEAT","船夹板","每场战斗的第二回合开始时，获得 [blue]14[/blue] 点格挡。",-1},
 {CaptainsWheel,"CAPTAINS_WHEEL","舵盘","每场战斗的第三回合开始时，获得 [blue]18[/blue] 点格挡。",-1},
 {MercuryHourglass,"MERCURY_HOURGLASS","水银沙漏","每个回合开始时，对所有敌人造成 [blue]3[/blue] 点伤害。",-1},
 {ArtOfWar,"ART_OF_WAR","孙子兵法","若上一回合没有打出攻击牌，在本回合开始时获得 [gold]1[/gold] 点能量。",-1},
 {RegalPillow,"REGAL_PILLOW","皇家枕头","在休息处休息时，额外回复 [green]15[/green] 点生命。",-1},
 {MealTicket,"MEAL_TICKET","餐券","进入商店时，回复 [green]15[/green] 点生命。",-1},
 {OldCoin,"OLD_COIN","古钱币","拾取时获得 [gold]300[/gold] 金币。",-1},
 {CentennialPuzzle,"CENTENNIAL_PUZZLE","百年积木","每场战斗中，第一次失去生命时抽 [blue]3[/blue] 张牌。",-1},
 {Kunai,"KUNAI","苦无","每在同一回合打出 [gold]3[/gold] 张攻击牌，获得 [blue]1[/blue] 点敏捷。",-1},
 {Shuriken,"SHURIKEN","手里剑","每在同一回合打出 [gold]3[/gold] 张攻击牌，获得 [gold]1[/gold] 点力量。",-1},
 {OrnamentalFan,"ORNAMENTAL_FAN","精致折扇","每在同一回合打出 [gold]3[/gold] 张攻击牌，获得 [blue]4[/blue] 点格挡。",-1},
 {LetterOpener,"LETTER_OPENER","开信刀","每在同一回合打出 [gold]3[/gold] 张技能牌，对所有敌人造成 [blue]5[/blue] 点伤害。",-1},
 {Nunchaku,"NUNCHAKU","双截棍","每打出 [gold]10[/gold] 张攻击牌，获得 [gold]1[/gold] 点能量。计数跨战斗保留。",-1},
 {CrackedCore,"CRACKED_CORE","破损核心","战斗开始时生成 [blue]1[/blue] 个闪电充能球。",-1},
 {DivineRight,"DIVINE_RIGHT","天赋君权","战斗开始时获得 [gold]3[/gold] 颗星星。",-1},
 {BoundPhylactery,"BOUND_PHYLACTERY","缚魂命匣","每个回合开始时召唤 [blue]1[/blue]。",-1}
};
const RelicDefinition* relic_definition(RelicMask id){for(const auto&d:relic_definitions)if(d.id==id)return &d;return nullptr;}
static void flash(State&s,RelicMask id){s.relic_flash|=id;++s.relic_revision;}
bool gain_relic(State&s,RelicMask id){
 const auto*d=relic_definition(id);if(!d||(s.relics&id)||s.relic_count>=RelicKinds)return false;
 s.relic_flash=0;s.relic_order[s.relic_count++]=(uint8_t)(d-relic_definitions);s.relics|=id;
 if(id==Strawberry||id==Pear||id==Mango)increase_max_hp(s,id==Strawberry?7:id==Pear?10:14);
 if(id==OldCoin)gain_gold(s,300);
 flash(s,id);if(d-relic_definitions>=30)record(s,LogKind::RelicIndex,(int)(d-relic_definitions));else record(s,LogKind::Relic,(int)id);s.revision++;return true;
}
RelicMask random_relic(State&s){
 RelicMask ids[RelicKinds];unsigned n=0;for(const auto&d:relic_definitions)if(d.id!=BurningBlood&&d.id!=RingOfTheSnake&&d.id!=CrackedCore&&d.id!=DivineRight&&d.id!=BoundPhylactery&&!(s.relics&d.id))ids[n++]=d.id;
 return n?ids[random_below(s.rng_world,n)]:0;
}
unsigned relic_counter(const State&s,RelicMask id){const auto*d=relic_definition(id);return d?s.relic_counters[d-relic_definitions]:0;}
void relic_hook(State&s,RelicHook hook){
 if(s.world_relics.active)return;
 for(unsigned i=0;i<s.relic_count;i++){
  unsigned index=s.relic_order[i];RelicMask id=relic_definitions[index].id;bool triggered=false;
  if(hook==RelicHook::CombatStart){
   if(id==CrackedCore){queue_extension(s,XOp::DChannel,(int)DOrb::Lightning,1);triggered=true;}
   else if(id==DivineRight){regent_gain_stars(s,3);triggered=true;}
   else if(id==BoundPhylactery){queue_extension(s,XOp::Summon,1);triggered=true;}
   else if(id==Vajra){s.strength+=1;triggered=true;}
   else if(id==Anchor){gain_unpowered_block(s,10);triggered=true;}
   else if(id==OddlySmoothStone){s.dexterity+=1;triggered=true;}
   else if(id==BagOfMarbles){for(int j=0;j<s.enemy_count;j++)apply_vulnerable(s,j,1);triggered=true;}
   else if(id==BronzeScales){s.thorns+=3;triggered=true;}
   else if(id==BagOfPreparation||id==RingOfTheSnake)triggered=true; // draw count is applied by combat entry.
  }else if(hook==RelicHook::TurnStart){
   if(id==HornCleat&&s.turn==2){gain_unpowered_block(s,14);triggered=true;}
   else if(id==CaptainsWheel&&s.turn==3){gain_unpowered_block(s,18);triggered=true;}
   else if(id==ArtOfWar&&s.turn>1&&!s.attacked_this_turn){++s.energy;triggered=true;}
   else if(id==HappyFlower){auto&n=s.relic_counters[index];if(++n==3){n=0;++s.energy;triggered=true;}}
   else if(id==Akabeko&&s.turn==1){s.vigor+=8;triggered=true;}
   else if(id==Lantern&&s.turn==1){++s.energy;triggered=true;}
  }else if(hook==RelicHook::AfterHandDraw){
   if(id==MercuryHourglass){for(int j=0;j<s.enemy_count;j++)damage_unpowered(s,j,3);triggered=true;}
  }else if(hook==RelicHook::TurnEnd){
   if(id==Orichalcum&&s.block==0){gain_unpowered_block(s,6);triggered=true;}
  }else if(hook==RelicHook::Victory&&id==BurningBlood){heal(s,6);triggered=true;}
  if(triggered)flash(s,id);
 }
 // Blood Vial uses AfterPlayerTurnStartLate, after Mercury's ordinary
 // AfterPlayerTurnStart regardless of relic acquisition order.
 if(hook==RelicHook::AfterHandDraw&&s.turn==1&&(s.relics&BloodVial)){heal(s,2);flash(s,BloodVial);}
 if(hook==RelicHook::TurnStart)s.attacked_this_turn=0;
 if(hook==RelicHook::Victory)relic_reset_turn_counters(s);
}
void relic_after_hp_loss(State&s,int loss){
 if(s.world_relics.active)return;
 if(loss<=0||s.hp<=0||!combat_phase(s.phase)||!(s.relics&CentennialPuzzle)||s.relic_counters[24])return;
 s.relic_counters[24]=1;flash(s,CentennialPuzzle);draw_cards(s,3);
}
unsigned relic_counter_limit(RelicMask id){
 if(id==PenNib||id==Nunchaku)return 10;
 if(id==HappyFlower||relic_counter_is_turn_scoped(id))return 3;
 if(id==CentennialPuzzle)return 1;
 return 0;
}
bool relic_counter_is_turn_scoped(RelicMask id){
 return id==Kunai||id==Shuriken||id==OrnamentalFan||id==LetterOpener;
}
void relic_reset_turn_counters(State&s){
 for(int i=0;i<RelicKinds;i++)if(relic_counter_is_turn_scoped(relic_definitions[i].id))s.relic_counters[i]=0;
}
void relic_card_played(State&s,Card card){
 if(s.world_relics.active)return;
 if(s.hp<=0||s.phase!=Phase::Combat||card.kind>=CardKinds)return;
 const CardType type=card_type(s,card);
 // AfterCardPlayed runs once after the card's commands/choices, before its
 // result-pile move. Resolve in acquisition order; hits are not card plays.
 for(unsigned i=0;i<s.relic_count;i++){
  unsigned index=s.relic_order[i];RelicMask id=relic_definitions[index].id;
  bool relevant=id==LetterOpener?type==CardType::Skill:
      (id==Kunai||id==Shuriken||id==OrnamentalFan||id==Nunchaku)&&type==CardType::Attack;
  if(!relevant)continue;
  auto& count=s.relic_counters[index];if(++count<relic_counter_limit(id))continue;
  count=0;flash(s,id);
  if(id==Kunai){if(s.dexterity<9999)++s.dexterity;record(s,LogKind::Dexterity,1);}
  else if(id==Shuriken){if(s.strength<9999)++s.strength;record(s,LogKind::Strength,1);}
  else if(id==OrnamentalFan){gain_unpowered_block(s,4);}
  else if(id==LetterOpener){for(int j=0;j<s.enemy_count;j++)damage_unpowered(s,j,5);}
  else if(id==Nunchaku){if(s.energy<999)++s.energy;record(s,LogKind::Energy,1);}
 }
 // Finish performs victory after it moves the card out of limbo. Do not end
 // combat here: a lethal Letter Opener proc must not orphan the playing card.
}
void relic_attack_begin(State&s){
 s.double_attack=0;
 if(s.world_relics.active)return;
 if(!(s.relics&PenNib))return;
 auto&n=s.relic_counters[9];if(++n==10){n=0;s.double_attack=1;flash(s,PenNib);}
}
void relic_attack_end(State&s){s.double_attack=0;s.attack_vigor=0;}
int relic_attack_multiplier(const State&s,bool preview){
 int n=!s.world_relics.active&&(s.double_attack||(preview&&(s.relics&PenNib)&&relic_counter(s,PenNib)==9))?2:1;
 if(s.expansion.triple||(preview&&power_value(s,XPower::Gigantic)))n*=3;
 return n;
}
}
