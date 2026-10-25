#pragma once
#include <stdint.h>

namespace spire::character_particles {
// Visible CPUParticles2D systems in the pinned char_select_bg_*.tscn scenes.
// The 300 subpixel Regent stars are capped at48 independently aged samples.
// Selection owns the cosmetic seed; no run random stream is consumed.
struct Emitter {
    int16_t x,y,extent_x,extent_y,gravity_x,gravity_y;
    uint16_t life;
    uint8_t count,size,red,green,blue,alpha;
};
constexpr Emitter Systems[5][2]={
    {{-14,1163,800,500,50,-27,15000,20,2,255,174,109,185},
     {145,1163,800,500,50,-27,15000,20,1,255,210,170,156}},
    {{-107,1361,800,500,20,-10,15000,20,2,128,179,69,35},
     {-107,1361,800,500,20,-10,15000,20,1,128,179,69,110}},
    {{0,771,2000,1,0,-50,10000,12,2,22,54,80,255},{}},
    {{1382,307,1500,1000,0,-10,3000,48,1,224,127,100,156},{}},
    {{2054,1163,250,250,-50,-75,15000,20,3,255,33,255,190},
     {2054,1163,250,250,-100,-150,15000,20,1,255,90,172,230}}
};
struct Sample {int x,y,size;unsigned alpha;};
inline uint32_t hash(uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
inline int project(int value,bool vertical){
    const int pivot=vertical?600:1280,origin=vertical?-80:-388;
    return ((value-pivot)*12683/10000+pivot+origin)*4/21;
}
inline Sample sample(const Emitter&e,unsigned index,unsigned character,unsigned system,uint32_t elapsed,uint32_t seed){
    uint32_t random=hash(seed+index*0x9e3779b9u+system*0x8da6b343u+character*0x45d9f3bu);
    const unsigned age=(elapsed+random%e.life)%e.life;
    int x=e.x+int((random>>16)%(2u*e.extent_x+1))-e.extent_x;
    random=hash(random);int y=e.y+int(random%(2u*e.extent_y+1))-e.extent_y;
    if(character==3){
        // The stars' source node is rotated and vertically compressed.
        const int dx=x-e.x,dy=y-e.y;
        x=e.x+(dx*931+dy*119)/1024;y=e.y+(-dx*397+dy*283)/1024;
    }
    const int64_t squared=int64_t(age)*age;
    x+=int(squared*e.gravity_x/2000000);y+=int(squared*e.gravity_y/2000000);
    const unsigned fade=age<e.life/5?age*1275/e.life:age>e.life*4/5?(e.life-age)*1275/e.life:255;
    return {project(x,false),project(y,true),e.size,unsigned(e.alpha)*fade/255};
}
}
