#pragma once
#include "card_ui7_presentation.hpp"
#include "room_ui7_presentation.hpp"

namespace spire {
enum class RelicInspectSource:uint8_t { Single,Owned,Collection };
namespace relic_inspect_layout {
// The source paper/frame hierarchy occupies the left reading pane; related
// keywords retain the separate 104px column on the right of the LCD.
constexpr int PopupX=8,PopupY=8,PopupWidth=194,PopupHeight=224;
constexpr int NameX=20,NameY=18,NameWidth=170;
constexpr int RarityX=20,RarityY=46,RarityWidth=170;
constexpr int FrameX=68,FrameY=60,FrameSize=72,IconX=77,IconY=69,IconSize=54;
constexpr int BodyX=22,BodyY=139,BodyWidth=158,BodyHeight=69;
constexpr int LeftX=0,RightX=188,ArrowY=89,ArrowSize=24,Travel=19;
}
struct RelicInspectPresentation {
    RelicInspectSource source=RelicInspectSource::Single;
    uint32_t opened=0,slid=0,closed=0;
    int16_t model=-1,owned=-1,index=0,count=1;
    int16_t closed_left=relic_inspect_layout::LeftX,closed_right=relic_inspect_layout::RightX,scroll_maximum=0;
    int8_t direction=0;
    uint8_t backstop_from=255,closed_backstop=0,closed_popup=0,closed_brightness=0,closed_arrows=0;
    bool bound=false,ever_opened=false,exiting=false,reading_keywords=false;
    CardInspectPresentation::Arrow arrows[2]{};
    unsigned age(uint32_t now)const{return now-opened;}
    unsigned linear(unsigned elapsed,unsigned duration)const{return elapsed>=duration?255:elapsed*255/duration;}
    unsigned fade(uint32_t now,unsigned duration,unsigned initial)const{
        const unsigned elapsed=now-closed;return elapsed>=duration?0:initial*(duration-elapsed)/duration;
    }
    unsigned backstop(uint32_t now)const{
        if(exiting)return fade(now,250,closed_backstop);
        return unsigned(room_curve::interpolate(backstop_from,230,room_curve::linear(age(now),250)));
    }
    unsigned popup_alpha(uint32_t now)const{return exiting?fade(now,100,closed_popup):direction?255:linear(age(now),250);}
    unsigned popup_brightness(uint32_t now)const{
        if(!exiting)return direction?255:linear(age(now),250);
        return unsigned(room_curve::interpolate(closed_brightness,255,room_curve::linear(now-closed,100)));
    }
    unsigned arrow_alpha(uint32_t now)const{return exiting?fade(now,100,closed_arrows):linear(age(now)>100?age(now)-100:0,250);}
    unsigned arrow_brightness(uint32_t now)const{return exiting?closed_arrows:arrow_alpha(now);}
    int x(uint32_t now)const{return relic_inspect_layout::PopupX+(!direction?0:direction*relic_inspect_layout::Travel*int(4096-room_curve::expo_out(now-slid,250))/4096);}
    int y(uint32_t now)const{return relic_inspect_layout::PopupY+(direction?0:38*int(4096-room_curve::cubic_out(age(now),250))/4096);}
    int arrow_x(unsigned side,uint32_t now)const{
        if(exiting)return side?closed_right:closed_left;
        const unsigned elapsed=age(now)>100?age(now)-100:0;
        const int shift=relic_inspect_layout::Travel*(4096-room_curve::back_out(room_curve::linear(elapsed,250)))/4096;
        return side?relic_inspect_layout::RightX-shift:relic_inspect_layout::LeftX+shift;
    }
    bool closing(uint32_t now)const{return bound&&exiting&&now-closed<250;}
    void reset(){const bool opened_before=ever_opened;*this=RelicInspectPresentation{};ever_opened=opened_before;}
    void begin(uint32_t now){
        const unsigned from=exiting?backstop(now):ever_opened?0:255;
        *this=RelicInspectPresentation{};
        opened=now;backstop_from=uint8_t(from);ever_opened=bound=true;
    }
    void close(uint32_t now){
        if(!bound||exiting)return;
        closed_backstop=uint8_t(backstop(now));closed_popup=uint8_t(popup_alpha(now));closed_brightness=uint8_t(popup_brightness(now));closed_arrows=uint8_t(arrow_alpha(now));
        closed_left=int16_t(arrow_x(0,now));closed_right=int16_t(arrow_x(1,now));closed=now;exiting=true;
    }
};
static_assert(sizeof(RelicInspectPresentation)<=80,"Relic inspector keeps only bounded transient presentation");
}
