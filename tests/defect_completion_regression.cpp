#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static unsigned checks,variants,upgrade;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d [%s +%u] %s\n",__FILE__,__LINE__,scenario,upgrade,#x);std::exit(1);}}while(0)
// Literal canonical facts were transcribed from the pinned assembly. Expected
// combat results below do not read the runtime definitions to derive effects.
struct Canonical {CardKind id;const char*name;int base_cost,up_cost,base_kw,up_kw,rarity;CardType type;};
static const Canonical canonical[]={
 {DefectAdaptiveStrike,"AdaptiveStrike",2,2,0,0,4,CardType::Attack},
 {DefectAllForOne,"AllForOne",2,2,0,0,4,CardType::Attack},
 {DefectBallLightning,"BallLightning",1,1,0,0,2,CardType::Attack},
 {DefectBarrage,"Barrage",1,1,0,0,2,CardType::Attack},
 {DefectBeamCell,"BeamCell",0,0,0,0,2,CardType::Attack},
 {DefectBiasedCognition,"BiasedCognition",1,1,0,0,5,CardType::Power},
 {DefectBoostAway,"BoostAway",0,0,0,0,2,CardType::Skill},
 {DefectBootSequence,"BootSequence",0,0,3,3,3,CardType::Skill},
 {DefectBuffer,"Buffer",2,2,0,0,4,CardType::Power},
 {DefectBulkUp,"BulkUp",2,2,0,0,3,CardType::Power},
 {DefectCapacitor,"Capacitor",1,1,0,0,3,CardType::Power},
 {DefectChaos,"Chaos",1,1,0,0,3,CardType::Skill},
 {DefectChargeBattery,"ChargeBattery",1,1,0,0,2,CardType::Skill},
 {DefectChill,"Chill",0,0,1,0,3,CardType::Skill},
 {DefectClaw,"Claw",0,0,0,0,2,CardType::Attack},
 {DefectColdSnap,"ColdSnap",1,1,0,0,2,CardType::Attack},
 {DefectCompact,"Compact",1,1,0,0,3,CardType::Skill},
 {DefectCompileDriver,"CompileDriver",1,1,0,0,2,CardType::Attack},
 {DefectConsumingShadow,"ConsumingShadow",2,2,0,0,4,CardType::Power},
 {DefectCoolant,"Coolant",1,1,0,0,4,CardType::Power},
 {DefectCoolheaded,"Coolheaded",1,1,0,0,2,CardType::Skill},
 {DefectCreativeAi,"CreativeAi",3,2,0,0,4,CardType::Power},
 {DefectDarkness,"Darkness",1,1,0,0,3,CardType::Skill},
 {DefectDefend,"DefendDefect",1,1,0,0,1,CardType::Skill},
 {DefectDefragment,"Defragment",1,1,0,0,4,CardType::Power},
 {DefectDoubleEnergy,"DoubleEnergy",1,0,1,1,3,CardType::Skill},
 {DefectDualcast,"Dualcast",1,0,0,0,1,CardType::Skill},
 {DefectEchoForm,"EchoForm",3,3,8,0,4,CardType::Power},
 {DefectFeral,"Feral",2,1,0,0,3,CardType::Power},
 {DefectFightThrough,"FightThrough",1,1,0,0,3,CardType::Skill},
 {DefectFlakCannon,"FlakCannon",2,2,0,0,4,CardType::Attack},
 {DefectFocusedStrike,"FocusedStrike",1,1,0,0,2,CardType::Attack},
 {DefectFtl,"Ftl",0,0,0,0,3,CardType::Attack},
 {DefectFusion,"Fusion",1,1,1,0,3,CardType::Skill},
 {DefectGeneticAlgorithm,"GeneticAlgorithm",1,1,1,1,4,CardType::Skill},
 {DefectGlacier,"Glacier",2,2,0,0,3,CardType::Skill},
 {DefectGlasswork,"Glasswork",1,1,0,0,3,CardType::Skill},
 {DefectGoForTheEyes,"GoForTheEyes",0,0,0,0,2,CardType::Attack},
 {DefectGunkUp,"GunkUp",1,1,0,0,2,CardType::Attack},
 {DefectHailstorm,"Hailstorm",1,1,0,0,3,CardType::Power},
 {DefectHelixDrill,"HelixDrill",0,0,0,0,4,CardType::Attack},
 {DefectHologram,"Hologram",1,1,1,0,2,CardType::Skill},
 {DefectHotfix,"Hotfix",0,0,1,0,2,CardType::Skill},
 {DefectHyperbeam,"Hyperbeam",2,2,0,0,4,CardType::Attack},
 {DefectIceLance,"IceLance",3,3,0,0,4,CardType::Attack},
 {DefectIteration,"Iteration",1,1,0,0,3,CardType::Power},
 {DefectLeap,"Leap",1,1,0,0,2,CardType::Skill},
 {DefectLightningRod,"LightningRod",1,1,0,0,2,CardType::Skill},
 {DefectLoop,"Loop",1,1,0,0,3,CardType::Power},
 {DefectMachineLearning,"MachineLearning",1,1,0,2,4,CardType::Power},
 {DefectMeteorStrike,"MeteorStrike",5,5,0,0,4,CardType::Attack},
 {DefectModded,"Modded",0,0,0,0,4,CardType::Skill},
 {DefectMomentumStrike,"MomentumStrike",1,1,0,0,2,CardType::Attack},
 {DefectMultiCast,"MultiCast",-1,-1,0,0,4,CardType::Skill},
 {DefectNull,"Null",2,2,0,0,3,CardType::Attack},
 {DefectOverclock,"Overclock",0,0,0,0,3,CardType::Skill},
 {DefectQuadcast,"Quadcast",1,0,0,0,5,CardType::Skill},
 {DefectRainbow,"Rainbow",2,2,1,0,4,CardType::Skill},
 {DefectReboot,"Reboot",0,0,1,1,4,CardType::Skill},
 {DefectRefract,"Refract",3,3,0,0,3,CardType::Attack},
 {DefectRocketPunch,"RocketPunch",2,2,0,0,3,CardType::Attack},
 {DefectScavenge,"Scavenge",1,1,0,0,3,CardType::Skill},
 {DefectScrape,"Scrape",1,1,0,0,3,CardType::Attack},
 {DefectShadowShield,"ShadowShield",2,2,0,0,3,CardType::Skill},
 {DefectShatter,"Shatter",1,1,1,1,4,CardType::Attack},
 {DefectSignalBoost,"SignalBoost",1,0,1,1,4,CardType::Skill},
 {DefectSkim,"Skim",1,1,0,0,3,CardType::Skill},
 {DefectSmokestack,"Smokestack",1,1,0,0,3,CardType::Power},
 {DefectSpinner,"Spinner",1,1,0,0,4,CardType::Power},
 {DefectStorm,"Storm",1,1,0,0,3,CardType::Power},
 {DefectStrike,"StrikeDefect",1,1,0,0,1,CardType::Attack},
 {DefectSubroutine,"Subroutine",1,0,0,0,3,CardType::Power},
 {DefectSunder,"Sunder",3,3,0,0,3,CardType::Attack},
 {DefectSupercritical,"Supercritical",0,0,1,1,4,CardType::Skill},
 {DefectSweepingBeam,"SweepingBeam",1,1,0,0,2,CardType::Attack},
 {DefectSynchronize,"Synchronize",1,1,0,0,3,CardType::Skill},
 {DefectSynthesis,"Synthesis",2,2,0,0,3,CardType::Attack},
 {DefectTempest,"Tempest",-1,-1,0,0,3,CardType::Skill},
 {DefectTeslaCoil,"TeslaCoil",0,0,0,0,3,CardType::Attack},
 {DefectThunder,"Thunder",1,1,0,0,3,CardType::Power},
 {DefectTrashToTreasure,"TrashToTreasure",1,0,0,0,4,CardType::Power},
 {DefectTurbo,"Turbo",0,0,0,0,2,CardType::Skill},
 {DefectUproar,"Uproar",2,2,0,0,2,CardType::Attack},
 {DefectVoltaic,"Voltaic",3,3,1,0,4,CardType::Skill},
 {DefectWhiteNoise,"WhiteNoise",1,0,1,1,3,CardType::Skill},
 {DefectZap,"Zap",1,0,0,0,1,CardType::Skill},
};
static State fight(){
 State s{};new_run(s,719,false,Character::Defect);
 s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=20;s.hp=s.max_hp=75;
 s.deck_count=1;s.deck[0]={DefectDefend,0};s.expansion.orb_slots=3;
 for(auto&e:s.enemies)e={1000,1000,0,0,0,2};return s;
}
static int append(State&s,Card c,Zone zone){
 CHECK(s.card_count<MaxCards);int id=s.card_count++;s.cards[id]=c;s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(zone!=Zone::Hand||p.count<MaxHand);p.ids[p.count++]=(uint16_t)id;return id;
}
static void fill(State&s,Zone zone,int n,Card c={DefectDefend,0}){for(int i=0;i<n;i++)append(s,c,zone);}
static int hand_index(const State&s,int id){for(unsigned i=0;i<s.hand.count;i++)if(s.hand.ids[i]==id)return (int)i;return -1;}
static int count_kind(const State&s,CardKind kind,Zone zone){int n=0;for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind==kind&&s.zones[i]==zone)++n;return n;}
static int dp(const State&s,DPower p){return s.defect.powers[(unsigned)p];}
static void orb(State&s,DOrb kind,int raw=-1){
 CHECK(s.expansion.orb_count<s.expansion.orb_slots);unsigned i=s.expansion.orb_count++;s.defect.orb_kind[i]=(uint8_t)kind;
 s.expansion.dark[i]=(int16_t)(raw<0?(kind==DOrb::Dark?6:kind==DOrb::Glass?4:0):raw);
}
static void restore(State&s){
 CHECK(validate(s));SaveData a{},b{};CHECK(save(s,a));State t{};CHECK(load(t,a.bytes,a.size));CHECK(save(t,b));
 CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));s=t;
}
static void settle(State&s){
 for(int i=0;i<100;i++){
  CHECK(s.phase!=Phase::Error&&s.phase!=Phase::Dead);restore(s);
  if(s.phase==Phase::Combat||s.phase==Phase::Reward)return;
  if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned j=0;j<s.discard_required;j++)ids[j]=s.hand.ids[j];CHECK(choose_discard(s,ids,s.discard_required));}
  else if(s.phase==Phase::ExtendedChoice){uint16_t ids[MaxHand];unsigned n=choice_min(s);for(unsigned j=0;j<n;j++)ids[j]=choice_pile(s).ids[j];CHECK(choose_extended(s,ids,n));}
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
  else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
  else CHECK(false);
 }
 CHECK(false);
}
static int cast(State&s,CardKind kind,unsigned up=0,int target=0){int id=append(s,{kind,(uint8_t)up},Zone::Hand);CHECK(play(s,hand_index(s,id),target));settle(s);return id;}
static void quiet_turn(State&s){for(auto&e:s.enemies)e.move=2;CHECK(end_turn(s));settle(s);CHECK(s.phase==Phase::Combat);}
static void choose(State&s,int id){CHECK(s.phase==Phase::ExtendedChoice);restore(s);uint16_t choice=(uint16_t)id;CHECK(choose_extended(s,&choice,1));settle(s);}
static void move_hand(State&s,int id){
 Pile*from=s.zones[id]==Zone::Discard?&s.discard:s.zones[id]==Zone::Exhaust?&s.exhaust:s.zones[id]==Zone::Draw?&s.draw:nullptr;CHECK(from&&s.hand.count<MaxHand);
 bool found=false;for(unsigned i=0;i<from->count;i++)if(from->ids[i]==id){for(unsigned j=i+1;j<from->count;j++)from->ids[j-1]=from->ids[j];--from->count;found=true;break;}CHECK(found);
 s.hand.ids[s.hand.count++]=(uint16_t)id;s.zones[id]=Zone::Hand;
}
static void complete_model_matrix(){
 CardKind pool[CardKinds];CHECK(character_pool(Character::Defect,pool)==86);CHECK(sizeof canonical/sizeof canonical[0]==86);
 bool seen[CardKinds]{};
 for(const auto&m:canonical)for(upgrade=0;upgrade<2;upgrade++){
  scenario=m.name;if(!upgrade){CHECK(!seen[m.id]);seen[m.id]=true;}CHECK(card_in_pool(m.id,Character::Defect));
  Card c{m.id,(uint8_t)upgrade};int kw=upgrade?m.up_kw:m.base_kw;
  CHECK(cost(c)==(upgrade?m.up_cost:m.base_cost));CHECK(definitions[m.id].type==m.type);CHECK(card_rarity(m.id)==m.rarity);
  CHECK(card_exhausts(c)==((kw&1)!=0));CHECK(card_innate(c)==((kw&2)!=0));CHECK(card_retains(c)==((kw&4)!=0));
  CHECK((upgrade?definitions[m.id].upgraded_keywords:definitions[m.id].keywords)==kw);
  auto s=fight();int id=append(s,c,Zone::Hand);fill(s,Zone::Hand,3);fill(s,Zone::Draw,20);
  if(m.id==DefectBarrage){orb(s,DOrb::Lightning);orb(s,DOrb::Frost);}
  if(m.id==DefectCompileDriver||m.id==DefectSynchronize||m.id==DefectBulkUp){orb(s,DOrb::Lightning);orb(s,DOrb::Frost);orb(s,DOrb::Dark);}
  if(m.id==DefectDualcast||m.id==DefectQuadcast||m.id==DefectMultiCast)orb(s,DOrb::Dark);
  if(m.id==DefectTeslaCoil){orb(s,DOrb::Lightning);orb(s,DOrb::Lightning);}
  if(m.id==DefectAllForOne){append(s,{DefectClaw,0},Zone::Discard);append(s,{DefectStrike,0},Zone::Discard);}
  if(m.id==DefectHologram)append(s,{DefectZap,0},Zone::Discard);
  if(m.id==DefectCompact){s.cards[1]={Wound,0};s.cards[2]={Burn,0};}
  if(m.id==DefectFlakCannon){s.cards[1]={Wound,0};s.cards[4]={Dazed,0};append(s,{Burn,0},Zone::Discard);}
  if(m.id==DefectGoForTheEyes)s.enemies[0].move=0;
  if(m.id==DefectHelixDrill)s.defect.energy_spent_turn=2;
  if(m.id==DefectUproar)s.cards[4]={DefectStrike,0};
  if(m.id==DefectVoltaic)s.defect.lightning_channeled=2;
  if(m.id==DefectGeneticAlgorithm){s.deck[0]=c;s.defect.deck_origin[id]=1;}
  if(cost(c)==-1)s.energy=2;
  restore(s);CHECK(can_play_card(s,id));CHECK(play(s,0,0));settle(s);
  CHECK(s.phase==Phase::Combat&&s.playing==-1&&!s.effect_count);
  CHECK(s.zones[id]==(m.type==CardType::Power?Zone::Power:(kw&1)?Zone::Exhaust:Zone::Discard));
  int hit=0,all=0,random=0,block=0,draw=0;
  switch(m.id){
  case DefectAdaptiveStrike:hit=upgrade?23:18;CHECK(count_kind(s,m.id,Zone::Discard)==2);CHECK(cost(s,s.card_count-1)==0);break;
  case DefectAllForOne:hit=upgrade?14:10;CHECK(s.hand.count==4&&s.zones[24]==Zone::Hand&&s.zones[25]==Zone::Discard);break;
  case DefectBallLightning:hit=upgrade?10:7;CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Lightning&&s.defect.lightning_channeled==1);break;
  case DefectBarrage:hit=upgrade?14:10;break;
  case DefectBeamCell:hit=upgrade?4:3;CHECK(s.enemies[0].vulnerable==(upgrade?2:1));break;
  case DefectBiasedCognition:CHECK(power_value(s,XPower::Focus)==(upgrade?6:5)&&dp(s,DPower::BiasedCognition)==1);break;
  case DefectBoostAway:block=upgrade?9:6;CHECK(count_kind(s,Dazed,Zone::Discard)==1);break;
  case DefectBootSequence:block=upgrade?13:10;break;
  case DefectBuffer:CHECK(power_value(s,XPower::Buffer)==(upgrade?2:1));break;
  case DefectBulkUp:CHECK(s.strength==(upgrade?3:2)&&s.dexterity==(upgrade?3:2)&&s.expansion.orb_slots==2&&s.expansion.orb_count==2);break;
  case DefectCapacitor:CHECK(s.expansion.orb_slots==(upgrade?6:5));break;
  case DefectChaos:CHECK(s.expansion.orb_count==(upgrade?2:1));break;
  case DefectChargeBattery:block=upgrade?10:7;CHECK(dp(s,DPower::EnergyNextTurn)==1);break;
  case DefectChill:CHECK(s.expansion.orb_count==2&&s.defect.orb_kind[0]==(unsigned)DOrb::Frost&&s.defect.orb_kind[1]==(unsigned)DOrb::Frost);break;
  case DefectClaw:hit=upgrade?4:3;CHECK(s.defect.value[id]==(upgrade?3:2));break;
  case DefectColdSnap:hit=upgrade?9:6;CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Frost);break;
  case DefectCompact:block=upgrade?7:6;CHECK(s.cards[1].kind==Fuel&&s.cards[2].kind==Fuel&&s.cards[1].upgraded==upgrade&&s.cards[2].upgraded==upgrade);break;
  case DefectCompileDriver:hit=upgrade?10:7;draw=3;break;
  case DefectConsumingShadow:CHECK(s.expansion.orb_count==(upgrade?3:2)&&dp(s,DPower::ConsumingShadow)==1);break;
  case DefectCoolant:CHECK(dp(s,DPower::Coolant)==(upgrade?3:2));break;
  case DefectCoolheaded:draw=upgrade?2:1;CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Frost);break;
  case DefectCreativeAi:CHECK(dp(s,DPower::CreativeAi)==1);break;
  case DefectDarkness:CHECK(s.expansion.orb_count==1&&s.expansion.dark[0]==(upgrade?18:12));break;
  case DefectDefend:block=upgrade?8:5;break;
  case DefectDefragment:CHECK(power_value(s,XPower::Focus)==(upgrade?2:1));break;
  case DefectDoubleEnergy:CHECK(s.energy==(upgrade?40:38));break;
  case DefectDualcast:hit=12;CHECK(!s.expansion.orb_count);break;
  case DefectEchoForm:CHECK(dp(s,DPower::EchoForm)==1);break;
  case DefectFeral:CHECK(dp(s,DPower::Feral)==1);break;
  case DefectFightThrough:block=upgrade?17:13;CHECK(count_kind(s,Wound,Zone::Discard)==2);break;
  case DefectFlakCannon:random=upgrade?33:24;CHECK(s.exhaust.count==3&&s.zones[1]==Zone::Exhaust&&s.zones[4]==Zone::Exhaust&&s.zones[24]==Zone::Exhaust);break;
  case DefectFocusedStrike:hit=upgrade?11:9;CHECK(power_value(s,XPower::Focus)==(upgrade?2:1)&&s.defect.temporary_focus==(upgrade?2:1));break;
  case DefectFtl:hit=upgrade?6:5;draw=1;break;
  case DefectFusion:CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Plasma);break;
  case DefectGeneticAlgorithm:block=1;CHECK(s.defect.value[id]==(upgrade?4:3)&&s.defect.genetic_deck[0]==(upgrade?4:3));break;
  case DefectGlacier:block=upgrade?9:6;CHECK(s.expansion.orb_count==2);break;
  case DefectGlasswork:block=upgrade?8:5;CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Glass&&s.expansion.dark[0]==4);break;
  case DefectGoForTheEyes:hit=upgrade?4:3;CHECK(s.enemies[0].weak==(upgrade?2:1));break;
  case DefectGunkUp:hit=upgrade?15:12;CHECK(count_kind(s,Slimed,Zone::Discard)==1);break;
  case DefectHailstorm:CHECK(dp(s,DPower::Hailstorm)==(upgrade?8:6));break;
  case DefectHelixDrill:hit=upgrade?10:6;break;
  case DefectHologram:block=upgrade?5:3;CHECK(s.zones[24]==Zone::Hand&&s.hand.count==4);break;
  case DefectHotfix:CHECK(power_value(s,XPower::Focus)==2&&s.defect.temporary_focus==2);break;
  case DefectHyperbeam:all=upgrade?30:24;CHECK(power_value(s,XPower::Focus)==-3&&s.defect.temporary_focus==-3);break;
  case DefectIceLance:hit=upgrade?24:19;CHECK(s.expansion.orb_count==3);break;
  case DefectIteration:CHECK(dp(s,DPower::Iteration)==(upgrade?3:2));break;
  case DefectLeap:block=upgrade?12:9;break;
  case DefectLightningRod:block=upgrade?7:4;CHECK(dp(s,DPower::LightningRod)==2);break;
  case DefectLoop:CHECK(dp(s,DPower::Loop)==(upgrade?2:1));break;
  case DefectMachineLearning:CHECK(dp(s,DPower::MachineLearning)==1);break;
  case DefectMeteorStrike:hit=upgrade?30:24;CHECK(s.expansion.orb_count==3);for(unsigned i=0;i<3;i++)CHECK(s.defect.orb_kind[i]==(unsigned)DOrb::Plasma);break;
  case DefectModded:draw=upgrade?2:1;CHECK(s.expansion.orb_slots==4&&cost(s,id)==1);break;
  case DefectMomentumStrike:hit=upgrade?15:11;CHECK(cost(s,id)==0);break;
  case DefectMultiCast:hit=upgrade?18:12;CHECK(!s.energy&&!s.expansion.orb_count);break;
  case DefectNull:hit=upgrade?13:10;CHECK(s.enemies[0].weak==(upgrade?2:1)&&s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Dark);break;
  case DefectOverclock:draw=upgrade?3:2;CHECK(count_kind(s,Burn,Zone::Discard)==1);break;
  case DefectQuadcast:hit=24;CHECK(!s.expansion.orb_count);break;
  case DefectRainbow:CHECK(s.expansion.orb_count==3&&s.defect.orb_kind[0]==(unsigned)DOrb::Lightning&&s.defect.orb_kind[1]==(unsigned)DOrb::Frost&&s.defect.orb_kind[2]==(unsigned)DOrb::Dark);break;
  case DefectReboot:draw=upgrade?6:4;CHECK(s.hand.count==(unsigned)draw&&!s.discard.count);break;
  case DefectRefract:hit=upgrade?26:20;CHECK(s.expansion.orb_count==2&&s.defect.orb_kind[0]==(unsigned)DOrb::Glass);break;
  case DefectRocketPunch:hit=upgrade?14:13;draw=upgrade?2:1;break;
  case DefectScavenge:CHECK(s.exhaust.count==1&&dp(s,DPower::EnergyNextTurn)==(upgrade?3:2));break;
  case DefectScrape:hit=upgrade?10:7;draw=upgrade?5:4;CHECK(s.hand.count==3&&s.silent.discarded_turn==(unsigned)draw);break;
  case DefectShadowShield:block=upgrade?15:11;CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Dark);break;
  case DefectShatter:all=upgrade?11:7;break;
  case DefectSignalBoost:CHECK(dp(s,DPower::SignalBoost)==1);break;
  case DefectSkim:draw=upgrade?4:3;break;
  case DefectSmokestack:CHECK(dp(s,DPower::Smokestack)==(upgrade?7:5));break;
  case DefectSpinner:CHECK(dp(s,DPower::Spinner)==1&&s.expansion.orb_count==upgrade);break;
  case DefectStorm:CHECK(dp(s,DPower::Storm)==(upgrade?2:1)&&!s.expansion.orb_count);break;
  case DefectStrike:hit=upgrade?9:6;break;
  case DefectSubroutine:CHECK(dp(s,DPower::Subroutine)==1&&s.energy==(upgrade?20:19));break;
  case DefectSunder:hit=upgrade?34:26;CHECK(s.energy==17);break;
  case DefectSupercritical:CHECK(s.energy==(upgrade?26:24));break;
  case DefectSweepingBeam:all=upgrade?9:6;draw=1;break;
  case DefectSynchronize:CHECK(power_value(s,XPower::Focus)==(upgrade?6:3)&&s.defect.temporary_focus==(upgrade?6:3));break;
  case DefectSynthesis:hit=upgrade?20:14;CHECK(dp(s,DPower::FreePower)==1);break;
  case DefectTempest:CHECK(!s.energy&&s.expansion.orb_count==(upgrade?3:2)&&s.defect.lightning_channeled==(upgrade?3:2));break;
  case DefectTeslaCoil:hit=upgrade?16:9;break;
  case DefectThunder:CHECK(dp(s,DPower::Thunder)==(upgrade?11:8));break;
  case DefectTrashToTreasure:CHECK(dp(s,DPower::TrashToTreasure)==1);break;
  case DefectTurbo:CHECK(s.energy==(upgrade?23:22)&&count_kind(s,Void,Zone::Discard)==1);break;
  case DefectUproar:random=6;hit=upgrade?16:12;CHECK(s.defect.cards_finished==2&&s.zones[4]==Zone::Discard);break;
  case DefectVoltaic:CHECK(s.expansion.orb_count==2&&s.defect.lightning_channeled==4);break;
  case DefectWhiteNoise:CHECK(s.hand.count==4&&definitions[s.cards[s.card_count-1].kind].type==CardType::Power&&cost(s,s.card_count-1)==0);break;
  case DefectZap:CHECK(s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Lightning);break;
  default:CHECK(false);
  }
  CHECK(s.block==block);CHECK(s.silent.drawn_combat==(unsigned)draw);
  CHECK(s.enemies[0].hp+s.enemies[1].hp==2000-hit-2*all-random);
  if(!random){CHECK(s.enemies[0].hp==1000-hit-all);CHECK(s.enemies[1].hp==1000-all);}
  restore(s);++variants;
 }
 CHECK(variants==172);
}
// A newly queued synchronous effect is not an externally suspended state.
// Save/load checks belong after resolution or at an actual choice boundary.
static void effect(State&s,XOp op,int a=0,int b=0){queue_extension(s,op,a,b);resolve_pending(s);settle(s);}
static void orb_values_and_order(){
 scenario="five orb passive and evoke values";upgrade=0;
 const DOrb kinds[]={DOrb::Lightning,DOrb::Frost,DOrb::Dark,DOrb::Plasma,DOrb::Glass};
 for(DOrb kind:kinds){
  auto s=fight();orb(s,kind);gain_power(s,XPower::Focus,2);s.strength=9;s.dexterity=8;
  effect(s,XOp::DOrbPassive,0,-1);
  int passive=kind==DOrb::Lightning?5:kind==DOrb::Glass?12:0;
  CHECK(s.enemies[0].hp+s.enemies[1].hp==2000-passive);
  CHECK(s.block==(kind==DOrb::Frost?4:0));CHECK(s.energy==(kind==DOrb::Plasma?21:20));
  CHECK(s.expansion.dark[0]==(kind==DOrb::Dark?14:kind==DOrb::Glass?3:0));
  effect(s,XOp::DEvoke,0,1);CHECK(!s.expansion.orb_count);
  int evoked=kind==DOrb::Lightning?10:kind==DOrb::Dark?14:kind==DOrb::Glass?20:0;
  CHECK(s.enemies[0].hp+s.enemies[1].hp==2000-passive-evoked);
  CHECK(s.block==(kind==DOrb::Frost?11:0));CHECK(s.energy==(kind==DOrb::Plasma?23:20));
 }
 scenario="negative focus floor and glass does not decay at zero";
 for(DOrb kind:kinds){
  auto s=fight();orb(s,kind);gain_power(s,XPower::Focus,-9);effect(s,XOp::DOrbPassive,0,-1);
  CHECK(s.enemies[0].hp+s.enemies[1].hp==2000&&!s.block);
  CHECK(s.expansion.dark[0]==(kind==DOrb::Dark?6:kind==DOrb::Glass?4:0));
  effect(s,XOp::DEvoke,0,1);CHECK(s.enemies[0].hp+s.enemies[1].hp==2000-(kind==DOrb::Dark?6:0));
  CHECK(s.energy==(kind==DOrb::Plasma?23:20));
 }
 scenario="capacity drops newest; overflow evokes oldest";
 {auto s=fight();orb(s,DOrb::Dark,17);orb(s,DOrb::Frost);orb(s,DOrb::Glass);cast(s,DefectBulkUp);
  CHECK(s.expansion.orb_count==2&&s.defect.orb_kind[0]==(unsigned)DOrb::Dark&&s.defect.orb_kind[1]==(unsigned)DOrb::Frost);
  CHECK(s.enemies[0].hp+s.enemies[1].hp==2000);cast(s,DefectZap);
  CHECK(s.enemies[0].hp==983&&s.defect.orb_kind[0]==(unsigned)DOrb::Frost&&s.defect.orb_kind[1]==(unsigned)DOrb::Lightning);
 }
 scenario="zero slots distinguish base Defect from other characters";
 {auto s=fight();s.expansion.orb_slots=0;cast(s,DefectZap);CHECK(!s.expansion.orb_count&&!s.defect.lightning_channeled);}
 {auto s=fight();s.character=Character::Ironclad;s.expansion.orb_slots=0;effect(s,XOp::DChannel,(int)DOrb::Dark,1);CHECK(s.expansion.orb_slots==1&&s.expansion.orb_count==1);}
 scenario="dark targets lowest current health with stable ties";
 {auto s=fight();orb(s,DOrb::Dark,19);s.enemies[0].hp=950;s.enemies[1].hp=920;cast(s,DefectDualcast);CHECK(s.enemies[0].hp==950&&s.enemies[1].hp==882);}
 scenario="Thunder runs after each lightning evoke, never its passive";
 {auto s=fight();cast(s,DefectThunder);orb(s,DOrb::Lightning);effect(s,XOp::DOrbPassive,0,-1);CHECK(s.enemies[0].hp+s.enemies[1].hp==1997);cast(s,DefectDualcast);CHECK(s.enemies[0].hp+s.enemies[1].hp==1965);}
 scenario="Shatter evokes all five orbs twice";
 {auto s=fight();s.expansion.orb_slots=5;for(DOrb kind:kinds)orb(s,kind);cast(s,DefectShatter);
  CHECK(!s.expansion.orb_count&&s.block==10&&s.energy==23&&s.enemies[0].hp+s.enemies[1].hp==1926);
 }
 scenario="Voltaic snapshots channel history before its own loop";
 {auto s=fight();for(int i=0;i<3;i++)orb(s,DOrb::Lightning);s.defect.lightning_channeled=3;cast(s,DefectVoltaic);
  CHECK(s.defect.lightning_channeled==6&&s.expansion.orb_count==3&&s.enemies[0].hp+s.enemies[1].hp==1976);
 }
}
static void turn_boundaries(){
 upgrade=0;scenario="temporary focus lasts through natural end of turn orbs";
 {auto s=fight();orb(s,DOrb::Lightning);orb(s,DOrb::Frost);orb(s,DOrb::Dark);cast(s,DefectHotfix);quiet_turn(s);
  CHECK(power_value(s,XPower::Focus)==0&&!s.defect.temporary_focus&&s.expansion.dark[2]==14&&s.enemies[0].hp+s.enemies[1].hp==1995);
 }
 scenario="Loop precedes side-start focus loss and Coolant";
 {auto s=fight();orb(s,DOrb::Frost);cast(s,DefectBiasedCognition);cast(s,DefectCoolant);cast(s,DefectLoop);cast(s,DefectHailstorm);quiet_turn(s);
  CHECK(power_value(s,XPower::Focus)==4&&s.block==9&&s.enemies[0].hp==994&&s.enemies[1].hp==994);
 }
 scenario="Plasma natural trigger inspects queue after start powers";
 {auto s=fight();s.expansion.orb_slots=2;orb(s,DOrb::Glass);orb(s,DOrb::Plasma);cast(s,DefectLightningRod);cast(s,DefectSpinner);quiet_turn(s);
  CHECK(s.energy==5&&dp(s,DPower::LightningRod)==1&&s.expansion.orb_count==2);
  CHECK(s.defect.orb_kind[0]==(unsigned)DOrb::Lightning&&s.defect.orb_kind[1]==(unsigned)DOrb::Glass);
  CHECK(s.enemies[0].hp==990&&s.enemies[1].hp==990);quiet_turn(s);CHECK(!dp(s,DPower::LightningRod));
 }
 scenario="Creative AI creates a power before Machine Learning hand draw";
 {auto s=fight();cast(s,DefectCreativeAi);cast(s,DefectMachineLearning);fill(s,Zone::Draw,12);quiet_turn(s);
  CHECK(s.hand.count==7&&s.silent.drawn_combat==6);unsigned generated=0;
  for(unsigned i=0;i<s.hand.count;i++){Card c=s.cards[s.hand.ids[i]];if(definitions[c.kind].type==CardType::Power){++generated;CHECK(card_in_pool(c.kind,Character::Defect)&&card_rarity(c.kind)!=5);}}
  CHECK(generated==1);
 }
 scenario="temporary focus and Consuming Shadow respect acquisition order";
 for(int temp_first=0;temp_first<2;temp_first++){
  auto s=fight();s.expansion.orb_slots=5;
  if(temp_first)cast(s,DefectHotfix);
  cast(s,DefectConsumingShadow);if(!temp_first)cast(s,DefectHotfix);
  cast(s,DefectGlasswork);int before=s.enemies[0].hp;quiet_turn(s);
  // Glass passives for six, decays to three, then evokes for six or ten.
  CHECK(s.enemies[0].hp==before-(temp_first?12:16)&&s.enemies[1].hp==before-(temp_first?12:16));
  CHECK(!s.defect.temporary_focus&&power_value(s,XPower::Focus)==0&&s.expansion.orb_count==2);
 }
 scenario="Charge Battery and Scavenge queue energy only for next turn";
 {auto s=fight();cast(s,DefectChargeBattery);int fodder=append(s,{DefectDefend,0},Zone::Hand);(void)fodder;cast(s,DefectScavenge,1);CHECK(dp(s,DPower::EnergyNextTurn)==4);quiet_turn(s);CHECK(s.energy==7&&!dp(s,DPower::EnergyNextTurn));quiet_turn(s);CHECK(s.energy==3);}
}
static void card_instances_and_replays(){
 upgrade=0;scenario="Claw upgrades every extant instance but not later creations";
 {auto s=fight();int one=cast(s,DefectClaw);int two=append(s,{DefectClaw,1},Zone::Hand);int old=append(s,{DefectClaw,0},Zone::Exhaust);
  CHECK(s.defect.value[one]==2&&!s.defect.value[two]&&!s.defect.value[old]);
  CHECK(play(s,hand_index(s,two),0));settle(s);CHECK(s.enemies[0].hp==993);
  CHECK(s.defect.value[one]==5&&s.defect.value[two]==3&&s.defect.value[old]==3);
  int fresh=append(s,{DefectClaw,0},Zone::Hand);CHECK(damage(s,s.cards[fresh],0,fresh)==3);
 }
 scenario="duplicate Claw resolves its growth between repeated attacks";
 {auto s=fight();s.potions[0]=Potion::Duplicator;CHECK(use_potion(s,0,0));int id=cast(s,DefectClaw);
  CHECK(s.enemies[0].hp==992&&s.defect.value[id]==4&&s.defect.cards_finished==2&&s.defect.physical_started==1);
 }
 scenario="Genetic Algorithm keeps separate permanent deck instance growth";
 {auto s=fight();s.deck[0]={DefectGeneticAlgorithm,1};enter_combat(s);settle(s);CHECK(s.hand.count==1);
  int id=s.hand.ids[0];CHECK(s.defect.deck_origin[id]==1&&card_block(s,s.cards[id],id)==1);
  CHECK(play(s,0,0));settle(s);CHECK(s.block==1&&s.defect.genetic_deck[0]==4&&s.defect.value[id]==4);
  damage_unpowered(s,0,9999);damage_unpowered(s,1,9999);finish_combat_if_won(s);restore(s);
  enter_combat(s);settle(s);id=s.hand.ids[0];CHECK(s.defect.value[id]==4&&card_block(s,s.cards[id],id)==5);
  CHECK(play(s,0,0));settle(s);CHECK(s.block==5&&s.defect.genetic_deck[0]==8);
 }
 scenario="Adaptive Strike clones preserve modifiers but set combat cost zero";
 {auto s=fight();int id=cast(s,DefectAdaptiveStrike,1);int copy=s.discard.ids[0];if(copy==id)copy=s.discard.ids[1];
  CHECK(s.cards[copy].kind==DefectAdaptiveStrike&&s.cards[copy].upgraded&&cost(s,copy)==0&&cost(s,id)==2);
  move_hand(s,copy);CHECK(play(s,hand_index(s,copy),0));settle(s);CHECK(count_kind(s,DefectAdaptiveStrike,Zone::Discard)==3&&s.enemies[0].hp==954);
 }
 scenario="Echo Form repeats the first N physical plays once each";
 {auto s=fight();cast(s,DefectEchoForm);cast(s,DefectEchoForm);quiet_turn(s);s.energy=20;
  cast(s,DefectStrike);cast(s,DefectStrike,1);cast(s,DefectStrike);
  CHECK(s.enemies[0].hp==964&&s.defect.physical_started==3&&s.defect.cards_finished==5);
 }
 scenario="Signal Boost repeats one power and consumes one charge";
 {auto s=fight();cast(s,DefectSignalBoost);cast(s,DefectDefragment,1);CHECK(power_value(s,XPower::Focus)==4&&!dp(s,DPower::SignalBoost)&&s.defect.cards_finished==3);}
 scenario="Storm and Subroutine capture only pre-existing amounts";
 {auto s=fight();cast(s,DefectStorm);CHECK(!s.expansion.orb_count);cast(s,DefectSubroutine);CHECK(s.expansion.orb_count==1&&s.energy==18);
  cast(s,DefectDefragment);CHECK(s.expansion.orb_count==2&&s.energy==18);
  cast(s,DefectStorm,1);CHECK(dp(s,DPower::Storm)==3&&s.expansion.orb_count==3&&s.energy==18);
  cast(s,DefectDefragment,1);CHECK(s.enemies[0].hp+s.enemies[1].hp==1967&&s.energy==18&&s.defect.lightning_channeled==6);
 }
 scenario="Feral counts zero-energy attacks before acquisition";
 {auto s=fight();cast(s,DefectClaw);cast(s,DefectFeral);int second=cast(s,DefectClaw);CHECK(s.zones[second]==Zone::Discard);quiet_turn(s);s.energy=20;
  int first=cast(s,DefectClaw);CHECK(s.zones[first]==Zone::Hand);CHECK(play(s,hand_index(s,first),0));settle(s);CHECK(s.zones[first]==Zone::Discard);
 }
 scenario="Feral changes the final destination of an exhausting zero-cost attack";
 {auto s=fight();cast(s,DefectFeral);int id=cast(s,Shiv);CHECK(s.zones[id]==Zone::Hand&&s.defect.feral_used==1);
  CHECK(play(s,hand_index(s,id),0));settle(s);CHECK(s.zones[id]==Zone::Exhaust);
 }
 scenario="Synthesis free power cost is limited to hand and limbo";
 {auto s=fight();int old=cast(s,DefectDefragment);cast(s,DefectSynthesis);int next=append(s,{DefectDefragment,1},Zone::Hand);
  CHECK(cost(s,old)==1&&cost(s,next)==0&&s.energy==17);CHECK(play(s,hand_index(s,next),0));settle(s);
  CHECK(s.energy==17&&power_value(s,XPower::Focus)==3&&!dp(s,DPower::FreePower));
 }
 scenario="Momentum Strike keeps its zero combat cost on recall";
 {auto s=fight();int id=cast(s,DefectMomentumStrike);move_hand(s,id);CHECK(cost(s,id)==0);CHECK(play(s,hand_index(s,id),0));settle(s);CHECK(s.energy==19&&s.enemies[0].hp==978);}
 scenario="Modded adds combat cost after drawing and survives turns";
 {auto s=fight();int id=cast(s,DefectModded);move_hand(s,id);CHECK(play(s,hand_index(s,id),0));settle(s);CHECK(cost(s,id)==2&&s.expansion.orb_slots==5);
  quiet_turn(s);CHECK(cost(s,id)==2);if(s.zones[id]!=Zone::Hand)move_hand(s,id);CHECK(play(s,hand_index(s,id),0));settle(s);CHECK(cost(s,id)==3&&s.expansion.orb_slots==6);
 }
 scenario="Rocket Punch discount lasts across turns and expires when played";
 {auto s=fight();int rocket=append(s,{DefectRocketPunch,0},Zone::Hand);cast(s,DefectBoostAway);CHECK(cost(s,rocket)==1);cast(s,DefectFightThrough);CHECK(cost(s,rocket)==0);
  fill(s,Zone::Draw,10);quiet_turn(s);CHECK(cost(s,rocket)==0);if(s.zones[rocket]!=Zone::Hand)move_hand(s,rocket);
  CHECK(play(s,hand_index(s,rocket),0));settle(s);CHECK(cost(s,rocket)==2&&!s.defect.cost_delta[rocket]);
 }
 scenario="FTL uses finished plays and Helix Drill uses actual energy spent";
 {auto s=fight();fill(s,Zone::Draw,12);cast(s,DefectStrike);cast(s,DefectStrike);cast(s,DefectStrike);unsigned drawn=s.silent.drawn_combat;
  cast(s,DefectFtl);CHECK(s.silent.drawn_combat==drawn);int hp=s.enemies[0].hp;cast(s,DefectHelixDrill,1);CHECK(s.enemies[0].hp==hp-15);
 }
 scenario="Automatic attack resource value differs from actual energy expenditure";
 {auto s=fight();cast(s,DefectFeral);int sunder=append(s,{DefectSunder,0},Zone::Draw);cast(s,DefectUproar);
  CHECK(s.energy==16&&s.defect.energy_spent_turn==4);CHECK(s.zones[sunder]==Zone::Discard&&!s.defect.feral_used);
  int hp=s.enemies[0].hp;cast(s,DefectHelixDrill,1);CHECK(s.enemies[0].hp==hp-20);
 }
 scenario="Sunder energy requires this attack to kill";
 {auto s=fight();s.enemies[0].hp=26;cast(s,DefectSunder);CHECK(!s.enemies[0].hp&&s.energy==20&&s.phase==Phase::Combat);}
 scenario="Go for the Eyes leaves a non-attacking intent untouched";
 {auto s=fight();cast(s,DefectGoForTheEyes,1);CHECK(!s.enemies[0].weak&&s.enemies[0].hp==996);}
}
static void statuses_draws_and_choices(){
 upgrade=0;scenario="Iteration triggers only for the first status drawn this turn";
 {auto s=fight();cast(s,DefectIteration);fill(s,Zone::Draw,8);append(s,{Wound,0},Zone::Draw);append(s,{Void,0},Zone::Draw);
  draw_cards(s,1);settle(s);CHECK(s.silent.drawn_combat==3&&s.hand.count==3&&s.energy==18&&s.defect.status_drawn==1);
  append(s,{Void,0},Zone::Draw);draw_cards(s,1);settle(s);CHECK(s.silent.drawn_combat==4&&s.energy==17);
 }
 scenario="Iteration consults status history before acquisition";
 {auto s=fight();append(s,{Wound,0},Zone::Draw);draw_cards(s,1);cast(s,DefectIteration);append(s,{Wound,0},Zone::Draw);draw_cards(s,1);settle(s);CHECK(s.silent.drawn_combat==2);}
 scenario="Scrape excludes nested Iteration draws and discards X-cost cards";
 {auto s=fight();cast(s,DefectIteration);
  int strike=append(s,{DefectStrike,0},Zone::Draw),multi=append(s,{DefectMultiCast,0},Zone::Draw),claw=append(s,{DefectClaw,0},Zone::Draw);
  int nested1=append(s,{DefectDefend,0},Zone::Draw),nested2=append(s,{DefectDefend,0},Zone::Draw),wound=append(s,{Wound,0},Zone::Draw);
  cast(s,DefectScrape);CHECK(s.silent.drawn_combat==6&&s.silent.discarded_turn==3&&s.hand.count==3);
  CHECK(s.zones[strike]==Zone::Discard&&s.zones[multi]==Zone::Discard&&s.zones[wound]==Zone::Discard);
  CHECK(s.zones[claw]==Zone::Hand&&s.zones[nested1]==Zone::Hand&&s.zones[nested2]==Zone::Hand);
 }
 scenario="Scrape captured draw identities survive a nested automatic choice";
 {auto s=fight();cast(s,Hellraiser);fill(s,Zone::Draw,3);int sculpt=append(s,{NecroSculptingStrike,0},Zone::Draw),zap=append(s,{DefectZap,1},Zone::Hand);
  int scrape=append(s,{DefectScrape,0},Zone::Hand);CHECK(play(s,hand_index(s,scrape),0));
  CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::NEthereal&&s.expansion.depth==1);
  CHECK(s.defect.scrape_count[0]==1&&s.defect.scrape_drawn[0][0]==sculpt+1);
  choose(s,zap);CHECK(s.zones[zap]==Zone::Hand&&s.hand.count==1&&s.necro.ethereal[zap]);
  CHECK(s.zones[sculpt]==Zone::Discard&&s.silent.drawn_combat==4&&s.silent.discarded_turn==3);
  for(unsigned f=0;f<=MaxAutoFrames;f++)CHECK(!s.defect.scrape_waiting[f]&&!s.defect.scrape_count[f]);
 }
 scenario="status creation triggers each power once and discounts Rocket Punch";
 {auto s=fight();cast(s,DefectSmokestack);cast(s,DefectTrashToTreasure);int rocket=append(s,{DefectRocketPunch,0},Zone::Hand);
  cast(s,DefectFightThrough);CHECK(s.enemies[0].hp==990&&s.enemies[1].hp==990&&s.expansion.orb_count==2&&cost(s,rocket)==0);
  cast(s,DefectBoostAway);CHECK(s.enemies[0].hp==985&&s.enemies[1].hp==985&&s.expansion.orb_count==3);
 }
 scenario="Compact transforms statuses to Fuel without status generation";
 {auto s=fight();cast(s,DefectSmokestack);cast(s,DefectTrashToTreasure);int wound=append(s,{Wound,0},Zone::Hand);cast(s,DefectCompact,1);
  CHECK(s.cards[wound].kind==Fuel&&s.cards[wound].upgraded&&!s.expansion.orb_count&&s.enemies[0].hp==1000);
  int energy=s.energy;CHECK(play(s,hand_index(s,wound),0));settle(s);CHECK(s.energy==energy+2&&s.zones[wound]==Zone::Exhaust);
 }
 scenario="Flak Cannon exhausts captured statuses in every pile before its hits";
 {auto s=fight();cast(s,DarkEmbrace);cast(s,FeelNoPain);fill(s,Zone::Draw,8);
  int hand=append(s,{Wound,0},Zone::Hand),draw=append(s,{Dazed,0},Zone::Draw),discard=append(s,{Void,0},Zone::Discard),old=append(s,{Burn,0},Zone::Exhaust);
  cast(s,DefectFlakCannon);CHECK(s.zones[hand]==Zone::Exhaust&&s.zones[draw]==Zone::Exhaust&&s.zones[discard]==Zone::Exhaust&&s.zones[old]==Zone::Exhaust);
  CHECK(s.silent.drawn_combat==3&&s.block==9&&s.enemies[0].hp+s.enemies[1].hp==1976);
 }
 scenario="Compact announces replacement Fuel to cross-character creation powers";
 {auto s=fight();cast(s,RegentArsenal);cast(s,RegentPillarOfCreation);int wound=append(s,{Wound,0},Zone::Hand),burn=append(s,{Burn,0},Zone::Hand);
  int created=s.regent.cards_created;cast(s,DefectCompact);
  CHECK(s.cards[wound].kind==Fuel&&s.cards[burn].kind==Fuel);CHECK(s.regent.cards_created==created+2&&s.strength==2&&s.block==10);
 }
 scenario="Flak snapshot operation includes an active ancestor Status and cleanup is idempotent";
 {auto s=fight();cast(s,Hellraiser);int sculpt=append(s,{NecroSculptingStrike,0},Zone::Draw),slimed=append(s,{Slimed,0},Zone::Hand);
  int zap=append(s,{DefectZap,1},Zone::Hand);CHECK(play(s,hand_index(s,slimed),0));
  CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::NEthereal&&s.expansion.depth==1);
  CHECK(s.zones[slimed]==Zone::Limbo&&s.zones[sculpt]==Zone::Limbo);restore(s);
  // A direct operation-boundary fixture: Hellraiser cannot play Flak Cannon,
  // which has no Strike tag. Resume the real choice with Flak's queued body
  // to exercise its PlayPile snapshot while the Slimed ancestor is active.
  queue_extension(s,XOp::DFlak,8,0,true);choose(s,zap);
  CHECK(s.zones[slimed]==Zone::Exhaust&&s.zones[sculpt]==Zone::Discard&&s.exhaust.count==1);
  CHECK(s.enemies[0].hp+s.enemies[1].hp==1983&&s.defect.cards_finished==3);
 }
 scenario="Reboot moves the hand directly without discard listeners";
 {auto s=fight();append(s,{Reflex,0},Zone::Hand);append(s,{Tactician,0},Zone::Hand);fill(s,Zone::Draw,8);cast(s,DefectReboot);
  CHECK(!s.silent.discarded_turn&&s.silent.drawn_combat==4&&s.hand.count==4&&s.energy==20);
 }
 scenario="Slimed draws one; Burn uses block; Dazed is ethereal";
 {auto s=fight();fill(s,Zone::Draw,4);int slime=cast(s,Slimed);CHECK(s.silent.drawn_combat==1&&s.energy==19&&s.zones[slime]==Zone::Exhaust);}
 {auto s=fight();int burn=append(s,{Burn,0},Zone::Hand),dazed=append(s,{Dazed,0},Zone::Hand);s.block=1;
  CHECK(!can_play_card(s,burn)&&!can_play_card(s,dazed));quiet_turn(s);CHECK(s.hp==74&&s.zones[dazed]==Zone::Exhaust);
 }
 scenario="Hologram resumes selected instance with cost history intact";
 {auto s=fight();int modded=cast(s,DefectModded);int hologram=append(s,{DefectHologram,1},Zone::Hand);CHECK(play(s,hand_index(s,hologram),0));
  CHECK(s.phase==Phase::ExtendedChoice);choose(s,modded);CHECK(s.zones[modded]==Zone::Hand&&cost(s,modded)==1&&s.block==5);
 }
 scenario="Nightmare copies Defect instance values from the choice snapshot";
 {auto s=fight();int claw=cast(s,DefectClaw);move_hand(s,claw);int nightmare=append(s,{Nightmare,0},Zone::Hand);CHECK(play(s,hand_index(s,nightmare),0));choose(s,claw);
  CHECK(play(s,hand_index(s,claw),0));settle(s);CHECK(s.defect.value[claw]==4);unsigned old_count=s.card_count;quiet_turn(s);
  CHECK(s.card_count==old_count+3);for(unsigned i=old_count;i<s.card_count;i++)CHECK(s.cards[i].kind==DefectClaw&&s.defect.value[i]==2&&!s.defect.deck_origin[i]);
 }
}
static void character_start_and_validation(){
 upgrade=0;scenario="Defect canonical start and Cracked Core";
 {State s{};new_run(s,719,false,Character::Defect);CHECK(s.hp==75&&s.max_hp==75&&s.gold==99&&s.deck_count==10);
  int strikes=0,defends=0,zaps=0,dualcasts=0;for(unsigned i=0;i<s.deck_count;i++){Card c=s.deck[i];CHECK(!c.upgraded);strikes+=c.kind==DefectStrike;defends+=c.kind==DefectDefend;zaps+=c.kind==DefectZap;dualcasts+=c.kind==DefectDualcast;}
  CHECK(strikes==4&&defends==4&&zaps==1&&dualcasts==1);s.current_node=0;enter_combat(s);settle(s);
  CHECK(s.expansion.orb_slots==3&&s.expansion.orb_count==1&&s.defect.orb_kind[0]==(unsigned)DOrb::Lightning&&s.defect.lightning_channeled==1);
  CHECK(card_strike_tag(DefectStrike)&&!card_strike_tag(DefectDefend));
 }
 scenario="Ancient cards are playable but excluded from ordinary generation";
 CHECK(!card_can_generate(DefectBiasedCognition)&&!card_can_generate(DefectQuadcast));
 {auto s=fight();for(int i=0;i<256;i++){unsigned id=random_reward_card(s);CHECK(card_in_pool(id,Character::Defect)&&card_rarity(id)>=2&&card_rarity(id)<=4);}}
 scenario="existing orb potions share the complete five-orb queue";
 {auto s=fight();s.character=Character::Ironclad;s.expansion.orb_slots=0;s.potions[0]=Potion::PotionOfCapacity;CHECK(use_potion(s,0,0));CHECK(s.expansion.orb_slots==2);
  s.potions[0]=Potion::EssenceOfDarkness;CHECK(use_potion(s,0,0));CHECK(s.expansion.orb_count==2&&s.expansion.dark[0]==6&&s.defect.orb_kind[0]==(unsigned)DOrb::Dark);
  s.potions[0]=Potion::FocusPotion;CHECK(use_potion(s,0,0));CHECK(power_value(s,XPower::Focus)==2);effect(s,XOp::DOrbPassive,0,-1);CHECK(s.expansion.dark[0]==14);
 }
 scenario="dynamic descriptions retain unpowered amounts and instance growth";
 {auto s=fight();s.strength=8;s.dexterity=7;int out=-1;
  CHECK(character_description_value(s,{DefectThunder,1},0,-1,"Damage",out)&&out==11);
  CHECK(character_description_value(s,{DefectCoolant,1},0,-1,"Block",out)&&out==3);
  int claw=cast(s,DefectClaw);CHECK(character_description_value(s,s.cards[claw],0,claw,"Damage",out)&&out==13);
 }
 scenario="malformed Defect state and queued operands are rejected";
 {auto s=fight();orb(s,DOrb::Lightning);restore(s);auto bad=s;bad.defect.orb_kind[0]=(uint8_t)DOrb::Count;CHECK(!validate(bad));
  bad=s;bad.expansion.dark[0]=1;CHECK(!validate(bad));bad=s;bad.defect.temporary_focus=1;CHECK(!validate(bad));
  bad=s;bad.defect.powers[(unsigned)DPower::Loop]=1;CHECK(!validate(bad));
  bad=s;queue_extension(bad,XOp::DExhaustStatus,append(bad,{DefectDefend,0},Zone::Hand));CHECK(!validate(bad));
  bad=s;queue_extension(bad,XOp::DOrbPassive,MaxOrbs,-1);CHECK(!validate(bad));
 }
}
int main(){
 complete_model_matrix();orb_values_and_order();turn_boundaries();card_instances_and_replays();statuses_draws_and_choices();character_start_and_validation();
 std::printf("Defect completion: %u canonical variants, %u checks passed\n",variants,checks);return 0;
}
