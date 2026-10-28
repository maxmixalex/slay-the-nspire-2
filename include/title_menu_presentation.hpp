#pragma once
#include <stdint.h>
#include "room_ui7_presentation.hpp"
namespace spire {
namespace title_menu_motion {
struct LabelVisual {int16_t scale=4096;uint8_t red=255,green=246,blue=226,alpha=255;};
inline int mix(int from,int to,unsigned q){return from+(to-from)*int(q)/4096;}
struct Label {
 uint32_t changed=0;LabelVisual from{};uint8_t state=0;
 LabelVisual sample(uint32_t now)const{
  const uint32_t age=now-changed;LabelVisual out=from;
  const unsigned scale_q=state==1?room_curve::linear(age,50):state==2||state==3?room_curve::cubic_out(age,200):room_curve::expo_out(age,500);
  out.scale=int16_t(mix(from.scale,state==1||state==3?4301:state==2?3891:4096,scale_q));
  // Press/Release append their color tween after the200ms scale tween.
  const unsigned color_q=state==1?room_curve::linear(age,50):state==2||state==3?(age<=200?0:room_curve::cubic_out(age-200,200)):4096;
  out.red=uint8_t(mix(from.red,state==1?239:255,color_q));
  out.green=uint8_t(mix(from.green,state==1?200:state==2?255:246,color_q));
  out.blue=uint8_t(mix(from.blue,state==1?81:state==2?255:226,color_q));
  out.alpha=uint8_t(mix(from.alpha,state==2?128:255,color_q));return out;
 }
 LabelVisual observe(bool focused,bool pressed,uint32_t now){
  unsigned next=pressed?2:focused?1:0;
  if(next==1&&(state==2||state==3))next=3;
  if(next!=state){from=sample(now);changed=now;state=uint8_t(next);
   if(next==2)from={4301,239,200,81,255};
   else if(next==0){from.red=255;from.green=246;from.blue=226;from.alpha=255;}}
  return sample(now);
 }
};
struct Reticle {
 uint32_t changed=0;int16_t left=0,top=0,width=0;uint16_t from_tint=0;int8_t row=-1,from_travel=0;uint8_t from_alpha=0;
 unsigned alpha(uint32_t now)const{
  const unsigned q=room_curve::linear(now-changed,row<0?250:50);
  return unsigned(mix(from_alpha,row<0?0:255,q));
 }
 int travel(uint32_t now)const{return row<0?from_travel:28*room_curve::back_out(room_curve::linear(now-changed,250))/4096;}
 unsigned tint(uint32_t now)const{return row<0?unsigned(mix(from_tint,0,room_curve::linear(now-changed,250))):room_curve::linear(now-changed,50);}
 void observe(int next,int x,int y,int w,uint32_t now){
  if(next==row)return;from_alpha=uint8_t(next<0?alpha(now):0);from_tint=uint16_t(next<0?tint(now):0);from_travel=int8_t(next<0?travel(now):0);changed=now;row=int8_t(next);
  if(next>=0){left=int16_t(x);top=int16_t(y);width=int16_t(w);}
 }
};
struct ContinueVisual {int16_t y=0;uint8_t alpha=0;}; // y is source pixels in Q8.
struct Continue {
 uint32_t changed=0;ContinueVisual from{};bool shown=false;
 ContinueVisual sample(uint32_t now)const{
  const unsigned q=room_curve::linear(now-changed,200);
  const int position=shown?room_curve::back_out(q):int(q);
  return {int16_t(int(from.y)+((shown?-20*256:0)-int(from.y))*position/4096),uint8_t(mix(from.alpha,shown?255:0,q))};
 }
 ContinueVisual observe(bool next,uint32_t now){if(next!=shown){from=sample(now);shown=next;changed=now;}return sample(now);}
};
}
struct TitleMenuPresentation {
 title_menu_motion::Label labels[6]{};
 title_menu_motion::Reticle reticle{};
 title_menu_motion::Continue run_info{};
 uint8_t rows=0;
 void bind(unsigned count){if(rows!=count){*this={};rows=uint8_t(count);}}
};
static_assert(sizeof(TitleMenuPresentation)<=128,"Title presentation stays bounded and outside canonical saves");
}
