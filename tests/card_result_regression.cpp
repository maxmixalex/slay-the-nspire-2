#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static State s,loaded;
static SaveData before,after;
static unsigned checks;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"card result: %s:%d [%s] %s\n",__FILE__,__LINE__,scenario,#x);std::exit(1);}}while(0)
static void reset(){
 new_run(s,719,false,Character::Ironclad);s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=99;s.hp=s.max_hp=80;s.expansion.stars=99;
 s.deck_count=1;s.deck[0]={Defend,0};for(auto&e:s.enemies)e={2000,2000,0,0,0,2};CHECK(validate(s));
}
static int append(CardKind kind,Zone zone=Zone::Hand,unsigned up=0){
 CHECK(s.card_count<MaxCards);const int id=s.card_count++;s.cards[id]={kind,uint8_t(up)};s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:zone==Zone::Exhaust?s.exhaust:s.powers;
 CHECK(zone!=Zone::Hand||p.count<MaxHand);p.ids[p.count++]=uint16_t(id);return id;
}
static int index_of(const Pile&p,int id){for(unsigned i=0;i<p.count;++i)if(p.ids[i]==id)return int(i);return -1;}
static unsigned rebound(){return unsigned(s.event_cards.powers[unsigned(EventCardPower::Rebound)]);}
static void roundtrip(){
 CHECK(validate(s)&&save(s,before));CHECK(load(loaded,before.bytes,before.size));CHECK(save(loaded,after));
 CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));s=loaded;
}
static void preview(int id,Zone expected,bool top){
 CHECK(save(s,before));Zone result=Zone::Limbo;bool actual_top=!top;
 for(unsigned i=0;i<3;++i)CHECK(card_play_result(s,id,result,actual_top)&&result==expected&&actual_top==top);
 CHECK(save(s,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
static void play_id(int id){CHECK(play(s,index_of(s.hand,id),0));CHECK(s.phase==Phase::Combat);CHECK(!s.effect_count&&s.playing<0);CHECK(validate(s));}
static int cast(CardKind kind,unsigned up=0){int id=append(kind,Zone::Hand,up);play_id(id);return id;}
static void ordered_modifiers(){
 scenario="Rebound then Corruption consumes Rebound before the final Exhaust";
 reset();cast(EventCardRebound);cast(Corruption);const int old_exhaust=append(Dazed,Zone::Exhaust);int skill=append(Defend);preview(skill,Zone::Exhaust,true);CHECK(rebound()==1);play_id(skill);
 // OnPlayWrapper uses CardCmd.Exhaust for this pile, whose insertion is always
 // Bottom even though the complete captured modifier location retains Top.
 CHECK(s.zones[skill]==Zone::Exhaust&&!rebound()&&s.exhaust.ids[0]==old_exhaust&&s.exhaust.ids[s.exhaust.count-1]==skill);CHECK(s.event_cards.power_count==0);
 CHECK(s.expansion.result_order[0]==unsigned(CardResultActor::Corruption)&&!s.expansion.result_order[1]);roundtrip();
 scenario="depleted Rebound reacquires after the existing Corruption listener";
 cast(EventCardRebound);skill=append(Defend);preview(skill,Zone::Exhaust,false);play_id(skill);CHECK(rebound()==1&&s.zones[skill]==Zone::Exhaust);roundtrip();
 scenario="Corruption then Rebound leaves the unused Rebound charge";
 reset();cast(Corruption);cast(EventCardRebound);skill=append(Defend);preview(skill,Zone::Exhaust,false);play_id(skill);CHECK(rebound()==1&&s.zones[skill]==Zone::Exhaust);roundtrip();
 scenario="Rebound then Feral consumes both changed listeners";
 reset();cast(EventCardRebound);cast(DefectFeral);int anchor=append(Defend),attack=append(DefectClaw);preview(attack,Zone::Hand,true);CHECK(!s.defect.feral_used&&rebound()==1);play_id(attack);
 CHECK(s.zones[attack]==Zone::Hand&&s.hand.ids[0]==attack&&s.hand.ids[1]==anchor&&!rebound()&&s.defect.feral_used==1);roundtrip();
 scenario="Feral then Rebound does not consume the latter";
 reset();cast(DefectFeral);cast(EventCardRebound);attack=append(DefectClaw);preview(attack,Zone::Hand,true);play_id(attack);
 CHECK(s.zones[attack]==Zone::Hand&&rebound()==1&&s.defect.feral_used==1);roundtrip();
 scenario="Nostalgia preceding Rebound owns an otherwise identical Draw result";
 reset();gain_power(s,XPower::Nostalgia,3);cast(EventCardRebound);attack=append(Strike);preview(attack,Zone::Draw,true);play_id(attack);
 CHECK(s.zones[attack]==Zone::Draw&&s.draw.ids[s.draw.count-1]==attack&&rebound()==1);roundtrip();
 scenario="Rebound preceding Nostalgia consumes its charge even with the same final pile";
 reset();cast(EventCardRebound);gain_power(s,XPower::Nostalgia,3);attack=append(Strike);preview(attack,Zone::Draw,true);play_id(attack);
 CHECK(s.zones[attack]==Zone::Draw&&!rebound());roundtrip();
}
static void power_lifecycle(){
 scenario="stacking keeps the power's original list position and Feral's internal history";
 reset();cast(DefectClaw);cast(DefectFeral);cast(Corruption);gain_power(s,XPower::Nostalgia,3);cast(EventCardRebound);
 CHECK(s.defect.zero_attacks_started==1&&s.defect.feral_used==1);
 const CardResultActor original[]={CardResultActor::Feral,CardResultActor::Corruption,CardResultActor::Nostalgia,CardResultActor::Rebound};
 for(unsigned i=0;i<CardResultActors;++i)CHECK(s.expansion.result_order[i]==unsigned(original[i]));
 queue_extension(s,XOp::DPower,int(DPower::Feral),1);queue_extension(s,XOp::IPower,int(IPower::Corruption),1);resolve_pending(s);
 gain_power(s,XPower::Nostalgia,2);cast(EventCardRebound);
 CHECK(s.defect.powers[unsigned(DPower::Feral)]==2&&s.defect.feral_used==1&&s.ironclad.powers[unsigned(IPower::Corruption)]==1&&rebound()==2);
 for(unsigned i=0;i<CardResultActors;++i)CHECK(s.expansion.result_order[i]==unsigned(original[i]));roundtrip();
 scenario="negative removal compacts the list and a new instance rejoins at its end";
 queue_extension(s,XOp::DPower,int(DPower::Feral),-9999);resolve_pending(s);
 CHECK(s.expansion.result_order[0]==unsigned(CardResultActor::Corruption)&&s.expansion.result_order[1]==unsigned(CardResultActor::Nostalgia)&&s.expansion.result_order[2]==unsigned(CardResultActor::Rebound)&&!s.expansion.result_order[3]);
 queue_extension(s,XOp::DPower,int(DPower::Feral),1);resolve_pending(s);
 CHECK(s.expansion.result_order[3]==unsigned(CardResultActor::Feral)&&s.defect.feral_used==s.defect.zero_attacks_started);roundtrip();
 queue_extension(s,XOp::IPower,int(IPower::Corruption),-9999);resolve_pending(s);gain_power(s,XPower::Nostalgia,-9999);
 CHECK(s.expansion.result_order[0]==unsigned(CardResultActor::Rebound)&&s.expansion.result_order[1]==unsigned(CardResultActor::Feral)&&!s.expansion.result_order[2]);roundtrip();
 scenario="Rebound side-end removal leaves the other power's acquisition order intact";
 CHECK(end_turn(s)&&s.phase==Phase::Combat&&s.turn==2&&!rebound());
 CHECK(s.expansion.result_order[0]==unsigned(CardResultActor::Feral)&&!s.expansion.result_order[1]&&!s.defect.feral_used);roundtrip();
 scenario="removing a power while its physical card is paused does not recalculate that card";
 reset();gain_power(s,XPower::Nostalgia,1);append(Strike,Zone::Discard);const int recalled=append(Defend,Zone::Discard),headbutt=append(Headbutt);
 CHECK(play(s,index_of(s.hand,headbutt),0)&&s.phase==Phase::ReturnChoice);
 gain_power(s,XPower::Nostalgia,-9999);CHECK(!s.expansion.result_order[0]);preview(headbutt,Zone::Draw,true);roundtrip();
 CHECK(choose_return_to_draw(s,recalled)&&s.zones[headbutt]==Zone::Draw&&s.draw.ids[s.draw.count-1]==headbutt);roundtrip();
}
static void base_locations(){
 scenario="Particle Wall supplies Hand Bottom before power modifiers";
 reset();gain_power(s,XPower::Nostalgia,3);cast(EventCardRebound);int anchor=append(Defend),wall=append(RegentParticleWall);preview(wall,Zone::Hand,false);play_id(wall);
 CHECK(s.zones[wall]==Zone::Hand&&s.hand.ids[0]==anchor&&s.hand.ids[s.hand.count-1]==wall&&rebound()==1);
 scenario="Corruption can override Particle Wall's intrinsic Hand result";
 cast(Corruption);preview(wall,Zone::Exhaust,false);play_id(wall);CHECK(s.zones[wall]==Zone::Exhaust&&rebound()==1);roundtrip();
 scenario="Havoc's forced Exhaust precedes Particle Wall's conditional base override";
 reset();wall=append(RegentParticleWall,Zone::Draw);cast(Havoc);CHECK(s.zones[wall]==Zone::Exhaust&&!s.ironclad.forced_exhaust[wall]);roundtrip();
 scenario="Shining Strike's intrinsic Draw result leaves Rebound unused";
 reset();cast(EventCardRebound);gain_power(s,XPower::Nostalgia,4);int shining=append(RegentShiningStrike);preview(shining,Zone::Draw,true);play_id(shining);
 CHECK(s.zones[shining]==Zone::Draw&&s.draw.ids[s.draw.count-1]==shining&&rebound()==1);roundtrip();
 scenario="Shining Strike cannot undo a forced Exhaust in its card body";
 reset();shining=append(RegentShiningStrike,Zone::Draw);cast(Havoc);CHECK(s.zones[shining]==Zone::Exhaust&&!s.ironclad.forced_exhaust[shining]);roundtrip();
 scenario="Feral can replace a zero-cost Shining Strike's intrinsic Draw result";
 reset();cast(DefectFeral);cast(EventCardRebound);shining=append(RegentShiningStrike);s.expansion.turn_cost[shining]=1;preview(shining,Zone::Hand,true);play_id(shining);
 CHECK(s.zones[shining]==Zone::Hand&&rebound()==1&&s.defect.feral_used==1);roundtrip();
 scenario="base preview does not consume ExhaustOnNextPlay";
 reset();wall=append(RegentParticleWall);s.ironclad.forced_exhaust[wall]=1;preview(wall,Zone::Exhaust,false);CHECK(s.ironclad.forced_exhaust[wall]);play_id(wall);
 CHECK(s.zones[wall]==Zone::Exhaust&&!s.ironclad.forced_exhaust[wall]);roundtrip();
}
static void physical_capture_and_history(){
 scenario="a Corruption acquired during Havoc does not alter Havoc's captured result";
 reset();const int corruption=append(Corruption,Zone::Draw),havoc=cast(Havoc);CHECK(s.zones[corruption]==Zone::Power&&s.zones[havoc]==Zone::Discard);roundtrip();
 scenario="Nostalgia counts Attack and Skill history without playable statuses";
 reset();gain_power(s,XPower::Nostalgia,1);cast(Slimed);CHECK(!s.expansion.nonpower_started);const int strike=append(Strike);preview(strike,Zone::Draw,true);play_id(strike);
 CHECK(s.zones[strike]==Zone::Draw&&s.expansion.nonpower_started==1);roundtrip();
 scenario="a repeated physical attack consumes one Feral result modifier";
 reset();cast(DefectFeral);gain_power(s,XPower::Duplicator,1);const int claw=cast(DefectClaw);
 CHECK(s.zones[claw]==Zone::Hand&&s.defect.feral_used==1&&s.defect.zero_attacks_started==2);roundtrip();
 scenario="captured Feral survives a pending choice and cannot be consumed by preview";
 reset();cast(DefectFeral);append(Strike,Zone::Discard);int recalled=append(Defend,Zone::Discard),headbutt=append(Headbutt);s.expansion.turn_cost[headbutt]=1;
 CHECK(play(s,index_of(s.hand,headbutt),0)&&s.phase==Phase::ReturnChoice);CHECK(s.defect.feral_used==1&&s.expansion.feral_result[0]==2);
 preview(headbutt,Zone::Hand,true);roundtrip();CHECK(choose_return_to_draw(s,recalled));CHECK(s.zones[headbutt]==Zone::Hand&&s.defect.feral_used==1);roundtrip();
 scenario="a relic acquisition selection retains the suspended card's combat frames";
 reset();cast(DefectFeral);append(Strike,Zone::Discard);recalled=append(Defend,Zone::Discard);headbutt=append(Headbutt);s.expansion.turn_cost[headbutt]=1;
 // Both commands need more than the requested single candidate to retain
 // their original suspension boundary; source singleton commands return all.
 s.deck[s.deck_count++]={Strike,0};CHECK(validate(s));
 CHECK(play(s,index_of(s.hand,headbutt),0)&&s.phase==Phase::ReturnChoice);CHECK(gain_world_relic(s,WorldRelicDollysMirror));
 CHECK(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice==WorldRelicChoice::Duplicate&&s.world_relics.return_phase==unsigned(Phase::ReturnChoice));
 preview(headbutt,Zone::Hand,true);roundtrip();
 s.world_relics.return_phase=unsigned(Phase::Map);CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 const uint16_t permanent=0;CHECK(world_relic_select(s,&permanent,1)&&s.phase==Phase::ReturnChoice);
 CHECK(choose_return_to_draw(s,recalled));CHECK(s.zones[headbutt]==Zone::Hand&&s.deck_count==3&&s.defect.feral_used==1);roundtrip();
 scenario="nested automatic plays preserve the outer captured result through save and resume";
 reset();cast(DefectFeral);append(Strike,Zone::Discard);recalled=append(Defend,Zone::Discard);headbutt=append(Headbutt,Zone::Draw);const int uproar=append(DefectUproar);s.expansion.turn_cost[uproar]=1;
 CHECK(play(s,index_of(s.hand,uproar),0)&&s.phase==Phase::ReturnChoice&&s.expansion.depth==1);
 preview(uproar,Zone::Hand,true);preview(headbutt,Zone::Discard,false);CHECK(s.defect.feral_used==1);roundtrip();
 CHECK(choose_return_to_draw(s,recalled));CHECK(s.zones[uproar]==Zone::Hand&&s.zones[headbutt]==Zone::Discard&&s.defect.feral_used==1);roundtrip();
 scenario="Abundance's virtual offer preserves its captured Exhaust through save and resume";
 reset();const int abundance=append(EventCardAbundance);
 CHECK(play(s,index_of(s.hand,abundance),0)&&s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::EventAbundance);
 CHECK(s.expansion.choices.count==3&&s.expansion.choice_min==1&&s.expansion.choice_max==1);
 for(unsigned i=0;i<3;++i){const int token=s.expansion.choices.ids[i];CHECK(token==MaxCards+int(i));const Card offered=choice_card(s,token);CHECK(card_type(s,offered)==CardType::Power&&!card_upgradeable(offered));}
 preview(abundance,Zone::Exhaust,false);roundtrip();
 s.expansion.choices.ids[0]=MaxCards+3;CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 const uint16_t choice=s.expansion.choices.ids[1];const Card selected=choice_card(s,choice);
 CHECK(choose_extended(s,&choice,1));CHECK(s.zones[abundance]==Zone::Exhaust&&s.hand.count==1);
 const int generated=s.hand.ids[0];CHECK(card_token(s.cards[generated])==card_token(selected)&&cost(s,generated)==0);roundtrip();
 scenario="full-hand Feral preserves its original Top insertion when redirected to discard";
 reset();cast(DefectFeral);const int old_discard=append(Defend,Zone::Discard);headbutt=append(DefectScrape);s.expansion.turn_cost[headbutt]=1;
 for(unsigned i=0;i<MaxHand-1;++i)append(Defend);for(unsigned i=0;i<5;++i)append(DefectClaw,Zone::Draw);
 play_id(headbutt);CHECK(s.hand.count==MaxHand&&s.zones[headbutt]==Zone::Discard&&s.discard.ids[0]==headbutt&&index_of(s.discard,old_discard)>0);roundtrip();
}
static void dupes_and_validation(){
 scenario="Corruption modifies even a skill Dupe's base None result";
 reset();cast(Corruption);int id=append(Defend,Zone::Draw);s.world_relics.card_is_dupe[id]=1;preview(id,Zone::Exhaust,false);
 queue_extension(s,XOp::AutoInstance,id,0,true);resolve_pending(s);CHECK(s.zones[id]==Zone::Exhaust&&s.block==5);roundtrip();
 scenario="ordinary skill Dupe remains outside recyclable piles";
 reset();id=append(Defend,Zone::Draw);s.world_relics.card_is_dupe[id]=1;preview(id,Zone::Power,false);queue_extension(s,XOp::AutoInstance,id,0,true);resolve_pending(s);
 CHECK(s.zones[id]==Zone::Power&&!s.exhaust.count&&!s.discard.count);roundtrip();
 scenario="Feral ignores true Dupe attacks without consuming its charge";
 reset();cast(DefectFeral);id=append(DefectClaw,Zone::Draw);s.world_relics.card_is_dupe[id]=1;preview(id,Zone::Power,false);queue_extension(s,XOp::AutoInstance,id,0,true);resolve_pending(s);
 CHECK(s.zones[id]==Zone::Power&&!s.defect.feral_used);roundtrip();
 scenario="an automatically skipped unplayable Dupe bypasses result modifiers";
 reset();cast(Corruption);id=append(Dazed,Zone::Draw);s.world_relics.card_is_dupe[id]=1;queue_extension(s,XOp::AutoInstance,id,0,true);resolve_pending(s);CHECK(s.zones[id]==Zone::Power&&!s.exhaust.count);roundtrip();
 scenario="result listener order rejects gaps, duplicates and inactive entries";
 reset();cast(DefectFeral);cast(Corruption);CHECK(save(s,before));
 s.expansion.result_order[1]=unsigned(CardResultActor::Feral);CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 s.expansion.result_order[0]=0;CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 s.expansion.result_order[2]=unsigned(CardResultActor::Nostalgia);CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 s.expansion.result_location[0]=12;CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 s.expansion.depth=MaxAutoFrames+1;CHECK(!validate(s));CHECK(load(s,before.bytes,before.size));
 for(auto result:s.expansion.result_location)CHECK(!result);
}
int main(){ordered_modifiers();power_lifecycle();base_locations();physical_capture_and_history();dupes_and_validation();std::printf("card result: %u checks passed\n",checks);}
