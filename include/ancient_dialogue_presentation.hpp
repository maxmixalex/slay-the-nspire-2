#pragma once
#include <stdint.h>
#include "room_ui7_presentation.hpp"
namespace spire {
// Original dialogue is UI-only. Keys reference the installed original locale;
// advancing or reviewing a line never consumes a gameplay RNG or save field.
struct AncientDialoguePresentation {
 static constexpr unsigned Capacity=8,KeyCapacity=96;
 uint64_t seed=0;
 uint32_t serial=0,revision=0,start=0,entered=0,hover_start=0,random=0x41cef1eau;
 uint16_t id=0,heights[Capacity]{},option_heights[3]{};
 uint8_t act=255,point=255,character=255,profile=255,count=0,current=0,character_lines=0;
 bool bound=false,finished=false,measured=false,laid_out=false,english=false,history_complete=true,history_hovered=false;
 uint8_t hover_from=64;
 int16_t from_y=232,target_y=232,scroll=0,max_scroll=0;
 char keys[Capacity][KeyCapacity]{};
 bool last()const{return !count||current+1>=count;}
 int y(uint32_t now)const{return room_curve::interpolate(from_y,target_y,room_curve::expo_out(now-start,1000));}
 unsigned choose(unsigned count){random^=random<<13;random^=random>>17;random^=random<<5;return count?random%count:0;}
 void move_to(int destination,uint32_t now,bool initial=false){
  // Source completes an interrupted content tween before starting the next.
  from_y=initial?(count?217:232):target_y;target_y=int16_t(destination);start=now;scroll=0;
 }
 unsigned history_alpha(uint32_t now)const{return unsigned(room_curve::interpolate(hover_from,history_hovered?255:64,room_curve::linear(now-hover_start,100)));}
 void hover_history(bool next,uint32_t now){if(next==history_hovered)return;hover_from=uint8_t(history_alpha(now));hover_start=now;history_hovered=next;}
};
static_assert(sizeof(AncientDialoguePresentation)<896,"Bounded ancient dialogue presentation");
}
