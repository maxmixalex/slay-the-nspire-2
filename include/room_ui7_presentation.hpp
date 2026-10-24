#pragma once
#include <stdint.h>
#include "health_bar.hpp"
#include "merchant_layout.hpp"

namespace spire {
namespace room_curve {
constexpr unsigned Q=4096;
inline unsigned linear(uint32_t age,uint32_t duration){return age>=duration?Q:unsigned(age*Q/duration);}
inline unsigned multiply(unsigned a,unsigned b){return (a*b+Q/2)/Q;}
inline unsigned cubic_out(uint32_t age,uint32_t duration){const unsigned v=Q-linear(age,duration);return Q-multiply(multiply(v,v),v);}
inline unsigned quint_out(uint32_t age,uint32_t duration){const unsigned v=Q-linear(age,duration),v2=multiply(v,v);return Q-multiply(multiply(v2,v2),v);}
inline unsigned expo_out(uint32_t age,uint32_t duration){return age>=duration?Q:health_bar::expo_out(age*health_bar::TweenMs/duration);}
inline unsigned sine_in_out(uint32_t age,uint32_t duration){
 if(age>=duration)return Q;
 static constexpr uint16_t samples[]={0,39,156,345,600,910,1264,1648,2048,2448,2832,3186,3496,3751,3940,4057,4096};
 const unsigned scaled=age*16*Q/duration,index=scaled/Q,fraction=scaled%Q;
 return samples[index]+(samples[index+1]-samples[index])*fraction/Q;
}
inline unsigned sine_out(uint32_t age,uint32_t duration){
 if(age>=duration)return Q;
 static constexpr uint16_t samples[]={0,401,799,1189,1567,1931,2276,2598,2896,3166,3406,3612,3784,3919,4017,4076,4096};
 const unsigned scaled=age*16*Q/duration,index=scaled/Q,fraction=scaled%Q;
 return samples[index]+(samples[index+1]-samples[index])*fraction/Q;
}
inline int back_out(unsigned q){if(q>=Q)return int(Q);const int u=int(q)-int(Q);return int(Q)+(u*u/int(Q))*((11066*u/int(Q))+6970)/int(Q);}
inline int sine_turn(unsigned phase){const unsigned quadrant=phase/1024%4,within=phase%1024;const int value=int(sine_out(quadrant&1?1024-within:within,1024));return quadrant>=2?-value:value;}
inline int interpolate(int from,int to,unsigned progress){return from+(to-from)*int(progress)/int(Q);}
}

// Presentation only: no saved room fields, gameplay RNG or dynamic buffers.
struct RoomUi7Presentation {
 struct FocusRecovery {uint32_t start=0;bool recovering=false;};
 struct Merchant {
  static constexpr int ClosedY=-222,OpenY=MerchantInventoryLayout::RugY;
  uint32_t start=0,hand_return=0,dialogue_start=0,dialogue_serial=0,remove_start=0,failure_start=0,dialogue_random=0x39a719d5u;
  int16_t dialogue_x=253;
  uint16_t sold_mask=0;
  int16_t from_y=ClosedY;uint16_t from_alpha=0;
  int16_t hand_x=52*16,hand_y=-12*16,target_x=52*16,target_y=-12*16;
  int8_t focused=-1,dialogue=-1,failed=-1;bool open=false,initialized=false,hand_lingering=false;
  FocusRecovery slots[14]{};
  int y(uint32_t now)const{return room_curve::interpolate(from_y,open?OpenY:ClosedY,open?room_curve::quint_out(now-start,700):room_curve::cubic_out(now-start,500));}
  unsigned alpha(uint32_t now)const{return unsigned(room_curve::interpolate(from_alpha,open?204:0,room_curve::sine_in_out(now-start,open?1000:800)));}
  int offset(uint32_t now)const{return y(now)-OpenY;}
  bool visible(uint32_t now)const{return open||(initialized&&now-start<800);}
  void set_open(bool next,uint32_t now){
   if(initialized&&next==open)return;
   from_y=int16_t(y(now));from_alpha=uint16_t(alpha(now));start=now;open=next;initialized=true;
   if(!next){focused=-1;hand_lingering=false;target_x=52*16;target_y=-12*16;dialogue=-1;}
  }
  void observe_focus(int next,int x,int y,uint32_t now){
   if(next!=focused){
    if(focused>=0&&focused<14){slots[focused]={now,true};hand_return=now;hand_lingering=true;}
    focused=int8_t(next);
   }
   if(next>=0){target_x=int16_t(x*16);target_y=int16_t(y*16);hand_lingering=false;}
  }
  unsigned scale(unsigned item,uint32_t now)const{
   if(item>=14)return 650;if(int(item)==focused)return 800;
   const auto&s=slots[item];return s.recovering?800-150*room_curve::expo_out(now-s.start,500)/room_curve::Q:650;
  }
  void step(uint32_t now,uint32_t ms){
   if(hand_lingering&&now-hand_return>=2000){hand_lingering=false;target_x=52*16;target_y=-12*16;}
   const unsigned weight=ms>=250?1000:ms*4;
   hand_x=int16_t(int(hand_x)+(int(target_x)-hand_x)*int(weight)/1000);
   hand_y=int16_t(int(hand_y)+(int(target_y)-hand_y)*int(weight)/1000);
  }
  void speak(int kind,uint32_t now){
   dialogue=int8_t(kind);dialogue_start=now;++dialogue_serial;
   // Rng.Chaotic is cosmetic in the source. A private presentation stream
   // keeps its varied line/position behavior without consuming saved RNG.
   dialogue_random^=dialogue_random<<13;dialogue_random^=dialogue_random>>17;dialogue_random^=dialogue_random<<5;
   dialogue_x=int16_t(MerchantInventoryLayout::project(450+int(dialogue_random%1001)));
  }
  void failure(int item,int kind,uint32_t now){failed=int8_t(item);failure_start=now;speak(kind,now);}
  int wiggle(unsigned item,uint32_t now)const{
   const uint32_t age=now-failure_start;if(int(item)!=failed||age>=400)return 0;
   const unsigned inverse=room_curve::Q-room_curve::linear(age,400),q=2*(room_curve::Q-room_curve::multiply(inverse,inverse));
   return 2*room_curve::sine_turn(q)/int(room_curve::Q);
  }
  unsigned dialogue_alpha(uint32_t now)const{
   const uint32_t age=now-dialogue_start;if(dialogue<0||age>=2000)return 0;
   if(age<250)return age*255/250;if(age<1500)return 255;
   return 255-255*room_curve::sine_out(age-1500,500)/room_curve::Q;
  }
 } merchant;
 struct Legend {
  FocusRecovery items[6]{};int8_t focused=-1;
  void observe(int next,uint32_t now){if(next==focused)return;if(focused>=0&&focused<6)items[focused]={now,true};focused=int8_t(next);}
  unsigned scale(unsigned item,uint32_t now)const{
   if(item>=6)return 4096;if(int(item)==focused)return 5120;
   return items[item].recovering?5120-room_curve::expo_out(now-items[item].start,500)/4:4096;
  }
 } legend;
 struct Rest {
  struct Button {
   uint32_t start=0;int16_t from_scale=4096,from_outline=3686,from_label=0,from_value=1000;uint8_t state=0;
   struct Sample {int scale,outline,label,value;};
   Sample sample(uint32_t now)const{
    const unsigned q=state==1?room_curve::linear(now-start,50):room_curve::expo_out(now-start,1000);
    return {room_curve::interpolate(from_scale,state==1?4506:state==2?3686:4096,q),
      room_curve::interpolate(from_outline,state==1?4096:3686,q),
      room_curve::interpolate(from_label,state==1?6*4096:0,q),
      state==1?1200:room_curve::interpolate(from_value,1000,q)};
   }
   Sample observe(uint8_t next,uint32_t now){
    if(next!=state){const auto v=sample(now);from_scale=int16_t(v.scale);from_outline=int16_t(v.outline);from_label=int16_t(v.label);from_value=1200;state=next;start=now;}
    return sample(now);
   }
  } buttons[32]{};
  uint32_t mask=0,start=0,description_start=0;int8_t focused=-1,description=-1;bool bound=false;
  void observe(uint32_t next,uint32_t now){if(bound&&mask==next)return;mask=next;start=now;bound=true;focused=description=-1;for(auto&button:buttons)button=Button{};}
  unsigned alpha(uint32_t now)const{return 255*room_curve::cubic_out(now-start,500)/room_curve::Q;}
  bool mouse_ready(uint32_t now)const{return now-start>=500;}
  void focus(int next,uint32_t now){if(next==focused)return;if(next<0)description_start=now;else description=int8_t(next);focused=int8_t(next);}
  unsigned description_alpha(uint32_t now)const{return description<0?0:focused>=0?255:255-255*room_curve::expo_out(now-description_start,1000)/room_curve::Q;}
 } rest;
 struct Event {
  struct Button {
   uint32_t start=0;int16_t from_scale=4096,from_shade=191,from_glow=0;uint8_t state=0;
   struct Sample {int scale,shade,glow;};
   Sample sample(uint32_t now)const{
    const uint32_t age=now-start;const unsigned q=state==1?room_curve::linear(age,50):room_curve::expo_out(age,500);
    const int scale=state==1?4137:state==2?4055:4096,shade=state==1?255:191,glow=state==1||state==2?255:0;
    return {room_curve::interpolate(from_scale,scale,q),room_curve::interpolate(from_shade,shade,q),
      room_curve::interpolate(from_glow,glow,room_curve::linear(age,state==1?50:300))};
   }
   Sample observe(uint8_t next,uint32_t now){if(next!=state){const auto v=sample(now);from_scale=int16_t(v.scale);from_shade=int16_t(v.shade);from_glow=int16_t(v.glow);state=next;start=now;}return sample(now);}
  } buttons[16]{};
  uint32_t revision=0,start=0,press_start=0;uint16_t id=0;
  uint8_t page=0;int8_t pressed=-1;bool bound=false,fast=false,finished=false,ancient=false;
  void observe(uint16_t next_id,uint8_t next_page,bool next_finished,uint32_t next_revision,bool quick,uint32_t now){
   if(bound&&id==next_id&&page==next_page&&finished==next_finished&&revision==next_revision)return;
   id=next_id;page=next_page;finished=next_finished;revision=next_revision;fast=quick;start=now;pressed=-1;bound=true;
   for(auto&button:buttons)button=Button{};
  }
  unsigned title_alpha(uint32_t now)const{const unsigned delay=fast?200:500;return now-start<=delay?0:255*room_curve::linear(now-start-delay,fast?250:500)/room_curve::Q;}
  unsigned body_progress(uint32_t now)const{const unsigned delay=(fast?200:500)+250;return now-start<=delay?0:room_curve::linear(now-start-delay,fast?500:1000);}
  unsigned body_reveal(uint32_t now)const{const unsigned delay=(fast?200:500)+250;return now-start<=delay?0:room_curve::sine_out(now-start-delay,fast?500:1000);}
  unsigned option_progress(unsigned index,uint32_t now)const{const unsigned delay=(fast?250:500)+index*(fast?100:200);return now-start<=delay?0:room_curve::linear(now-start-delay,fast?250:500);}
  bool option_ready(unsigned index,uint32_t now)const{return option_progress(index,now)==room_curve::Q;}
  int option_x(unsigned index,uint32_t now)const{return -13*(int(room_curve::Q)-room_curve::back_out(option_progress(index,now)))/int(room_curve::Q);}
  void press(int index,bool held,uint32_t now){if(!held){pressed=-1;return;}if(pressed!=index){pressed=int8_t(index);press_start=now;}}
 } event;
 void reset(){*this=RoomUi7Presentation{};}
};
}
