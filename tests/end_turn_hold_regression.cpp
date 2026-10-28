#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "end_turn_hold_oracle.hpp"
#include "host_physical_input.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static App a;
static SaveData before,after;
static unsigned checks;
static const char* scenario="setup";
static const char* captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"end-turn hold [%s]:%d %s (turn=%d owner=%u timer=%u phase=%u)\n",scenario,__LINE__,#x,a.game.turn,a.end_turn_hold.owner,a.end_turn_hold.timer,unsigned(a.game.phase));std::exit(1);}}while(0)
static void advance(unsigned milliseconds){while(milliseconds){const unsigned step=milliseconds>100?100:milliseconds;a.advance(step);a.render();milliseconds-=step;}}
static void snapshot(){CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void combat(bool enabled=true){
 a.physical_keys_cancel();a.pointer_cancel();new_run(a.game,0x1e1701,true);a.title=a.paused=a.settings_open=a.help=a.library=a.character_select=false;
 a.reset_presentation();a.progress.tutorial_flags=FtueInitialized|FtueSeenMask;a.input_bindings=default_input_bindings();a.keyboard_only=a.calculator_keyboard=false;a.long_press=enabled;
 a.render();for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}
 advance(2500);CHECK(a.game.phase==Phase::Combat&&!a.combat_actions_locked()&&!a.ftue.active());
 if(enabled)CHECK(a.end_turn_hold_available());snapshot();
}
static void capture(const char*name){
 if(!captures)return;a.render();FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);
 std::fprintf(f,"P6\n320 240\n255\n");for(uint16_t p:a.pixels){const uint8_t bytes[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(bytes,1,3,f)==3);}CHECK(!std::fclose(f));
}
static HitRegion end_button(){a.render();for(int i=a.hit_count-1;i>=0;--i)if(a.hits[i].kind==HitKind::Command&&a.hits[i].value==int(Key::EndTurn)){CHECK(a.hits[i].enabled);return a.hits[i];}CHECK(false);return {};}
static void point_button(){const auto h=end_button();CHECK(h.x==246&&h.y==156&&h.w==70&&h.h==28);a.pointer_move(h.x+h.w/2,h.y+h.h/2);}
static void physical(){
 scenario="off remains immediate; direct combat key cannot bypass on";combat(false);const int off_turn=a.game.turn;
 CHECK(a.physical_key('E',true));CHECK(a.game.turn==off_turn+1);CHECK(a.physical_key('E',false));
 combat();a.input(Key::EndTurn);unchanged();CHECK(!a.end_turn_hold.owner);
 scenario="strict500ms and one trigger across held key/repeat/new turn";
 CHECK(a.physical_key('E',true));CHECK(a.end_turn_hold.owner=='E');advance(499);unchanged();CHECK(a.physical_key('E',true));advance(1);unchanged();
 const int turn=a.game.turn;advance(1);CHECK(a.game.turn==turn+1&&a.end_turn_hold.fired);capture("end-turn-hold-fired");
 advance(4000);const int held_turn=a.game.turn;CHECK(a.physical_key('E',true));advance(700);CHECK(a.game.turn==held_turn&&a.end_turn_hold.owner=='E');
 CHECK(a.physical_key('E',false));CHECK(!a.end_turn_hold.owner);CHECK(a.physical_key('E',true));advance(501);CHECK(a.game.turn==held_turn+1);CHECK(a.physical_key('E',false));
 scenario="released progress decays and a second physical key cannot steal";combat();
 CHECK(a.physical_key('E',true));advance(300);CHECK(a.physical_key('E',false));advance(100);CHECK(a.end_turn_hold.timer==200);unchanged();
 CHECK(rebind_input(a.input_bindings,0,unsigned(InputCommand::EndTurn),'B'));CHECK(a.physical_key('B',true));advance(100);CHECK(a.end_turn_hold.timer==300);
 a.command_key(Key::EndTurn,true);CHECK(a.end_turn_hold.owner=='B');a.command_keys_cancel();a.command_key(Key::EndTurn,false);
 CHECK(a.end_turn_hold.owner=='B'&&a.end_turn_hold.pressed&&a.end_turn_hold.timer==300);CHECK(a.physical_key('B',true));CHECK(a.end_turn_hold.timer==300);
 advance(200);unchanged();advance(1);CHECK(a.game.turn==turn+1);CHECK(a.physical_key('B',false));
 scenario="cancel and focus change cannot accept a paused/new context";combat();CHECK(a.physical_key('E',true));advance(250);a.physical_keys_cancel();advance(600);unchanged();CHECK(!a.end_turn_hold.owner);
 CHECK(a.physical_key('E',true));advance(250);a.input(Key::Back);CHECK(a.paused&&!a.end_turn_hold.pressed);advance(700);unchanged();a.input(Key::Back);CHECK(!a.paused);
 CHECK(a.physical_key('E',true));advance(600);unchanged();CHECK(a.physical_key('E',false));
 scenario="calculator physical mappings use the same down/up timer";combat();a.calculator_keyboard=true;a.input_bindings=default_input_bindings(true);
 CHECK(rebind_input(a.input_bindings,0,unsigned(InputCommand::EndTurn),'B'));
 uint16_t now[8]{},previous[8]{};uint8_t repeats[CalculatorPhysicalKeyCount]{};
 const auto edge=[&](uint16_t code,bool pressed){CHECK(a.physical_key(code,pressed));};now[4]=0x20;
 calculator_physical_edges(now,previous,repeats,false,edge);std::memcpy(previous,now,sizeof now);advance(500);unchanged();advance(1);CHECK(a.game.turn==turn+1);
 now[4]=0;calculator_physical_edges(now,previous,repeats,false,edge);CHECK(!a.end_turn_hold.owner);
 scenario="keyboard-only binding and ordinary Confirm mapping";combat();a.keyboard_only=true;
 const uint16_t mapped=a.input_bindings.keys[1][unsigned(InputCommand::EndTurn)];CHECK(a.physical_key(mapped,true));advance(500);unchanged();advance(1);CHECK(a.game.turn==turn+1);CHECK(a.physical_key(mapped,false));
 combat();const uint16_t confirm=a.input_bindings.keys[0][unsigned(InputCommand::Confirm)];CHECK(a.physical_key(confirm,true));advance(300);CHECK(a.physical_key(confirm,false));advance(300);unchanged();
 scenario="covered Combat keeps the key's ordinary pile action";combat();a.input(Key::Inspect);CHECK(a.panel==1&&!a.pile_tab);
 const bool upgrades=a.pile_upgraded;a.keyboard_only=true;const uint16_t pile_key=a.input_bindings.keys[1][unsigned(InputCommand::EndTurn)];
 CHECK(a.physical_key(pile_key,true));CHECK(a.pile_upgraded!=upgrades&&!a.end_turn_hold.owner);CHECK(a.physical_key(pile_key,false));unchanged();
 a.command_key(Key::EndTurn,true);CHECK(a.pile_upgraded!=upgrades);a.command_key(Key::EndTurn,false);CHECK(a.pile_upgraded==upgrades&&!a.end_turn_hold.owner);unchanged();
}
static void mouse_and_semantic(){
 scenario="mouse hold cancels when leaving and does not restart on reentry";combat();point_button();a.pointer_button(true);advance(250);CHECK(a.end_turn_hold.owner==EndTurnHold::Mouse);
 capture("end-turn-hold-half");a.pointer_move(200,150);CHECK(!a.end_turn_hold.pressed);point_button();advance(600);unchanged();a.pointer_button(false);CHECK(!a.end_turn_hold.owner);
 scenario="mouse release, blur and full hold";point_button();a.pointer_button(true);advance(250);a.pointer_button(false);advance(250);unchanged();
 point_button();a.pointer_button(true);advance(300);a.pointer_cancel();advance(400);unchanged();
 point_button();a.pointer_button(true);advance(500);unchanged();const int turn=a.game.turn;advance(1);CHECK(a.game.turn==turn+1);
 advance(3000);a.pointer_button(false);const int released=a.game.turn;advance(600);CHECK(a.game.turn==released);
 scenario="semantic toolbar release and cancel are distinct";combat();a.command_key(Key::EndTurn,true);advance(200);a.command_key(Key::EndTurn,false);advance(300);unchanged();
 a.command_key(Key::EndTurn,true);advance(200);a.physical_keys_cancel();a.command_key(Key::EndTurn,false);advance(600);unchanged();
 a.command_key(Key::EndTurn,true);advance(500);unchanged();advance(1);CHECK(a.game.turn==turn+1);advance(3000);a.command_key(Key::EndTurn,true);advance(700);CHECK(a.game.turn==turn+1);a.command_key(Key::EndTurn,false);
 scenario="ordinary semantic buttons keep release execution";combat(false);a.command_key(Key::EndTurn,true);advance(100);unchanged();a.command_key(Key::EndTurn,false);CHECK(a.game.turn==turn+1);
 combat(false);a.command_key(Key::Back,true);CHECK(!a.paused);a.command_key(Key::Back,false);CHECK(a.paused);a.command_key(Key::Back,false);CHECK(a.paused);unchanged();
}
static void source_bar_pixels(){
 scenario="source additive204x6 bar, inherited outline and250ms fade";
 a.reset_presentation();a.long_press=true;a.pointer.visible=false;a.game.phase=Phase::Combat;
 const unsigned times[]={125,250,400,500,501,626,751};
 for(unsigned age:times){
  a.end_turn_hold={};CHECK(a.end_turn_hold.start('E',1,1));a.end_turn_hold.advance(age>500?500:age);
  if(age>500){a.end_turn_hold.advance(1);a.end_turn_hold.advance(age-501);}
  Canvas c{a};for(auto&p:a.pixels)p=rgb(17,32,48);a.hit_count=0;native_end_turn(c,true);
  const std::vector<uint16_t>actual(a.pixels,a.pixels+ScreenW*ScreenH);
  const EndTurnHold held=a.end_turn_hold;a.end_turn_hold={};for(auto&p:a.pixels)p=rgb(17,32,48);a.hit_count=0;native_end_turn(c,true);
  std::vector<uint16_t>expected(a.pixels,a.pixels+ScreenW*ScreenH);a.end_turn_hold=held;
  const double q=std::min(age,500u)/500.,fade=age>500?std::max(0.,1.-double(age-501)/250):1.;
  const unsigned alpha=unsigned(std::lround((1.-std::pow(1.-.75*q,3))*fade*255));
  const int width=int(std::lround((age>500?204.:q*204.)*4/21));
  const unsigned outline=unsigned(std::lround(alpha*128./255));
  const auto blend_channel=[](unsigned back,unsigned source,unsigned opacity){return unsigned(std::lround((back*(255-opacity)+source*opacity)/255.));};
  if(alpha){for(int y=152;y<155;++y)for(int x=248;x<250+width;++x){const auto old=expected[y*ScreenW+x];expected[y*ScreenW+x]=uint16_t(blend_channel(old>>11,0,outline)<<11|blend_channel((old>>5)&63,0,outline)<<5|blend_channel(old&31,0,outline));}
   // The source uses unclamped HDR Color then additive modulation. Allow
   // only one channel quantum for fixed-point/RGB565 projection rounding.
   const int red=std::min(255,int(std::lround(std::floor(q*2.5*255)*alpha/255.))),green=std::min(255,int(std::lround(std::floor((.6+q)*255)*alpha/255.))),blue=int(std::lround(153*alpha/255.));
   for(int x=249;x<249+width;++x){const auto old=expected[153*ScreenW+x];expected[153*ScreenW+x]=uint16_t(std::min(31,int(old>>11)+(red>>3))<<11|std::min(63,int((old>>5)&63)+(green>>2))<<5|std::min(31,int(old&31)+(blue>>3)));}}
  for(int y=151;y<156;++y)for(int x=247;x<291;++x){const auto got=actual[y*ScreenW+x],want=expected[y*ScreenW+x];
   CHECK(std::abs(int(got>>11)-int(want>>11))<=1&&std::abs(int((got>>5)&63)-int((want>>5)&63))<=1&&std::abs(int(got&31)-int(want&31))<=1);}
 }
 a.end_turn_hold={};
}
static void off_residual(){
 scenario="disabled setting still cancels a zero-time captured owner";combat();
 CHECK(a.physical_key('E',true));CHECK(a.end_turn_hold.owner=='E'&&a.end_turn_hold.pressed&&!a.end_turn_hold.timer);
 a.long_press=false;a.advance(0);CHECK(a.end_turn_hold.owner=='E'&&!a.end_turn_hold.pressed&&!a.end_turn_hold.timer);
 CHECK(a.physical_key('E',false));CHECK(!a.end_turn_hold.owner);unchanged();
 scenario="disabled setting keeps released residual progress decaying";combat();
 CHECK(a.physical_key('E',true));advance(250);a.long_press=false;a.advance(100);
 CHECK(a.end_turn_hold.owner=='E'&&!a.end_turn_hold.pressed&&a.end_turn_hold.timer==150);
 CHECK(a.physical_key('E',false));CHECK(!a.end_turn_hold.owner);a.advance(149);CHECK(a.end_turn_hold.timer==1);
 a.advance(1);CHECK(!a.end_turn_hold.timer&&!a.end_turn_hold.fired);unchanged();
 scenario="disabled setting completes an ownerless completion fade";combat();
 CHECK(a.physical_key('E',true));const int turn=a.game.turn;advance(501);
 CHECK(a.game.turn==turn+1&&a.end_turn_hold.fired&&!a.end_turn_hold.fade_age);
 CHECK(a.physical_key('E',false));CHECK(!a.end_turn_hold.owner&&!a.end_turn_hold.timer);a.long_press=false;snapshot();
 a.advance(125);CHECK(a.end_turn_hold.fade_age==125&&a.end_turn_hold.visual().alpha_q12>0);
 a.advance(124);CHECK(a.end_turn_hold.fade_age==249);a.advance(1);
 CHECK(a.end_turn_hold.fade_age==250&&!a.end_turn_hold.visual().alpha_q12);a.advance(250);
 CHECK(a.end_turn_hold.fade_age==250&&!a.end_turn_hold.owner&&!a.end_turn_hold.pressed);unchanged();
}
int main(int argc,char**argv){
 std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(a.content.open(pack.data(),pack.size())&&a.content.current_profile());}if(argc>2)captures=argv[2];
 CHECK(source_end_turn_hold_timeline());physical();mouse_and_semantic();source_bar_pixels();off_residual();
 std::printf("Source end-turn long press: %u checks; strict threshold, decay, keyboard/calculator/mouse/toolbar ownership and source bar passed\n",checks);
}
