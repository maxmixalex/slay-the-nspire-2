#pragma once
#include <stdint.h>
#include "health_bar.hpp"

namespace spire {
// A detached compendium viewer. These cursors, clips and scroll positions never
// belong to State, a gameplay RNG stream or the run save codec.
struct BestiaryPresentation {
 static constexpr int Capacity=112,RowHeight=14,Top=29,Bottom=211,Left=230,Right=303,Unit=256;
 struct RowMotion {uint32_t at=0;uint16_t from=1024;bool hover=false;};
 struct Preview {
  uint8_t model=0,skin=0,count=0,current=0;
  int16_t state=-1,clips[12]{};
  uint32_t start=0;
  int16_t idle=-1;
  bool hold=false;
 } actors[3]{};
 int16_t rows[Capacity]{},row_count=0,selected=-1,selected_row=-1,focused=-1;
 RowMotion motions[Capacity]{};
 uint32_t selected_at=0,mode_at=0,filter_at=0,arrow_at=0;
 int32_t position=0,target=0,arrow_from=0;
 uint8_t actor_count=0,action=0,filter=0,focus=0;
 bool open=false,actions=true,dragging=false,thumb_drag=false;
 int16_t drag_y=0;
 static int clamp(int value,int low,int high){return value<low?low:value>high?high:value;}
 static int fraction(int value,int thousandths){return value/1000*thousandths+value%1000*thousandths/1000;}
 int maximum()const{return row_count*RowHeight>Bottom-Top?(row_count*RowHeight-(Bottom-Top))*Unit:0;}
 int row_y(int row)const{return Top+row*RowHeight-position/Unit;}
 int arrow_y(uint32_t now)const{
  if(selected_row<0)return Top;
  const unsigned age=now-arrow_at;const int target_y=selected_row*RowHeight*Unit;
  const int eased=age>=250?4096:int(health_bar::expo_out(age*4));
  return Top+(arrow_from+(target_y-arrow_from)*eased/4096-position)/Unit+RowHeight/2;
 }
 void reveal(int row){
  if(row<0||row>=row_count)return;
  const int low=row*RowHeight*Unit,high=(row+1)*RowHeight*Unit-(Bottom-Top)*Unit;
  if(target>low)target=low;else if(target<high)target=high;
  target=clamp(target,0,maximum());
 }
 void wheel(int rows){target=clamp(target+clamp(rows,-64,64)*80*Unit/9,-(Bottom-Top)*Unit,maximum()+(Bottom-Top)*Unit);}
 int thumb_y()const{return Top+8+(maximum()?clamp(thumb_drag?target:position,0,maximum())*(Bottom-Top-16)/maximum():0);}
 void drag_thumb(int y){target=maximum()*clamp(y-Top-8,0,Bottom-Top-16)/(Bottom-Top-16);}
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
static_assert(sizeof(BestiaryPresentation)<2048,"Bestiary retains only a bounded list and clip queue");
}
