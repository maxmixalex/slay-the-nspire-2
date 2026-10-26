#pragma once
#include "core.hpp"
#include "combat_card_motion.hpp"
#include "card_reward_presentation.hpp"
#include "ui_history_presentation.hpp"
#include "card_transform_presentation.hpp"

namespace spire {
enum class RewardPickupKind:uint8_t {None,Potion,Relic,Card};
struct RewardAcquisitionSnapshot {
 uint32_t serial=0,seed=0,run=0;
 uint16_t deck_count=0,relic_count=0;
 int16_t x=0,y=0;
 int16_t potion_x=0,potion_y=0,relic_x=0,relic_y=0;
 int gold=0;
 Potion potions[MaxPotionSlots]{};
 uint8_t context=0; // visible reward row or card selection, never a gameplay RNG probe
};
struct RewardWindowVisual {int y=0;unsigned alpha=255;};
struct RewardPickupVisual {int x=0,y=0,scale=1024,angle=0;unsigned alpha=255,shade=255;bool visible=false;};
struct RewardPresentation {
 CardRewardPresentation cards{};
 UiHistoryScroll list_scroll{};
 uint32_t list_key=0;
 uint16_t list_count=0;
 int16_t list_focus=-1;
 uint8_t list_mode=0;
 static int row_offset(unsigned index){return int(index)*28;}
 void configure_list(uint8_t mode,uint32_t key,unsigned count,int focused,uint32_t now){
  (void)now;if(!mode)return;
  if(mode!=list_mode||key!=list_key){list_scroll.reset();list_focus=-1;list_key=key;list_mode=mode;}
  list_count=uint16_t(count);list_scroll.configure(count?row_offset(count-1)+24:0,128);
  if(focused>=0&&unsigned(focused)<count&&list_focus!=focused){
   list_focus=int16_t(focused);
   list_scroll.target=UiHistoryScroll::clamp((row_offset(unsigned(focused))-52)*256,0,list_scroll.maximum);
  }
 }
 void scroll_list(int rows){list_scroll.target=UiHistoryScroll::clamp(list_scroll.target+UiHistoryScroll::clamp(rows,-64,64)*160*256/21,-128*256,list_scroll.maximum+128*256);}
 int list_first()const{const int above=list_scroll.offset()-29;return above>0?above/28:0;}
 static constexpr unsigned MaxFlights=12;
 struct Flight {
  RewardPickupKind kind=RewardPickupKind::None;
  uint16_t model=0,slot=0;
  int16_t x=0,y=0;
  uint16_t speed=1100,accel=2000,duration=1000,arrival=0;
  uint32_t start=0;
  Card card{};
  EventDeckSnapshot snapshot{};
  bool has_snapshot=false;
 };
 Flight flights[MaxFlights]{};
 struct RemovedCard {
  Card card{};EventCardFacts facts{};
  uint16_t genetic=0,scythe=0;
  int16_t energy=0,stars=0;
  uint32_t start=0;
  bool active=false;
  char description[1024]{};
  bool alive(uint32_t now)const{return active&&uint32_t(now-start)<1700;}
  unsigned scale(uint32_t now)const{
   const uint32_t age=now-start;if(age>=250)return 4096;
   const unsigned inverse=4096-age*4096/250;
   return 4096-unsigned(uint64_t(inverse)*inverse*inverse/16777216);
  }
 } removed{};
 CardTransformPresentation transforms{};
 // NCardUpgradeVfx displays only cards whose upgrade actually committed.
 // The bounded four-card fan covers Smith/Gnarled Hammer without a State
 // copy; model facts stay attached even if a later deck operation reorders it.
 struct Upgrades {
  struct CardView {Card card{};EventCardFacts facts{};int16_t deck=-1;};
  static constexpr unsigned Capacity=4;
  CardView cards[Capacity]{};uint32_t start=0;uint8_t count=0;
  bool alive(uint32_t now)const{return count&&uint32_t(now-start)<2250u+(count-1u)*100u;}
  void add(Card card,const EventCardFacts&facts,int deck,uint32_t now){
   if(!alive(now)||uint32_t(now-start)>400u){count=0;start=now;}if(count<Capacity)cards[count++]={card,facts,int16_t(deck)};
  }
 } upgrades{};
 uint32_t serial=0,window_start=0,window_key=0,empty_start=0;
 unsigned window_from=0;
 uint8_t window_mode=0;
 bool empty=false,gold_bound=false;
 int gold_from=0,gold_to=0;
 uint32_t gold_start=0;
 bool gold_active=false;
 void reset(){*this=RewardPresentation{};}
 static int back_out(unsigned q){
  if(q>=4096)return 4096;
  const int u=int(q)-4096;
  return 4096+(u*u/4096)*((11066*u/4096)+6970)/4096;
 }
 RewardWindowVisual window(uint32_t now)const{
  if(!window_mode)return {0,0};
  const uint32_t elapsed=now-window_start;
  const unsigned q=elapsed>=500?4096:elapsed*4096/500;
  RewardWindowVisual v{(20*(4096-back_out(q))+2048)/4096,elapsed>=500?255:elapsed*255/500};
  if(empty){const uint32_t age=now-empty_start;v.alpha=age>=250?0:window_from*(250-age)/250;}
  return v;
 }
 void observe_window(uint8_t mode,uint32_t key,unsigned count,uint32_t now){
  if(!mode){window_mode=0;return;}
  if(mode!=window_mode||key!=window_key){window_mode=mode;window_key=key;window_start=now;empty=count==0;empty_start=now;window_from=0;}
  else if(!count&&!empty){window_from=window(now).alpha;empty_start=now;empty=true;}
 }
 static unsigned quad_out(unsigned q){const unsigned inverse=4096-q;return 4096-inverse*inverse/4096;}
 static unsigned sine_out(unsigned q){return unsigned(CombatCardMotion::sine(int(q)/4))*4;}
 static unsigned card_progress(const Flight&f,uint32_t elapsed){
  const uint64_t time=uint64_t(f.speed)*elapsed/1000+uint64_t(f.accel)*elapsed*elapsed/2000000;
  return unsigned(time*4096/f.duration);
 }
 static uint32_t card_arrival(const Flight&f){
  // One bounded integer search per new flight avoids floating-point roots.
  uint32_t lo=0,hi=1600;
  for(unsigned i=0;i<11;++i){const uint32_t mid=(lo+hi)/2;if(card_progress(f,mid)<4096)lo=mid+1;else hi=mid;}
  return hi;
 }
 static bool alive(const Flight&f,uint32_t now){
  if(f.kind==RewardPickupKind::None)return false;
  const uint32_t age=now-f.start;
  return f.kind==RewardPickupKind::Card?age<f.arrival+uint32_t(f.duration)*2/5:age<350;
 }
 static bool retained(const Flight&f,uint32_t now){
  // Arrival unhides the real inventory icon immediately; its source flash
  // remains a presentation-only tail, not an extended acquisition flight.
  if(f.kind==RewardPickupKind::Potion)return uint32_t(now-f.start)<1350;
  if(f.kind==RewardPickupKind::Relic)return uint32_t(now-f.start)<2260;
  return alive(f,now);
 }
 void append(RewardPickupKind kind,unsigned model,unsigned slot,int x,int y,uint32_t now,Card card={},const EventDeckSnapshot*snapshot=nullptr){
  Flight*target=nullptr;
  for(auto&f:flights)if(!retained(f,now)){target=&f;break;}
  if(!target){target=&flights[0];for(auto&f:flights)if(uint32_t(now-f.start)>uint32_t(now-target->start))target=&f;}
  // Original NCardFlyVfx uses Chaotic presentation randomness. A private hash
  // supplies its bounded parameters without reading or advancing game RNG.
  uint32_t hash=(now^serial*0x9e3779b9u^model*0x85ebca6bu)+0x165667b1u;hash^=hash>>15;hash*=0x2c1b3c6du;hash^=hash>>12;
  *target=Flight{kind,uint16_t(model),uint16_t(slot),int16_t(x),int16_t(y),uint16_t(1100+hash%151),
                 uint16_t(2000+(hash>>8)%501),uint16_t(1000+(hash>>16)%751),0,now,card};
  if(kind==RewardPickupKind::Card)target->arrival=uint16_t(card_arrival(*target));
  if(snapshot){target->snapshot=*snapshot;target->has_snapshot=true;}
 }
 bool hidden(RewardPickupKind kind,unsigned slot,uint32_t now)const{
  for(const auto&f:flights)if(f.kind==kind&&f.slot==slot&&alive(f,now))return true;
  return false;
 }
 RewardPickupVisual sample(const Flight&f,int tx,int ty,uint32_t now)const{
  RewardPickupVisual v;v.visible=alive(f,now);if(!v.visible)return v;
  const uint32_t age=now-f.start;
  if(f.kind!=RewardPickupKind::Card){
   const unsigned q=age*4096/350,ease=f.kind==RewardPickupKind::Potion?quad_out(q):sine_out(q);
   v.x=f.x+(tx-f.x)*int(ease)/4096;v.y=f.y+(ty-f.y)*int(ease)/4096;
   return v;
  }
  const unsigned q=card_progress(f,age);
  if(q>=4096){const uint32_t after=age-f.arrival;v.x=tx;v.y=ty;v.scale=102-int(after*256/f.duration);v.shade=0;return v;}
  // The original top-pile curve uses a control point 500 source pixels
  // below the midpoint. Normalize that camera distance to the 218px room.
  const int cy=(f.y+ty)/2+101,cx=(f.x+tx)/2;const int inverse=4096-int(q);
  v.x=int((int64_t(inverse)*inverse*f.x+2ll*inverse*q*cx+int64_t(q)*q*tx)/16777216);
  v.y=int((int64_t(inverse)*inverse*f.y+2ll*inverse*q*cy+int64_t(q)*q*ty)/16777216);
  const int dx=inverse*(cx-f.x)+int(q)*(tx-cx),dy=inverse*(cy-f.y)+int(q)*(ty-cy);
  v.angle=CombatCardMotion::direction(dy,dx)+1024;
  const unsigned shrink=q*3>4096?4096:q*3;v.scale=1024-int(shrink*922/4096);v.shade=255-shrink*255/4096;
  return v;
 }
 void begin_gold(int before,int after,uint32_t now){gold_bound=true;gold_from=before;gold_to=after;gold_start=now;gold_active=before!=after;}
 // NTopBarGold waits 250ms + 150ms, then consumes the remaining delta in
 // 75/10/1 steps. Its final <10 sequence uses the authored 10..110ms delays.
 int remaining_gold(uint32_t now,uint32_t*done=nullptr)const{
  if(!gold_active){if(done)*done=0;return 0;}
  uint32_t at=400;const int delta=gold_to-gold_from,sign=delta<0?-1:1;unsigned remaining=unsigned(delta<0?-delta:delta);
  const uint32_t elapsed=now-gold_start;
  // Skip the regular 75/10 phases arithmetically; at most fifty final
  // authored one-unit steps remain, independent of the reward magnitude.
  const auto consume=[&](unsigned threshold,unsigned step){if(remaining<=threshold||elapsed<at)return;
   unsigned count=(remaining-threshold+step-1)/step,ready=(elapsed-at)/10+1;if(count>ready)count=ready;
   remaining-=count*step;at+=count*10;};
  consume(100,75);consume(50,10);
  while(remaining<=50&&remaining&&elapsed>=at){--remaining;at+=10+(remaining<10?10-remaining:0)*10;}
  if(done)*done=at;return int(remaining)*sign;
 }
 int displayed_gold(uint32_t now)const{return gold_to-remaining_gold(now);}
};
}
