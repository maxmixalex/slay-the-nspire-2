#pragma once
#include "room_ui7_presentation.hpp"
#include "card_ui7_presentation.hpp"
#include "game_over_ui.hpp"
namespace spire {
// NGameOverScreen -> NScoreLine -> NBadge -> NRunSummary, excluding the
// deliberately unsupported Timeline unlock bar. All clocks are UI-only.
struct GameOverPresentation {
 struct Item {int offset=0;unsigned alpha=0;bool visible=false;};
 uint64_t run=0,badge_tiers=0; // The 19 immutable tiers occupy 38 bits.
 uint32_t seed=0,opened=0,summary_started=0;
 uint16_t floor=0;
 uint8_t character=0,phase=0,rows=0,badges=0,discoveries=0,quote_from=0;
 bool bound=false,event_room=false,summary=false;
 void reset(){*this={};}
 unsigned intro_delay()const{return event_room?1950u:1750u;}
 uint32_t initial_age(uint32_t now)const{return now-opened;}
 uint32_t summary_age(uint32_t now)const{return now-summary_started;}
 bool continue_ready(uint32_t now)const{return bound&&!summary&&initial_age(now)>=intro_delay();}
 unsigned ui_alpha(uint32_t now)const{const unsigned age=initial_age(now),start=intro_delay()-250;return age<start?0:255*room_curve::linear(age-start,250)/4096;}
 unsigned initial_quote_alpha(uint32_t now)const{const unsigned age=initial_age(now);return age<intro_delay()?0:255*room_curve::linear(age-intro_delay(),1500)/4096;}
 unsigned badge_start()const{return unsigned(rows)*400+750;}
 unsigned discovery_start()const{return badge_start()+unsigned(badges)*250+500;}
 unsigned menu_start()const{return discovery_start()+(discoveries?100+unsigned(discoveries)*300:0);}
 bool menu_ready(uint32_t now)const{return bound&&summary&&summary_age(now)>=menu_start();}
 bool ready(uint32_t now)const{return summary?menu_ready(now):continue_ready(now);}
 unsigned summary_opacity(uint32_t now)const{return summary?255*room_curve::linear(summary_age(now),500)/4096:0;}
 unsigned button_alpha(uint32_t now)const{
  const unsigned age=summary?summary_age(now):initial_age(now),start=summary?menu_start():intro_delay();
  return age<start?0:255*room_curve::linear(age-start,500)/4096;
 }
 int continue_offset(uint32_t now)const{
  const unsigned age=initial_age(now);if(age<intro_delay())return 36;
  const unsigned q=4096-room_curve::linear(age-intro_delay(),500),q2=room_curve::multiply(q,q);
  return int(36*room_curve::multiply(q2,q2)/4096); // Quart out, source +190px.
 }
 int banner_y(uint32_t now)const{return game_over_ui::BannerInitialY-(summary?int(6*room_curve::cubic_out(summary_age(now),500)/4096):0);}
 static Item spring_item(uint32_t age,unsigned start,unsigned duration,int from){
  if(age<start)return {};age-=start;const int q=age>=duration?4096:CardInspectPresentation::spring(age*150/duration);
  return {from*(4096-q)/4096,255*room_curve::linear(age,duration)/4096,true};
 }
 Item score(unsigned index,uint32_t now)const{return summary?spring_item(summary_age(now),index*400,300,-10):Item{};}
 Item badge(unsigned index,uint32_t now)const{return summary?spring_item(summary_age(now),badge_start()+index*250,250,8):Item{};}
 unsigned discovery_header_alpha(uint32_t now)const{const unsigned age=summary_age(now),start=discovery_start();return !summary||!discoveries||age<start?0:255*room_curve::linear(age-start,250)/4096;}
 Item discovery(unsigned index,uint32_t now)const{
  const unsigned age=summary_age(now),start=discovery_start()+100+index*300;if(!summary||age<start)return {};
  return {19*(4096-room_curve::back_out(room_curve::linear(age-start,300)))/4096,255*room_curve::linear(age-start,300)/4096,true};
 }
 unsigned quote_alpha(uint32_t now)const{
  if(!summary)return initial_quote_alpha(now);
  const unsigned age=summary_age(now);if(!quote_from)return 255*room_curve::linear(age,1500)/4096;
  if(age<250)return quote_from*(4096-room_curve::linear(age,250))/4096;
  return age<1250?0:255*room_curve::linear(age-1250,1500)/4096;
 }
 int quote_y(uint32_t now)const{
  const unsigned age=summary?summary_age(now):initial_age(now);const unsigned start=summary?(quote_from?1250u:0u):intro_delay();
  return banner_y(now)+17+(age<start?0:int(13*room_curve::expo_out(age-start,2000)/4096));
 }
 void begin_summary(uint32_t now,unsigned score_rows,unsigned badge_count,unsigned discovery_count){
  quote_from=uint8_t(initial_quote_alpha(now));summary=true;summary_started=now;rows=uint8_t(score_rows);badges=uint8_t(badge_count);discoveries=uint8_t(discovery_count);
 }
};
static_assert(sizeof(GameOverPresentation)<=40,"Terminal presentation must not add large run caches");
}
