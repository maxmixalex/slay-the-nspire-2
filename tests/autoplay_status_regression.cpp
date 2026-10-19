#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static State fight(){
 State s{};new_run(s,719,false,Character::Defect);s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;s.enemy_count=1;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=10;s.enemies[0]={1000,1000,0,0,0,2};return s;
}
static int add(State&s,CardKind kind,Zone zone){
 int id=s.card_count++;s.cards[id]={kind,0};s.zones[id]=zone;
 Pile&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:s.discard;p.ids[p.count++]=(uint16_t)id;return id;
}
static void roundtrip(const State&s){
 SaveData a{},b{};CHECK(validate(s)&&save(s,a));State copy{};CHECK(load(copy,a.bytes,a.size));CHECK(save(copy,b));CHECK(a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size));
}
int main(){
 // v0.111.0 AutoPlay / MoveToResultPileWithoutPlaying: an unplayable status
 // moves piles, with exhaust hooks when applicable, and creates no CardPlay.
 for(CardKind kind:{Wound,Dazed,Void,Burn}){
  auto s=fight();add(s,Catastrophe,Zone::Hand);int id=add(s,kind,Zone::Draw);roundtrip(s);
  CHECK(play(s,0,0));CHECK(s.phase==Phase::Combat&&s.draw.count==0&&s.zones[id]==Zone::Discard);
  CHECK(s.expansion.cards_finished==1&&s.ironclad.attacks_started==0&&s.energy==8);roundtrip(s);
 }
 for(bool force:{false,true}){
  auto s=fight();int id=add(s,Wound,Zone::Draw);s.ironclad.forced_exhaust[id]=(uint8_t)force;
  s.pain=3;s.power_order[0]=0;s.power_count=1;s.potions[0]=Potion::DistilledChaos;
  roundtrip(s);CHECK(use_potion(s,0,0));CHECK(s.phase==Phase::Combat&&s.energy==10&&!s.draw.count);
  CHECK(s.zones[id]==(force?Zone::Exhaust:Zone::Discard));CHECK(s.block==(force?3:0));
  CHECK(!s.expansion.cards_finished&&!s.ironclad.attacks_started&&!s.ironclad.forced_exhaust[id]);roundtrip(s);
 }
 // Havoc uses the same command directly and installs ExhaustOnNextPlay first.
 auto s=fight();add(s,Havoc,Zone::Hand);int id=add(s,Wound,Zone::Draw);s.pain=3;s.power_order[0]=0;s.power_count=1;
 CHECK(play(s,0,0));CHECK(s.zones[id]==Zone::Exhaust&&s.block==3&&s.energy==9&&s.expansion.cards_finished==1);roundtrip(s);
 std::printf("automatic status result piles: %u checks passed\n",checks);
}
