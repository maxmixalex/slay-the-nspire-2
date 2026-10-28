#pragma once
#include "combat_feedback.hpp"
#include "combat_layout.hpp"
#include "sovereign_blade_presentation.hpp"

namespace spire {
// Original pile commands and the hand holders have different lifetimes:
// Add-to-hand is awaited, while the discard flight continues independently.
// This display-only queue keeps those lifetimes without retaining a State or
// another raster pool. All times are the existing 33 ms presentation ticks.
struct CombatCardMotion {
 static constexpr unsigned Capacity=64,Visible=MaxHand*3,CountCapacity=32;
 static constexpr uint8_t NewCard=6,Tracked=128;
 enum Mode : uint8_t {Empty,Hand,Play,Fly,Tail,Preview,Exhaust,Power,Shuffle};
 struct Counts {uint16_t draw=0,discard=0,exhaust=0;};
 struct Cue {
  uint32_t at=0;uint16_t id=0;uint8_t from=0,to=0;
  Card card{};int16_t energy=0,stars=0,target_x=0,target_y=0;bool fast=false,preview=false;
 };
 struct CountCue {Counts value{};uint32_t at=0;};
 struct Sprite {
  struct TrailPoint {int16_t x=0,y=0;};
  static constexpr unsigned TrailPoints=6;
  TrailPoint trail[TrailPoints]{};
  HandPose origin{};
  int32_t x=0,y=0; // Q10 position retains subpixel motion between frames.
  uint32_t at=0,progress=0,speed=0,duration=0,accel=0;
  Card card{};uint16_t id=0;
  int16_t scale=0,angle=0,energy=0,stars=0,arc=0,target_x=0,target_y=0;
  uint8_t mode=Empty,to=0,trail_count=0;bool fast=false,generated=false;
 };
 Sprite sprites[Visible]{};
 Cue cues[Capacity]{};
 CountCue count_cues[CountCapacity]{};
 CombatHand::Cards hand{};
 Counts counts{},observed_counts{};
 SovereignBladePresentation blades{};
 uint8_t zones[MaxCards]{};
 uint32_t seed=0,clock=0,cosmetic=0x906b1a4du,manual_started=0,manual_min_end=0,wait_at=0;
 int floor=-1,combat=-1,encounter=-1,focus=-1,selected=-1,manual=-1;
 int16_t manual_energy=0,manual_stars=0;
 int16_t selected_x=160,selected_y=174,selected_scale=HandFocusScale;
 uint16_t card_count=0;
 uint8_t head=0,count=0,count_head=0,count_count=0,wait_excess=0;
 bool active=false,overflow=false,count_overflow=false,selected_centered=false;

