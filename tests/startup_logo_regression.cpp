// First-host-start behavior with an independent source timing oracle. The
// optional private-pack mode also renders the real sampled Spine artwork;
// native timing is never a calculator performance claim.
#include "../src/app.cpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static unsigned checks;
static App a;
static State canonical;
static UiPreferences preferences;
static const char*captures=nullptr;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"startup logo:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void clocks(){
 StartupLogoPresentation logo;
 CHECK(logo.begin(100,true,false)&&logo.active()&&logo.attempted);
 CHECK(!logo.begin(200,true,false));
 for(unsigned age:{0u,799u,800u,1799u}){const auto f=logo.sample(100+age);CHECK(!f.menu&&!f.alpha&&!f.background);}
 const auto first=logo.sample(1900);CHECK(!first.menu&&!first.alpha&&first.image==0&&first.y==-152);
 for(unsigned elapsed=0;elapsed<=500;elapsed+=7){
  const double t=elapsed/500.0,u=t-1,back=1+u*u*(2.70158*u+1.70158);
  const auto f=logo.sample(1900+elapsed);
  CHECK(std::abs(f.y-(-800.0*4/21*(1-back)))<2.0);
  CHECK(std::abs(f.alpha-255*t)<1.1);
 }
 for(unsigned elapsed=0;elapsed<3000;elapsed+=13){
  const auto f=logo.sample(1900+elapsed);CHECK(f.image==elapsed*12/1000&&!f.menu);
 }
 const auto settled=logo.sample(4900);CHECK(settled.image==36&&settled.alpha==255&&!settled.background&&settled.y==0);
 const auto color=logo.sample(5900);CHECK(color.background==3584&&color.alpha==255);
 const auto hold=logo.sample(6900);CHECK(hold.background==4096&&!hold.black);
 CHECK(!logo.sample(8699).menu);CHECK(logo.sample(8700).menu&&logo.sample(8700).black==255);
 CHECK(logo.sample(9700).black==223);CHECK(!logo.sample(10700).black);
 logo.advance(10699);CHECK(logo.active());logo.advance(10700);CHECK(!logo.active());
 CHECK(!logo.begin(10800,true,false));
 for(unsigned age:{400u,1200u,2050u}){
  StartupLogoPresentation cancel;CHECK(cancel.begin(0,true,false));cancel.cancel(age);
  const unsigned menu=age==400?1200:age==1200?2600:3100;
  CHECK(cancel.menu_at()==menu);cancel.cancel(age+1);CHECK(cancel.menu_at()==menu);
  CHECK(!cancel.sample(menu-1).menu&&cancel.sample(menu).menu);
  if(age==2050){const auto start=cancel.sample(2050),mid=cancel.sample(2175),end=cancel.sample(2300);
   CHECK(start.alpha==127&&mid.alpha<5&&mid.y==start.y&&!end.alpha);}
  cancel.advance(menu+1999);CHECK(cancel.active());cancel.advance(menu+2000);CHECK(!cancel.active());
 }
 StartupLogoPresentation hold_cancel;CHECK(hold_cancel.begin(0,true,false));hold_cancel.cancel(4800);
 CHECK(hold_cancel.menu_at()==8600&&hold_cancel.canceled_at==UINT32_MAX);
 StartupLogoPresentation skip;CHECK(skip.begin(0,true,true)&&skip.sample(0).menu);
 skip.cancel(500);CHECK(skip.menu_at()==0);skip.advance(1999);CHECK(skip.active());skip.advance(2000);CHECK(!skip.active());
 StartupLogoPresentation old;CHECK(!old.begin(0,false,false)&&old.attempted&&!old.active());CHECK(!old.begin(1,true,false));
 StartupLogoPresentation wrap;CHECK(wrap.begin(UINT32_MAX-999,true,false));wrap.advance(9600);CHECK(!wrap.active());
}
static std::vector<uint8_t> read(const std::string&path){
 FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);
 CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>b(size_t(n),0);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));return b;
}
static bool load_preferences(UiPreferences&out){out=preferences;return true;}
static void unchanged(){CHECK(!std::memcmp(&canonical,&a.game,sizeof canonical));}
static void advance(unsigned ms){while(ms){const unsigned step=ms>33?33:ms;a.advance(step);ms-=step;}a.render();unchanged();CHECK(a.content.healthy());}
static void capture(const char*name){
 if(!captures)return;FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(unsigned y=0;y<240;++y){unsigned char row[960];for(unsigned x=0;x<320;++x){const unsigned p=a.pixels[y*320+x];row[x*3]=(p>>11)*255/31;row[x*3+1]=((p>>5)&63)*255/63;row[x*3+2]=(p&31)*255/31;}
  CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static unsigned ink(){unsigned count=0;for(auto pixel:a.pixels)count+=pixel!=0;return count;}
static void actual(const std::string&root){
 auto common=read(root+"/nSpire2.common.tns"),eng=read(root+"/nSpire2.eng.tns");SplitResourceSource split;
 CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(eng.data(),eng.size()),ResourceLanguage::English));
 Callbacks callbacks{};callbacks.load_preferences=load_preferences;new_run(a.game,0x41cef1ea,true);
 CHECK(a.init(split.source(),callbacks)&&a.content.has_startup_logo_assets()&&a.content.current_profile());canonical=a.game;
 CHECK(boot_probe_language(split.source())==BootLanguage::English);
 CHECK(a.begin_startup_logo()&&!a.hit_count);a.render();CHECK(!ink());
 a.input(Key::Right);a.ui_action(UiAction::OpenStandardRun);CHECK(a.startup_logo.canceled_at==UINT32_MAX&&a.title&&!a.character_select&&a.run_mode.screen==RunMenu::Closed);unchanged();
 const unsigned began=a.map_clock_ms;
 for(unsigned frame=0;frame<=36;++frame){
  const unsigned at=1800+(frame*1000+11)/12;advance(began+at-a.map_clock_ms);
  CHECK(!a.hit_count);if(frame>=12)CHECK(ink()>500);
  if(frame==6)capture("logo-enter");if(frame==18)capture("logo-spine-motion");if(frame==36)capture("logo-final-pose");
 }
 advance(began+6800-a.map_clock_ms);CHECK(a.pixels[0]==rgb(7,66,84));capture("logo-source-background");
 advance(began+8600-a.map_clock_ms);CHECK(a.startup_logo.active()&&!ink());
 advance(1000);capture("logo-main-menu-fade");advance(1000);CHECK(!a.startup_logo.active()&&a.title&&a.hit_count);capture("logo-main-menu");
 a.reset_presentation();CHECK(!a.begin_startup_logo()&&a.startup_logo.attempted);unchanged();
 CHECK(a.reload_language(split.source()));CHECK(!a.startup_logo.active()&&!a.begin_startup_logo());unchanged();a.settings_open=false;
 // Simulate a fresh process. A select press is consumed until its release,
 // even when the user continues to hold Enter as the title becomes visible.
 a.startup_logo={};a.map_clock_ms=0;CHECK(a.begin_startup_logo());advance(2000);
 CHECK(a.physical_key('K',true)&&a.startup_logo.canceled_at==UINT32_MAX);CHECK(a.physical_key('K',false));
 CHECK(a.physical_key(13,true));CHECK(a.startup_logo.canceled_at==2000);advance(3050);CHECK(!a.startup_logo.active());
 CHECK(a.physical_key(13,true)&&a.title&&!a.character_select);CHECK(a.physical_key(13,false));unchanged();
 a.startup_logo={};a.map_clock_ms=0;CHECK(a.begin_startup_logo());advance(1900);a.pointer_move(160,120);a.pointer_button(true);
 CHECK(a.startup_logo.canceled_at==1900&&!a.pointer.held);advance(3050);a.pointer_button(false);CHECK(a.title&&!a.character_select);unchanged();
 // A persisted skip bypasses every logo frame; regular initialization and
 // language changes continue to preserve the first-host-launch guard.
 preferences.skip_intro_logo=true;CHECK(a.init(split.source(),callbacks));a.startup_logo={};a.map_clock_ms=0;
 CHECK(a.skip_intro_logo&&a.begin_startup_logo()&&a.startup_logo.sample(0).menu);advance(2000);CHECK(!a.startup_logo.active());
 preferences.skip_intro_logo=false;
 for(unsigned language=1;language<=LocaleLanguageCount;++language){
  const auto info=locale_language_info(language);auto localized=read(root+"/"+info.file);
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(localized.data(),localized.size()),ResourceLanguage(language)));
  CHECK(boot_probe_language(split.source())==BootLanguage(language));CHECK(a.init(split.source(),callbacks)&&a.content.has_startup_logo_assets()&&a.content.current_profile());
  for(const char*key:{"SKIP_INTRO_LOGO_HEADER","SKIP_INTRO_LOGO_DESCRIPTION","TOAST_SKIP_INTRO_LOGO_ON","TOAST_SKIP_INTRO_LOGO_OFF"}){
   const std::string value=a.content.world_string("settings_ui",key);CHECK(!value.empty());
   for(const char*text=value.c_str();*text;){const uint32_t cp=utf8(text);if(cp==10||cp==13)continue;
    for(unsigned face:{0u,5u})for(int size:{10,12}){const auto glyph=a.content.glyph(cp,size,face);CHECK(glyph.pixels&&glyph.w&&glyph.h);}}
  }
  CHECK(!a.begin_startup_logo()&&!a.startup_logo.active());unchanged();CHECK(a.content.healthy());
 }
}
int main(int argc,char**argv){CHECK(argc>=1&&argc<=3);clocks();if(argc>2)captures=argv[2];if(argc>1)actual(argv[1]);
 std::printf("startup logo: %u source-clock, skip, host-lifetime and locale checks%s\n",checks,argc>1?"; real private Spine resources rendered":"");}
