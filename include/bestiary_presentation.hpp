#pragma once
#include <stdint.h>
#include "health_bar.hpp"
#include "reference_viewport.hpp"

namespace spire {
// A detached compendium viewer. These cursors, clips and scroll positions never
// belong to State, a gameplay RNG stream or the run save codec.
struct BestiaryPresentation {
 static constexpr int Capacity=112,Unit=256;
 static constexpr int Top=reference_viewport::project(120),Bottom=240-Top;
 static constexpr int Left=212,Right=300;
 // Compact labels keep their measured one/two-line height; positions and
 // tweens remain in source space so arrow and scroll coordinates agree.
 static constexpr int EntryHeight=74,HeaderHeight=95,Separation=5;
 static constexpr unsigned ActionsPerPage=5;
 static constexpr int ModeX=76,ModeY=215;
 static constexpr int ModeWidth=118,ModeHeight=22;
 static constexpr unsigned PhysicalBase=192,PhysicalEnd=PhysicalBase+25;
 struct RowMotion {uint32_t at=0;uint16_t from=1024;bool hover=false;};
 struct MoveMotion {
  uint32_t focus_at=0,press_at=0;
  uint16_t focus_from=1024,press_from=1024;
  uint8_t phase=0;
  bool focused=false;
  static unsigned cubic(unsigned q){const unsigned inverse=4096-q;return 4096-(inverse*inverse/4096)*inverse/4096;}
  static unsigned bounce(unsigned q){
   // Godot Tween::TRANS_BOUNCE / EASE_OUT, in fixed point.
   int t=int(q);if(t<1489)return unsigned(121*t*t/16/4096);
   if(t<2979){t-=2234;return unsigned(121*t*t/16/4096+3072);}
   if(t<3724){t-=3351;return unsigned(121*t*t/16/4096+3840);}
   t-=3909;return unsigned(121*t*t/16/4096+4032);
  }
  int focus_scale(uint32_t now)const{
   const unsigned age=now-focus_at,duration=focused?50:250;
   const unsigned q=age>=duration?4096:age*4096/duration;
   return int(focus_from)+(int(focused?1075:1024)-int(focus_from))*int(focused?q:cubic(q))/4096;
  }
  int press_scale(uint32_t now)const{
   const unsigned age=now-press_at,duration=phase==1?200:phase==2?300:100;
   const unsigned q=age>=duration?4096:age*4096/duration;
   const unsigned curve=phase==2?bounce(q):health_bar::expo_out(age>=duration?1000:age*1000/duration);
   return int(press_from)+(int(phase==1?922:1024)-int(press_from))*int(curve)/4096;
  }
  void focus(bool value,uint32_t now){if(focused==value)return;focus_from=uint16_t(focus_scale(now));focused=value;focus_at=now;
   if(!value&&phase==1){press_from=uint16_t(press_scale(now));phase=0;press_at=now;}}
  void press(bool value,uint32_t now){press_from=uint16_t(press_scale(now));phase=value?1:2;press_at=now;}
 } moves[9]{};
 struct ModeMotion {
  uint32_t at=0;
  uint16_t from=1024,to=1024,duration=0;
  bool focused=false,pressed=false;
  int scale(uint32_t now)const{const unsigned age=now-at;
   const unsigned q=!duration||age>=duration?4096:to==1075?age*4096/duration:health_bar::expo_out(age*1000/duration);
   return int(from)+(int(to)-int(from))*int(q)/4096;}
  void observe(bool focus,bool press,uint32_t now){
   if(focused!=focus){from=uint16_t(scale(now));focused=focus;to=focus?1075:1024;duration=focus?50:500;at=now;}
   if(press&&!pressed){from=uint16_t(scale(now));to=973;duration=250;at=now;}
   // NBestiaryModeButton has no OnRelease scale tween. The press endpoint
   // remains until the next source focus/unfocus transition.
   pressed=press;
  }
 } mode_motion{};
 struct Preview {
  uint8_t model=0,skin=0,count=0,current=0;
  int16_t state=-1,clips[12]{};
  uint32_t start=0,delay_until=0,action_at=0;
  int16_t idle=-1,waiting=-1;
  uint8_t facts[40]{};
  bool hold=false;
 } actors[3]{};
 int16_t rows[Capacity]{},row_count=0,selected=-1,selected_row=-1,focused=-1;
 uint16_t row_offsets[Capacity+1]{};
 RowMotion motions[Capacity]{};
 uint32_t opened=0,selected_at=0,mode_at=0,filter_at=0,arrow_at=0,input_serial=0;
 int32_t position=0,target=0,arrow_from=0;
 uint8_t actor_count=0,action=0,filter=0,focus=0;
 bool open=false,actions=true,dragging=false,thumb_drag=false;
 int16_t drag_y=0;
 static int clamp(int value,int low,int high){return value<low?low:value>high?high:value;}
 static int fraction(int value,int thousandths){return value/1000*thousandths+value%1000*thousandths/1000;}
 void cancel_presses(uint32_t now){for(auto&move:moves)if(move.phase==1){move.press_from=uint16_t(move.press_scale(now));move.phase=0;move.press_at=now;}}
 void layout_rows(const uint8_t*heights=nullptr){unsigned y=0;for(int row=0;row<row_count;++row){row_offsets[row]=uint16_t(y);y+=(heights?(unsigned(heights[row])*21+2)/4:rows[row]<0?HeaderHeight:EntryHeight)+Separation;}
  row_offsets[row_count]=uint16_t(y?y-Separation:0);}
 int content_height()const{return reference_viewport::project(row_offsets[row_count]);}
 int maximum()const{return content_height()>Bottom-Top?(content_height()-(Bottom-Top))*Unit:0;}
 int row_top(int row)const{return reference_viewport::project(row_offsets[row]);}
 int row_height(int row)const{return reference_viewport::project(row_offsets[row+1]-(row+1<row_count?Separation:0))-row_top(row);}
 int row_y(int row)const{return Top+row_top(row)-position/Unit;}
 int row_center(int row)const{return (row_top(row)+row_height(row)/2)*Unit;}
 int arrow_y(uint32_t now)const{
  if(selected_row<0)return Top;
  const unsigned age=now-arrow_at;const int target_y=row_center(selected_row);
  const int eased=age>=250?4096:int(health_bar::expo_out(age*4));
  return Top+(arrow_from+(target_y-arrow_from)*eased/4096-position)/Unit;
 }
 void reveal(int row){
  if(row<0||row>=row_count)return;
  const int low=row_top(row)*Unit,high=(row_top(row)+row_height(row)-(Bottom-Top))*Unit;
  if(target>low)target=low;else if(target<high)target=high;
  target=clamp(target,0,maximum());
 }
 void wheel(int rows){target=clamp(target+clamp(rows,-64,64)*80*Unit/9,-(Bottom-Top)*Unit,maximum()+(Bottom-Top)*Unit);}
 static constexpr int ScrollTop=reference_viewport::project(130),ScrollBottom=240-ScrollTop,ThumbRadius=5;
 int thumb_y()const{return ScrollTop+ThumbRadius+(maximum()?clamp(thumb_drag?target:position,0,maximum())*(ScrollBottom-ScrollTop-2*ThumbRadius)/maximum():0);}
 void drag_thumb(int y){target=maximum()*clamp(y-ScrollTop-ThumbRadius,0,ScrollBottom-ScrollTop-2*ThumbRadius)/(ScrollBottom-ScrollTop-2*ThumbRadius);}
 void advance(unsigned ms){
  if(!open||!ms)return;if(ms>250)ms=250;
  position+=fraction(target-position,clamp(int(ms)*15,0,1000));
  if(target-position>-Unit/2&&target-position<Unit/2)position=target;
  if(!dragging&&!thumb_drag){const int end=clamp(target,0,maximum());target+=fraction(end-target,clamp(int(ms)*12,0,1000));
   if(end-target>-Unit/2&&end-target<Unit/2)target=end;}
 }
 int scale(unsigned row,bool hover,uint32_t now){
  if(row>=Capacity)return 1024;auto&m=motions[row];
  const unsigned age=now-m.at;const unsigned duration=m.hover?50:300;
  const int t=age>=duration?4096:int(age*4096/duration);
  const int easing=m.hover?t:4096-(4096-t)*(4096-t)/4096*(4096-t)/4096;
  const int scale=m.from+((m.hover?1126:1024)-m.from)*easing/4096;
  if(m.hover!=hover){m.hover=hover;m.from=uint16_t(scale);m.at=now;}
  return scale;
 }
};
static_assert(sizeof(BestiaryPresentation)<2304,"Bestiary retains only bounded preview facts, list and clip queues");
}
