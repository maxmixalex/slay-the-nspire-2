#include "combat_card_motion.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <tuple>
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
static bool same_motion(const CombatCardMotion&a,const CombatCardMotion&b){
 // Compare every named field, including inactive queue/sprite storage. Clang
 // leaves the three bytes after selected_centered unspecified on assignment;
 // object-representation equality is therefore not a motion invariant.
 const auto counts=[](CombatCardMotion::Counts x,CombatCardMotion::Counts y){return x.draw==y.draw&&x.discard==y.discard&&x.exhaust==y.exhaust;};
 for(unsigned i=0;i<CombatCardMotion::Visible;++i){const auto&x=a.sprites[i];const auto&y=b.sprites[i];
  for(unsigned j=0;j<CombatCardMotion::Sprite::TrailPoints;++j)if(x.trail[j].x!=y.trail[j].x||x.trail[j].y!=y.trail[j].y)return false;
  if(!same_pose(x.origin,y.origin)||
     std::tie(x.x,x.y,x.at,x.progress,x.speed,x.duration,x.accel,x.card.kind,x.card.upgraded,x.id,x.scale,x.angle,x.energy,x.stars,x.arc,x.target_x,x.target_y,x.mode,x.to,x.trail_count,x.fast,x.generated)!=
     std::tie(y.x,y.y,y.at,y.progress,y.speed,y.duration,y.accel,y.card.kind,y.card.upgraded,y.id,y.scale,y.angle,y.energy,y.stars,y.arc,y.target_x,y.target_y,y.mode,y.to,y.trail_count,y.fast,y.generated))return false;
 }
 for(unsigned i=0;i<CombatCardMotion::Capacity;++i){const auto&x=a.cues[i];const auto&y=b.cues[i];
  if(std::tie(x.at,x.id,x.from,x.to,x.card.kind,x.card.upgraded,x.energy,x.stars,x.target_x,x.target_y,x.fast,x.preview)!=
     std::tie(y.at,y.id,y.from,y.to,y.card.kind,y.card.upgraded,y.energy,y.stars,y.target_x,y.target_y,y.fast,y.preview))return false;
 }
 for(unsigned i=0;i<CombatCardMotion::CountCapacity;++i)if(a.count_cues[i].at!=b.count_cues[i].at||!counts(a.count_cues[i].value,b.count_cues[i].value))return false;
 if(a.hand.count!=b.hand.count||std::memcmp(a.hand.ids,b.hand.ids,sizeof a.hand.ids)||
    !counts(a.counts,b.counts)||!counts(a.observed_counts,b.observed_counts)||std::memcmp(a.zones,b.zones,sizeof a.zones))return false;
 return std::tie(a.seed,a.clock,a.cosmetic,a.manual_started,a.manual_min_end,a.wait_at,a.floor,a.combat,a.encounter,a.focus,a.selected,a.manual,a.manual_energy,a.manual_stars,
                 a.selected_x,a.selected_y,a.selected_scale,a.card_count,a.head,a.count,a.count_head,a.count_count,a.wait_excess,a.active,a.overflow,a.count_overflow,a.selected_centered)==
        std::tie(b.seed,b.clock,b.cosmetic,b.manual_started,b.manual_min_end,b.wait_at,b.floor,b.combat,b.encounter,b.focus,b.selected,b.manual,b.manual_energy,b.manual_stars,
                 b.selected_x,b.selected_y,b.selected_scale,b.card_count,b.head,b.count,b.count_head,b.count_count,b.wait_excess,b.active,b.overflow,b.count_overflow,b.selected_centered);
}
static void body_modulation(){
 // Colors.White.Lerp(Colors.Black,t) must not introduce another channel.
 // In particular packed red/blue division once produced blue=28 for red.
 CHECK(CombatCardMotion::modulate(0xf800,128)==0x7800);
 CHECK(CombatCardMotion::modulate(0x07e0,128)==0x03e0);
 CHECK(CombatCardMotion::modulate(0x001f,128)==0x000f);
 CHECK(CombatCardMotion::modulate(0xffff,128)==0x7bef);
 CHECK(CombatCardMotion::modulate(0xffd5,128)==0x7bea);
 for(unsigned level=0;level<256;++level){
  CHECK(!(CombatCardMotion::modulate(0xf800,level)&0x07ff));
  CHECK(!(CombatCardMotion::modulate(0x07e0,level)&0xf81f));
  CHECK(!(CombatCardMotion::modulate(0x001f,level)&0xffe0));
 }
 for(unsigned pixel:{0u,0xf800u,0x07e0u,0x001fu,0xffffu,0xffd5u}){
  CHECK(CombatCardMotion::modulate(uint16_t(pixel),255)==pixel);
  CHECK(CombatCardMotion::modulate(uint16_t(pixel),0)==0);
 }
}
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
 CHECK(focused.cx==before.cx&&focused.cy==174&&focused.scale==1024&&focused.sine==0);
 m.advance(96);CHECK(m.settled(0,true)&&same_pose(m.hand_pose_at(0,true),hand_pose(2,0,true)));
 m.set_focus(-1);m.advance(192);CHECK(m.settled(0,false)&&same_pose(m.hand_pose_at(0,false),hand_pose(2,0,false)));
}
static void hover_and_selection_layout(){
 hand(5);const auto saved=state;CombatCardMotion m;m.begin(state,0);m.set_focus(2);m.advance(96);
 const int hovered[]={101,123,160,197,219};for(unsigned i=0;i<5;++i)CHECK(m.hand_pose_at(i,i==2).cx==hovered[i]);
 const auto before=m.hand_pose_at(0,false);m.set_selected(2);CHECK(m.hand_pose_at(0,false).cx==before.cx);
 m.advance(192);const int contracted[]={124,148,160,172,196};
 for(unsigned i=0;i<5;++i)CHECK(m.hand_pose_at(i,false).cx==contracted[i]);
 CHECK(m.hand_pose_at(2,false).cy==174&&m.hand_pose_at(2,false).scale==1024);
 m.set_selection_pose({170,90,1024,0,1024});m.advance(288);CHECK(m.hand_pose_at(2,false).cx==170&&m.hand_pose_at(2,false).cy==90);
 m.set_selected(-1);m.set_focus(-1);m.advance(384);
 for(unsigned i=0;i<5;++i)CHECK(same_pose(m.hand_pose_at(i,false),hand_pose(5,int(i),false)));
 CHECK(!std::memcmp(&saved,&state,sizeof state));
}
static void draw_and_play(){
 hand(0);CombatCardMotion m;m.begin(state,0);m.apply(transfer(0,Zone::Draw,Zone::Hand));
 CombatHand::Cards next;next.count=1;next.ids[0]=0;m.set_hand(next);
 const auto origin=m.hand_pose_at(0,false);CHECK(origin.cx==16&&origin.cy==225&&origin.scale==1024&&!m.settled(0,false));
 m.advance(1);const auto moved=m.hand_pose_at(0,false);CHECK(moved.cx>origin.cx&&moved.cx<160&&moved.cy<origin.cy&&moved.cy>218);
 m.advance(96);CHECK(m.settled(0,false)&&same_pose(m.hand_pose_at(0,false),hand_pose(1,0,false)));
 for(unsigned fast=0;fast<2;++fast){
  hand(3);m.begin(state,0);m.set_focus(0);const auto start=m.hand_pose_at(0,true);
  m.apply(transfer(0,Zone::Hand,Zone::Limbo,fast!=0));
  for(unsigned tick=1;tick<=9;++tick){
   m.advance(tick);const auto*v=m.find(0);CHECK(v&&v->mode==CombatCardMotion::Play);
   const double t=std::fmin(1.0,tick*.033/(fast?.1:.25)),p=1-std::pow(1-t,3);
   const double st=std::fmin(1.0,tick*.033/.25),sp=1-std::pow(1-st,3);
   CHECK(std::abs(v->x/1024.0-(start.cx+(160-start.cx)*p))<.3);
   CHECK(std::abs(v->y/1024.0-(start.cy+(101-start.cy)*p))<.3);
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
   const double p=time/duration,x=start.cx+(302-start.cx)*p,y=start.cy+(225-start.cy)*p-2*initial.arc*p*(1-p);
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
 CHECK(same_motion(serial,batched));
 for(unsigned i=0;i<10;++i)CHECK(serial.find(i+10)&&serial.find(i+10)->mode==CombatCardMotion::Hand);
 const auto original_state=state.hand;const auto original_seed=state.seed;
 for(unsigned i=0;i<=CombatCardMotion::Capacity;++i)serial.append(transfer(i,Zone::Draw,Zone::Hand,false,i));
 CHECK(serial.overflow&&serial.count==CombatCardMotion::Capacity&&state.seed==original_seed&&!std::memcmp(&state.hand,&original_state,sizeof original_state));
 // A true return redirects the actual holder, including a card whose
 // previous discard flight has not finished. Its counter waits for arrival.
 hand(1);serial.begin(state,0);serial.apply(transfer(0,Zone::Hand,Zone::Discard));serial.set_hand({});serial.advance(2);
 const auto origin=CombatCardMotion::pose(*serial.find(0));
 serial.apply(transfer(0,Zone::Discard,Zone::Draw,false,2));serial.counts.draw=1;
 CHECK(serial.find(0)&&serial.find(0)->mode==CombatCardMotion::Fly&&serial.find(0)->to==unsigned(Zone::Draw));
 CHECK(same_pose(serial.find(0)->origin,origin)&&serial.visible_counts().draw==0);
 serial.advance(130);CHECK(!serial.find(0)&&serial.visible_counts().draw==1);
 // Automatic one-card shortcuts have no selected grid holder. They begin
 // at the actual discard pile and use the same draw-pile destination.
 hand(0);serial.begin(state,0);serial.apply(transfer(5,Zone::Discard,Zone::Draw));
 CHECK(serial.find(5)&&same_pose(serial.find(5)->origin,CombatCardMotion::pile(unsigned(Zone::Discard))));
 CHECK(serial.find(5)->to==unsigned(Zone::Draw));
}
static void generated_preview(){
 hand(0);CombatCardMotion m;m.begin(state,0);const auto saved=state;
 for(unsigned i=0;i<2;++i){auto cue=transfer(i,Zone::Draw,Zone::Discard);cue.from=CombatCardMotion::NewCard;cue.preview=true;cue.card={Slimed,0};m.apply(cue);}
 m.counts.discard=2;CHECK(m.find(0)&&m.find(1));CHECK(m.find(0)->mode==CombatCardMotion::Preview&&m.find(0)->x<m.find(1)->x);
 CHECK(m.find(0)->scale==0&&m.visible_counts().discard==0);
 m.advance(8);CHECK(m.find(0)->scale==1024&&m.find(1)->scale==1024);
 m.advance(43);CHECK(m.find(0)->mode==CombatCardMotion::Preview);
 m.advance(44);CHECK(m.find(0)->mode==CombatCardMotion::Fly&&m.find(1)->mode==CombatCardMotion::Fly);
 m.advance(172);CHECK(!m.find(0)&&!m.find(1)&&m.visible_counts().discard==2);
 CHECK(!std::memcmp(&state,&saved,sizeof state));
}
int main(){
 CHECK(sizeof(CombatCardMotion)<=8*1024);CHECK(sizeof(SovereignBladePresentation)<4096);body_modulation();holder_responses();hover_and_selection_layout();draw_and_play();discard_curve();skipped_frames_and_bounds();generated_preview();
 std::printf("card movement: %u checks passed; %zu bytes of bounded metadata\n",checks,sizeof(CombatCardMotion));
}
