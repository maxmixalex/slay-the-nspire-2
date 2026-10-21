#pragma once
#include "world_map_layout.hpp"

namespace spire {
// Original single-player map effects. This storage belongs to App only: it
// neither owns a route decision nor advances a saved gameplay RNG stream.
// All deadlines use the unscaled, wrapping host millisecond clock.
struct MapPointPresentation {
    uint32_t circle_start=0,path_start=0;
    uint16_t path_ms=0;
    uint8_t circle_mode=0,path_from=reference::NoPoint;
};
struct MapCircleSample {unsigned frame=4,alpha=0,scale=4096;};
struct MapPathDotSample {unsigned scale=4096;bool traveled=false,fresh=false;};
struct MapMarkerSample {
    unsigned point=reference::NoPoint,scale_x=4096,scale_y=4096;
    int source_y=0; // Q12 source pixels; the map projects these at 0.22.
};

struct MapPresentationState {
    uint64_t seed=0;
    uint32_t shape=0,marker_start=0,travel_start=0;
    uint16_t travel_path_ms=0;
    uint8_t act=255,marker_point=reference::NoPoint,marker_mode=0;
    uint8_t travel_from=reference::NoPoint,travel_to=reference::NoPoint;
    bool pending=false,marker_waiting=false,map_open=false;
    MapPointPresentation points[reference::MaxPoints]{};

    void reset();
    void bind(const State&,uint32_t now);
    void begin(const State&,unsigned from,unsigned to,uint32_t now,unsigned path_ms);
    void update(const State&,uint32_t now);
    void committed(const State&,uint32_t now);
    void cancel(const State&,uint32_t now);
    void set_open(const State&,bool visible,uint32_t now);
    void initial_marker(unsigned point,uint32_t now);
    bool traveling()const{return pending;}
    unsigned target()const{return pending?travel_to:reference::NoPoint;}
    bool traveled(const State&,unsigned point)const;
    MapCircleSample circle(unsigned point,uint32_t now)const;
    MapPathDotSample path_dot(const State&,unsigned from,unsigned to,unsigned index,
                             unsigned count,uint32_t now)const;
    MapMarkerSample marker(uint32_t now)const;

private:
    void finish_marker();
    void set_marker(unsigned point,uint32_t now);
};

// This shares the renderer's existing projected path geometry. It returns
// zero for free travel without an authored graph edge and consumes no RNG.
unsigned world_map_path_dot_count(const State&,unsigned from,unsigned to);
}
