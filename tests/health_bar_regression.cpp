#include "health_bar.hpp"
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
int main(){ratios();forecasts();motion(1000);motion(UINT32_MAX-1200);easing();std::printf("health bar: %u checks\n",checks);}
