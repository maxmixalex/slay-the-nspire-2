#include "combat_card_motion.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static State state;
static void hand(unsigned count){
 state.hand.count=uint16_t(count);state.card_count=uint16_t(count);state.seed=0x41cef1ea;
 state.draw.count=state.discard.count=state.exhaust.count=0;
 for(unsigned i=0;i<count;++i){state.hand.ids[i]=uint16_t(i);state.zones[i]=Zone::Hand;state.cards[i]={Strike,0};}
}
static CombatCardMotion::Cue transfer(unsigned id,Zone from,Zone to,bool fast=false,uint32_t at=0){
 CombatCardMotion::Cue cue;cue.id=uint16_t(id);cue.from=uint8_t(from);cue.to=uint8_t(to);cue.card={Strike,0};cue.fast=fast;cue.at=at;return cue;
}
static bool same_pose(HandPose a,HandPose b){return a.cx==b.cx&&a.cy==b.cy&&a.scale==b.scale&&a.sine==b.sine&&a.cosine==b.cosine;}
static void holder_responses(){
 hand(3);CombatCardMotion m;m.begin(state,0);
 const auto old=hand_pose(3,1,false);CombatHand::Cards next;next.count=2;next.ids[0]=1;next.ids[1]=2;
 m.apply(transfer(0,Zone::Hand,Zone::Discard));m.set_hand(next);const auto target=hand_pose(2,0,false);
 double x=old.cx,y=old.cy,scale=old.scale;
 for(unsigned tick=1;tick<=10;++tick){
  x+=(target.cx-x)*.033*7;y+=(target.cy-y)*.033*7;scale+=(target.scale-scale)*.033*8;
  m.advance(tick);const auto*v=m.find(1);CHECK(v&&v->mode==CombatCardMotion::Hand);
  CHECK(std::abs(v->x/1024.0-x)<.02&&std::abs(v->y/1024.0-y)<.3&&std::abs(v->scale-scale)<3.1);
 }
 const auto before=m.hand_pose_at(0,false);m.set_focus(1);const auto focused=m.hand_pose_at(0,true);
 CHECK(focused.cx==before.cx&&focused.cy==178&&focused.scale==1024&&focused.sine==0);
 m.advance(96);CHECK(m.settled(0,true)&&same_pose(m.hand_pose_at(0,true),hand_pose(2,0,true)));
 m.set_focus(-1);m.advance(192);CHECK(m.settled(0,false)&&same_pose(m.hand_pose_at(0,false),hand_pose(2,0,false)));
}
static void draw_and_play(){
 hand(0);CombatCardMotion m;m.begin(state,0);m.apply(transfer(0,Zone::Draw,Zone::Hand));
 CombatHand::Cards next;next.count=1;next.ids[0]=0;m.set_hand(next);
 const auto origin=m.hand_pose_at(0,false);CHECK(origin.cx==20&&origin.cy==226&&origin.scale==1024&&!m.settled(0,false));
 m.advance(1);const auto moved=m.hand_pose_at(0,false);CHECK(moved.cx>origin.cx&&moved.cx<160&&moved.cy<origin.cy);
 m.advance(96);CHECK(m.settled(0,false)&&same_pose(m.hand_pose_at(0,false),hand_pose(1,0,false)));
 for(unsigned fast=0;fast<2;++fast){
  hand(3);m.begin(state,0);m.set_focus(0);const auto start=m.hand_pose_at(0,true);
  m.apply(transfer(0,Zone::Hand,Zone::Limbo,fast!=0));
  for(unsigned tick=1;tick<=9;++tick){
   m.advance(tick);const auto*v=m.find(0);CHECK(v&&v->mode==CombatCardMotion::Play);
   const double t=std::fmin(1.0,tick*.033/(fast?.1:.25)),p=1-std::pow(1-t,3);
   const double st=std::fmin(1.0,tick*.033/.25),sp=1-std::pow(1-st,3);
   CHECK(std::abs(v->x/1024.0-(start.cx+(160-start.cx)*p))<.3);
   CHECK(std::abs(v->y/1024.0-(start.cy+(110-start.cy)*p))<.3);
   CHECK(std::abs(v->scale-(start.scale+(819-start.scale)*sp))<2.1);
  }
 }
}
static void discard_curve(){
 for(unsigned fast=0;fast<2;++fast){
  hand(1);CombatCardMotion m;m.begin(state,0);const auto start=m.hand_pose_at(0,false);
  m.counts.discard=1;m.apply(transfer(0,Zone::Hand,Zone::Discard,fast!=0));m.set_hand({});
  const auto initial=*m.find(0);CHECK(m.visible_counts().discard==0);
  CHECK(initial.speed>=72090&&initial.speed<=81920&&initial.accel>=131072&&initial.accel<=163840&&initial.duration>=65536&&initial.duration<=114689);
  double speed=initial.speed/65536.0,accel=initial.accel/65536.0,time=0,duration=initial.duration/65536.0;
  bool arrived=false;
  for(unsigned tick=1;tick<96;++tick){
   time+=speed*.033;speed+=accel*.033;m.advance(tick);const auto*v=m.find(0);CHECK(v);
   if(v->mode==CombatCardMotion::Tail){CHECK(time>=duration-.002&&m.visible_counts().discard==1);arrived=true;break;}
   CHECK(v->mode==CombatCardMotion::Fly&&m.visible_counts().discard==0);
   const double p=time/duration,x=start.cx+(302-start.cx)*p,y=start.cy+(226-start.cy)*p-2*initial.arc*p*(1-p);
   CHECK(std::abs(v->x/1024.0-x)<.35&&std::abs(v->y/1024.0-y)<.5);
   CHECK(std::abs(v->scale-start.scale*(1-.9*std::fmin(1.0,3*p)))<2);
   CHECK(std::abs(int(m.brightness(*v))-255*(1-std::fmin(1.0,3*p)))<2);
  }
  CHECK(arrived);m.advance(1000);CHECK(!m.find(0)&&m.visible_counts().discard==1);
 }
}
static void skipped_frames_and_bounds(){
 hand(10);CombatCardMotion serial,batched;serial.begin(state,0);
 for(unsigned id=0;id<10;++id)serial.apply(transfer(id,Zone::Hand,Zone::Discard));serial.set_hand({});
 for(unsigned id=10;id<20;++id)serial.apply(transfer(id,Zone::Draw,Zone::Hand));
 CombatHand::Cards incoming;incoming.count=10;for(unsigned i=0;i<10;++i)incoming.ids[i]=uint16_t(i+10);serial.set_hand(incoming);batched=serial;
 for(unsigned tick=1;tick<=70;++tick)serial.advance(tick);
 for(unsigned tick:{7u,15u,16u,31u,38u,70u})batched.advance(tick);
 CHECK(!std::memcmp(&serial,&batched,sizeof serial));
 for(unsigned i=0;i<10;++i)CHECK(serial.find(i+10)&&serial.find(i+10)->mode==CombatCardMotion::Hand);
 const auto original_state=state.hand;const auto original_seed=state.seed;
 for(unsigned i=0;i<=CombatCardMotion::Capacity;++i)serial.append(transfer(i,Zone::Draw,Zone::Hand,false,i));
 CHECK(serial.overflow&&serial.count==CombatCardMotion::Capacity&&state.seed==original_seed&&!std::memcmp(&state.hand,&original_state,sizeof original_state));
 // An unrelated unsupported transition removes an old visual rather than
 // continuing a discard flight after the card was returned or shuffled.
 hand(1);serial.begin(state,0);serial.apply(transfer(0,Zone::Hand,Zone::Discard));serial.set_hand({});serial.advance(2);
 serial.apply(transfer(0,Zone::Discard,Zone::Draw));CHECK(!serial.find(0));
}
int main(){
 CHECK(sizeof(CombatCardMotion)<=6*1024);holder_responses();draw_and_play();discard_curve();skipped_frames_and_bounds();
 std::printf("card movement: %u checks passed; %zu bytes of bounded metadata\n",checks,sizeof(CombatCardMotion));
}
