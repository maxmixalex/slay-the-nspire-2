#pragma once
#include <stdint.h>

namespace spire {
// Presentation inputs, deliberately separate from saved combat state. Poison
// is the next trigger's total damage (including Accelerant), not its stacks.
struct HealthBarValues {
 int hp=0,max_hp=0,block=0,tracked_block=0,poison_damage=0,doom=0;
 bool infinite=false;
};
namespace health_bar {
// NHealthBar / health_bar.tscn, pinned v0.111.0. The six-pixel body, readable
// 8/10px labels and sixteen-pixel shield are the 320x240 adaptations.
constexpr int Height=6,Inset=2,ForegroundHeight=4,Cap=2,MinimumFill=5;
constexpr uint32_t HoldMs=1000,TweenMs=1000;
inline int bounded(int value,int low,int high){return value<low?low:value>high?high:value;}
inline int inner_width(int width){return width<=2*Inset?0:bounded(width,0,320)-2*Inset;}
// Horizontal nine-patch sampling keeps the authored end caps. In particular,
// health_bar.png is 12px wide with 6+6 margins and no middle source column.
inline int patch_source(int position,int width,int source_width,int margin,int cap=Cap){
 if(width<=0||source_width<=1)return 0;
 position=bounded(position,0,width-1);cap=bounded(cap,0,width/2);
 const int left=bounded(margin,0,(source_width+1)/2),right=bounded(margin,0,source_width-left);
 if(cap&&position<cap)return bounded((2*position+1)*left/(2*cap),0,source_width-1);
 if(cap&&position>=width-cap)return bounded(source_width-right+(2*(position-width+cap)+1)*right/(2*cap),0,source_width-1);
 const int middle=source_width-left-right,span=width-2*cap;
 return bounded(left+(span>0?(2*(position-cap)+1)*middle/(2*span):0),0,source_width-1);
}

// floor(amount*width/total), without overflowing billion-HP encounters or
// invoking an ARM 64-bit divide. Width is a screen-space bound, not game data.
inline int ratio_width(int amount,int total,int width){
 width=bounded(width,0,320);if(amount<=0||total<=0||!width)return 0;
 if(amount>=total)return width;
 const int quotient=total/width,remainder=total%width;
 int low=0,high=width;
 while(low<high){const int middle=(low+high+1)/2;
  const int threshold=quotient*middle+(remainder*middle+width-1)/width;
  if(threshold<=amount)low=middle;else high=middle-1;
 }return low;
}
inline int foreground_width(int amount,int total,int width){
 if(amount<=0||total<=0||width<=0)return 0;
 return bounded(ratio_width(amount,total,width),bounded(MinimumFill,0,width),width);
}
inline int poison_next_damage(int stacks,int accelerant){
 // Both powers are signed 16-bit game facts. Clamp malformed/preview input
 // before computing the descending arithmetic series in signed 32 bits.
 stacks=bounded(stacks,0,32767);accelerant=bounded(accelerant,0,32767);
 const int count=bounded(1+accelerant,0,stacks);
 return count*(2*stacks-count+1)/2;
}
struct Segments {
 int current=0,hp=0,poison_left=0,poison_right=0,doom_right=0;
 bool blocked=false,poison_lethal=false,doom_lethal=false;
};
inline Segments segments(const HealthBarValues&v,int width){
 width=bounded(width,0,320);Segments out;
 out.blocked=v.block>0||v.tracked_block>0;
 out.current=out.hp=foreground_width(v.hp,v.max_hp,width);
 if(v.hp<=0||v.max_hp<=0||v.infinite)return out;
 const int poison=bounded(v.poison_damage,0,v.hp),remaining=v.hp-poison;
 out.poison_lethal=poison>0&&poison>=v.hp;
 out.doom_lethal=v.doom>0&&v.doom>=remaining;
 if(poison>0){
  out.hp=foreground_width(remaining,v.max_hp,width);
  out.poison_left=bounded(out.hp-Cap,0,width);out.poison_right=out.current;
 }
 if(v.doom>0&&!out.poison_lethal){
  if(out.doom_lethal){out.doom_right=out.hp;out.hp=0;}
  else out.doom_right=bounded(foreground_width(v.doom,v.max_hp,width)+Cap,0,width);
 }return out;
}

// Public Godot EXPO_OUT equation: 1.001*(1-2^(-10*t)), with exact endpoints.
// Q12 samples at 1/64 intervals have <=0.466px error at width 320, or
// <=0.085px on the player's 58px fill. No floating point or per-frame pow.
inline unsigned expo_out(uint32_t elapsed){
 static constexpr uint16_t table[]={
  0,421,799,1137,1442,1714,1959,2179,2376,2553,2712,2854,2982,3097,3200,3292,
  3375,3450,3516,3576,3630,3678,3722,3760,3795,3827,3855,3880,3902,3923,3941,3957,
  3972,3985,3997,4008,4017,4026,4033,4040,4046,4052,4057,4061,4065,4069,4072,4075,
  4077,4080,4082,4084,4085,4087,4088,4089,4091,4092,4092,4093,4094,4095,4095,4096,
  4096
 };
 if(elapsed>=TweenMs)return 4096;
 const unsigned position=elapsed*64,index=position/TweenMs,remainder=position%TweenMs;
 return (unsigned(table[index])*(TweenMs-remainder)+unsigned(table[index+1])*remainder+TweenMs/2)/TweenMs;
}
}

// Per-creature, App-owned trailing fill. bind/observe reacts only to new
// facts; sample is pure, so redraws never advance a tween. Replacement,
// death/revival and changed geometry snap to the new creature's own health.
struct HealthBarMotion {
 uint32_t identity=0,start_ms=0,block_start_ms=0,block_break_ms=0;
 int block=0;bool block_entering=false,block_breaking=false;
 int32_t hp=0,max_hp=0,from_q8=0,to_q8=0;
 uint16_t width=0;
 bool valid=false,delayed=false;
 int sample_q8(uint32_t now)const{
  if(!valid)return 0;
  uint32_t elapsed=now-start_ms;
  if(delayed){if(elapsed<=health_bar::HoldMs)return from_q8;elapsed-=health_bar::HoldMs;}
  if(elapsed>=health_bar::TweenMs)return to_q8;
  const int progress=int(health_bar::expo_out(elapsed));
  return from_q8+(to_q8-from_q8)*progress/4096;
 }
 int sample(uint32_t now)const{return health_bar::bounded((sample_q8(now)+128)/256,0,width);}
 // NHealthBar.AnimateInBlock only animates the first 0 -> positive gain.
 // Position uses Back.Out from -20 authored px; opacity uses Sine.Out.
 void observe_block(int value,uint32_t now,bool restored=false){
  if(restored){block=value;block_entering=block_breaking=false;return;}
  if(block<=0&&value>0){block_start_ms=now;block_entering=true;}
  // RefreshBlockUi triggers on losing the visible shield, including the
  // normal turn-start removal. Loading a zero-block state is not a break.
  if(block>0&&value<=0){block_break_ms=now;block_breaking=true;}
  if(block_breaking&&now-block_break_ms>=600u)block_breaking=false;
  if(value<=0||now-block_start_ms>=500u)block_entering=false;block=value;
 }
 static unsigned block_break_progress(unsigned elapsed,bool opacity=false){
  // Numeric samples of the authored Bezier tracks: 250ms horizontal
  // handles at both ends, zero vertical handles; position400ms/alpha600ms.
  static constexpr uint16_t position[]={0,15,64,157,310,545,895,1399,2048,2697,3201,3551,3786,3939,4032,4081,4096};
  static constexpr uint16_t fade[]={0,32,129,296,533,838,1203,1613,2048,2483,2893,3258,3563,3800,3967,4064,4096};
  const unsigned duration=opacity?600u:400u;if(elapsed>=duration)return 4096;
  const auto*table=opacity?fade:position;const unsigned p=elapsed*16u,i=p/duration,r=p%duration;
  return (unsigned(table[i])*(duration-r)+unsigned(table[i+1])*r+duration/2u)/duration;
 }
 unsigned block_alpha(uint32_t now)const{
  if(!block_entering||now-block_start_ms>=500u)return 255;
  static constexpr uint16_t sine[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
  const unsigned p=(now-block_start_ms)*16u,i=p/500u,r=p%500u;
  return (unsigned(sine[i])*(500u-r)+unsigned(sine[i+1])*r)*255u/512000u;
 }
 int block_y(uint32_t now)const{
  if(!block_entering||now-block_start_ms>=500u)return 0;
  const int q=int((now-block_start_ms)*1024u/500u)-1024,q2=q*q/1024,q3=q2*q/1024;
  return (2766*q3/1024+1742*q2/1024)*4/1024;
 }
 void reset(){*this=HealthBarMotion{};}
 void observe(uint32_t owner,int current_hp,int maximum_hp,int available,uint32_t now){
  available=health_bar::bounded(available,0,320);
  const int target=health_bar::foreground_width(current_hp,maximum_hp,available)*256;
  if(!valid||identity!=owner||width!=available||current_hp<=0||hp<=0){
   identity=owner;start_ms=now;hp=current_hp;max_hp=maximum_hp;from_q8=to_q8=target;
   width=uint16_t(available);valid=true;delayed=false;return;
  }
  if(current_hp==hp&&maximum_hp==max_hp)return;
  // Killing a live tween starts at its current visible width, including a
  // heal that still falls short of an earlier hit's held orange segment.
  const int previous=sample_q8(now);
  from_q8=previous;to_q8=target;start_ms=now;delayed=target<previous;
  hp=current_hp;max_hp=maximum_hp;
 }
};
}
