#include "button_presentation.hpp"
#include "nine_patch.hpp"
#include <cstdio>
#include <cstdlib>
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
 std::printf("%u button motion and nine-patch bounds checks passed\n",checks);
}
