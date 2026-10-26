#pragma once
#include "core.hpp"
#include "health_bar.hpp"
#include "original_events.hpp"
#include "reference_viewport.hpp"
#include "world_run.hpp"

namespace spire {
// Preserve the historical HudInfo floor id. Keyboard positions follow the
// source neighbor chain and therefore place Room before Floor.
enum HudInfoItem {HudHp,HudGold,HudFloor,HudRoom,HudBoss,HudPortrait,HudTimer};
constexpr int HudRelicFocusFirst=32;
// One geometry owns bottles, focus, popup anchors and acquisition flashes.
// The canonical inventory supports sixteen slots; five remain visible at once.
namespace hud_potion_layout {
constexpr int Visible=5,Left=114,PageWidth=6;
inline unsigned pages(unsigned slots){return (slots+Visible-1)/Visible;}
inline int cell(unsigned slots){return slots>=5?13:slots==4?14:16;}
inline int page_x(unsigned slots){return Left+int(slots<Visible?slots:Visible)*cell(slots);}
inline int width(unsigned slots){return page_x(slots)-Left+(slots>Visible?PageWidth:0);}
inline int x(unsigned slot,unsigned slots){return Left+int(slot%Visible)*cell(slots);}
inline int center_x(unsigned slot,unsigned slots){return x(slot,slots)+cell(slots)/2;}
inline int center_y(){return 11;}
inline int landing_x(unsigned slot,unsigned page,unsigned slots){return slot/Visible==page?center_x(slot,slots):page_x(slots)+PageWidth/2;}
}
inline bool hud_portrait_enabled(const State&s){return world_ascension(s)>0||s.run_options.mode!=RunMode::Standard;}
struct HudRoomInfo {
    const char* room_key=nullptr;
    const char* room_tip="ROOM_MAP";
    EncounterId boss=EncounterId::None,second_boss=EncounterId::None;
    bool room_enabled=false,boss_enabled=false,second_placeholder=false;
};
inline HudRoomInfo hud_room_info(const State&s){
    HudRoomInfo out;const auto&w=s.world;
    if(!w.active||w.act_index>=WorldActCount)return out;
    // AbstractRoom.IsVictoryRoom is TheArchitect, including its event before
    // the final summary. It does not mean every dead/finished run.
    if(w.final_event)return out;
    const auto&map=w.maps[w.act_index];const auto&rooms=w.rooms[w.act_index];
    const auto base=WorldRoom(w.event_combat_parent?w.parent_room_type:w.room_type);
    const auto point=w.current_point<map.count?map.points[w.current_point].type:reference::PointType::Unassigned;
    out.room_enabled=true;
    switch(point){
    case reference::PointType::Unknown:
        if(base==WorldRoom::Monster){out.room_key="UNKNOWN_MONSTER";out.room_tip="ROOM_UNKNOWN_ENEMY";}
        else if(base==WorldRoom::Elite){out.room_key="UNKNOWN_ELITE";out.room_tip="ROOM_UNKNOWN_ELITE";}
        else if(base==WorldRoom::Treasure){out.room_key="UNKNOWN_TREASURE";out.room_tip="ROOM_UNKNOWN_TREASURE";}
        else if(base==WorldRoom::Shop){out.room_key="UNKNOWN_SHOP";out.room_tip="ROOM_UNKNOWN_MERCHANT";}
        else {out.room_key="EVENT";out.room_tip="ROOM_UNKNOWN_EVENT";}
        break;
    case reference::PointType::Shop:out.room_key="SHOP";out.room_tip="ROOM_MERCHANT";break;
    case reference::PointType::Treasure:out.room_key="TREASURE";out.room_tip="ROOM_TREASURE";break;
    case reference::PointType::Rest:out.room_key="REST_SITE";out.room_tip="ROOM_REST";break;
    case reference::PointType::Monster:out.room_key="MONSTER";out.room_tip="ROOM_ENEMY";break;
    case reference::PointType::Elite:out.room_key="ELITE";out.room_tip="ROOM_ELITE";break;
    case reference::PointType::Start:out.room_key=original_event_key(OriginalEventId(rooms.ancient));out.room_tip="ROOM_ANCIENT";break;
    case reference::PointType::Boss:{
        const unsigned index=w.current_point==map.second_boss&&rooms.boss_count>1?1:0;
        const auto*definition=rooms.boss_count?encounter_definition(rooms.bosses[index]):nullptr;
        out.room_key=definition?definition->key:nullptr;out.room_tip="ROOM_BOSS";break;
    }
    default:break;
    }
    if(base==WorldRoom::Map)out.room_key=nullptr;
    const bool double_boss=map.second_boss!=reference::NoPoint&&rooms.boss_count>1;
    const bool second_only=double_boss&&w.current_point==map.boss;
    out.boss_enabled=rooms.boss_count&&(base!=WorldRoom::Boss||second_only);
    if(rooms.boss_count)out.boss=rooms.bosses[second_only?1:0];
    if(double_boss&&!second_only){out.second_boss=rooms.bosses[1];out.second_placeholder=w.current_point!=map.boss&&w.current_point!=map.second_boss;}
    return out;
}
struct HudRoomGeometry {int room,floor,boss,timer;};
inline HudRoomGeometry hud_room_geometry(const State&s){
    // Equal four-pixel gaps separate potion belt, room, floor, boss and
    // timer. Shrink only the fourth/fifth potion cell, keeping all five
    // bottles available without consuming the metadata's spacing.
    const auto info=hud_room_info(s);const int room=hud_potion_layout::Left+hud_potion_layout::width(potion_slot_count(s))+4;
    const int floor=room+14,boss=floor+26;
    return {room,floor,boss,info.boss_enabled?boss+(info.second_boss==EncounterId::None?11:17)+4:boss};
}
namespace hud_relic_layout {
constexpr int Columns=(reference_viewport::Width-12-84)/68;
constexpr int Size=reference_viewport::project(68),Icon=reference_viewport::project(60);
// The port keeps its 22px readable top bar; the inventory's 68px cells,
// no-spacing wrap, margins and relative icon geometry use the source scale.
inline int x(unsigned position){return reference_viewport::project(12+int(position%Columns)*68);}
inline int y(unsigned position){return 22+reference_viewport::project(int(position/Columns)*68);}
inline int center_x(unsigned position){return reference_viewport::project(46+int(position%Columns)*68);}
inline int center_y(unsigned position){return 22+reference_viewport::project(34+int(position/Columns)*68);}
}
struct HudRelicMotion {
    struct Holder {uint32_t start=0;int16_t from=4096;uint16_t identity=0;bool focused=false;} holders[WorldRelicCapacity]{};
    static int scale(const Holder&v,uint32_t now){
        const unsigned age=now-v.start;
        if(v.focused)return age>=50?5120:v.from+(5120-v.from)*int(age)/50;
        return age>=1000?4096:4096+(v.from-4096)*int(4096-health_bar::expo_out(age))/4096;
    }
    int observe(unsigned position,uint16_t identity,bool focused,uint32_t now){
        if(position>=WorldRelicCapacity)return 4096;auto&v=holders[position];
        if(v.identity!=identity){v={};v.identity=identity;v.start=now;}
        if(v.focused!=focused){v.from=int16_t(scale(v,now));v.focused=focused;v.start=now;}
        return scale(v,now);
    }
};
}
