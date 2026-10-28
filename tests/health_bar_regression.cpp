#include "health_bar.hpp"
#include "star_counter_motion.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <initializer_list>

using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"health bar: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static void ratios(){
 for(int width=0;width<=320;++width)for(int total:{0,1,2,7,80,9999,1000000000,INT_MAX}){
  for(int hp:{0,1,2,7,40,9999,INT_MAX/2,INT_MAX}){
   const int reference=total>0?int((int64_t(hp>=total?total:hp)*width)/total):0;
   CHECK(health_bar::ratio_width(hp,total,width)==reference);
   const int filled=health_bar::foreground_width(hp,total,width);
   CHECK(filled>=0&&filled<=width);
   if(hp>0&&total>0)CHECK(filled>=health_bar::bounded(health_bar::MinimumFill,0,width));else CHECK(!filled);
  }
 }
 CHECK(health_bar::ratio_width(-1,80,62)==0);
 CHECK(health_bar::ratio_width(80,80,INT_MAX)==320);
 CHECK(health_bar::inner_width(INT_MIN)==0);
 CHECK(health_bar::inner_width(INT_MAX)==316);
 // End caps keep the same sampling while only the center gets longer.
 for(int width:{20,34,62,90,320})for(int x=0;x<2;++x){
  CHECK(health_bar::patch_source(x,width,12,6)==health_bar::patch_source(x,20,12,6));
  CHECK(health_bar::patch_source(width-1-x,width,12,6)==health_bar::patch_source(19-x,20,12,6));
 }
 for(int width=1;width<100;++width)for(int x=0;x<width;++x){
  CHECK(health_bar::patch_source(x,width,12,6)>=0&&health_bar::patch_source(x,width,12,6)<12);
  CHECK(health_bar::patch_source(x,width,1,9)==0);
 }
}
static void forecasts(){
 HealthBarValues v;v.hp=60;v.max_hp=80;
 auto s=health_bar::segments(v,80);CHECK(s.current==60&&s.hp==60&&!s.blocked);
 v.block=12;s=health_bar::segments(v,80);CHECK(s.blocked);
 v.block=0;v.tracked_block=12;s=health_bar::segments(v,80);CHECK(s.blocked);
 v.poison_damage=15;s=health_bar::segments(v,80);
 CHECK(s.hp==45&&s.poison_left==43&&s.poison_right==60&&!s.poison_lethal);
 v.doom=20;s=health_bar::segments(v,80);CHECK(s.hp==45&&s.doom_right==22&&!s.doom_lethal);
 v.doom=45;s=health_bar::segments(v,80);CHECK(s.hp==0&&s.doom_right==45&&s.doom_lethal);
 v.poison_damage=60;s=health_bar::segments(v,80);
 CHECK(s.hp==0&&s.poison_left==0&&s.poison_right==60&&s.poison_lethal&&s.doom_right==0);
 v.infinite=true;s=health_bar::segments(v,80);
 CHECK(s.hp==60&&s.poison_right==0&&s.doom_right==0&&!s.poison_lethal&&!s.doom_lethal);
 v.hp=0;s=health_bar::segments(v,80);CHECK(!s.current&&!s.hp&&!s.poison_right&&!s.doom_right);
 CHECK(health_bar::poison_next_damage(5,0)==5);
 CHECK(health_bar::poison_next_damage(5,2)==12);
 CHECK(health_bar::poison_next_damage(5,10)==15);
 CHECK(health_bar::poison_next_damage(-1,2)==0);
 CHECK(health_bar::poison_next_damage(32767,32767)==536854528);
}
static void motion(uint32_t now){
 HealthBarMotion m;m.observe(1,80,80,80,now);CHECK(m.sample(now)==80);
 m.observe(1,40,80,80,now+100);
 CHECK(m.sample(now+100)==80&&m.sample(now+1100)==80&&m.sample(now+2100)==40);
 const int between=m.sample(now+1300);CHECK(between>40&&between<80);
 const auto started=m.start_ms;const auto from=m.from_q8;
 for(int i=0;i<20;++i){m.observe(1,40,80,80,now+1300);CHECK(m.start_ms==started&&m.from_q8==from&&m.sample(now+1300)==between);}
 // A second hit kills the old tween at the actual current width.
 m.observe(1,20,80,80,now+1300);CHECK(m.sample(now+1300)==between&&m.delayed);
 CHECK(m.sample(now+2300)==between&&m.sample(now+3300)==20);
 // Healing catches up immediately when it exceeds the held middle width.
 m.observe(1,70,80,80,now+2500);CHECK(!m.delayed);
 CHECK(m.sample(now+2600)>m.sample(now+2500)&&m.sample(now+3500)==70);
 // A small heal still below the pending damage trail retains the delay,
 // matching NHealthBar's target-vs-middle comparison rather than HP sign.
 m.reset();m.observe(1,80,80,80,now);m.observe(1,20,80,80,now+1);m.observe(1,40,80,80,now+2);
 CHECK(m.delayed&&m.sample(now+2)==80);
 // Slot reuse, changed maximum/geometry, death, revival and restored state.
 m.observe(2,30,80,80,now+3);CHECK(m.sample(now+3)==30&&!m.delayed);
 m.observe(2,30,60,80,now+4);CHECK(!m.delayed&&m.sample(now+1004)==40);
 m.observe(2,30,60,40,now+5);CHECK(m.sample(now+5)==20);
 m.observe(2,0,60,40,now+6);CHECK(m.sample(now+6)==0);
 m.observe(2,30,60,40,now+7);CHECK(m.sample(now+7)==20&&!m.delayed);
 m.reset();CHECK(!m.valid&&m.sample(now)==0);
 m.observe(2,15,60,40,now+8);CHECK(m.sample(now+8)==10);
}
static void easing(){
 int previous=0;double worst=0;
 for(unsigned ms=0;ms<=1000;++ms){
  const unsigned actual=health_bar::expo_out(ms);
  const double expected=ms==1000?1.:1.001*(1.-std::pow(2.,-10.*ms/1000.));
  const double error=std::abs(double(actual)/4096.-expected)*320;
  if(error>worst)worst=error;
  CHECK(actual>=unsigned(previous)&&actual<=4096);CHECK(error<0.5);previous=int(actual);
 }
 CHECK(health_bar::expo_out(UINT32_MAX)==4096);
 std::printf("health easing maximum width error %.6f px\n",worst);
}
static void star_counter(){
 // Original MathHelper.SmoothDamp with NStarCounter's smoothTime=.1.
 // The reference uses floats only in this native test; the calculator path
 // must not depend on a software floating-point arithmetic runtime.
 const auto reference=[](float current,int target,float&velocity,unsigned ms){
  const float dt=float(ms)/1000.f,omega=20.f,x=omega*dt;
  const float decay=1.f/(1.f+x+0.48f*x*x+0.235f*x*x*x);
  const float change=current-float(target),step=(velocity+omega*change)*dt;
  velocity=(velocity-omega*step)*decay;
  float next=float(target)+(change+step)*decay;
  if((float(target)>current)==(next>float(target))){next=float(target);velocity=0;}
  return next;
 };
 float worst=0;
 for(unsigned dt:{1u,16u,17u,33u,50u,100u,250u,1000u})for(int target:{1,3,23,500,9999}){
  StarCounterMotion fixed;float value=0,velocity=0;
  for(unsigned frame=0;frame<2000/dt+5;++frame){
   const int before=fixed.value;fixed.advance(target,dt);value=reference(value,target,velocity,dt);
   CHECK(fixed.value>=before&&fixed.value<=target*fixed.Scale);
   const float error=std::fabs(float(fixed.value)/fixed.Scale-value);if(error>worst)worst=error;
   CHECK(error<.031f);
   const int expected=int(std::nearbyint(value));
   // Normal UI intervals preserve every original rounded label. At an
   // artificial 1ms update and a 9999-star gain, original float rounding
   // accumulates up to .03 stars; only a half-integer boundary may differ.
   if(dt>=16)CHECK(fixed.displayed()==expected);
   else CHECK(std::abs(fixed.displayed()-expected)<=1);
  }
  fixed.advance(target,0);CHECK(fixed.displayed()==target);
  fixed.set(0);CHECK(fixed.value==0&&fixed.velocity==0&&fixed.displayed()==0);
 }
 StarCounterMotion fixed;float value=0,velocity=0;
 for(unsigned frame=0;frame<40;++frame){const int target=frame<3?3:frame<9?8:23;
  fixed.advance(target,33);value=reference(value,target,velocity,33);CHECK(fixed.displayed()==int(std::nearbyint(value)));
 }
 fixed.set(0);value=velocity=0;
 for(unsigned frame=0;frame<40;++frame){const int target=frame<3?23:1;
  if(frame==3){fixed.decrease(1);value=1;CHECK(fixed.displayed()==1&&fixed.velocity>0);}
  fixed.advance(target,33);value=reference(value,target,velocity,33);CHECK(fixed.displayed()==int(std::nearbyint(value)));
 }
 fixed.set(2);CHECK(fixed.displayed()==2);fixed.advance(2,1000);CHECK(fixed.value==2*fixed.Scale&&!fixed.velocity);
 fixed.value=fixed.Scale/2;CHECK(fixed.displayed()==0);
 fixed.value=fixed.Scale+fixed.Scale/2;CHECK(fixed.displayed()==2);
 fixed.value=2*fixed.Scale+fixed.Scale/2;CHECK(fixed.displayed()==2);
 std::printf("star count maximum original-float difference %.6f stars\n",worst);
}
static void block_entry(){
 HealthBarMotion motion;motion.observe(1,80,80,58,1000);motion.observe_block(0,1000,true);
 motion.observe_block(12,1033);CHECK(motion.block_entering&&motion.block_alpha(1033)==0&&motion.block_y(1033)==-4);
 CHECK(motion.block_alpha(1283)>170&&motion.block_alpha(1283)<190);
 const auto at=motion.block_start_ms;motion.observe_block(20,1333);CHECK(motion.block_start_ms==at);
 CHECK(motion.block_alpha(1533)==255&&motion.block_y(1533)==0);
 motion.observe_block(0,1600);CHECK(!motion.block_entering);motion.observe_block(7,1633);CHECK(motion.block_entering&&motion.block_alpha(1633)==0);
 motion.observe_block(14,1700,true);CHECK(!motion.block_entering&&motion.block_alpha(1700)==255&&motion.block_y(1700)==0);
}
int main(){ratios();forecasts();motion(1000);motion(UINT32_MAX-1200);easing();star_counter();block_entry();std::printf("health bar: %u checks\n",checks);}
