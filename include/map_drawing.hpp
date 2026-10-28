#pragma once
#include <stdint.h>

namespace spire {
// NMapDrawings stores ordered pen/eraser strokes on the paper, independently
// of the map camera and simulation. Source coordinates are integer units in
// its 1920 x 3240 control, before the half-size draw viewport. At the 0.22
// calculator projection one source unit is less than a quarter screen pixel.
constexpr unsigned MapDrawingCapacity=2048;
enum class MapDrawingMode:uint8_t {None,Draw,Erase};
struct MapDrawingPoint {
    uint16_t x=0,y_flags=0;
    enum:uint16_t {Start=0x8000,Erase=0x4000,YMask=0x3fff};
    unsigned y()const{return y_flags&YMask;}
    bool start()const{return y_flags&Start;}
    bool erase()const{return y_flags&Erase;}
};
struct MapDrawingDocument {
    uint64_t seed=0;
    uint32_t serial=0;
    uint16_t count=0;
    uint8_t act=0;
    MapDrawingPoint points[MapDrawingCapacity]{};
    bool matches(uint64_t run_seed,uint32_t run_serial,unsigned run_act)const{
        return seed==run_seed&&serial==run_serial&&act==run_act;
    }
    void bind(uint64_t run_seed,uint32_t run_serial,unsigned run_act){
        seed=run_seed;serial=run_serial;act=uint8_t(run_act);count=0;
    }
    bool append(int x,int y,MapDrawingMode mode,bool start){
        if(mode==MapDrawingMode::None||x<0||x>1920||y<0||y>3240||count>=MapDrawingCapacity)return false;
        if(!start&&count){const auto&p=points[count-1];const int dx=x-int(p.x),dy=y-int(p.y());
            if(dx*dx+dy*dy<4)return false;
        }
        points[count++]={uint16_t(x),uint16_t(y|(start?MapDrawingPoint::Start:0)|
            (mode==MapDrawingMode::Erase?MapDrawingPoint::Erase:0))};return true;
    }
};
struct MapDrawingInput {
    MapDrawingMode mode=MapDrawingMode::None;
    bool pressed=false,keyboard=false,dirty=false;
    uint8_t temporary_button=0,held_directions=0;
    int8_t tool_focus=-1;
    int32_t cursor_x=160*256,cursor_y=120*256;
    int paper_origin=0;
    bool transform_valid=false;
    uint32_t revision=1;
    void stop_line(){pressed=false;}
    void cancel(){stop_line();temporary_button=0;mode=MapDrawingMode::None;keyboard=false;held_directions=0;tool_focus=-1;++revision;}
};
// The same inverse/forward transform is used for all strokes. Its scale is a
// parameter so a future camera zoom cannot leave marks fixed to the screen.
struct MapDrawingTransform {
    int origin=0,scale=22,divisor=100;
    int source_x(int screen_x)const{return 960+(screen_x-160)*divisor/scale;}
    int source_y(int screen_y)const{return (screen_y-origin+458)*divisor/scale;}
    int screen_x_q8(unsigned source_x)const{return 160*256+(int(source_x)-960)*scale*256/divisor;}
    int screen_y_q8(unsigned source_y)const{return (origin-458)*256+int(source_y)*scale*256/divisor;}
};
static_assert(sizeof(MapDrawingPoint)==4&&sizeof(MapDrawingDocument)<=8216,"Bounded optional map notes");
} // namespace spire
