#pragma once
#include <stdint.h>
#include "core.hpp"
#include "room_ui7_presentation.hpp"
namespace spire {
struct AncientBannerPresentation {
 uint64_t seed=0;uint32_t start=0;uint16_t id=0;uint8_t act=255,point=255;bool bound=false;
 void observe(const State&s,uint32_t now){
  if(bound&&seed==s.world.run_seed&&act==s.world.act_index&&point==s.world.current_point&&id==unsigned(s.original_events.id))return;
  seed=s.world.run_seed;act=s.world.act_index;point=s.world.current_point;id=unsigned(s.original_events.id);start=now;bound=true;
 }
 static unsigned circ(uint32_t age,uint32_t duration){
  const unsigned q=room_curve::linear(age,duration),v=room_curve::Q-q;
  unsigned remainder=room_curve::Q*room_curve::Q-v*v,result=0,bit=1u<<24;
  while(bit>remainder)bit>>=2;
  while(bit){if(remainder>=result+bit){remainder-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;
 }
 static unsigned spring(uint32_t age){
  if(age>=3000)return 4096;
  // Quantized Godot4.5.1 Spring::out, easing_equations.h. Samples retain
  // its overshoot; the renderer applies it to glyph X scale, not rotation.
  static constexpr uint16_t samples[]={0,224,439,645,841,1030,1211,1385,1553,1714,1870,2022,2168,2311,2450,2585,2717,2846,2972,3095,3214,3331,3444,3554,3659,3761,3857,3948,4034,4113,4185,4249,4305,4352,4390,4417,4434,4441,4437,4422,4398,4365,4325,4278,4228,4176,4126,4080,4040,4010,3990,3983,3987,4001,4024,4050,4078,4102,4120,4129,4132,4127,4119,4108,4096};
  const unsigned p=age*64*4096/3000,i=p/4096,f=p%4096;
  return unsigned(int(samples[i])+(int(samples[i+1])-int(samples[i]))*int(f)/4096);
 }
 unsigned alpha(uint32_t now)const{const uint32_t age=now-start;return age<4500?255:age>=5500?255:255-255*room_curve::cubic_out(age-4500,1000)/4096;}
 unsigned epithet_alpha(uint32_t now)const{
  const uint32_t age=now-start;
  if(age>=5500)return 128*circ(age-5500,2000)/4096;
  return age<1500?0:age<2500?(age-1500)*255/1000:alpha(now);
 }
};
}
