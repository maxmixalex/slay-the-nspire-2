// Focused real-asset boundary and pointer regression for UI12. This is native
// correctness evidence only; calculator presentation/performance uses Firebird.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;static SaveData before,after;static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"UI12 frontend line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static HitRegion hit(HitKind kind,int value=-1){app.render();for(int i=app.hit_count-1;i>=0;--i)if(app.hits[i].kind==kind&&(value<0||app.hits[i].value==value))return app.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){auto h=hit(kind,value);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
static bool intersects(HitRegion a,HitRegion b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
int main(int argc,char**argv){
 if(argc!=2){std::fprintf(stderr,"usage: frontend_ui12_regression logical-private-pack\n");return 2;}
 FILE*f=std::fopen(argv[1],"rb");CHECK(f);std::vector<uint8_t>pack(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);
 CHECK(app.init(pack.data(),pack.size(),{}));new_run(app.game,0x12120,false);app.title=true;app.map_clock_ms=4000;CHECK(save(app.game,before));
 // A large original tab and a hover-brightened button must retain alpha on
 // both sides of the former 64px rejection, including partly clipped cases.
 for(const int width:{76,96,135}){uint16_t pixels[320*48]{};uint8_t alpha[320*48]{};Canvas c{app,pixels,320,320,48,alpha};
  small_material_icon(c,app.content.ui_image(I10_SettingsTab),0,0,width,24,1000,1200);
  unsigned left=0,right=0;for(int y=0;y<24;++y)for(int x=0;x<width;++x){if(x<width/2)left+=alpha[y*320+x]!=0;else right+=alpha[y*320+x]!=0;}CHECK(left>20&&right>20);
 }
 app.settings_open=true;app.settings_tab=0;app.settings_item=0;app.settings_top=0;app.pointer.visible=true;
 auto track=hit(HitKind::SettingsScroll);app.pointer_move(track.x+track.w/2,track.y+track.h-1);app.pointer_button(true);CHECK(app.settings_top==settings_layout::GeneralMaximum);
 app.pointer_move(track.x+track.w/2,track.y);CHECK(app.settings_top==0);app.pointer_move(track.x+track.w/2,track.y+track.h-1);CHECK(app.settings_top==settings_layout::GeneralMaximum);app.pointer_button(false);CHECK(!app.pointer.held);
 auto skip=hit(HitKind::UiAction,int(UiAction::SettingsSkipIntroLogo));CHECK(skip.enabled&&skip.x+skip.w<settings_layout::TooltipLeft&&!app.skip_intro_logo);
 click(HitKind::UiAction,int(UiAction::SettingsSkipIntroLogo));CHECK(app.skip_intro_logo);app.render();
 CHECK(app.hit_at(skip.x+skip.w/2,skip.y+skip.h/2).value==int(UiAction::SettingsSkipIntroLogo));
 click(HitKind::UiAction,int(UiAction::SettingsSkipIntroLogo));CHECK(!app.skip_intro_logo);
 auto credits=hit(HitKind::UiAction,int(UiAction::SettingsCredits));CHECK(credits.x+credits.w<settings_layout::TooltipLeft);click(HitKind::UiAction,int(UiAction::SettingsCredits));CHECK(app.auxiliary.screen==AuxiliaryScreen::Credits);
 app.map_clock_ms+=2200;app.input(Key::Back);CHECK(app.auxiliary.closing);for(int tick=0;tick<4;++tick)app.advance(250);CHECK(app.auxiliary.screen==AuxiliaryScreen::None&&app.settings_open);
 click(HitKind::UiAction,int(UiAction::SettingsInputTab));CHECK(app.settings_tab==1);
 track=hit(HitKind::SettingsScroll);app.pointer_move(track.x+track.w/2,track.y+track.h-1);app.pointer_button(true);CHECK(app.settings_top==settings_layout::InputMaximum);app.pointer_move(track.x,track.y);CHECK(app.settings_top==0);app.pointer_button(false);
 app.input(Key::Back);CHECK(!app.settings_open);app.compendium_open=true;app.progress_page_started=app.map_clock_ms-500;
 auto back=hit(HitKind::UiAction,int(UiAction::Back));for(unsigned i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value!=int(UiAction::Back))CHECK(!intersects(back,app.hits[i]));
 app.compendium_open=false;app.statistics_open=true;app.progress.characters_started=31;app.render();
 const UiStatisticsLayout stats(app.progress);CHECK(stats.count==23&&stats.character_count==5);CHECK(UiStatisticsLayout::Left>=52);
 back=hit(HitKind::UiAction,int(UiAction::Back));for(unsigned i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::StatisticsEntry)CHECK(!intersects(back,app.hits[i]));
 CHECK(UiHistoryLayout::Left>=back.x+back.w);UiRunHistory history{};history.floor_count=40;history.relic_count=25;const UiHistoryLayout layout(history,40);
 for(unsigned i=0;i<history.floor_count;++i){auto p=layout.floor(history,i);CHECK(p.x>=52&&p.x+p.w<=290);}for(unsigned i=0;i<40;++i){auto p=layout.card(i);CHECK(p.x>=52&&p.x+p.w<=290);}
 CHECK(profile_overflow_offset(90,0)==0&&profile_overflow_offset(90,1400)==0);CHECK(profile_overflow_offset(90,7250)==90);
 CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));CHECK(app.content.healthy());
 std::printf("UI12 frontend: %u checks; real source textures, scrollbar drag, Credits flow, Back separation and canonical bytes pass\n",checks);
}