 static Counts capture_counts(const State&s){return {s.draw.count,s.discard.count,s.exhaust.count};}
 static bool same(Counts a,Counts b){return a.draw==b.draw&&a.discard==b.discard&&a.exhaust==b.exhaust;}
 static int magnitude(int n){return n<0?-n:n;}
 static unsigned square_root(unsigned value){unsigned result=0,bit=1u<<30;while(bit>value)bit>>=2;
  while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;}
 static uint16_t modulate(uint16_t pixel,unsigned brightness){
  if(brightness>=255)return pixel;
  const unsigned r=(pixel>>11)*brightness/255u,g=((pixel>>5)&63u)*brightness/255u,b=(pixel&31u)*brightness/255u;
  return uint16_t((r<<11)|(g<<5)|b);
 }
 static int angle_delta(int from,int to){int n=(to-from)&4095;return n>2048?n-4096:n;}
 static int sine(int angle){
  static const int16_t quarter[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
  unsigned phase=unsigned(angle)&4095;const int sign=phase>=2048?-1:1;phase%=2048;if(phase>1024)phase=2048-phase;
  const unsigned i=phase/64,j=phase%64;
  return sign*(i==16?quarter[i]:(quarter[i]*(64-j)+quarter[i+1]*j)/64);
 }
 static int direction(int y,int x){
  // atan2 table in 1/4096 turns; no software floating-point per card pixel.
  static const uint16_t atan[]={0,20,41,61,81,101,121,140,160,179,197,215,234,251,269,286,302,318,334,349,364,378,393,406,420,433,445,457,469,480,491,502,512};
  const unsigned ax=unsigned(magnitude(x)),ay=unsigned(magnitude(y));if(!ax&&!ay)return 0;
  const unsigned lo=ax<ay?ax:ay,hi=ax<ay?ay:ax;
  const unsigned ratio=lo*1024/hi,index=ratio/32,part=ratio%32;
  int a=index==32?512:(atan[index]*(32-part)+atan[index+1]*part)/32;
  if(ay>ax)a=1024-a;if(x<0)a=2048-a;if(y<0)a=-a;return a;
 }
 static HandPose pile(uint8_t zone){
  if(zone==unsigned(Zone::Draw))return {combat_controls::DrawX+12,combat_controls::DrawY+12,HandFocusScale,0,1024};
  if(zone==unsigned(Zone::Discard))return {combat_controls::DiscardX+12,combat_controls::DiscardY+12,HandFocusScale,0,1024};
  if(zone==unsigned(Zone::Exhaust))return {combat_controls::ExhaustX+12,combat_controls::ExhaustY+12,HandFocusScale,0,1024};
  // PileType.Play uses the full viewport center minus100 source pixels.
  // AppendPileLerpTween targets source scale .8 independently of FastMode.
  return {160,hand_project(630-100),(HandFocusScale*4+2)/5,0,1024};
 }
 void reset(){
  blades.reset();
  active=overflow=count_overflow=false;head=count=count_head=count_count=0;
  focus=selected=manual=-1;selected_centered=false;hand.count=0;for(auto&s:sprites)s.mode=Empty;
 }
 Sprite* find(unsigned id){for(auto&s:sprites)if(s.mode!=Empty&&s.id==id)return &s;return nullptr;}
 const Sprite* find(unsigned id)const{for(const auto&s:sprites)if(s.mode!=Empty&&s.id==id)return &s;return nullptr;}
 static void place(Sprite&s,HandPose p){s.x=p.cx*1024;s.y=p.cy*1024;s.scale=int16_t(p.scale);s.angle=int16_t(direction(p.sine,p.cosine));}
 Sprite& claim(unsigned id){
  if(auto*s=find(id))return *s;
  for(auto&s:sprites)if(s.mode==Empty){s={};s.id=uint16_t(id);return s;}
  // Ten hand cards always fit. A pathological chain may replace an older
  // independent flight, never a retained hand card or the canonical result.
  Sprite*old=nullptr;for(auto&s:sprites)if(s.mode!=Hand&&(!old||int32_t(s.at-old->at)<0))old=&s;
  if(!old)old=&sprites[0];*old={};old->id=uint16_t(id);overflow=true;return *old;
 }
 unsigned random(){cosmetic^=cosmetic<<13;cosmetic^=cosmetic>>17;cosmetic^=cosmetic<<5;return cosmetic;}
 void begin(const State&s,uint32_t now){
  reset();active=true;seed=s.seed;floor=s.floor;combat=s.world.combat_count;encounter=int(s.enemy_world.encounter);clock=now;
  cosmetic=0x906b1a4du^(seed?seed:1u);hand=CombatHand::capture(s.hand);counts=observed_counts=capture_counts(s);
  blades.begin(s,now*33u);
  card_count=s.card_count;for(unsigned id=0;id<card_count;++id)zones[id]=uint8_t(s.zones[id]);
  for(unsigned i=0;i<hand.count;++i){auto&v=claim(hand.ids[i]);v.mode=Hand;v.card=s.cards[v.id];place(v,hand_pose(hand.count,int(i),false));zones[v.id]|=Tracked;}
 }
 void append(Cue cue){
  if(count==Capacity&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Capacity){overflow=true;return;}cues[count++]=cue;
 }
 void append_counts(Counts value,uint32_t at){
  if(same(value,observed_counts))return;observed_counts=value;if(count_overflow)return;
  if(count_count>count_head&&count_cues[count_count-1].at==at){count_cues[count_count-1].value=value;return;}
  if(count_count==CountCapacity&&count_head){for(unsigned i=count_head;i<count_count;++i)count_cues[i-count_head]=count_cues[i];count_count-=count_head;count_head=0;}
  if(count_count==CountCapacity){count_overflow=true;return;}count_cues[count_count++]={value,at};
 }
 void set_hand(const CombatHand::Cards&cards){
  hand=cards;bool focused=false;
  for(unsigned i=0;i<hand.count;++i){auto&v=claim(hand.ids[i]);if(v.mode!=Hand){v.mode=Hand;place(v,hand_pose(hand.count,int(i),false));}focused|=int(v.id)==focus;}
  if(!focused)focus=-1;
  for(auto&v:sprites)if(v.mode==Hand){bool kept=false;for(unsigned i=0;i<hand.count;++i)kept|=v.id==hand.ids[i];if(!kept)v.mode=Empty;}
 }
 void set_focus(int id){
  if(focus==id)return;focus=id;if(id<0)return;
  if(id==selected)return;
  for(unsigned i=0;i<hand.count;++i)if(hand.ids[i]==id){auto*v=find(unsigned(id));if(!v)return;
   // RefreshLayout snaps the focused holder's Y, angle and scale, while X
   // continues to approach its target through AnimPosition.
   const auto p=hand_pose(hand.count,int(i),true);v->y=p.cy*1024;v->scale=int16_t(p.scale);v->angle=0;return;}
 }
 void set_selected(int id){
  if(selected==id)return;selected=id;selected_centered=false;set_selection_pose(hand_selected_pose());
  // BeginDrag resets angle and scale immediately before either the mouse
  // follower or CenterCard supplies a position target.
  if(id>=0)if(auto*v=find(unsigned(id))){v->angle=0;v->scale=HandFocusScale;}
 }
 void set_selection_pose(HandPose p){selected_x=int16_t(p.cx);selected_y=int16_t(p.cy);selected_scale=int16_t(p.scale);}
 void fit_centered_selection(){
  if(selected<0)return;auto*v=find(unsigned(selected));if(!v)return;
  // BeginDrag already snaps angle/scale. The LCD adaptation also keeps the
  // first interpolated aiming frame inside the two rails and the screen,
  // so keyboard selection never spends several frames with cropped text.
  if(v->x<114*1024)v->x=114*1024;if(v->x>202*1024)v->x=202*1024;
  if(v->y<84*1024)v->y=84*1024;if(v->y>174*1024)v->y=174*1024;
 }
 HandPose hand_target(unsigned index,bool focused)const{
  if(index>=hand.count)return hand_pose(1,0,focused);
  if(int(hand.ids[index])==selected)return {selected_x,selected_y,selected_scale,0,1024};
  // StartCardPlay reparents its holder out of CardHolderContainer. The
  // remaining hand therefore closes up as soon as selection starts, while
  // canonical pile order and the eventual played-card cue stay untouched.
  int active_count=hand.count,active_index=int(index),focus_index=-1;
  for(unsigned i=0;i<hand.count;++i)if(int(hand.ids[i])==selected){--active_count;if(i<index)--active_index;break;}
  if(selected<0)for(unsigned i=0;i<hand.count;++i)if(int(hand.ids[i])==focus){focus_index=int(i);break;}
  return hand_pose(active_count,active_index,focused&&selected<0,focus_index);
 }
 static unsigned cubic(unsigned elapsed,unsigned duration){
  if(elapsed>=duration)return 1024;const unsigned rest=(duration-elapsed)*1024/duration;return 1024-rest*rest/1024*rest/1024;
 }
 static HandPose pose(const Sprite&s){
  return {(s.x+512)/1024,(s.y+512)/1024,s.scale,sine(s.angle),sine(s.angle+1024)};
 }
 HandPose hand_pose_at(unsigned index,bool focused)const{
  if(index>=hand.count)return hand_pose(1,0,focused);
  const auto*v=find(hand.ids[index]);if(!v||v->mode!=Hand)return hand_target(index,focused);
  const auto p=hand_target(index,focused);
  if(magnitude(v->x-p.cx*1024)<256&&magnitude(v->y-p.cy*1024)<256&&magnitude(v->scale-p.scale)<2&&magnitude(angle_delta(v->angle,direction(p.sine,p.cosine)))<2)return p;
  return pose(*v);
 }
 bool settled(unsigned index,bool focused)const{
  if(index>=hand.count)return true;const auto p=hand_pose_at(index,focused),target=hand_target(index,focused);
  return magnitude(p.cx-target.cx)<=1&&magnitude(p.cy-target.cy)<=1&&magnitude(p.scale-target.scale)<=2&&magnitude(p.sine-target.sine)<=3;
 }
 void fly(Sprite&v,uint8_t to,uint32_t at){
  v.origin=pose(v);v.mode=Fly;v.to=to;v.at=at;v.progress=0;v.trail_count=0;v.generated=false;
  v.speed=72090u+random()%9831u;v.accel=131072u+random()%32769u;v.duration=65536u+random()%49153u;
  v.arc=int16_t((600u+random()%301u)*4u/21u);
 }
 struct PowerPoint {int16_t x,y;uint16_t length;};
 static PowerPoint power_point(const Sprite&v,unsigned index){
  static constexpr PowerPoint points[]={
#include "combat_power_path.inc"
  };
  if(index<41)return points[index];
  // The last source point is moved to the owner's actual VfxSpawnPosition.
  const int t=int(index-40),q=8-t,ex=(v.target_x-v.origin.cx)*16,ey=(v.target_y-v.origin.cy)*16;
  const int x=(q*q*q*(-509)+3*q*q*t*(-1003)+(3*q*t*t+t*t*t)*ex)/512;
  const int y=(q*q*q*488+3*q*q*t*461+(3*q*t*t+t*t*t)*ey)/512;
  return {int16_t(x),int16_t(y),0};
 }
 static unsigned power_length(const Sprite&v){
  auto previous=power_point(v,40);unsigned length=previous.length;
  for(unsigned i=41;i<=48;++i){const auto p=power_point(v,i);const int dx=p.x-previous.x,dy=p.y-previous.y;length+=square_root(unsigned(dx*dx+dy*dy));previous=p;}return length;
 }
 static void power_pose(Sprite&v,unsigned fraction){
  const unsigned wanted=v.accel*fraction/1024u;auto previous=power_point(v,0);unsigned length=0;
  for(unsigned i=1;i<=48;++i){const auto p=power_point(v,i);const int dx=p.x-previous.x,dy=p.y-previous.y;
   const unsigned next=i<=40?p.length:length+square_root(unsigned(dx*dx+dy*dy));
   if(wanted<=next||i==48){const unsigned segment=next-length,t=segment?(wanted-length)*1024u/segment:0;
    v.x=v.origin.cx*1024+int(previous.x)*64+dx*int(t)/16;
    v.y=v.origin.cy*1024+int(previous.y)*64+dy*int(t)/16;
    const int turn=angle_delta(v.angle,direction(dy,dx));const int limit=67+int(fraction)*3233/1024;
    v.angle=int16_t(v.angle+(turn<-limit?-limit:turn>limit?limit:turn));return;
   }previous=p;length=next;
  }
 }
 static void keep_trail(Sprite&v){
  const unsigned count=v.trail_count<Sprite::TrailPoints?v.trail_count++:Sprite::TrailPoints-1;
  for(unsigned i=count;i>0;--i)v.trail[i]=v.trail[i-1];v.trail[0]={int16_t((v.x+512)/1024),int16_t((v.y+512)/1024)};
 }
 void arrange_previews(uint32_t at){
  unsigned total=0;for(const auto&v:sprites)if(v.mode==Preview&&v.at==at)++total;
  unsigned index=0;
  for(auto&v:sprites)if(v.mode==Preview&&v.at==at){
   if(total<=5){v.x=((1680+(2*int(index)-int(total)+1)*325)*2+10)/21*1024;v.y=hand_project(630+50)*1024;}
   else{
    // The native Messy container samples 150-source-pixel spaced points.
    // Bounded rejection keeps that separation in the smaller viewport.
    int x=156,y=127;for(unsigned attempt=0;attempt<48;++attempt){
     if(index){x=50+int(random()%213u);y=37+int(random()%181u);}bool close=false;
     unsigned previous=0;for(const auto&p:sprites){if(&p==&v)break;if(p.mode==Preview&&p.at==at){++previous;const int dx=p.x/1024-x,dy=p.y/1024-y;close|=dx*dx+dy*dy<817;}}
     if(!close||!previous)break;
    }v.x=x*1024;v.y=y*1024;
   }++index;
  }
 }
 void step(){
  ++clock;
  for(auto&v:sprites){
   if(v.mode==Fly||v.mode==Power||v.mode==Shuffle)keep_trail(v);
   if(v.mode==Hand){
    int index=-1;for(unsigned i=0;i<hand.count;++i)if(hand.ids[i]==v.id){index=int(i);break;}if(index<0){v.mode=Empty;continue;}
    const auto p=hand_target(unsigned(index),int(v.id)==focus);
    const int dx=p.cx*1024-v.x,dy=p.cy*1024-v.y,ds=p.scale-v.scale,da=angle_delta(v.angle,direction(p.sine,p.cosine));
    v.x=magnitude(dx)<256?p.cx*1024:v.x+dx*231/1000;v.y=magnitude(dy)<256?p.cy*1024:v.y+dy*231/1000;
    v.scale=int16_t(magnitude(ds)<=3?p.scale:v.scale+ds*264/1000);v.angle=int16_t(magnitude(da)<=3?direction(p.sine,p.cosine):v.angle+da*330/1000);
   }else if(v.mode==Play){
    const auto to=pile(unsigned(Zone::Limbo));const unsigned age=(clock-v.at)*33u,p=cubic(age,v.fast?100u:250u),scale=cubic(age,250u);
    v.x=(v.origin.cx*int(1024-p)+to.cx*int(p));v.y=(v.origin.cy*int(1024-p)+to.cy*int(p));
    v.scale=int16_t((v.origin.scale*int(1024-scale)+to.scale*int(scale))/1024);
    v.angle=int16_t(direction(v.origin.sine,v.origin.cosine)*int(1024-p)/1024);
   }else if(v.mode==Preview){
    const unsigned age=(clock-v.at)*33u;
    v.scale=int16_t(unsigned(HandFocusScale)*cubic(age,250u)/1024u);
    if(age>=1450u)fly(v,v.to,clock);
   }else if(v.mode==Exhaust){
    if((clock-v.at)*33u>=1000u)v.mode=Empty;
   }else if(v.mode==Power){
    const unsigned age=(clock-v.at)*33u,p=age>=v.duration?1024u:age*1024u/v.duration;
    power_pose(v,p*p/1024u);
    v.scale=int16_t(age<300u?v.origin.scale*(3000-int(age)*9)/3000:v.origin.scale/10);
    if(p>=922u)v.scale=int16_t(v.origin.scale*int(1024u-p)/1024);
    if(p==1024u){v.mode=Tail;v.at=clock;v.scale=0;}
   }else if(v.mode==Fly||v.mode==Shuffle){
    v.progress+=v.speed*33u/1000u;v.speed+=v.accel*33u/1000u;
    if(v.progress>=v.duration){const auto end=pile(v.to);v.x=end.cx*1024;v.y=end.cy*1024;v.scale=int16_t(v.origin.scale/10);v.mode=Tail;v.at=clock;v.progress=0;continue;}
    const unsigned p=v.progress*1024u/v.duration,q=1024-p;
    const auto end=pile(v.to);v.x=v.origin.cx*int(q)+end.cx*int(p);
    v.y=v.origin.cy*int(q)+end.cy*int(p)-2*v.arc*int(p)*int(q)/1024;
    const int dx=end.cx-v.origin.cx,dy=end.cy-v.origin.cy-2*v.arc*(1024-2*int(p))/1024;
    const int target=direction(dy,dx)+1024;v.angle=int16_t(v.angle+angle_delta(v.angle,target)*396/1000);
    const unsigned shrink=p*3>1024?1024:p*3;v.scale=int16_t(v.origin.scale*int(10240-shrink*9)/10240);
   }else if(v.mode==Tail){
    // NCardTrailVfx stops emitting and fades its existing sprites for .5s.
    if((clock-v.at)*33u>=500u){v.mode=Empty;continue;}
   }
  }
 }
 void advance(uint32_t now){
  blades.advance(now*33u);
  int32_t ticks=int32_t(now-clock);if(ticks<=0)return;
  // All holder responses settle and all finite flights/previews finish inside 128
  // ticks. Large skipped intervals do not create an unbounded catch-up loop.
  const unsigned n=unsigned(ticks)>128?128:unsigned(ticks);for(unsigned i=0;i<n;++i)step();clock=now;
 }
 void apply(const Cue&cue){
  auto*old=find(cue.id);
  if(cue.preview&&cue.from==NewCard&&cue.to!=unsigned(Zone::Hand)){
   auto&v=claim(cue.id);v.mode=Preview;v.at=cue.at;v.to=cue.to;v.scale=v.angle=0;v.fast=cue.fast;
   v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;arrange_previews(cue.at);return;
  }
  if(cue.to==unsigned(Zone::Hand)){
   auto&v=claim(cue.id);if(cue.from==NewCard){v.mode=Hand;v.at=cue.at;v.generated=true;place(v,{160,200,0,0,1024});v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;}
   if(v.mode==Empty)place(v,pile(cue.from));v.mode=Hand;
   v.trail_count=0;v.generated=false;
   v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;
  }
  if(cue.to==unsigned(Zone::Limbo)&&cue.from==unsigned(Zone::Hand)){
   auto&v=claim(cue.id);if(v.mode==Empty)place(v,pile(cue.from));v.origin=pose(v);v.mode=Play;v.at=cue.at;v.fast=cue.fast;
   v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;
  }
  if((cue.to==unsigned(Zone::Draw)||cue.to==unsigned(Zone::Discard))&&old&&(cue.from==unsigned(Zone::Hand)||old->mode==Play)){
   auto&v=*old;fly(v,cue.to,cue.at);v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;
  }
  if(cue.to==unsigned(Zone::Exhaust)){
   auto&v=claim(cue.id);if(v.mode==Empty)place(v,pile(cue.from));v.origin=pose(v);v.mode=Exhaust;v.at=cue.at;v.trail_count=0;
   v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;
  }
  if(cue.to==unsigned(Zone::Power)){
   auto&v=claim(cue.id);if(v.mode==Empty)place(v,pile(cue.from));v.origin=pose(v);v.mode=Power;v.at=cue.at;v.to=cue.to;v.trail_count=0;
   v.target_x=cue.target_x;v.target_y=cue.target_y;v.accel=power_length(v);v.duration=v.accel*7u/64u;if(!v.duration)v.duration=1;
   v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;
  }
  if(cue.id==MaxCards&&cue.from==unsigned(Zone::Discard)&&cue.to==unsigned(Zone::Draw)){
   auto&v=claim(cue.id);place(v,pile(cue.from));fly(v,cue.to,cue.at);v.mode=Shuffle;
   v.arc=int16_t((200u+random()%701u)*4u/21u);v.card=cue.card;v.energy=cue.energy;v.stars=cue.stars;return;
  }
  if(old)old->mode=Empty;
 }
 unsigned brightness(const Sprite&v)const{
  if(v.mode==Tail)return 0;if(v.mode!=Fly)return 255;
  const unsigned p=v.progress*3u*255u/v.duration;return p>=255?0:255-p;
 }
 Counts visible_counts()const{
  Counts result=counts;
  for(const auto&v:sprites)if(v.mode==Fly||v.mode==Preview){if(v.to==unsigned(Zone::Draw)&&result.draw)--result.draw;else if(v.to==unsigned(Zone::Discard)&&result.discard)--result.discard;}
  return result;
 }
};
// UI14 adds per-instance blade damage and64 source-clock Forge/attack cues
// (under2.5KiB). Preserve the original motion capacities within an8KiB total.
static_assert(sizeof(CombatCardMotion)<=8*1024,"Card and blade movement share an8KiB transient view");
}
