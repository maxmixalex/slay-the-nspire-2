#pragma once
#include <stdint.h>
#include "core.hpp"
#include "room_ui7_presentation.hpp"

namespace spire {
// CrystalSphereScreen, 1680x1260, uniformly projected to the calculator.
// The background retains its 1.04 parent scale and centered 2560px width.
namespace crystal_layout {
constexpr int SphereX=43,SphereY=59,SphereW=158,SphereH=157;
constexpr int FogX=42,FogY=57,FogW=159,FogH=160;
constexpr int GridX=61,GridY=77;
constexpr int cell_edge(int n){return (6100+n*1129+50)/100;}
constexpr int cell_top(int n){return (7700+n*1129+50)/100;}
constexpr bool exists(int x,int y){return x>=0&&y>=0&&x<11&&y<11&&
 (x<5?x:10-x)+(y<5?y:10-y)>=3;}
inline int nearest(const OriginalEventState&e,int source){
 int best=-1,distance=1000;
 // Original cell children are created x-major; stable ties retain that order.
 for(int x=0;x<11;++x)for(int y=0;y<11;++y){const int at=y*11+x;
  if(!exists(x,y)||e.crystal_clear[at])continue;
  const int dx=x-source%11,dy=y-source/11,score=dx*dx+dy*dy;
  if(score<distance){distance=score;best=at;}
 }return best;
}
}

struct CrystalPresentation {
 uint64_t seed=0;uint32_t start=0,dialogue_start=0,random=0x7f48a25du;
 uint32_t cell_start[CrystalCells]{},item_start[MaxCrystalItems]{},hover_start=0;
 uint8_t cells[CrystalCells]{},items[MaxCrystalItems]{},hover_from[CrystalCells]{};
 uint16_t point=0xffff;int16_t hover=-1,dialogue_x=218;
 uint8_t act=255,tool=0,dialogue=0,line=1;bool bound=false,finished=false,started=false;
 void speak(unsigned kind,uint32_t now){
  random^=random<<13;random^=random>>17;random^=random<<5;
  dialogue=uint8_t(kind);dialogue_start=now;line=uint8_t(1+random%(kind==2?5:kind==3?3:2));
  dialogue_x=int16_t((110000+int(random%10001))*4/2100);
 }
 bool observe(const State&s,uint32_t now){
  const auto&e=s.original_events;bool fresh=!bound||seed!=s.world.run_seed||point!=s.world.current_point||act!=s.world.act_index;
  if(fresh){*this=CrystalPresentation{};bound=true;seed=s.world.run_seed;point=s.world.current_point;act=s.world.act_index;start=now;
   for(unsigned i=0;i<CrystalCells;++i)cells[i]=e.crystal_clear[i];
   for(unsigned i=0;i<MaxCrystalItems;++i)items[i]=e.crystal_items[i].revealed;
  }
  if(!started&&now-start>=500){started=true;speak(e.finished?4:1,now);}
  for(unsigned i=0;i<CrystalCells;++i)if(!cells[i]&&e.crystal_clear[i]){cells[i]=1;cell_start[i]=now?now:1;}
  for(unsigned i=0;i<e.crystal_item_count&&i<MaxCrystalItems;++i)if(!items[i]&&e.crystal_items[i].revealed){
   items[i]=1;item_start[i]=now?now:1;speak(e.crystal_items[i].kind==1?3:2,now);
  }
  if(e.finished&&!finished){finished=true;if(started)speak(4,now);}
  return fresh;
 }
 unsigned cell_alpha(unsigned i,uint32_t now)const{
  if(i>=CrystalCells||!cells[i])return 255;
  return !cell_start[i]?0:255-255*room_curve::linear(now-cell_start[i],500)/room_curve::Q;
 }
 bool highlighted(unsigned at,int center,unsigned size)const{
  if(center<0||!crystal_layout::exists(int(at%11),int(at/11)))return false;
  const int dx=int(at%11)-center%11,dy=int(at/11)-center/11;
  return size==2?dx>=-1&&dx<=1&&dy>=-1&&dy<=1:int(at)==center;
 }
 unsigned hover_alpha(unsigned at,uint32_t now)const{
  return unsigned(room_curve::interpolate(hover_from[at],highlighted(at,hover,tool)?128:0,room_curve::linear(now-hover_start,150)));
 }
 void focus(int next,unsigned next_tool,uint32_t now){
  if(hover==next&&tool==next_tool)return;
  for(unsigned i=0;i<CrystalCells;++i)hover_from[i]=uint8_t(hover_alpha(i,now));
  hover=int16_t(next);tool=uint8_t(next_tool);hover_start=now;
 }
 unsigned screen_alpha(uint32_t now)const{return 255*room_curve::linear(now-start,500)/room_curve::Q;}
 unsigned item_brightness(unsigned i,uint32_t now)const{
  if(!items[i]||!item_start[i])return 255;
  const uint32_t age=now-item_start[i];return age<250?255:255*room_curve::linear(age-250,500)/room_curve::Q;
 }
 unsigned item_scale(unsigned i,uint32_t now)const{
  if(!items[i]||!item_start[i])return 1024;const uint32_t age=now-item_start[i];
  if(age<250||age>=900)return 1024;
  return age<400?1024+205*room_curve::linear(age-250,150)/room_curve::Q:1229-205*room_curve::linear(age-400,500)/room_curve::Q;
 }
 unsigned dialogue_alpha(uint32_t now)const{
  const uint32_t age=now-dialogue_start;if(!dialogue||age>=2500)return 0;
  return age<250?age*255/250:age<2000?255:255-255*room_curve::sine_out(age-2000,500)/room_curve::Q;
 }
};
static_assert(sizeof(CrystalPresentation)<1536,"Crystal animation must remain a small presentation-only record");
}
