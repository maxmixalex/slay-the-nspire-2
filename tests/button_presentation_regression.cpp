#include "button_presentation.hpp"
#include "nine_patch.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <initializer_list>
static unsigned checks=0;
static void check(bool v){++checks;if(!v){std::fprintf(stderr,"Button presentation check %u failed\n",checks);std::exit(1);}}
int main(){
 using namespace spire;
 for(int source:{1,2,12,20,54,170})for(int width:{1,2,3,16,64,320})
  for(int left:{0,1,6,28})for(int right:{0,1,6,46}){
   int previous=-1;for(int x=0;x<width;++x){int value=nine_patch::coordinate(x,width,source,left,right);check(value>=0&&value<source);check(value>=previous);previous=value;}
  }
 check(nine_patch::coordinate(0,100,12,6,6)==0);
 check(nine_patch::coordinate(99,100,12,6,6)==11);
 check(nine_patch::coordinate(50,100,12,6,6)==5);
 ButtonMotion m;auto idle=m.observe(1,ButtonFamily::Action,ButtonState::Rest,100);
 check(idle.scale==4096&&idle.hover==0);
 auto focus=m.observe(1,ButtonFamily::Action,ButtonState::Focus,110);check(focus.scale==4301&&focus.hover==4096);
 auto press=m.observe(1,ButtonFamily::Action,ButtonState::Press,120);check(press.scale==focus.scale&&press.shade==255);
 auto half=m.sample(245);check(half.scale<focus.scale&&half.scale>3891&&half.shade<255&&half.shade>128);
 auto release=m.observe(1,ButtonFamily::Action,ButtonState::Rest,245);check(release.scale==half.scale&&release.shade==half.shade);
 check(m.sample(745).scale==4096&&m.sample(745).shade==255);
 auto disabled=m.observe(1,ButtonFamily::Action,ButtonState::Disabled,746);check(disabled.hover==0&&disabled.shade==128);
 auto replacement=m.observe(2,ButtonFamily::Pause,ButtonState::Rest,750);check(replacement.scale==3686&&replacement.shade==255);
 auto pause=m.observe(2,ButtonFamily::Pause,ButtonState::Focus,760);check(pause.hover==4096&&pause.scale==3686);
 check(m.sample(810).scale==3891);
 m.observe(3,ButtonFamily::EndTurn,ButtonState::Focus,0xfffffff0u);m.observe(3,ButtonFamily::EndTurn,ButtonState::Rest,0xfffffff5u);
 check(m.sample(600).y==0&&m.sample(600).hover==0);
 EndTurnGlowMotion glow;EndTurnGlowIdentity combat{37,2,4,260};
 auto same=[](EndTurnGlowVisual a,EndTurnGlowVisual b){return a.base_alpha==b.base_alpha&&a.base_scale==b.base_scale&&a.pulse_alpha==b.pulse_alpha&&a.pulse_scale==b.pulse_scale;};
 auto rest=glow.observe(combat,false,1000);check(rest.base_alpha==0&&rest.pulse_alpha==0);
 auto onset=glow.observe(combat,true,1010);check(onset.base_alpha==0&&onset.base_scale==3686&&onset.pulse_alpha==1638&&onset.pulse_scale==4096);
 // Independent floating-point Godot Back/Quart equations bound the fixed
 // point implementation's error across the complete enable and loop period.
 for(uint32_t t=0;t<3000;++t){
  auto v=glow.sample(1010+t);double x=double(t)/800-1;
  double back=t>=800?1:x*x*((1.70158+1)*x+1.70158)+1;
  double phase=double(t%1500)/1500;
  check(std::abs(v.base_alpha-3072*back)<5);
  check(std::abs(v.base_scale-4096*(.9+.1*back))<3);
  check(std::abs(v.pulse_alpha-4096*.4*(1-phase))<2);
  check(std::abs(v.pulse_scale-4096*(1+.4*(1-std::pow(1-phase,4))))<3);
  check(v.base_alpha>=0&&v.base_alpha<3400&&v.pulse_alpha>=0&&v.pulse_alpha<=1638);
 }
 check(glow.sample(1410).base_alpha>3072); // authored Back overshoot survives
 check(glow.sample(1809).base_alpha>=3071&&glow.sample(1810).base_alpha==3072);
 check(glow.sample(2509).pulse_alpha<=2&&glow.sample(2510).pulse_alpha==1638);
 check(glow.sample(2510).pulse_scale==4096);
 const auto before=glow.sample(1460);const auto stopped=glow.observe(combat,false,1460);
 check(same(before,stopped));
 for(unsigned t=0;t<=500;++t){auto v=glow.sample(1460+t);
  double q=t==0?0:t==500?1:1.001*(1-std::pow(2.,-10.*t/500));
  check(std::abs(v.base_alpha-stopped.base_alpha*(1-q))<7);
  check(std::abs(v.pulse_alpha-stopped.pulse_alpha*(1-q))<4);
  check(v.base_scale==stopped.base_scale&&v.pulse_scale==stopped.pulse_scale);
 }
 check(glow.sample(1960).base_alpha==0&&glow.sample(1960).pulse_alpha==0);
 const auto interrupted=glow.sample(1560);const auto resumed=glow.observe(combat,true,1560);
 check(resumed.base_alpha==interrupted.base_alpha&&resumed.base_scale==3686);
 check(resumed.pulse_alpha==1638&&resumed.pulse_scale==4096);
 const auto sampled=glow.sample(1610);
 for(int i=0;i<8;i++)check(same(glow.observe(combat,true,1610),sampled)&&glow.start==1560);
 // Exact encounter identity prevents a previous battle's looping pulse
 // from carrying into a new one, even without an intervening visible frame.
 for(unsigned field=0;field<4;field++){
  auto next=combat;if(field==0)++next.seed;if(field==1)++next.floor;if(field==2)++next.combat;if(field==3)++next.encounter;
  auto fresh=glow.observe(next,true,1700+field);check(same(fresh,onset));combat=next;
 }
 ++combat.combat;auto new_idle=glow.observe(combat,false,1800);check(new_idle.base_alpha==0&&new_idle.pulse_alpha==0);
 EndTurnGlowMotion wrapped,ordinary;
 wrapped.observe(combat,true,0xfffffff0u);ordinary.observe(combat,true,1000);
 for(uint32_t t:{0u,16u,400u,799u,800u,1499u,1500u,2200u})check(same(wrapped.sample(0xfffffff0u+t),ordinary.sample(1000+t)));
 auto wrap_stop=wrapped.observe(combat,false,0x10u);ordinary.observe(combat,false,1032);
 for(uint32_t t:{0u,100u,499u,500u,1500u})check(same(wrapped.sample(0x10u+t),ordinary.sample(1032+t)));
 check(wrap_stop.base_alpha>0&&wrap_stop.pulse_alpha>0);
 wrapped={};check(wrapped.sample(9).base_alpha==0&&wrapped.sample(9).pulse_alpha==0);
 std::printf("%u button motion, glow timing and nine-patch bounds checks passed\n",checks);
}
