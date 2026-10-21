#pragma once
#include <cstdint>
// Generated original rest actors; periods and ordered blend planes only.
namespace spire {
struct WorldRestActor { const char* key; uint32_t duration_ms; uint8_t character,planes,blend[3]; };
constexpr WorldRestActor WorldRestActors[]={
 {"IRONCLAD",6667,0,1,{0,0,0}},
 {"SILENT",8000,1,1,{0,0,0}},
 {"DEFECT",8000,2,3,{0,1,0}},
 {"REGENT",8000,3,1,{0,0,0}},
 {"NECROBINDER",21333,4,1,{0,0,0}},
 {"OSTY",10000,4,1,{0,0,0}},
};
constexpr unsigned WorldRestActorCount=sizeof(WorldRestActors)/sizeof(WorldRestActors[0]);
constexpr unsigned world_rest_actor_frame(uint64_t elapsed,unsigned count,uint32_t duration){
 return count&&duration?unsigned((elapsed%duration)*count/duration):0;
}
}
