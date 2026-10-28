#pragma once
#include "world_run.hpp"
#include "world_card_factory.hpp"

namespace spire {
// NMapScreen's authored coordinates, scaled uniformly for the calculator.
// These helpers only own presentation RNG copies. Calling them never advances
// a saved run stream, even when inspecting a map during combat or a choice.
// The source map fills the screen behind the top HUD. Do not reserve
// extra strips for a duplicated act title or keyboard instructions.
constexpr int WorldMapTop=22,WorldMapBottom=240;
constexpr uint32_t WorldMapScaleBits=0x3e6147aeU; // binary32 0.22
// Points and paper share the viewport center. Moving only the graph to make
// space for the legend misregisters every route against its authored paper.
constexpr int WorldMapGraphCenter=160;
struct WorldMapPosition {int16_t x=0,y=0;int8_t angle=0;};
struct WorldMapLayout {
    WorldMapPosition points[reference::MaxPoints]{};
    int row_step=34,top=0,bottom=0;
    uint32_t jitter_calls=0;
};
struct WorldMapView {int origin=0,natural=0,minimum=0,maximum=0,scroll=0;};
inline int world_map_round(reference::math::Real x){
    const auto half=world_float(0x3f000000U);return world_float_truncate(x+(x.bits>>63?-half:half));
}
inline reference::math::Real world_map_random_float(reference::Random&r,reference::math::Real lo,reference::math::Real hi){
    return world_float_round(reference::math::Real::fraction53(r.next()>>11)*world_float_round(hi-lo)+lo);
}
inline int world_map_half_size(const reference::ReferenceMap&m,unsigned point){
    return point==m.boss||point==m.second_boss?(m.config.second_boss?38:50):point==m.root?24:18;
}
inline bool world_map_layout(const State&s,WorldMapLayout&out){
    using reference::math::Real;
    out={};const auto*m=world_current_map(s);if(!m||m->config.rows<2||m->count>reference::MaxPoints||m->root>=m->count)return false;
    const Real scale=world_float(WorldMapScaleBits),factor=m->config.second_boss?world_float(0x3f666666U):Real::integer(1);
    const Real dy=world_float_round(world_float_round(Real::integer(2325)/Real::integer(m->config.rows-1))*factor);
    out.row_step=world_map_round(world_float_round(dy*scale));
    char stream[]="map_jitter_0";stream[11]=char('0'+s.world.act_index);
    reference::Random jitter;jitter.named(s.world.run_seed,stream);
    // Original rotation/dot noise uses process-global Rng.Chaotic. Stable local
    // cosmetic noise keeps calculator redraws still; it is not gameplay RNG or
    // a claim that the original process-global rotations can be reproduced.
    reference::Random angles;angles.named(s.world.run_seed,"nspire_map_angles");
    for(unsigned col=0;col<reference::Columns;++col)for(unsigned row=0;row<m->config.rows;++row){
        const unsigned id=m->grid[col][row];if(id==reference::NoPoint||id>=m->count||!m->points[id].active)continue;
        const Real x=world_float_round(world_float_round(Real::integer(int(col)*150-500)+world_random_float(jitter,-21,21))+Real::integer(28));
        const Real y=world_float_round(world_float_round(world_float_round(world_float_round(Real::integer(row)*-dy)+Real::integer(740))+world_random_float(jitter,-25,25))+Real::integer(28));
        auto&p=out.points[id];p.x=int16_t(WorldMapGraphCenter+world_map_round(world_float_round(x*scale)));p.y=int16_t(world_map_round(world_float_round(y*scale)));
        int noise=0;for(unsigned n=0;n<12;++n)noise+=int(angles.below(257))-128;
        p.angle=int8_t(noise/32); // twelve uniforms: approximately sigma 8 degrees
    }
    out.jitter_calls=jitter.calls;
    auto place=[&](unsigned id,int x,Real y){if(id<m->count)out.points[id]={int16_t(WorldMapGraphCenter+world_map_round(world_float_round(Real::integer(x)*scale))),int16_t(world_map_round(world_float_round(y*scale))),0};};
    // Boss Control scales about its (187,153) pivot, so even A10's 0.75
    // scaling does not move the center used by markers, paths and selection.
    place(m->boss,-13,world_float_round(world_float_round(Real::integer(-1980)*factor)+Real::integer(153)));
    place(m->second_boss,-13,world_float_round(world_float_round(Real::integer(-2280)*factor)+Real::integer(153)));
    const auto&root=m->points[m->root];
    place(m->root,root.type==reference::PointType::Start?24:-52,
          world_float_round(world_float_round(Real::integer(root.row)*-dy)+Real::integer(root.type==reference::PointType::Start?824:828)));
    bool first=true;
    for(unsigned i=0;i<m->count;++i)if(m->points[i].active){
        int top=out.points[i].y-world_map_half_size(*m,i)-18,bottom=out.points[i].y+world_map_half_size(*m,i);
        if(first||top<out.top)out.top=top;if(first||bottom>out.bottom)out.bottom=bottom;first=false;
    }
    return !first;
}
inline WorldMapView world_map_view(const State&s,const WorldMapLayout&layout,int requested){
    WorldMapView view;const auto*m=world_current_map(s);if(!m)return view;
    const unsigned current=s.world.current_point<m->count?s.world.current_point:m->root;
    view.minimum=WorldMapBottom-layout.bottom;view.maximum=WorldMapTop-layout.top;
    if(view.maximum<view.minimum)view.maximum=view.minimum;
    const int natural=WorldMapBottom-world_map_half_size(*m,current)-layout.points[current].y;
    view.natural=natural<view.minimum?view.minimum:natural>view.maximum?view.maximum:natural;
    const int step=layout.row_step>0?layout.row_step:1;
    const int low=-((view.natural-view.minimum+step-1)/step),high=(view.maximum-view.natural+step-1)/step;
    view.scroll=requested<low?low:requested>high?high:requested;
    view.origin=view.natural+view.scroll*step;
    if(view.origin<view.minimum)view.origin=view.minimum;if(view.origin>view.maximum)view.origin=view.maximum;
    return view;
}
inline reference::PointType world_map_visible_type(const State&s,unsigned point){
    const auto*m=world_current_map(s);if(!m||point>=m->count)return reference::PointType::Unassigned;
    const auto type=m->points[point].type;
    if(type!=reference::PointType::Unknown||!s.world.visited[s.world.act_index][point])return type;
    switch(WorldRoom(s.world.visited_rooms[s.world.act_index][point])){
    case WorldRoom::Monster:return reference::PointType::Monster;
    case WorldRoom::Elite:return reference::PointType::Elite;
    case WorldRoom::Treasure:return reference::PointType::Treasure;
    case WorldRoom::Shop:return reference::PointType::Shop;
    default:return reference::PointType::Unknown;
    }
}
inline unsigned world_map_clip_duration(EncounterId id){
    switch(id){case EncounterId::CeremonialBeastBoss:return 6000;case EncounterId::QueenBoss:return 10300;case EncounterId::TheInsatiableBoss:return 10667;default:return 0;}
}
inline unsigned world_map_clip_frame(uint32_t milliseconds,unsigned count,unsigned duration){
    // Pinned map clips have at most 214 frames and a 10,667 ms duration.
    return count&&count<=256&&duration&&duration<=60000?(milliseconds%duration)*count/duration:0;
}
}
