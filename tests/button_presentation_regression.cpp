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
 // Original navigation controls have their own focus/press curve rather
 // than the generic 0.95 action-button press. Compare interrupted samples.
 ButtonMotion back;check(back.observe(4,ButtonFamily::Back,ButtonState::Rest,0).scale==4096);
 check(back.observe(4,ButtonFamily::Back,ButtonState::Focus,100).scale==4096);
 auto back_half=back.sample(125);check(back_half.scale==4198&&back_half.hover==2048);
 check(back.sample(150).scale==4301&&back.sample(150).hover==4096);
 back.observe(4,ButtonFamily::Back,ButtonState::Press,150);
 for(unsigned t=0;t<=250;++t){auto v=back.sample(150+t);double u=double(t)/250;
  double expo=t==0?0:t==250?1:1.001*(1-std::pow(2.,-10*u));
  check(std::abs(v.scale-(4301-205*expo))<3);
  check(std::abs(v.hover-4096*std::pow(1-u,3))<5);
 }
 check(back.sample(400).scale==4096&&back.sample(400).shade==128);
 back.observe(4,ButtonFamily::Back,ButtonState::Rest,400);check(back.sample(900).scale==4301);
 check(back.observe(5,ButtonFamily::Back,ButtonState::Rest,900).scale==4096);
 // Source Proceed uses a 50ms focus, a 500ms press/unfocus and an
 // independent repeating outline. It must not inherit Action's instant focus.
 ButtonMotion proceed;proceed.observe(6,ButtonFamily::Proceed,ButtonState::Rest,100);
 check(proceed.sample(600).hover==1024&&proceed.sample(1100).hover==3072&&proceed.sample(1600).hover==1024);
 proceed.observe(6,ButtonFamily::Proceed,ButtonState::Focus,1600);
 check(proceed.sample(1625).scale==4198&&proceed.sample(1650).scale==4301&&proceed.sample(1650).hover==4096);
 proceed.observe(6,ButtonFamily::Proceed,ButtonState::Press,1650);
 check(proceed.sample(2149).scale>=3891&&proceed.sample(2150).scale==3891&&proceed.sample(2150).shade==128);
 proceed.observe(6,ButtonFamily::Proceed,ButtonState::Rest,2150);check(proceed.sample(2650).scale==4096);
 ButtonMotion relic;relic.observe(7,ButtonFamily::TreasureRelic,ButtonState::Focus,0);
 check(relic.sample(25).scale==4198&&relic.sample(50).scale==4301);
 relic.observe(7,ButtonFamily::TreasureRelic,ButtonState::Press,50);
 check(relic.sample(450).scale==3891&&relic.sample(450).shade==255);
 // Independently project the original viewport ratio and source dimensions.
 check(std::abs(proceed_layout::X-1583.*320/1920)<.5&&std::abs(proceed_layout::Y-764.*240/1080)<.5);
 check(std::abs(proceed_layout::W-269.*320/1680)<.5&&std::abs(proceed_layout::ImageW-296.*320/1680)<.5);
 check(proceed_layout::X+proceed_layout::W<=320&&proceed_layout::Y+proceed_layout::H<=240);
 // The original enable/disable tweens use 500/800ms Expo.Out position
 // and independent linear Image/Outline modulation. Compare full curves
 // against the source equation, including interrupted/reversed movement.
 ProceedMotion slide;auto hidden=slide.observe(81,false,100);
 check(hidden.x==67&&hidden.shade==128&&hidden.outline==0);
 slide.observe(81,true,200);
 for(unsigned t=0;t<=500;++t){const auto v=slide.sample(200+t);const double u=double(t)/500;
  const double q=t==0?0:t==500?1:1.001*(1-std::pow(2.,-10*u));
  check(std::abs(v.x-67*(1-q))<1.1);
  check(std::abs(v.shade-(128+127*u))<1.1&&std::abs(v.outline-255*u)<1.1);
 }
 slide.observe(81,false,700);
 for(unsigned t=0;t<=800;++t){const auto v=slide.sample(700+t);const double u=double(t)/800;
  const double q=t==0?0:t==800?1:1.001*(1-std::pow(2.,-10*u));
  check(std::abs(v.x-67*q)<1.1);
  check(std::abs(v.shade-(255-127*u))<1.1&&std::abs(v.outline-255*(1-u))<1.1);
 }
 slide.observe(81,true,1600);const auto entering=slide.sample(1700);
 auto interrupted_slide=slide.observe(81,false,1700);check(interrupted_slide.x==entering.x&&interrupted_slide.shade==entering.shade&&interrupted_slide.outline==entering.outline);
 auto exiting=slide.sample(1750);auto reversed=slide.observe(81,true,1750);
 check(exiting.x==reversed.x&&exiting.shade==reversed.shade&&exiting.outline==reversed.outline);
 check(slide.sample(2250).x==0&&slide.sample(2250).shade==255);
 check(slide.observe(82,true,2300).x==67);
 slide.observe(83,true,0xfffffff0u);check(slide.sample(484).x==0);
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
