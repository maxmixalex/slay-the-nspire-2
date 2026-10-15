#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
using namespace spire;
static unsigned checks=0,transitions=0;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static State fight(std::initializer_list<Card> cards,int enemies=1){
    State s{};new_run(s,19,false,Character::Silent);s.phase=Phase::Combat;s.floor=1;s.turn=1;s.energy=3;
    s.enemy_count=(uint8_t)enemies;s.deck_count=s.card_count=0;
    for(auto card:cards){int id=s.card_count++;s.deck[s.deck_count++]=card;s.cards[id]=card;s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=(uint16_t)id;}
    for(int i=0;i<enemies;i++)s.enemies[i]={46,46,0,0,0,0};CHECK(validate(s));return s;
}
static void supply(State&s,int n){for(int i=0;i<n;i++){int id=s.card_count++;s.cards[id]={DefendSilent,0};s.zones[id]=Zone::Draw;s.draw.ids[s.draw.count++]=(uint16_t)id;}CHECK(validate(s));}
static int hand(const State&s,unsigned kind){for(int i=0;i<s.hand.count;i++)if(s.cards[s.hand.ids[i]].kind==kind)return i;return -1;}
static unsigned count(const State&s,Zone zone,unsigned kind){unsigned n=0;const auto&p=pile(s,zone);for(int i=0;i<p.count;i++)n+=s.cards[p.ids[i]].kind==kind;return n;}
static void roundtrip(const State&s){SaveData a{},b{};State copy{};CHECK(validate(s));CHECK(save(s,a));CHECK(a.bytes[4]==10);CHECK(load(copy,a.bytes,a.size));CHECK(save(copy,b));CHECK(a.size==b.size&&!memcmp(a.bytes,b.bytes,a.size));}
static void selection(State&s){CHECK(s.phase==Phase::DiscardChoice);CHECK(!end_turn(s));uint16_t ids[MaxHand];for(unsigned i=0;i<s.discard_required;i++)ids[i]=s.hand.ids[i];CHECK(choose_discard(s,ids,s.discard_required));CHECK(s.phase==Phase::Combat);roundtrip(s);}
static void starting(){
 State s{};new_run(s,7,false,Character::Silent);CHECK(s.hp==70&&s.max_hp==70&&s.deck_count==12&&s.gold==99);CHECK(s.relics==RingOfTheSnake&&s.relic_count==1);
 unsigned counts[CardKinds]={};for(unsigned i=0;i<s.deck_count;i++)++counts[s.deck[i].kind];CHECK(counts[StrikeSilent]==5&&counts[DefendSilent]==5&&counts[Neutralize]==1&&counts[Survivor]==1);
 CHECK(choose_node(s,0));CHECK(s.hand.count==7);CHECK(end_turn(s));CHECK(s.hand.count==5);roundtrip(s);
 new_run(s,7,true,Character::Silent);CHECK(s.hand.count==7&&s.deck_count==38);for(unsigned i=0;i<s.deck_count;i++)CHECK(card_in_pool(s.deck[i].kind,Character::Silent)||card_colorless(s.deck[i].kind));
 CHECK(character_pool(Character::Ironclad,nullptr)==29&&character_pool(Character::Silent,nullptr)==29);CHECK(card_generated(Shiv)&&!card_in_pool(Shiv,Character::Silent));
 for(unsigned seed=0;seed<100;seed++){
   new_run(s,seed,false,Character::Silent);s.current_node=1;s.next_row=1;s.phase=Phase::Map;CHECK(choose_node(s,1));
   for(unsigned i=0;i<5;i++)CHECK(card_in_pool(s.shop_cards[i],Character::Silent)&&card_rarity(s.shop_cards[i])>=2);
   for(unsigned i=5;i<7;i++)CHECK(card_colorless(s.shop_cards[i]));
   for(auto relic:s.shop_relics)CHECK(relic!=RingOfTheSnake&&relic!=BurningBlood);
   for(int k=0;k<30;k++){auto kind=random_reward_card(s);CHECK(card_in_pool(kind,Character::Silent)&&card_rarity(kind)>=2);}roundtrip(s);
 }
}
static void card_cases(){
 for(unsigned up=0;up<2;up++){
  uint8_t u=(uint8_t)up;auto s=fight({{StrikeSilent,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==40-3*(int)up);
  s=fight({{DefendSilent,u}});CHECK(play(s,0,0));CHECK(s.block==5+3*(int)up);roundtrip(s);
  s=fight({{Neutralize,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==43-(int)up&&s.enemies[0].weak==1+(int)up);CHECK(s.energy==3);
  s=fight({{Survivor,u},{StrikeSilent,0},{DefendSilent,0}});CHECK(play(s,0,0));CHECK(s.block==8+3*(int)up&&s.discard_required==1);selection(s);
  s=fight({{Acrobatics,u},{StrikeSilent,0}});supply(s,8);CHECK(play(s,0,0));CHECK(s.hand.count==4+up&&s.discard_required==1);roundtrip(s);selection(s);
  s=fight({{Backflip,u},{StrikeSilent,0}});supply(s,5);CHECK(play(s,0,0));CHECK(s.hand.count==3&&s.block==5+3*(int)up);
  s=fight({{DaggerThrow,u},{StrikeSilent,0}});supply(s,5);CHECK(play(s,0,0));CHECK(s.hand.count==2&&s.enemies[0].hp==37-3*(int)up);selection(s);
  s=fight({{DeadlyPoison,u}});s.enemies[0].block=20;CHECK(play(s,0,0));CHECK(s.enemies[0].poison==5+2*(int)up&&s.enemies[0].hp==46&&s.enemies[0].block==20);roundtrip(s);CHECK(end_turn(s));CHECK(s.enemies[0].hp==41-2*(int)up&&s.enemies[0].poison==4+2*(int)up);
  s=fight({{PoisonedStab,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==40-2*(int)up&&s.enemies[0].poison==3+(int)up);
  s=fight({{CloakAndDagger,u}});CHECK(play(s,0,0));CHECK(s.block==6&&count(s,Zone::Hand,Shiv)==1+up);roundtrip(s);
  s=fight({{BladeDance,u}});s.no_draw=1;CHECK(play(s,0,0));CHECK(count(s,Zone::Hand,Shiv)==3+up&&count(s,Zone::Exhaust,BladeDance)==1);roundtrip(s);
  s=fight({{DaggerSpray,u}},2);s.strength=1;s.enemies[1].vulnerable=1;CHECK(play(s,0,-1));CHECK(s.enemies[0].hp==46-2*(5+2*(int)up));CHECK(s.enemies[1].hp==46-2*((5+2*(int)up)*3/2));
  s=fight({{Dash,u}});CHECK(play(s,0,0));CHECK(s.block==10+3*(int)up&&s.enemies[0].hp==36-3*(int)up);
  s=fight({{LegSweep,u}});CHECK(play(s,0,0));CHECK(s.block==11+3*(int)up&&s.enemies[0].weak==2+(int)up);
  s=fight({{Footwork,u},{DefendSilent,0}});CHECK(play(s,0,0));CHECK(s.dexterity==2+(int)up&&s.powers.count==1);CHECK(play(s,0,0));CHECK(s.block==7+(int)up);
  s=fight({{NoxiousFumes,u}},2);CHECK(play(s,0,0));CHECK(s.noxious_fumes==2+(int)up&&s.enemies[0].poison==0);CHECK(end_turn(s));CHECK(s.enemies[0].poison==2+(int)up&&s.enemies[1].poison==2+(int)up);roundtrip(s);
  s=fight({{BouncingFlask,u}},2);auto copy=s;CHECK(play(s,0,-1));CHECK(play(copy,0,-1));CHECK(s.enemies[0].poison+s.enemies[1].poison==9+3*(int)up);CHECK(s.rng_cards==copy.rng_cards&&s.enemies[0].poison==copy.enemies[0].poison);
  s=fight({{Prepared,u},{StrikeSilent,0}});supply(s,8);CHECK(play(s,0,0));CHECK(s.hand.count==2+up&&s.discard_required==1+up);selection(s);
  s=fight({{Slice,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==40-3*(int)up&&s.energy==3);
  s=fight({{Deflect,u}});CHECK(play(s,0,0));CHECK(s.block==4+3*(int)up&&s.energy==3);
  s=fight({{SuckerPunch,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==38-2*(int)up&&s.enemies[0].weak==1+(int)up);
  s=fight({{Adrenaline,u}});supply(s,6);CHECK(play(s,0,0));CHECK(s.energy==4+(int)up&&s.hand.count==2&&s.exhaust.count==1);
  s=fight({{Accuracy,u},{Shiv,0}});CHECK(play(s,0,0));CHECK(s.accuracy==4+2*(int)up&&s.powers.count==1);CHECK(damage(s,s.cards[s.hand.ids[0]],0)==8+2*(int)up);CHECK(play(s,0,0));CHECK(s.enemies[0].hp==38-2*(int)up&&s.exhaust.count==1);roundtrip(s);
  s=fight({{LeadingStrike,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==43-3*(int)up&&count(s,Zone::Hand,Shiv)==2);
  s=fight({{Shiv,u}});CHECK(play(s,0,0));CHECK(s.enemies[0].hp==42-2*(int)up&&s.exhaust.count==1);roundtrip(s);
 }
}
static void boundaries(){
 auto s=fight({{Prepared,1},{Neutralize,0},{DefendSilent,0}});supply(s,6);CHECK(play(s,0,0));CHECK(s.discard_required==2);auto prior=s;
 uint16_t ids[2]={s.hand.ids[0],s.hand.ids[0]};CHECK(!choose_discard(s,ids,2));CHECK(!memcmp(&s,&prior,sizeof s));CHECK(!choose_discard(s,ids,1));CHECK(!choose_discard(s,nullptr,2));ids[1]=s.draw.ids[0];CHECK(!choose_discard(s,ids,2));CHECK(!memcmp(&s,&prior,sizeof s));
 SaveData saved{};CHECK(save(s,saved));State loaded{};CHECK(load(loaded,saved.bytes,saved.size()));CHECK(loaded.discard_required==2&&loaded.character==Character::Silent);ids[1]=s.hand.ids[1];CHECK(choose_discard(loaded,ids,2));CHECK(loaded.hand.count==2&&loaded.discard.count==3);roundtrip(loaded);
 s=fight({{Prepared,1}});s.no_draw=1;CHECK(play(s,0,0));CHECK(s.phase==Phase::Combat&&s.discard_required==0&&s.hand.count==0);roundtrip(s);
 s=fight({{BladeDance,1},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0},{StrikeSilent,0}});CHECK(play(s,0,0));CHECK(s.hand.count==10&&count(s,Zone::Hand,Shiv)==1&&count(s,Zone::Discard,Shiv)==3);roundtrip(s);
 s=fight({{Deflect,0}});s.hp=60;s.enemies[0].hp=5;s.enemies[0].poison=5;s.enemies[0].block=90;s.enemies[0].strength=500;CHECK(end_turn(s));CHECK(s.hp==60&&s.phase==Phase::Reward&&s.enemies[0].hp==0&&s.enemies[0].block==90);roundtrip(s);
 s=fight({{DaggerThrow,0},{Deflect,0}});s.enemies[0].hp=1;supply(s,3);CHECK(play(s,0,0));CHECK(s.phase==Phase::Reward&&s.discard_required==0);roundtrip(s);
 s=fight({{Shiv,0}});s.accuracy=4;CHECK(gain_relic(s,PenNib));s.relic_counters[9]=9;s.enemies[0].vulnerable=1;s.strength=2;CHECK(play(s,0,0));CHECK(s.enemies[0].hp==16&&relic_counter(s,PenNib)==0);
 s=fight({{DeadlyPoison,0}});s.noxious_fumes=3;s.accuracy=6;s.discard_required=0;enter_combat(s);CHECK(s.noxious_fumes==0&&s.accuracy==0&&s.enemies[0].poison==0&&s.hand.count==1);
}
static void old_v5(const char* path){
 FILE*f=fopen(path,"r");CHECK(f);std::vector<uint8_t>b;unsigned n;while(fscanf(f,"%2x",&n)==1)b.push_back((uint8_t)n);fclose(f);
 CHECK(b[4]==5);State s{};CHECK(load(s,b.data(),b.size()));CHECK(s.character==Character::Ironclad&&s.noxious_fumes==0&&s.discard_required==0);
 CHECK(s.relics&BronzeScales);bool found=false;for(unsigned i=0;i<s.log_count;i++)if(s.logs[i].kind==LogKind::Relic&&s.logs[i].a==BronzeScales)found=true;CHECK(found);roundtrip(s);
}
static void stress(){
 uint32_t rng=0x234ab124;
 for(unsigned seed=1;seed<=400;seed++){
  State s{};new_run(s,seed,true,Character::Silent);s.hp=s.max_hp=999;
  for(unsigned turn=0;turn<180&&combat_phase(s.phase);turn++){
   CHECK(validate(s));if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned i=0;i<s.discard_required;i++)ids[i]=s.hand.ids[i];CHECK(choose_discard(s,ids,s.discard_required));}
   else if(s.phase==Phase::ExhaustManyChoice){CHECK(choose_exhaust_many(s,nullptr,0));}
   else if(s.hand.count&&random_below(rng,5)){int target=s.enemy_count>1&&s.enemies[1].hp?1:0;if(!play(s,random_below(rng,s.hand.count),target))CHECK(end_turn(s));}
   else CHECK(end_turn(s));++transitions;roundtrip(s);
  }
 }
}
int main(int argc,char**argv){starting();card_cases();boundaries();if(argc>1)old_v5(argv[1]);stress();printf("Silent: %u assertions, %u generated transitions passed\n",checks,transitions);}
