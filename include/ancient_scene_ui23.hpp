#pragma once
#include <stdint.h>
#include "retired_assets_ui22.hpp"

namespace spire::ancient_scene_ui23 {
constexpr unsigned AuxiliaryFirst=retired_assets_ui22::AuxiliaryCount;
constexpr unsigned AuxiliaryCount=AuxiliaryFirst+2,StepMs=33;
struct Timeline {
    unsigned first=0,count=0,duration=0;
    static unsigned word(const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
    template<class Picture> bool read(Picture p){
        if(!p.pixels||p.w!=8||p.h!=1||p.format||p.alpha||p.palette)return false;
        const auto*d=p.pixels;
        first=word(d+4);count=word(d+6);duration=word(d+10)|(word(d+12)<<16);
        return word(d)==0x414e&&word(d+2)==1&&word(d+8)==StepMs&&!word(d+14)&&
            duration&&duration<=120000&&count==(duration+StepMs-1)/StepMs&&
            first+count<=65536;
    }
    unsigned frame(uint32_t elapsed,bool reduced)const{return reduced?0:(elapsed%duration)/StepMs;}
};
}
