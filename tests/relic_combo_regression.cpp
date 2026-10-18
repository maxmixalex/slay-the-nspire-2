#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
using namespace spire;
static unsigned checks=0,transitions=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static State fight(std::initializer_list<Card> cards,int enemies=1){
 State s{};new_run(s,91,false);s.phase=Phase::Combat;s.turn=s.floor=1;s.current_node=0;s.energy=9;s.enemy_count=(uint8_t)enemies;
 s.card_count=s.deck_count=0;for(Card c:cards){auto id=s.card_count++;s.cards[id]=s.deck[s.deck_count++]=c;s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=id;}
 for(int i=0;i<enemies;i++)s.enemies[i]={46,46,0,0,0,0};CHECK(validate(s));return s;
}
static void add(State&s,Zone zone,Card card={Defend,0}){auto id=s.card_count++;s.cards[id]=card;s.zones[id]=zone;auto&p=zone==Zone::Draw?s.draw:s.discard;p.ids[p.count++]=id;}
static SaveData snapshot(const State&s){SaveData b{};CHECK(save(s,b)&&b.bytes[4]==11);return b;}
static void same(const State&s,const SaveData&b){auto a=snapshot(s);CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));}
static void rt(const State&s){auto b=snapshot(s);State t{};CHECK(load(t,b.bytes,b.size));same(t,b);}
static void own(State&s){for(unsigned id:{Kunai,Shuriken,OrnamentalFan,LetterOpener,Nunchaku})CHECK(gain_relic(s,id));}
static void cards(){
 auto s=fight({{TwinStrike,0},{Slice,0},{Strike,0}});own(s);s.no_block=2;
 CHECK(play(s,0,0));CHECK(relic_counter(s,Kunai)==1&&s.enemies[0].hp==36&&s.strength==0);
 CHECK(play(s,0,0));CHECK(relic_counter(s,Shuriken)==2&&s.enemies[0].hp==30);
 CHECK(play(s,0,0));CHECK(s.enemies[0].hp==24&&s.strength==1&&s.dexterity==1&&s.block==4);
 CHECK(!relic_counter(s,Kunai)&&!relic_counter(s,Shuriken)&&relic_counter(s,Nunchaku)==3);rt(s);
 s=fight({{SwordBoomerang,1},{Whirlwind,0}});own(s);CHECK(play(s,0,-1));CHECK(relic_counter(s,Nunchaku)==1);
 s.energy=0;CHECK(play(s,0,-1));CHECK(relic_counter(s,Nunchaku)==2&&relic_counter(s,Kunai)==2);rt(s);
 CHECK(end_turn(s));CHECK(!relic_counter(s,Kunai)&&!relic_counter(s,LetterOpener)&&relic_counter(s,Nunchaku)==2);
 enter_combat(s);CHECK(relic_counter(s,Nunchaku)==2&&!relic_counter(s,Kunai));rt(s);
 s=fight({{Strike,0}});own(s);s.energy=0;auto old=snapshot(s);CHECK(!play(s,0,0));same(s,old);
 s=fight({{Headbutt,0},{Defend,0}});own(s);add(s,Zone::Discard);s.relic_counters[25]=s.relic_counters[26]=s.relic_counters[27]=2;s.relic_counters[29]=9;
 CHECK(play(s,0,0));CHECK(s.phase==Phase::ReturnChoice&&s.strength==0&&s.dexterity==0&&s.block==0&&s.energy==8);
 CHECK(relic_counter(s,Nunchaku)==9&&relic_counter(s,Kunai)==2);old=snapshot(s);State restored{};CHECK(load(restored,old.bytes,old.size));
 int id=s.discard.ids[0];CHECK(choose_return_to_draw(s,id));CHECK(choose_return_to_draw(restored,id));same(restored,snapshot(s));
 CHECK(s.strength==1&&s.dexterity==1&&s.block==4&&s.energy==9&&!relic_counter(s,Nunchaku));
 old=snapshot(s);CHECK(!choose_return_to_draw(s,id));same(s,old);rt(s);
 // Triggered fan block is unpowered: neither Dexterity nor No Block changes 4.
 s=fight({{Strike,0}});own(s);s.relic_counters[27]=2;s.dexterity=20;s.no_block=2;CHECK(play(s,0,0));CHECK(s.block==4);rt(s);
 s=fight({{Prepared,0},{Defend,0}},2);own(s);s.relic_counters[28]=2;s.strength=30;s.enemies[0].vulnerable=3;s.enemies[0].block=3;add(s,Zone::Draw);
 CHECK(play(s,0,0));CHECK(s.phase==Phase::DiscardChoice&&relic_counter(s,LetterOpener)==2&&s.enemies[0].hp==46);
 uint16_t chosen=s.hand.ids[0];CHECK(choose_discard(s,&chosen,1));CHECK(s.enemies[0].hp==44&&s.enemies[1].hp==41&&!relic_counter(s,LetterOpener));rt(s);
 s=fight({{Purity,0},{Defend,0}},2);own(s);s.relic_counters[28]=2;s.enemies[0].hp=s.enemies[1].hp=5;
 CHECK(play(s,0,0));CHECK(s.phase==Phase::ExhaustManyChoice);CHECK(choose_exhaust_many(s,nullptr,0));
 CHECK(s.phase==Phase::Reward&&s.exhaust.count==1&&s.playing==-1&&!s.effect_count);rt(s);
 s=fight({{Offering,0}});own(s);s.relic_counters[28]=2;s.hp=6;CHECK(play(s,0,0));CHECK(s.phase==Phase::Dead&&s.enemies[0].hp==46&&relic_counter(s,LetterOpener)==2);rt(s);
 // Powers and potions are not attacks or skills played.
 s=fight({{Inflame,0}});own(s);CHECK(play(s,0,0));s.potions[0]=Potion::Fire;CHECK(use_potion(s,0,0));
 CHECK(!relic_counter(s,LetterOpener)&&!relic_counter(s,Nunchaku));rt(s);
 // Saturation may not overflow the bounded stored stats.
 s=fight({{Whirlwind,0}});own(s);s.energy=0;s.strength=s.dexterity=s.block=9999;s.relic_counters[25]=s.relic_counters[26]=s.relic_counters[27]=2;
 CHECK(play(s,0,-1));CHECK(s.strength==9999&&s.dexterity==9999&&s.block==9999);rt(s);
}
static State chest(){State s{};new_run(s,124,false);s.phase=Phase::Treasure;s.current_node=19;s.next_row=6;s.floor=7;CHECK(validate(s));return s;}
static void treasure(){
 auto s=chest();auto closed=snapshot(s);CHECK(!take_treasure(s)&&!leave_treasure(s));same(s,closed);
 unsigned count=s.relic_count;uint32_t rng=s.rng_world;CHECK(open_treasure(s));CHECK(s.treasure_state==1&&s.treasure_relic&&s.relic_count==count&&s.rng_world!=rng);
 auto revealed=snapshot(s);CHECK(!open_treasure(s));same(s,revealed);rt(s);
 State t{};CHECK(load(t,revealed.bytes,revealed.size));CHECK(take_treasure(t));CHECK(t.phase==Phase::Treasure&&t.treasure_state==2&&t.relic_count==count+1&&t.rng_world==s.rng_world);
 auto claimed=snapshot(t);CHECK(!take_treasure(t)&&!open_treasure(t));same(t,claimed);rt(t);
 CHECK(leave_treasure(t));CHECK(t.phase==Phase::Map&&!t.treasure_state&&!t.treasure_relic&&t.relic_count==count+1);rt(t);
 CHECK(leave_treasure(s));CHECK(s.phase==Phase::Map&&s.relic_count==count&&!s.treasure_state);rt(s);
 s=chest();for(const auto&d:relic_definitions)gain_relic(s,d.id);CHECK(s.relic_count==30);int gold=s.gold;rng=s.rng_world;
 CHECK(open_treasure(s));CHECK(!s.treasure_relic&&s.rng_world==rng);CHECK(take_treasure(s));CHECK(s.gold==gold+50);rt(s);
 State bad=s;bad.phase=Phase::Map;CHECK(!validate(bad));bad=s;bad.treasure_state=3;CHECK(!validate(bad));bad=s;bad.treasure_relic=1u<<31;CHECK(!validate(bad));
 // Buying Old Coin spends first; hitting the gold cap must not eat the payment.
 s=chest();s.phase=Phase::Shop;s.gold=9900;s.shop_relics[0]=OldCoin;s.shop_prices[ShopRelicStart]=150;
 CHECK(buy(s,ShopRelicStart));CHECK(s.gold==9999);rt(s);
}
static void migration(const char*path){
 FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t>b;unsigned v;while(std::fscanf(f,"%2x",&v)==1)b.push_back((uint8_t)v);std::fclose(f);
 CHECK(b.size()>16&&b[4]==8);State s{};CHECK(load(s,b.data(),b.size()));CHECK(s.relic_count==25&&s.phase==Phase::ReturnChoice&&!s.treasure_state);
 for(unsigned i=25;i<RelicKinds;i++)CHECK(!s.relic_counters[i]);rt(s);CHECK(choose_return_to_draw(s,s.discard.ids[0]));rt(s);
 auto good=snapshot(s);for(size_t n=0;n<good.size;n+=4){State t=s;CHECK(!load(t,good.bytes,n));same(t,good);}
}
static void stress(){
 uint32_t rng=0x1212a;
 for(unsigned seed=1;seed<=160;seed++){
  State s{};new_run(s,seed,true,seed%2?Character::Silent:Character::Ironclad);own(s);s.hp=s.max_hp=999;
  for(int n=0;n<120&&combat_phase(s.phase);n++){
   if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
   else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
   else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
   else if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned j=0;j<s.discard_required;j++)ids[j]=s.hand.ids[j];CHECK(choose_discard(s,ids,s.discard_required));}
   else if(s.hand.count&&random_below(rng,5)){if(!play(s,random_below(rng,s.hand.count),0))CHECK(end_turn(s));}
   else CHECK(end_turn(s));++transitions;rt(s);
  }
 }
}
int main(int argc,char**argv){cards();treasure();if(argc>1)migration(argv[1]);stress();std::printf("relic combos: %u checks, %u transitions passed\n",checks,transitions);}
