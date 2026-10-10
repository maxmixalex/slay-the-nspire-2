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
 State s{};new_run(s,11,false);s.phase=Phase::Combat;s.floor=1;s.turn=1;s.enemy_count=(uint8_t)enemies;
 s.energy=9;s.deck_count=s.card_count=0;s.current_node=1;
 for(Card card:cards){unsigned id=s.card_count++;s.cards[id]=card;s.deck[s.deck_count++]=card;s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=(uint16_t)id;}
 for(int i=0;i<enemies;i++)s.enemies[i]={46,46,0,0,0,0};CHECK(validate(s));return s;
}
static void supply(State&s,unsigned count){for(unsigned i=0;i<count;i++){unsigned id=s.card_count++;s.cards[id]={Defend,0};s.zones[id]=Zone::Draw;s.draw.ids[s.draw.count++]=(uint16_t)id;}CHECK(validate(s));}
static int find(const State&s,unsigned kind){for(int i=0;i<s.hand.count;i++)if(s.cards[s.hand.ids[i]].kind==kind)return i;return -1;}
static void rt(const State&s){SaveData a{},b{};State copy{};CHECK(validate(s));CHECK(save(s,a));CHECK(a.bytes[4]==8);CHECK(load(copy,a.bytes,a.size));CHECK(save(copy,b));CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));}
static void unchanged(const State&a,const State&b){SaveData x{},y{};CHECK(save(a,x));CHECK(save(b,y));CHECK(x.size==y.size&&!std::memcmp(x.bytes,y.bytes,x.size));}
static void cards(){
 for(unsigned up=0;up<2;up++){
  uint8_t u=(uint8_t)up;auto s=fight({{Impatience,u},{Defend,0}});supply(s,20);CHECK(play(s,0,0));CHECK(s.hand.count==3+up);rt(s);
  s=fight({{Impatience,u},{Strike,0}});supply(s,20);CHECK(play(s,0,0));CHECK(s.hand.count==1&&s.draw.count==20);rt(s);
  s=fight({{Impatience,u},{Defend,0}});supply(s,20);s.no_draw=1;CHECK(play(s,0,0));CHECK(s.hand.count==1&&s.draw.count==20);
  s=fight({{MasterOfStrategy,u}});supply(s,20);CHECK(play(s,0,0));CHECK(s.hand.count==3+up&&s.exhaust.count==1&&s.energy==9);rt(s);
  s=fight({{Shockwave,u}},2);s.enemies[1].vulnerable=2;CHECK(play(s,0,-1));CHECK(s.enemies[0].hp==46&&s.enemies[0].weak==3+2*up&&s.enemies[1].weak==3+2*up&&s.enemies[1].vulnerable==5+2*up&&s.exhaust.count==1);rt(s);
  s=fight({{Scrawl,u},{Strike,0}});supply(s,20);CHECK(cost(s.cards[0])==1);CHECK(play(s,0,0));CHECK(s.hand.count==10&&s.exhaust.count==1&&s.energy==8);rt(s);
  s=fight({{Scrawl,u}});supply(s,20);s.no_draw=1;CHECK(play(s,0,0));CHECK(s.hand.count==0&&s.exhaust.count==1);rt(s);
  s=fight({{Purity,u},{Strike,0},{Defend,0},{Bash,0},{Inflame,0},{Defend,1}});supply(s,20);
  s.pain=3;s.embrace=1;s.power_count=2;s.power_order[0]=0;s.power_order[1]=1;
  CHECK(play(s,0,0));CHECK(s.phase==Phase::ExhaustManyChoice&&choice_min(s)==0&&choice_max(s)==3+2*up);rt(s);
  State before=s;uint16_t ids[2]={s.hand.ids[4],s.hand.ids[1]},dupe[2]={ids[0],ids[0]};CHECK(!choose_exhaust_many(s,dupe,2));unchanged(s,before);
  CHECK(!choose_exhaust_many(s,nullptr,2));unchanged(s,before);
  CHECK(choose_exhaust_many(s,ids,2));CHECK(s.phase==Phase::Combat&&s.hand.count==6&&s.exhaust.count==3&&s.block==9);
  CHECK(s.zones[ids[0]]==Zone::Exhaust&&s.zones[ids[1]]==Zone::Exhaust&&s.zones[0]==Zone::Exhaust);rt(s);
  s=before;CHECK(choose_exhaust_many(s,nullptr,0));CHECK(s.exhaust.count==1&&s.hand.count==6&&s.block==3);rt(s);
  s=fight({{Purity,u}});CHECK(play(s,0,0));CHECK(s.phase==Phase::Combat&&s.exhaust.count==1);rt(s);
  s=fight({{HandOfGreed,u}});s.enemies[0].hp=(int16_t)(20+5*up);int gold=s.gold;CHECK(play(s,0,0));CHECK(s.phase==Phase::Reward&&s.gold==gold+20+5*up);rt(s);
  s=fight({{HandOfGreed,u}},2);s.enemies[0].hp=20;s.enemies[0].block=20;gold=s.gold;CHECK(play(s,0,0));CHECK(s.gold==gold&&s.enemies[0].hp>0);rt(s);
  s=fight({{PanicButton,u},{Defend,0}});supply(s,20);CHECK(play(s,0,0));CHECK(s.block==30+10*up&&s.no_block==2);CHECK(card_block(s,{Defend,0})==0);CHECK(play(s,0,0));CHECK(s.block==30+10*up);
  CHECK(end_turn(s));CHECK(s.no_block==1);CHECK(play(s,0,0));CHECK(s.block==0);CHECK(end_turn(s));CHECK(s.no_block==0&&card_block(s,{Defend,0})==5);rt(s);
 }
 auto s=fight({{Purity,0},{Scrawl,1},{Scrawl,0},{Defend,0}});supply(s,20);CHECK(card_retains(s.cards[0])&&card_retains(s.cards[1])&&!card_retains(s.cards[2]));
 CHECK(end_turn(s));CHECK(s.zones[0]==Zone::Hand&&s.zones[1]==Zone::Hand&&s.zones[2]==Zone::Discard&&s.hand.count==7);rt(s);
 s=fight({{PanicButton,0},{Impervious,0}});s.pain=3;s.power_count=1;s.power_order[0]=0;
 CHECK(play(s,0,0));CHECK(s.block==33);CHECK(play(s,0,0));CHECK(s.block==36);s.potions[0]=Potion::Block;CHECK(use_potion(s,0,0));CHECK(s.block==48);rt(s);
 for(unsigned i=Impatience;i<=PanicButton;i++){CHECK(card_colorless(i)&&card_color(i)==CardColor::Colorless);CHECK(!card_in_pool(i,Character::Ironclad)&&!card_in_pool(i,Character::Silent));}
}
static void relics(){
 auto s=fight({{Defend,0}});CHECK(gain_relic(s,HornCleat));CHECK(gain_relic(s,CaptainsWheel));s.no_block=4;
 CHECK(end_turn(s));CHECK(s.turn==2&&s.block==14);CHECK(end_turn(s));CHECK(s.turn==3&&s.block==18);CHECK(end_turn(s));CHECK(s.turn==4&&s.block==0);rt(s);
 s=fight({{Strike,0},{Defend,0}});CHECK(gain_relic(s,ArtOfWar));s.energy=3;CHECK(s.energy==3);CHECK(play(s,0,0));CHECK(s.attacked_this_turn);rt(s);CHECK(end_turn(s));CHECK(s.energy==3&&!s.attacked_this_turn);CHECK(end_turn(s));CHECK(s.energy==4);rt(s);
 s=fight({{Whirlwind,0}});s.energy=0;CHECK(gain_relic(s,ArtOfWar));CHECK(play(s,0,-1));CHECK(s.attacked_this_turn);CHECK(end_turn(s));CHECK(s.energy==3);rt(s);
 s=fight({{Defend,0}},2);CHECK(gain_relic(s,MercuryHourglass));enter_combat(s);for(int i=0;i<s.enemy_count;i++)CHECK(s.enemies[i].hp==s.enemies[i].max_hp-3);
 s.enemies[0].hp=3;s.enemies[1].hp=0;CHECK(end_turn(s));CHECK(s.phase==Phase::Reward);rt(s);
 s=fight({{Defend,0}});CHECK(gain_relic(s,RegalPillow));CHECK(rest_heal_amount(s)==39);s.hp=10;s.phase=Phase::Camp;CHECK(camp(s,false));CHECK(s.hp==49);rt(s);
 s=fight({{Defend,0}});CHECK(gain_relic(s,MealTicket));s.hp=10;s.phase=Phase::Map;s.current_node=1;s.next_row=1;CHECK(choose_node(s,1));CHECK(s.phase==Phase::Shop&&s.hp==25);rt(s);
 SaveData b{};CHECK(save(s,b));State copy{};CHECK(load(copy,b.bytes,b.size));CHECK(copy.hp==25);rt(copy);
 s=fight({{Defend,0}});int gold=s.gold;CHECK(gain_relic(s,OldCoin));CHECK(s.gold==gold+300);CHECK(!gain_relic(s,OldCoin));CHECK(s.gold==gold+300);rt(s);
 s=fight({{Bloodletting,0},{Bloodletting,0}});CHECK(gain_relic(s,CentennialPuzzle));supply(s,20);CHECK(play(s,0,0));CHECK(s.hand.count==4&&relic_counter(s,CentennialPuzzle)==1);rt(s);CHECK(play(s,find(s,Bloodletting),0));CHECK(s.hand.count==3);rt(s);
 s=fight({{Defend,0}});CHECK(gain_relic(s,CentennialPuzzle));supply(s,20);s.block=99;CHECK(end_turn(s));CHECK(!relic_counter(s,CentennialPuzzle));CHECK(end_turn(s));CHECK(relic_counter(s,CentennialPuzzle)==1&&s.hand.count==8);rt(s);
 s=fight({{Bloodletting,0}});CHECK(gain_relic(s,CentennialPuzzle));supply(s,20);s.no_draw=1;CHECK(play(s,0,0));CHECK(s.hand.count==0&&relic_counter(s,CentennialPuzzle)==1);rt(s);
 s=fight({{Bloodletting,0}});CHECK(gain_relic(s,CentennialPuzzle));supply(s,20);s.hp=3;CHECK(play(s,0,0));CHECK(s.phase==Phase::Dead&&s.hand.count==0&&s.draw.count==20);rt(s);
 new_run(s,7,true);for(const auto&d:relic_definitions)gain_relic(s,d.id);CHECK(s.relic_count==25&&s.relics==(1u<<25)-1);rt(s);enter_combat(s);CHECK(relic_counter(s,CentennialPuzzle)==0);rt(s);
}
static void migration(const char*path){
 FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t>b;unsigned v;while(std::fscanf(f,"%2x",&v)==1)b.push_back((uint8_t)v);std::fclose(f);
 CHECK(b[4]==6);State s{};CHECK(load(s,b.data(),b.size()));CHECK(s.character==Character::Silent&&s.phase==Phase::DiscardChoice&&s.discard_required==1);CHECK(!s.no_block&&!s.exhaust_limit&&!s.attacked_this_turn);rt(s);
 uint16_t id=s.hand.ids[0];CHECK(choose_discard(s,&id,1));CHECK(s.phase==Phase::Combat);rt(s);
}
static void stress(){
 uint32_t r=0x75adba1u;
 for(unsigned seed=1;seed<=300;seed++){
  State s{};new_run(s,seed,true,seed%2?Character::Silent:Character::Ironclad);s.max_hp=s.hp=999;
  for(int i=17;i<RelicKinds;i++)gain_relic(s,relic_definitions[i].id);enter_combat(s);
  for(unsigned n=0;n<180&&combat_phase(s.phase);n++){
   if(s.phase==Phase::ExhaustManyChoice){uint16_t ids[MaxHand];unsigned count=random_below(r,s.exhaust_limit+1);for(unsigned j=0;j<count;j++)ids[j]=s.hand.ids[j];CHECK(choose_exhaust_many(s,ids,count));}
   else if(s.phase==Phase::ExhaustChoice){CHECK(choose_exhaust(s,0));}
   else if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned j=0;j<s.discard_required;j++)ids[j]=s.hand.ids[j];CHECK(choose_discard(s,ids,s.discard_required));}
   else if(s.phase==Phase::ReturnChoice){CHECK(choose_return_to_draw(s,s.discard.ids[0]));}
   else if(s.hand.count&&random_below(r,5)){if(!play(s,random_below(r,s.hand.count),0))CHECK(end_turn(s));}
   else CHECK(end_turn(s));
   ++transitions;rt(s);
  }
 }
}
int main(int argc,char**argv){cards();relics();if(argc>1)migration(argv[1]);stress();std::printf("shared: %u checks, %u controlled transitions passed\n",checks,transitions);}
