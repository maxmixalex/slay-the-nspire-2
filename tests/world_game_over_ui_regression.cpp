#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "run_score.hpp"
#include "game_over_ui.hpp"
#include "ui_core_observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static const char* capture_directory;
static const char* character_names[]={"ironclad","silent","defect","regent","necrobinder"};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world game-over UI: %s:%d %s (event=%s phase=%u)\n",__FILE__,__LINE__,#x,original_event_key(a.game.original_events.id),unsigned(a.game.phase));std::exit(1);}}while(0)

struct TextIndex {const char* key;uint16_t index;};
static const TextIndex world_text[]={
#include "../src/world_text_index.inc"
};
static const TextIndex world_images[]={
#include "../src/world_image_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=uint8_t(value>>(8*i));}
static bool ends(const std::string&s,const char*tail){size_t n=std::strlen(tail);return s.size()>=n&&s.compare(s.size()-n,n,tail)==0;}
static std::vector<uint8_t> synthetic_profile(){
 // Distinct glyphs and image colors make saved facts and navigation visible.
 // This is an interaction fixture, not replacement artwork.
 constexpr unsigned images=U_Count>CardKinds?U_Count:CardKinds,glyphs=4*95;
 constexpr unsigned glyphdir=40+images*16,stringdir=glyphdir+glyphs*16;
 constexpr unsigned clipdir=stringdir+ContentStrings*8,carddir=clipdir+AnimationKinds*16;
 constexpr unsigned uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
 std::vector<uint8_t>b(payload);
 put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,20,glyphs);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
 for(unsigned i=0;i<images;i++){
  unsigned w=i?2:82,h=i?2:60;
  // Native controls preserve the original image aspect ratio. A square test
  // texture would turn the Proceed button into a tall opaque panel and cover
  // the saved-facts row, unlike the 104x51 source component.
  for(const auto&entry:world_images)if(entry.index==i&&(!std::strcmp(entry.key,"common.PROCEED")||!std::strcmp(entry.key,"common.PROCEED_OUTLINE"))){w=104;h=51;}
  unsigned offset=unsigned(b.size()),length=w*h*2;
  put(b,40+i*16,offset);put(b,44+i*16,w,2);put(b,46+i*16,h,2);put(b,52+i*16,length);b.resize(offset+length);
  for(unsigned pixel=0;pixel<w*h;pixel++)put(b,offset+pixel*2,((i*13+pixel*5)&31)<<11|((i*29+pixel*7)&63)<<5|((i*7+pixel*3)&31),2);
 }
 const unsigned sizes[]={8,10,12,16};unsigned glyph=0;
 for(unsigned size:sizes)for(unsigned cp=32;cp<127;cp++,glyph++){
  unsigned at=glyphdir+glyph*16,offset=unsigned(b.size());put(b,at,cp);put(b,at+4,offset);put(b,at+8,size,2);b[at+10]=5;b[at+11]=7;put(b,at+12,6,2);b.resize(offset+35);
  for(unsigned y=0;y<7;y++)for(unsigned x=0;x<5;x++)b[offset+y*5+x]=cp==' '?0:uint8_t((cp>>((x+y*3)%7)&1)?255:0);
 }
 std::vector<std::string>strings(ContentStrings,"SYNTHETIC_UI_TEXT");
 for(unsigned i=0;i<CardKinds;i++){strings[i*3]=definitions[i].key;strings[i*3+1]="CARD_DESCRIPTION";strings[i*3+2]="UPGRADED_DESCRIPTION";}
 for(const auto&row:world_text){
  CHECK(row.index<strings.size());std::string key=row.key;
  strings[row.index]=key;
  if(ends(key,".description")||ends(key,".eventDescription")||key.find(".talk.firstVisitEver.")!=std::string::npos){
   strings[row.index]="EVENT_READER_TEXT ";for(unsigned i=0;i<40;i++)strings[row.index]+="visible rules and saved choices ";
  }
  if(key=="ancients.PROCEED.title"||key=="events.PROCEED.title")strings[row.index]="PROCEED";
  if(key=="game_over_screen.SCORE_LINE.floorsClimbed")strings[row.index]="CLIMBED {FloorCount} floors";
  if(key=="game_over_screen.SCORE_LINE.ascension")strings[row.index]="ASCENSION {AscensionLevel}";
  if(key=="game_over_screen.SCORE_LINE.goldGained")strings[row.index]="TOTAL GOLD {GoldAmount}";
 }
 for(unsigned i=0;i<ContentStrings;i++){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
 for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
 for(unsigned i=0;i<CardKinds;i++){put(b,carddir+i*16,0);for(unsigned j=1;j<4;j++)put(b,carddir+i*16+j*4,i*3+j-1);}
 for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static bool actual_profile;
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void present(){a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.run_settings=false;a.map_preview.open=false;a.reset_presentation();a.physical_keys_cancel();a.pointer.visible=false;}
static void advance_ms(unsigned ms){while(ms){const unsigned step=ms>250?250:ms;a.advance(step);ms-=step;}}
static void advance_to(unsigned age){
 const auto&m=a.game_over_presentation;const unsigned now=m.summary?m.summary_age(a.map_clock_ms):m.initial_age(a.map_clock_ms);
 CHECK(now<=age);advance_ms(age-now);
}
static void settle_game_over(){
 a.sync_game_over_presentation();const auto&m=a.game_over_presentation;
 const unsigned age=m.summary?m.summary_age(a.map_clock_ms):m.initial_age(a.map_clock_ms),end=(m.summary?m.menu_start():m.intro_delay())+500;
 for(unsigned left=age<end?end-age:0;left;){const unsigned step=left>250?250:left;a.advance(step);left-=step;}
}
static bool has_action(UiAction action){for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(action)&&a.hits[i].enabled)return true;return false;}
static void press(UiAction action){
 a.render();HitRegion h{};unsigned count=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(action)){h=a.hits[i];++count;}
 CHECK(count==1&&h.enabled&&h.x>=0&&h.y>=0&&h.x+h.w<=ScreenW&&h.y+h.h<=ScreenH);
 a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;
}
static uint32_t pixels(int x,int y,int w,int h){uint32_t hash=2166136261u;for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++){hash^=a.pixels[yy*ScreenW+xx];hash*=16777619u;}return hash;}
static void capture(const char*name){
 if(!capture_directory)return;a.pointer.visible=false;a.render();char path[1024]{};int n=std::snprintf(path,sizeof path,"%s/%s.ppm",capture_directory,name);CHECK(n>0&&unsigned(n)<sizeof path);
 FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH;y++){for(unsigned x=0;x<ScreenW;x++){uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void capture_stage(const char*prefix,const char*stage){
 char name[160];const int n=std::snprintf(name,sizeof name,"gameover-%s-%s",prefix,stage);CHECK(n>0&&unsigned(n)<sizeof name);capture(name);
}
static unsigned detail_hits(bool discoveries){
 unsigned n=0;for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::GameOverDetail&&a.hits[i].enabled&&(a.hits[i].value>=32)==discoveries)++n;return n;
}
static uint16_t select_key(){a.keyboard_only=true;return a.input_bindings.keys[1][unsigned(InputCommand::Select)];}
static void intro_stages(const char*prefix){
 a.sync_game_over_presentation();CHECK(!a.game_over_summary&&a.game_over_presentation.initial_age(a.map_clock_ms)==0);
 const uint16_t select=select_key();CHECK(a.physical_key(select,true)&&!a.game_over_summary);
 capture_stage(prefix,"intro-start");advance_to(100);capture_stage(prefix,"intro-100ms");
 advance_to(200);capture_stage(prefix,"intro-200ms");
 if(actual_profile&&a.content.version()>=GameOverTransitionContentVersion&&a.game_over_presentation.event_room){a.render();CHECK(a.pixels[239*ScreenW+319]==0);}
 advance_to(950);capture_stage(prefix,"intro-950ms");a.render();CHECK(!has_action(UiAction::GameOverSummary));
 advance_to(a.game_over_presentation.intro_delay()-1);a.render();CHECK(!has_action(UiAction::GameOverSummary));unchanged();
 advance_ms(1);CHECK(a.game_over_button_ready());CHECK(a.physical_key(select,false)&&!a.game_over_summary);
 settle_game_over();capture_stage(prefix,"intro-settled");unchanged();
}
static void summary_stages(const char*prefix){
 CHECK(a.game_over_summary&&a.game_over_presentation.summary_age(a.map_clock_ms)==0);
 const uint16_t select=select_key();CHECK(a.physical_key(select,true)&&!a.title);
 capture_stage(prefix,"summary-start");advance_to(150);capture_stage(prefix,"summary-score");a.render();CHECK(!has_action(UiAction::GameOverMainMenu));
 const auto&m=a.game_over_presentation;
 if(m.rows>1){advance_to(550);capture_stage(prefix,"summary-score-next");}
 advance_to(m.badge_start()-1);a.render();CHECK(!detail_hits(false)&&!detail_hits(true));
 if(m.badges){advance_to(m.badge_start()+125);capture_stage(prefix,"summary-badges");a.render();CHECK(detail_hits(false)==1&&!detail_hits(true));}
 if(m.discoveries){advance_to(m.discovery_start()+125);capture_stage(prefix,"summary-discovery-header");a.render();CHECK(detail_hits(true)==1);
  advance_to(m.discovery_start()+250);capture_stage(prefix,"summary-discoveries");a.render();CHECK(detail_hits(true)==1);}
 advance_to(m.menu_start()-1);a.render();CHECK(!has_action(UiAction::GameOverMainMenu));unchanged();
 advance_ms(1);CHECK(a.game_over_button_ready());CHECK(a.physical_key(select,false)&&!a.title);
 settle_game_over();capture_stage(prefix,"summary-settled");unchanged();
}
static void start(Character character=Character::Ironclad){
 new_run(a.game,0x7195c3,false,character);CHECK(world_set_seed(a.game,"NSPIRE-GAME-OVER-017"));CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,3));present();
 a.begin_progress();
}
static void dead(Character character){start(character);player_kill(a.game,true);resolve_pending(a.game);CHECK(a.game.phase==Phase::Dead&&a.game.world.finished&&a.game.event_run.completion_recorded);present();snapshot();}
static void victory(){
 start();
 a.progress.current.partial=1;a.progress.current.facts.complete=0;
 // Supply room outcomes to the existing controller, as in the routing test.
 // This isolates the end-screen contract without claiming combat victories.
 for(unsigned step=0;step<192&&a.game.phase!=Phase::Complete;step++){
  if(world_can_travel(a.game)){uint8_t candidates[reference::Columns];unsigned count=world_available_points(a.game,candidates,reference::Columns);CHECK(count);CHECK(world_choose_point(a.game,candidates[0]));}
  else if(a.game.original_events.active&&a.game.original_events.id==OriginalEventId::TheArchitect){CHECK(original_event_choose(a.game,0));}
  else{if(a.game.original_events.active)a.game.original_events=OriginalEventState{};if(world_room_is_combat(a.game))a.game.phase=Phase::Reward;CHECK(world_finish_room(a.game));}
 }
 CHECK(a.game.phase==Phase::Complete&&a.game.world.finished&&a.game.original_events.id==OriginalEventId::TheArchitect&&a.game.event_run.completion_recorded);present();snapshot();
}
static void overlays_do_not_advance(){
 a.ui_action(UiAction::GameOverMainMenu);CHECK(!a.title&&!a.game_over_summary);unchanged();
 a.help=true;a.input(Key::Confirm);CHECK(!a.help&&!a.game_over_summary);unchanged();
 a.paused=true;a.menu=0;a.input(Key::Confirm);CHECK(!a.paused&&!a.game_over_summary);unchanged();
 a.panel=1;a.pile_tab=0;a.inspect_cursor=0;a.input(Key::Confirm);CHECK(a.panel==1&&!a.game_over_summary);if(a.detail_card>=0)a.input(Key::Back);a.input(Key::Back);CHECK(!a.panel);unchanged();
 struct Flag{bool*value;};Flag flags[]={{&a.library},{&a.character_select},{&a.map_preview.open}};
 for(auto f:flags){*f.value=true;a.ui_action(UiAction::GameOverSummary);CHECK(!a.game_over_summary);*f.value=false;unchanged();}
 a.settings_open=true;a.ui_action(UiAction::GameOverSummary);CHECK(!a.game_over_summary);a.settings_open=false;unchanged();
 for(uint8_t modal=1;modal<=3;modal++){a.frontend_confirm=modal;a.ui_action(UiAction::GameOverSummary);CHECK(!a.game_over_summary);a.frontend_confirm=0;unchanged();}
 a.status_actor=0;a.ui_action(UiAction::GameOverSummary);CHECK(!a.game_over_summary);a.status_actor=-1;unchanged();
 a.open_card_detail(a.game.deck[0],0);a.ui_action(UiAction::GameOverSummary);CHECK(!a.game_over_summary);
 // Inspect-card Confirm toggles the original upgrade tickbox; Back closes.
 const Card viewed=card_from_token(a.detail_card);a.input(Key::Confirm);CHECK(a.detail_card>=0&&!a.game_over_summary);
 if(viewed.upgraded||card_upgradeable(viewed))CHECK(card_from_token(a.detail_card).upgraded!=viewed.upgraded);
 unchanged();a.input(Key::Back);CHECK(a.detail_card<0&&!a.game_over_summary);unchanged();
 a.detail_item=0;a.ui_action(UiAction::GameOverSummary);CHECK(!a.game_over_summary);a.input(Key::Confirm);CHECK(a.detail_item<0&&!a.game_over_summary);unchanged();
}
static void saved_facts(){
 char text[512]{},number[32]{};a.localized_text("game_over_screen","SCORE_LINE.floorsClimbed",text,sizeof text);std::snprintf(number,sizeof number,"%u",a.game.world.total_floor);CHECK(std::strstr(text,number));
 a.localized_text("game_over_screen","SCORE_LINE.ascension",text,sizeof text);std::snprintf(number,sizeof number,"%u",a.game.world.ascension);CHECK(std::strstr(text,number));
 const auto*r=a.game_over_run();CHECK(r&&r->outcome);
 if(!actual_profile){a.localized_text("game_over_screen","SCORE_LINE.goldGained",text,sizeof text);
  if(ui_run_score(*r).complete){std::snprintf(number,sizeof number,"%u",r->gold_gained);CHECK(std::strstr(text,number)&&!std::strchr(text,'{'));}
  else CHECK(std::strstr(text,"GoldAmount"));}
 // The original summary now uses observed GoldGained. Changing only the
 // canonical balance cannot rewrite a completed historical summary.
 a.render();uint32_t old=pixels(80,103,160,96);int gold=a.game.gold;a.game.gold=int16_t(gold+7);a.render();CHECK(pixels(80,103,160,96)==old);a.game.gold=int16_t(gold);a.render();unchanged();
}
static void death_flow(){
 for(unsigned i=0;i<CharacterKinds;i++){
  dead(Character(i));a.render();CHECK(!has_action(UiAction::GameOverSummary)&&!has_action(UiAction::GameOverMainMenu));a.input(Key::Confirm);CHECK(!a.game_over_summary);intro_stages(character_names[i]);a.render();CHECK(has_action(UiAction::GameOverSummary));unchanged();
  if(!i){overlays_do_not_advance();capture("gameover-defeat");press(UiAction::GameOverSummary);}else a.input(Key::Confirm);
  CHECK(!a.title&&a.game_over_summary&&a.game.phase==Phase::Dead);a.render();CHECK(!has_action(UiAction::GameOverMainMenu)&&!has_action(UiAction::GameOverSummary));a.input(Key::Confirm);CHECK(!a.title);summary_stages(character_names[i]);a.render();CHECK(has_action(UiAction::GameOverMainMenu)&&!has_action(UiAction::GameOverViewRun));unchanged();saved_facts();
  if(!i){capture("gameover-defeat-summary");a.ui_action(UiAction::GameOverViewRun);a.open_game_over_history();CHECK(!a.history_open&&!a.history_from_game_over&&!a.compendium_open&&a.game_over_summary);unchanged();
   a.input(Key::Inspect);CHECK(a.panel==1);a.render();a.input(Key::Back);CHECK(!a.panel&&a.game_over_summary);a.input(Key::Relics);CHECK(a.panel==5);a.render();a.input(Key::Back);CHECK(!a.panel&&a.game_over_summary);unchanged();press(UiAction::GameOverMainMenu);}else a.input(Key::Confirm);
  CHECK(a.title&&!a.game_over_summary&&a.game.phase==Phase::Dead&&a.game.world.finished);unchanged();
 }
}
static void victory_flow(){
 victory();unsigned wins=a.game.event_run.wins;uint32_t wongo=a.game.event_run.wongo_total;intro_stages("victory");capture("gameover-victory");
 const auto select=select_key();CHECK(a.physical_key(select,true)&&!a.game_over_summary);CHECK(a.physical_key(select,true)&&!a.game_over_summary);CHECK(a.physical_key(select,false)&&a.game_over_summary&&!a.title);
 summary_stages("victory");saved_facts();capture("gameover-victory-summary");
 a.render();a.pointer_scroll(1);a.render();unchanged();press(UiAction::GameOverMainMenu);CHECK(a.title&&a.game.phase==Phase::Complete&&a.game.event_run.wins==wins&&a.game.event_run.wongo_total==wongo);unchanged();
 // Loading/reopening a terminal save starts on Continue, without repeating
 // any completion accounting. The UI flag is never serialized into the run.
 CHECK(load(a.game,before.bytes,before.size));present();CHECK(!a.game_over_summary);a.render();CHECK(!has_action(UiAction::GameOverSummary));settle_game_over();a.render();CHECK(has_action(UiAction::GameOverSummary));unchanged();
}
static void combat_backdrop_flow(){
 start(Character::Silent);
 // Supply only the opening ancient-room outcome to reach the actual first
 // combat. Its omitted choice is explicitly partial history; no score or
 // unseen earlier badge is invented to exercise the retained combat layer.
 a.progress.current.partial=1;a.progress.current.facts.complete=0;a.game.original_events={};CHECK(world_finish_room(a.game));
 uint8_t available[reference::Columns]{};CHECK(world_available_points(a.game,available,reference::Columns));
 {UiCoreObserver observer(a);CHECK(world_choose_point(a.game,available[0]));}
 CHECK(combat_phase(a.game.phase));
 {UiCoreObserver observer(a);player_kill(a.game,true);resolve_pending(a.game);}
 CHECK(a.game.phase==Phase::Dead&&a.game.world.finished);present();snapshot();
 a.sync_game_over_presentation();CHECK(!a.game_over_presentation.event_room);intro_stages("combat-silent");
 const auto select=select_key();CHECK(a.physical_key(select,true)&&!a.game_over_summary);CHECK(a.physical_key(select,false)&&a.game_over_summary);
 summary_stages("combat-silent");press(UiAction::GameOverMainMenu);CHECK(a.title);unchanged();
}
static void legacy_and_capacity(){
 new_run(a.game,1,true,Character::Ironclad);a.game.phase=Phase::Complete;present();a.input(Key::Confirm);CHECK(a.title&&!a.game_over_summary);
 start();a.game.phase=Phase::Error;present();a.render();CHECK(!has_action(UiAction::GameOverSummary));a.input(Key::Confirm);CHECK(a.title&&a.game.phase==Phase::Error&&!a.game_over_summary);
}
int main(int argc,char**argv){
 std::vector<uint8_t>data;actual_profile=argc>1;if(actual_profile){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=synthetic_profile();
 if(argc>2)capture_directory=argv[2];CHECK(a.init(data.data(),data.size(),{}));CHECK(a.content.current_profile());
 death_flow();victory_flow();combat_backdrop_flow();legacy_and_capacity();std::printf("world game-over UI: %u checks passed; %s profile; five full actors, source transition/summary stages, real release gates and unchanged saves\n",checks,actual_profile?"actual":"synthetic");
}
