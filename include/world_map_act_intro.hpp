#pragma once
#include <stdint.h>

// Integer adaptations of Godot 4.5.1 easing equations.
// Copyright (c) 2014-present Godot Engine contributors.
// Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.
// Copyright (c) 2001 Robert Penner.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

namespace spire {
namespace world_map_act_intro {
constexpr unsigned One=4096,ExpoOne=16777216;
constexpr uint32_t InputGuard=200;
constexpr int32_t CameraSpan=2400*int32_t(One);
// round(2^(-10*i/256) * 2^24). Interpolating this monotone public-math
// table avoids libm/soft-float while retaining Godot's .0005/1.0005 terms.
static constexpr uint32_t ExpoDecay[]={
 16777216,16329050,15892855,15468313,15055111,14652947,14261526,13880561,13509772,13148888,12797645,12455784,
 12123055,11799215,11484025,11177254,10878679,10588079,10305242,10029960,9762032,9501261,9247455,9000430,
 8760003,8525999,8298246,8076577,7860829,7650844,7446469,7247553,7053950,6865520,6682122,6503624,
 6329894,6160805,5996233,5836057,5680159,5528427,5380747,5237012,5097117,4960959,4828438,4699457,
 4573921,4451739,4332820,4217079,4104429,3994788,3888076,3784215,3683128,3584741,3488983,3395782,
 3305071,3216784,3130854,3047221,2965821,2886595,2809486,2734437,2661393,2590300,2521105,2453760,
 2388213,2324417,2262325,2201892,2143074,2085826,2030108,1975878,1923097,1871726,1821727,1773063,
 1725700,1679601,1634735,1591066,1548564,1507198,1466937,1427751,1389611,1352491,1316362,1281198,
 1246974,1213664,1181244,1149689,1118978,1089087,1059994,1031679,1004120,977297,951191,925782,
 901051,876982,853555,830754,808563,786964,765942,745481,725567,706185,687321,668961,
 651091,633699,616771,600295,584260,568652,553462,538678,524288,510283,496652,483385,
 470472,457905,445673,433768,422180,410903,399926,389243,378845,368725,358876,349289,
 339959,330877,322039,313436,305063,296914,288983,281263,273750,266437,259320,252393,
 245651,239089,232702,226486,220436,214547,208816,203238,197809,192525,187382,182377,
 177505,172763,168148,163657,159285,155030,150889,146858,142935,139117,135401,131784,
 128263,124837,121502,118257,115098,112023,109031,106118,103283,100524,97839,95226,
 92682,90206,87796,85451,83169,80947,78785,76680,74632,72638,70698,68809,
 66971,65182,63441,61746,60097,58491,56929,55408,53928,52488,51085,49721,
 48393,47100,45842,44617,43425,42265,41136,40037,38968,37927,36914,35928,
 34968,34034,33125,32240,31379,30541,29725,28931,28158,27406,26674,25961,
 25268,24593,23936,23296,22674,22068,21479,20905,20347,19803,19274,18759,
 18258,17770,17296,16834,16384
};
inline int32_t scaled(int32_t value,uint32_t numerator,uint32_t denominator){
 const uint32_t magnitude=value<0?uint32_t(-int64_t(value)):uint32_t(value);
 const int32_t result=int32_t((uint64_t(magnitude)*numerator+denominator/2)/denominator);
 return value<0?-result:result;
}
inline int32_t expo_q24(uint32_t elapsed,uint32_t duration){
 if(!elapsed)return 0;if(elapsed>=duration)return ExpoOne;
 const bool first=elapsed<duration-elapsed;
 const uint32_t distance=first?duration-elapsed*2:elapsed*2-duration;
 const uint32_t position=distance*256,index=position/duration,remainder=position%duration;
 const uint32_t decay=uint32_t((uint64_t(ExpoDecay[index])*(duration-remainder)+
     uint64_t(ExpoDecay[index+1])*remainder+duration/2)/duration);
 return first?int32_t(decay/2)-int32_t((ExpoOne+1000)/2000):
     int32_t((uint64_t(2*ExpoOne-decay)*2001+2000)/4000);
}
inline int32_t camera_y(int32_t from,uint32_t elapsed,uint32_t duration){
 const int32_t progress=expo_q24(elapsed,duration);
 // Expo briefly exceeds its endpoints just inside t=0/d in the source.
 const uint32_t magnitude=progress<0?uint32_t(-progress):uint32_t(progress);
 const int32_t movement=scaled(from,magnitude,ExpoOne);
 return progress<0?from+movement:from-movement;
}
inline uint16_t alpha(uint32_t elapsed,uint32_t delay,uint32_t duration,unsigned target=One){
 return elapsed<=delay?0:elapsed-delay>=duration?uint16_t(target):
     uint16_t((uint64_t(elapsed-delay)*target+duration/2)/duration);
}
inline int32_t lerp_y(int32_t from,int32_t target,uint32_t delta){
 if(!delta)return from;if(delta>=67)return target;
 const int32_t next=from+scaled(target-from,delta*15,1000);
 const int32_t distance=target-next;
 return distance>-2048&&distance<2048?target:next;
}
}
struct MapActBannerSample {
 uint16_t background_alpha=0,name_alpha=0,number_alpha=0;
 int32_t number_y_q12=0;
};
struct MapActBanner {
 uint32_t elapsed=0;
 uint8_t act=0,region=0;
 bool fast=false;
 uint32_t duration()const{return fast?3250:4750;}
 MapActBannerSample sample()const{
  using namespace world_map_act_intro;
  const uint32_t fade_at=fast?2250:3750;
  uint32_t root=One;
  if(elapsed>=duration())root=0;
  else if(elapsed>fade_at){const uint32_t remaining=duration()-elapsed;root=(remaining*remaining*One+500000)/1000000;}
  MapActBannerSample out;
  out.background_alpha=uint16_t((alpha(elapsed,500,500,One/4)*root+One/2)/One);
  out.name_alpha=uint16_t((alpha(elapsed,250,1000)*root+One/2)/One);
  out.number_alpha=uint16_t((alpha(elapsed,500,1000)*root+One/2)/One);
  // From(450) is written when its delay starts; authored y is 440 before it.
  if(elapsed>=500&&elapsed<1750){const uint32_t remaining=1750-elapsed;
   out.number_y_q12=int32_t((uint64_t(10)*One*remaining*remaining+781250)/1562500);}
  return out;
 }
};
// The original MapScreen survives room changes and retains its child banners.
// Camera/banner time only advances while that node can process. The separate
// 200 ms Task.Delay guard follows the unaccelerated running clock even hidden.
struct MapActIntroState {
 static constexpr unsigned MaxBanners=16;
 MapActBanner banners[MaxBanners]{};
 uint8_t banner_count=0;
 bool played=false,interruptible=false,camera_running=false,property_started=false;
 bool fast_at_construction=false,position_valid=false,input_guard=false;
 uint32_t camera_elapsed=0,guard_start=0;
 // Q12 original map Y relative to -600. Zero is the first-floor target.
 int32_t y_q12=0,target_q12=0,camera_from_q12=0;
 uint32_t delay()const{return fast_at_construction?500:1000;}
 uint32_t sweep_duration()const{return fast_at_construction?1500:3000;}
 uint32_t camera_duration()const{return delay()+sweep_duration();}
 uint32_t interrupt_at()const{return delay()+sweep_duration()/4;}
 void reset(bool fast=false){*this=MapActIntroState{};fast_at_construction=fast;}
 // SetMap resets this one flag. In particular, it does not clear the old
 // interruptible latch, existing act tween, or already-created banner nodes.
 void install_map(){played=false;}
 void snap(int32_t target){position_valid=true;y_q12=target_q12=target;}
 bool add_banner(unsigned act,unsigned region,bool fast){
  if(banner_count>=MaxBanners)return false;
  banners[banner_count++]={0,uint8_t(act),uint8_t(region),fast};return true;
 }
 void open_intro(bool sweep,unsigned act,unsigned region,bool banner_fast){
  played=true;add_banner(act,region,banner_fast);
  if(!sweep){snap(0);return;}
  camera_running=true;property_started=false;camera_elapsed=0;
  position_valid=true;y_q12=world_map_act_intro::CameraSpan;target_q12=0;
 }
 bool can_scroll()const{return !input_guard&&(!camera_running||interruptible);}
 bool cancel(uint32_t now){
  if(!camera_running||!interruptible)return false;
  camera_running=false;interruptible=false;target_q12=0;
  input_guard=true;guard_start=now;return true;
 }
 void update(uint32_t delta,uint32_t now,bool active){
  if(input_guard&&uint32_t(now-guard_start)>=world_map_act_intro::InputGuard)input_guard=false;
  if(!active||!delta)return;
  unsigned write=0;
  for(unsigned i=0;i<banner_count;++i){auto b=banners[i];
   const uint32_t left=b.duration()-b.elapsed;
   if(delta>=left)continue;b.elapsed+=delta;banners[write++]=b;}
  banner_count=uint8_t(write);
  if(!camera_running){
   if(position_valid)y_q12=world_map_act_intro::lerp_y(y_q12,target_q12,delta);
   return;
  }
  const uint32_t left=camera_duration()-camera_elapsed;
  camera_elapsed+=delta>=left?left:delta;
  if(camera_elapsed>=delay()){
   if(!property_started){property_started=true;camera_from_q12=y_q12;}
   y_q12=world_map_act_intro::camera_y(camera_from_q12,camera_elapsed-delay(),sweep_duration());
  }
  if(camera_elapsed>=interrupt_at())interruptible=true;
  if(camera_elapsed>=camera_duration())camera_running=false;
 }
};
}

