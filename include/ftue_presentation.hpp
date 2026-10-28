#pragma once
#include "ftue.hpp"
#include "run_options.hpp"
namespace spire {
// Modal state is a presentation projection. None of its clocks, targets or
// pending notices enter State, the run RNG, or the run-save format.
struct FtuePresentation {
 FtueId id=FtueId::Count;
 uint32_t opened=0,page_started=0,serial=0,room_since=0,shuffle_at=0;
 uint32_t closing_at=0,banner_at=0;
 uint16_t backstop_from=0,closing_alpha=0;
 uint64_t room_seed=0;
 uint16_t pending=0,manual_cards=0;
 uint16_t encounter=0;
 uint16_t shuffle_draw=0,shuffle_discard=0,shuffle_exhaust=0;
 uint16_t text_scroll=0,text_maximum=0,text_rows=0;
 int32_t text_from=0,text_target=0;
 uint32_t text_started=0;
 uint8_t page=0,focus=1,room_act=255,room_point=255,room_type=255,no_play_turns=0;
 int16_t target=-1;
 int8_t page_direction=0;
 bool shuffle_waiting=false,skip_end_turn_check=false;
 bool closing=false,banner=false,banner_player=false;
 RunOptions embark{};
 bool active()const{return id!=FtueId::Count;}
 uint32_t age(uint32_t now)const{return now-opened;}
 int text_offset(uint32_t now)const{
  const uint32_t age=now-text_started;if(age>=180)return text_target;
  const int inverse=4096-int(age*4096/180),q=4096-inverse*inverse/4096;
  return text_from+(int(text_target)-text_from)*q/4096;
 }
 void scroll_text(int pixels,uint32_t now){if(pixels==text_target)return;text_from=text_offset(now);text_target=pixels;text_started=now;}
 void reset_text(){text_scroll=text_maximum=text_rows=0;text_from=text_target=0;text_started=0;}
 void clear(){id=FtueId::Count;pending=0;target=-1;reset_text();shuffle_waiting=closing=banner=banner_player=false;no_play_turns=0;encounter=UINT16_MAX;room_seed=0;room_act=room_point=room_type=255;++serial;}
};
static_assert(sizeof(FtuePresentation)<=128,"Tutorial modals require no framebuffer or State copy");
}
