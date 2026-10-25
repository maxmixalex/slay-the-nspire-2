#include "app.hpp"
#include "content_fixture.hpp"
#include "rich_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static SaveData before,after;
static unsigned checks,exports;
static bool export_ok=true;
static const char*captures;
static std::string report;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"auxiliary UI8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t value,unsigned bytes=4){CHECK(at+bytes<=b.size());for(unsigned i=0;i<bytes;++i)b[at+i]=uint8_t(value>>(8*i));}
static Bytes synthetic(){
 // Public fixtures contain arbitrary labels/shapes, never source prose or art.
 constexpr unsigned ni=A_Count,ng=95*5,ns=Ui7StringCount,nc=PhobiaAnimationKinds;
 const unsigned ga=40+ni*16,sa=ga+ng*16,ca=sa+ns*8,base=ca+nc*16+CardKinds*16+HistoryUi8AuxiliaryCount*4;
 const unsigned table=base+64,costs=table+16,descriptors=costs+((ni+1)&~1u);
 Bytes b(descriptors+16);put(b,0,0x3250534e);put(b,4,HistoryUi8ContentVersion);put(b,8,0x41cef1ea);
 put(b,16,ni);put(b,20,ng);put(b,24,ns);put(b,28,nc);put(b,32,CardKinds);
 const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,descriptors,table,770,1,524288,617323,costs,ni};
 for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
 const unsigned pixel=unsigned(b.size());b.push_back(0);b.push_back(0);
 for(unsigned i=0;i<ni;++i){put(b,40+i*16,pixel);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 unsigned glyph=0;for(unsigned encoded:{6u,8u,10u,104u,106u})for(unsigned cp=32;cp<127;++cp,++glyph){
  const unsigned at=ga+glyph*16;put(b,at,cp);put(b,at+4,unsigned(b.size()));put(b,at+8,encoded,2);
  b[at+10]=b[at+11]=1;put(b,at+12,encoded<32?4:5,2);b.push_back(cp==' '?0:255);
 }
 const unsigned empty=unsigned(b.size());b.push_back(0);for(unsigned i=0;i<ns;++i)put(b,sa+i*8,empty);
 const auto text=[&](unsigned slot,const std::string&value){put(b,sa+slot*8,unsigned(b.size()));put(b,sa+slot*8+4,unsigned(value.size()));b.insert(b.end(),value.begin(),value.end());b.push_back(0);};
 text(LocaleMarkerString,"eng");text(AuxiliaryCreditsString,"I0\nHSynthetic project\nRRole\tName\nCShared work\nMOne\nMTwo\nPAlpha\nPBeta\nPGamma\nI1\nI2\nI3\nS476\nI4");
 for(unsigned i=0;i<nc;++i){put(b,ca+i*16+4,1,2);put(b,ca+i*16+6,33,2);}
 put(b,12,unsigned(b.size()));return b;
}
static bool write_report(const char*bytes,unsigned size){++exports;if(!export_ok)return false;report.assign(bytes,size);return true;}
static void advance(unsigned ms){while(ms){const unsigned step=ms>100?100:ms;app.advance(step);ms-=step;}app.render();}
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static HitRegion find(HitKind kind,int value){app.render();for(int i=app.hit_count-1;i>=0;--i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return app.hits[i];CHECK(false);return {};}
static bool exists(HitKind kind,int value){app.render();for(unsigned i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return true;return false;}
static void click(HitKind kind,int value){const auto hit=find(kind,value);CHECK(hit.enabled);app.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);app.pointer_button(true);app.pointer_button(false);}
static void press(uint16_t code){CHECK(app.physical_key(code,true));CHECK(app.physical_key(code,false));}
static void capture(const char*name){if(!captures)return;app.render();FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(uint16_t pixel:app.pixels){uint8_t rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void retired_services(){
 snapshot();CHECK(!exists(HitKind::AuxiliaryControl,AuxiliaryOpenPatchNotes));
 app.open_auxiliary(AuxiliaryScreen::PatchNotes);CHECK(app.auxiliary.screen==AuxiliaryScreen::None);
 app.open_auxiliary(AuxiliaryScreen::Feedback);CHECK(app.auxiliary.screen==AuxiliaryScreen::None);
 app.auxiliary_control(AuxiliaryOpenFeedback);app.auxiliary_control(AuxiliaryOpenPatchNotes);
 CHECK(app.auxiliary.screen==AuxiliaryScreen::None);CHECK(!app.auxiliary_text('A'));
 char report[32]{};CHECK(!app.feedback_report(report,sizeof report));CHECK(!exports);unchanged();
}
static void settings_entries(){
 app.has_save=false;app.menu=1;app.input(Key::Confirm);CHECK(app.settings_open);
 CHECK(!exists(HitKind::UiAction,int(UiAction::SettingsFeedback)));
 // Traverse to a row below the first seven visible settings with real keys.
 app.pointer.visible=false;for(unsigned i=0;i<9;++i)app.input(Key::Down);
 CHECK(app.settings_item==9&&app.settings_top>0);const unsigned top=app.settings_top;
 app.input(Key::Confirm);CHECK(app.auxiliary.screen==AuxiliaryScreen::Credits);snapshot();
 app.input(Key::Back);CHECK(!app.auxiliary.closing);advance(2000);CHECK(app.auxiliary.credits_position>0);capture("credits-start");
 const int before_scroll=app.auxiliary.credits_target;app.pointer_scroll(2);CHECK(app.auxiliary.credits_target>before_scroll);
 advance(100);app.auxiliary.credits_position=app.auxiliary.credits_target=560*256;capture("credits-roles");
 app.input(Key::Back);CHECK(app.auxiliary.closing);advance(999);CHECK(app.auxiliary.screen==AuxiliaryScreen::Credits);advance(1);
 CHECK(app.auxiliary.screen==AuxiliaryScreen::None&&app.settings_open&&app.settings_top==top&&app.settings_item==9);unchanged();
 app.settings_action(UiAction::SettingsFeedback);CHECK(app.auxiliary.screen==AuxiliaryScreen::None&&!exports);
 app.input(Key::Down);CHECK(app.settings_item==10);app.input(Key::Confirm);CHECK(app.frontend_confirm==4);app.input(Key::Back);
}

}
int main(int argc,char**argv){
 Bytes pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);}else pack=synthetic();
 if(argc>2)captures=argv[2];CHECK(app.content.open(pack.data(),pack.size())&&app.content.current_profile());app.callbacks.write_feedback=write_report;
 new_run(app.game,0x1617,false);app.title=true;app.input_bindings=default_input_bindings();retired_services();settings_entries();
 CHECK(app.content.healthy());std::printf("Auxiliary UI8: %u checks; offline services removed, credits timing, settings scrolling and return passed\n",checks);
}
