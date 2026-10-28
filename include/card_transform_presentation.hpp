#pragma once
#include "core.hpp"
#include "room_ui7_presentation.hpp"

namespace spire {
// NCardTransformVfx -> NCardTransformShineVfx (the scene's overrides, not
// the C# exported defaults). All data is detached from the changing deck.
struct CardTransformPresentation {
 static constexpr unsigned Capacity=MaxDeck,Columns=3;
 static constexpr uint32_t Enter=250,Shine=750,Swap=1125,ShineParticles=1225,
     EndParticles=1390,Fly=2190,GroupInterval=2500;
 struct Face {EventDeckSnapshot model{};int16_t energy=0,stars=0;};
 struct Pair {
  Face before{},after{};
  uint32_t start=0;
  uint16_t expected=0;
  int16_t removed_index=-1,deck=-1;
  bool ready=false,launched=false;
 };
 Pair pairs[Capacity]{};
 uint32_t start=0;
 uint16_t count=0,pending_token=0;
 bool pending=false;
 static bool active(const Pair&p,uint32_t now){
  return p.ready&&(int32_t(now-p.start)<0||uint32_t(now-p.start)<4300);
 }
 bool active(uint32_t now)const{
  if(pending)return true;for(unsigned i=0;i<count;++i)if(!pairs[i].ready||active(pairs[i],now))return true;return false;
 }
 int add(const Face&before,unsigned expected,int removed_index,uint32_t now){
  if(!active(now)){count=0;start=now;}
  if(!count)start=now;if(count>=Capacity)return -1;
  const unsigned i=count++;pairs[i]=Pair{};auto&p=pairs[i];p.before=before;p.expected=uint16_t(expected);p.removed_index=int16_t(removed_index);
  p.start=start+(i/Columns)*GroupInterval+(i%Columns)*100u;return int(i);
 }
 void finish(unsigned i,const Face&after,int deck,uint32_t now){
  if(i>=count)return;auto&p=pairs[i];p.after=after;p.deck=int16_t(deck);p.ready=true;
  if(int32_t(now-p.start)>0)p.start=now;
 }
 unsigned group_size(unsigned i)const{
  const unsigned first=i/Columns*Columns,remaining=count-first;return remaining<Columns?remaining:Columns;
 }
 int center(unsigned i)const{return 160+(int(i%Columns)*2-int(group_size(i))+1)*46;}
 static unsigned power(unsigned q,unsigned n){unsigned r=4096;while(n--)r=unsigned(uint64_t(r)*q/4096);return r;}
 static unsigned overlay(uint32_t age){
  if(age<Shine||age>=Swap+100)return 0;
  if(age>=Swap)return power(4096-room_curve::linear(age-Swap,100),4)*255/4096;
  const unsigned q=room_curve::linear(age-Shine,250);
  const unsigned eased=q<2048?power(q*2,4)/2:4096-power((4096-q)*2,4)/2;
  return eased*255/4096;
 }
 static int hermite(int a,int b,int out,int in,unsigned span,unsigned q){
  const int t=int(q),t2=int(int64_t(t)*t/4096),t3=int(int64_t(t2)*t/4096);
  return int((int64_t(2*t3-3*t2+4096)*a+int64_t(t3-2*t2+t)*out*span/4096+
      int64_t(-2*t3+3*t2)*b+int64_t(t3-t2)*in*span/4096)/4096);
 }
 static int reveal(unsigned q,bool y){
  const unsigned knots[]={0,1341,y?2765u:2728u,4096};
  const int values_x[]={4113,3271,4512,4113},values_y[]={4113,4911,3669,4113};
  const int*values=y?values_y:values_x;
  for(unsigned i=0;i<3;++i)if(q<=knots[i+1]){
   const unsigned span=knots[i+1]-knots[i],t=(q-knots[i])*4096/span;
   return hermite(values[i],values[i+1],i?0:y?4990:-6503,0,span,t);
  }return 4113;
 }
 static int scale(uint32_t age,bool y){
  if(age<Enter)return int(room_curve::cubic_out(age,Enter));
  if(age<Shine)return 4096;
  if(age<Swap)return hermite(4113,y?3581:4601,0,y?-951:951,4096,room_curve::linear(age-Shine,Swap-Shine));
  if(age<Swap+250)return reveal(room_curve::linear(age-Swap,250),y);
  return 4096;
 }
 static unsigned glow(uint32_t age){return age<Swap||age>=Swap+250?0:power(4096-room_curve::linear(age-Swap,250),5)*255/4096;}
 static unsigned glow_scale(uint32_t age){return 4096+1434*(255-glow(age))/255;}
 static unsigned shine_frame(uint32_t age){
  const unsigned q=room_curve::linear(age-ShineParticles,350);
  const int curve=hermite(0,4096,2840,0,4096,q);
  const unsigned frame=unsigned(curve)*891/409600;return frame<9?frame:8;
 }
};
static_assert(sizeof(CardTransformPresentation)<24u*1024u,"All legal transform snapshots fit24KiB");
}
