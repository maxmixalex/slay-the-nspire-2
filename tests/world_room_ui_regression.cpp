#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "world_room_assets.hpp"
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
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world room UI: %s:%d %s (event=%s phase=%u)\n",__FILE__,__LINE__,#x,original_event_key(a.game.original_events.id),unsigned(a.game.phase));std::exit(1);}}while(0)

struct TextIndex {const char* key;uint16_t index;};
static const TextIndex world_text[]={
#include "../src/world_text_index.inc"
};
static const TextIndex room_clips[]={
#include "../src/world_clip_index.inc"
};
static const TextIndex room_images[]={
#include "../src/world_image_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=uint8_t(value>>(8*i));}
static bool ends(const std::string&s,const char*tail){size_t n=std::strlen(tail);return s.size()>=n&&s.compare(s.size()-n,n,tail)==0;}
static std::vector<uint8_t> synthetic_profile(){
 // Distinct test glyphs and image colors make a changed price, stock icon or
 // leaked old offer observable. This is an interaction fixture, not artwork.
 constexpr unsigned image_base=U_Count>CardKinds?U_Count:CardKinds,actor_base=image_base+99,images=actor_base+4,glyphs=4*95;
 constexpr unsigned glyphdir=40+images*16,stringdir=glyphdir+glyphs*16;
 constexpr unsigned clipdir=stringdir+ContentStrings*8,carddir=clipdir+AnimationKinds*16;
 constexpr unsigned uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
 std::vector<uint8_t>b(payload);
 put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,20,glyphs);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
 for(unsigned i=0;i<images;i++){
  unsigned w=i?2:82,h=i?2:60;
  for(const auto&entry:room_images)if(entry.index==i&&(!std::strcmp(entry.key,"common.PROCEED")||!std::strcmp(entry.key,"common.PROCEED_OUTLINE"))){w=104;h=51;}
  bool holder=false;for(const auto&entry:room_images)if(entry.index==i&&!std::strcmp(entry.key,"relics.VAJRA"))holder=true;
  const bool actor=i>=actor_base;unsigned offset=unsigned(b.size()),length=w*h*(actor||holder?3:2);
  put(b,40+i*16,offset);put(b,44+i*16,w,2);put(b,46+i*16,h,2);put(b,52+i*16,length);b.resize(offset+length);
  if(actor){
   // A real transparent frame keeps the newly appended actor clips present
   // without letting the old opaque image 0 hide the room backgrounds.
   // Defect alone has three diagnostic planes: body, additive light, front.
   constexpr uint16_t colors[4][4]={{0,0,0,0},{0x4184,0x4184,0x4184,0x4184},{0x18a7,0x18a7,0,0x18a7},{0,0x0849,0,0x1306}};
   constexpr uint8_t alpha[4][4]={{0,0,0,0},{255,255,255,255},{255,255,0,255},{0,255,0,128}};
   put(b,48+i*16,1);for(unsigned pixel=0;pixel<4;pixel++){put(b,offset+pixel*2,colors[i-actor_base][pixel],2);b[offset+8+pixel]=alpha[i-actor_base][pixel];}
  }else if(holder){
   // An opaque pixel, half-alpha pixel and transparent hole distinguish
   // source RGB modulation from a black rectangle over the composition.
   constexpr uint8_t alpha[]={255,128,0,255};put(b,48+i*16,1);
   for(unsigned pixel=0;pixel<4;pixel++){put(b,offset+pixel*2,0xffff,2);b[offset+8+pixel]=alpha[pixel];}
  }else for(unsigned pixel=0;pixel<w*h;pixel++)put(b,offset+pixel*2,((i*13+pixel*5)&31)<<11|((i*29+pixel*7)&63)<<5|((i*7+pixel*3)&31),2);
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
  if(key.find("rest_site_ui.")==0)strings[row.index]=key;
 }
 for(unsigned i=0;i<ContentStrings;i++){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
 for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
 unsigned next_frame=image_base;
 for(const auto&entry:room_clips)if(!std::strncmp(entry.key,"treasure.",9)){
  const bool opening=ends(entry.key,".OPENING");unsigned count=opening?23:10;
  put(b,clipdir+entry.index*16,next_frame);put(b,clipdir+entry.index*16+4,count,2);put(b,clipdir+entry.index*16+6,opening?49:50,2);next_frame+=count;
 }
 CHECK(next_frame==actor_base);
 for(const auto&entry:room_clips)if(!std::strncmp(entry.key,"rest_actor.",11)){
  unsigned frame=actor_base;if(!std::strncmp(entry.key,"rest_actor.DEFECT.",18))frame+=1+(ends(entry.key,".P1")?1:ends(entry.key,".P2")?2:0);
  put(b,clipdir+entry.index*16,frame);
 }
 for(unsigned i=0;i<CardKinds;i++){put(b,carddir+i*16,0);for(unsigned j=1;j<4;j++)put(b,carddir+i*16+j*4,i*3+j-1);}
 for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static SaveData stored;
static bool actual_profile;
static bool memory_save(const State&s){return save(s,stored);}
static bool memory_load(State&s){return stored.size&&load(s,stored.bytes,stored.size);}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void present(){a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.run_settings=false;a.map_preview.open=false;a.reset_presentation();a.pointer.visible=false;a.fast_mode=false;a.tick_remainder=0;}
static HitRegion hit(HitKind kind,int value){
 unsigned count=0;HitRegion result{};for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value){result=a.hits[i];++count;}
 CHECK(count==1&&result.x>=0&&result.y>=0&&result.w>0&&result.h>0&&result.x+result.w<=ScreenW&&result.y+result.h<=ScreenH);return result;
}
static bool exists(HitKind kind,int value,bool enabled=true){for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value&&(!enabled||a.hits[i].enabled))return true;return false;}
static unsigned choices(){unsigned count=0;for(int i=0;i<a.hit_count;i++)count+=a.hits[i].kind==HitKind::Choice;return count;}
static void click(HitKind kind,int value){a.render();auto h=hit(kind,value);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;}
static void click(UiAction action){click(HitKind::UiAction,int(action));}
static void capture(const char*name){
 if(!capture_directory)return;a.pointer.visible=false;a.render();char path[1024]{};int n=std::snprintf(path,sizeof path,"%s/%s.ppm",capture_directory,name);CHECK(n>0&&unsigned(n)<sizeof path);
 FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH;y++){for(unsigned x=0;x<ScreenW;x++){uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static uint16_t scene_pixel(){a.render();return a.pixels[100*ScreenW+5];}
static void rest_scene_pixel(const char*key){
 if(actual_profile)return;const int id=a.content.world_image("rest_background",key);CHECK(id>=0);
 const auto image=a.content.image(id);CHECK(image.pixels&&image.w==2&&image.h==2&&!image.alpha&&!image.palette);
 const uint16_t expected=uint16_t(image.pixels[0]|uint16_t(image.pixels[1])<<8);
 // The fixture's ordinary actor is transparent. This uncovered point must
 // come from the exact registered scene, never the legacy fallback image.
 CHECK(scene_pixel()==expected);
}
static void advance_to(unsigned tick){while(a.animation<tick)a.advance(33);CHECK(a.animation==tick);a.render();}
static void room_clock(unsigned milliseconds){while(milliseconds){const unsigned step=milliseconds>250?250:milliseconds;a.advance(step);milliseconds-=step;}a.render();}
static void settle_rest_buttons(){a.render();const auto&m=a.room_presentation.rest;if(!m.mouse_ready(a.map_clock_ms))room_clock(500-(a.map_clock_ms-m.start));}
static void treasure_alpha(unsigned chest,unsigned collection,unsigned banner,int y=0){
 CHECK(a.treasure_chest_opacity()==chest);CHECK(a.treasure_collection_opacity()==collection);CHECK(a.treasure_banner_opacity()==banner);CHECK(a.treasure_banner_offset()==y);
}
static void cleared_treasure(){
 CHECK(!a.treasure_presentation&&!a.treasure_open_tick&&!a.treasure_banner_tick&&!a.treasure_close_tick);
 CHECK(a.treasure_chest_from==65535&&!a.treasure_banner_from&&!a.treasure_banner_y);
 CHECK(!a.treasure_holder_delay_us);
 CHECK(!a.treasure_collection_started&&!a.treasure_collection_start_ms&&!a.treasure_skip_ready);
 CHECK(!a.treasure_room_seed&&a.treasure_room_act==255&&a.treasure_room_point==0xffff);
}
static void add(RelicId relic){CHECK(gain_world_relic(a.game,relic));CHECK(!world_relic_pending(a.game));}
static void finish_routing_fixture(){
 // Supply outcomes only for unrelated rooms, matching world_map_regression.
 // The room under test is entered by the real generated map controller.
 if(a.game.original_events.active)a.game.original_events=OriginalEventState{};
 if(world_room_is_combat(a.game))a.game.phase=Phase::Reward;
 CHECK(world_finish_room(a.game));CHECK(world_valid_run(a.game));
}
static bool route_to_shop(const reference::ReferenceMap&map,unsigned point,unsigned depth=0){
 if(point>=map.count||depth>reference::MaxRows+2)return false;
 if(map.points[point].type==reference::PointType::Shop)return true;
 const auto&children=map.points[point].children;
 for(unsigned i=0;i<children.used;++i)if(route_to_shop(map,children.slots[i],depth+1))return true;
 return false;
}
static void begin_room(WorldRoom room,reference::ActKind region=reference::ActKind::Overgrowth,unsigned act=0,const RelicId*relics=nullptr,unsigned count=0,Character character=Character::Ironclad){
 new_run(a.game,0x7195c3,false,character);CHECK(world_set_seed(a.game,"NSPIRE-WORLD-017"));CHECK(world_begin_run(a.game,region,0));
 for(unsigned i=0;i<count;i++)add(relics[i]);
 for(unsigned guard=0;guard<120;guard++){
  if(a.game.world.room_open&&a.game.world.room_type==unsigned(room)&&a.game.world.act_index==act){present();if(room==WorldRoom::Rest)settle_rest_buttons();CHECK(validate(a.game));return;}
  CHECK(a.game.phase!=Phase::Dead&&a.game.phase!=Phase::Complete&&a.game.phase!=Phase::Error);
  if(world_can_travel(a.game)){uint8_t ids[reference::Columns];unsigned n=world_available_points(a.game,ids,reference::Columns);CHECK(n);unsigned chosen=ids[0];
   // The historical leftmost route contains no shop. Choose a generated,
   // reachable branch to this newly covered room instead of finishing a run.
   if(room==WorldRoom::Shop&&a.game.world.act_index==act){const auto*map=world_current_map(a.game);CHECK(map);bool found=false;
    for(unsigned i=0;i<n;++i)if(route_to_shop(*map,ids[i])){chosen=ids[i];found=true;break;}CHECK(found);}
   CHECK(world_choose_point(a.game,chosen));}
  else finish_routing_fixture();
 }
 CHECK(false);
}
static void blocked_actions(UiAction action,bool capture_pointer=true){
 snapshot();
 bool*flags[]={&a.title,&a.paused,&a.help,&a.library,&a.character_select,&a.map_preview.open,&a.event_reader,&a.settings_open};
 for(bool*flag:flags){*flag=true;a.ui_action(action);*flag=false;unchanged();}
 for(uint8_t modal=1;modal<=3;modal++){a.frontend_confirm=modal;a.ui_action(action);a.frontend_confirm=0;unchanged();}
 int*indexes[]={&a.detail_card,&a.detail_item,&a.status_actor,&a.deck_candidate};
 for(int*index:indexes){*index=0;a.ui_action(action);*index=-1;unchanged();}
 a.panel=1;a.ui_action(action);a.panel=0;unchanged();
 if(!capture_pointer)return;
 // A press captured on a real room action cannot be released through Pause.
 a.render();auto h=hit(HitKind::UiAction,int(action));CHECK(h.enabled);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);
 a.input(Key::Back);CHECK(a.paused);a.pointer_button(false);unchanged();a.input(Key::Back);CHECK(!a.paused);a.pointer.visible=false;
}
static void ordinary_rest(){
 begin_room(WorldRoom::Rest);CHECK(a.camp_options(nullptr,0)==2);a.render();CHECK(choices()==2&&!world_rest_can_proceed(a.game));
 snapshot();a.input(Key::EndTurn);unchanged();click(HitKind::Choice,1);CHECK(a.game.phase==Phase::Upgrade);a.input(Key::Back);CHECK(a.game.phase==Phase::Camp&&!a.game.world.rest_used&&!world_rest_can_proceed(a.game));
 a.game.hp=30;present();settle_rest_buttons();const int heal_before=a.game.hp;const uint16_t lit=scene_pixel();rest_scene_pixel("OVERGROWTH");capture("rest-overgrowth-choices");click(HitKind::Choice,0);
 CHECK(a.game.phase==Phase::Camp&&a.game.hp>heal_before&&a.game.world.room_open&&world_rest_can_proceed(a.game));CHECK(!a.camp_options(nullptr,0));a.render();CHECK(!choices()&&exists(HitKind::UiAction,int(UiAction::LeaveCamp)));
 if(!actual_profile)CHECK(scene_pixel()!=lit);rest_scene_pixel("OVERGROWTH_DARK");capture("rest-overgrowth-completed");blocked_actions(UiAction::LeaveCamp);
 snapshot();CHECK(a.save_current_run());a.advance(250);CHECK(a.load_saved_run());CHECK(a.game.phase==Phase::Camp&&!a.camp_options(nullptr,0));a.render();unchanged();
 const int hp=a.game.hp;const unsigned rooms=a.game.world.rooms_completed;a.input(Key::Confirm);CHECK(a.game.phase==Phase::Camp&&a.game.world.room_open==2&&a.map_window.is_open&&a.game.hp==hp&&a.game.world.rooms_completed==rooms+1);snapshot();a.ui_action(UiAction::LeaveCamp);unchanged();
}
static void rest_actor_layers(){
 begin_room(WorldRoom::Rest,reference::ActKind::Overgrowth,0,nullptr,0,Character::Defect);
 for(unsigned dark=0;dark<2;dark++){
  if(dark){click(HitKind::Choice,0);CHECK(a.game.phase==Phase::Camp&&!a.camp_options(nullptr,0));}
  snapshot();const uint32_t cards=a.game.rng_cards,world=a.game.rng_world;reference::Random streams[WorldRngCount];for(unsigned i=0;i<WorldRngCount;i++)streams[i]=a.game.world.rng[i];
  for(unsigned frame=0;frame<5;frame++){
   a.advance(frame?250:0);a.render();
   if(!actual_profile){
    // Uncovered points in all four quadrants verify normal/add/normal order:
    // body + light, opaque front, body alone, then translucent front.
    CHECK(a.pixels[100*ScreenW+5]==0x5a2b);CHECK(a.pixels[100*ScreenW+315]==0x0849);
    CHECK(a.pixels[200*ScreenW+5]==0x4184);
    // The original Proceed arrow extends above its label and shadows the
    // old (315,200) sample after resting. This point is still in the lower
    // right actor quadrant, clear of both the description and button.
    CHECK(a.pixels[150*ScreenW+315]==0x32a8);
   }
   unchanged();
  }
  CHECK(a.game.rng_cards==cards&&a.game.rng_world==world);
  for(unsigned i=0;i<WorldRngCount;i++){CHECK(streams[i].calls==a.game.world.rng[i].calls&&streams[i].good==a.game.world.rng[i].good);for(unsigned j=0;j<4;j++)CHECK(streams[i].words[j]==a.game.world.rng[i].words[j]);}
 }
}
static void combat_backgrounds(){
 struct Region{reference::ActKind start;unsigned act;const char*key;};
 const Region regions[]={{reference::ActKind::Overgrowth,0,"OVERGROWTH"},{reference::ActKind::Underdocks,0,"UNDERDOCKS"},{reference::ActKind::Overgrowth,1,"HIVE"},{reference::ActKind::Overgrowth,2,"GLORY"}};
 for(const auto&r:regions){
  begin_room(WorldRoom::Monster,r.start,r.act);const auto*encounter=encounter_definition(a.game.enemy_world.encounter);
  CHECK(encounter&&a.content.world_image("backgrounds",encounter->key)<0);
  const int expected=a.content.world_image("backgrounds",r.key);CHECK(expected>=0&&expected!=a.content.ui_image(U_BattleBackground));
  snapshot();CHECK(a.combat_background()==expected);a.render();unchanged();
 }
 // This fixed real route reaches The Kin. Its dedicated scene must retain
 // priority over the current region after region-key normalization.
 begin_room(WorldRoom::Boss);const auto*encounter=encounter_definition(a.game.enemy_world.encounter);
 CHECK(encounter&&!std::strcmp(encounter->key,"THE_KIN_BOSS"));
 const int boss=a.content.world_image("backgrounds","THE_KIN_BOSS"),region=a.content.world_image("backgrounds","OVERGROWTH");
 CHECK(boss>=0&&region>=0&&boss!=region);snapshot();CHECK(a.combat_background()==boss);a.render();unchanged();
}
static void paged_tent(){
 const RelicId relics[]={WorldRelicMiniatureTent,WorldRelicShovel,WorldRelicGirya,WorldRelicPumpkinCandle,WorldRelicMeatCleaver};
 begin_room(WorldRoom::Rest,reference::ActKind::Overgrowth,0,relics,5);uint8_t visible[MaxWorldRestOptions];CHECK(a.camp_options(visible,MaxWorldRestOptions)==6);a.render();CHECK(choices()==4);
 snapshot();click(UiAction::NextCampPage);CHECK(a.cursor==4);a.render();CHECK(choices()==2&&exists(HitKind::Choice,4)&&exists(HitKind::Choice,5));unchanged();capture("rest-tent-page-2");
 CHECK(a.game.world.rest_options[4]==unsigned(WorldRestOption::Kindle));int candle=-1;for(unsigned i=0;i<a.game.world_relics.count;i++)if(a.game.world_relics.entries[i].id==WorldRelicPumpkinCandle)candle=int(i);CHECK(candle>=0);const int previous=a.game.world_relics.entries[candle].values[0];
 click(HitKind::Choice,4);CHECK(a.game.phase==Phase::Camp&&a.game.world_relics.entries[candle].values[0]==previous+5&&world_rest_can_proceed(a.game));CHECK(!world_rest_option_visible(a.game,4)&&a.camp_options(visible,MaxWorldRestOptions)==5&&visible[4]==5);
 snapshot();a.cursor=3;a.input(Key::Right);CHECK(a.cursor==5);a.input(Key::Right);CHECK(a.cursor==0);a.input(Key::Left);CHECK(a.cursor==5);unchanged();
 // Both disabled options remain visible for their real core reason. Their
 // original slot IDs survive removing an option on a different page.
 while(a.game.deck_count>1)CHECK(event_remove_deck_card(a.game,a.game.deck_count-1));a.game.deck[0].upgraded=1;
 CHECK(!world_rest_option_enabled(a.game,1)&&!world_rest_option_enabled(a.game,5));a.cursor=5;a.render();CHECK(choices()==1&&!hit(HitKind::Choice,5).enabled);snapshot();click(HitKind::Choice,5);a.input(Key::Confirm);unchanged();
 click(UiAction::PreviousCampPage);a.cursor=1;a.render();CHECK(!hit(HitKind::Choice,1).enabled);snapshot();click(HitKind::Choice,1);a.input(Key::Confirm);unchanged();capture("rest-disabled-smith");
 click(UiAction::LeaveCamp);CHECK(a.game.phase==Phase::Camp&&a.game.world.room_open==2&&a.map_window.is_open);snapshot();
}
static void closed_chest(){
 begin_room(WorldRoom::Treasure);CHECK(!a.game.treasure_state&&a.game.world.treasure_relic!=NoWorldRelic);a.render();
 CHECK(exists(HitKind::UiAction,int(UiAction::TreasureOpen))&&!a.treasure_can_proceed()&&!world_can_travel(a.game));
 snapshot();a.ui_action(UiAction::TreasureLeave);a.input(Key::EndTurn);unchanged();
 const int gold=a.game.gold;const unsigned relics=a.game.world_relics.count;
 capture("treasure-unopened");a.input(Key::Map);a.advance(250);a.advance(200);a.render();
 CHECK(a.map_window.is_open&&a.panel==3&&!a.map_routes_active());
 for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::WorldNode||!a.hits[i].enabled);
 a.input(Key::Confirm);CHECK(!a.map_travel.pending&&a.game.phase==Phase::Treasure);unchanged();
 a.input(Key::Map);a.advance(250);a.advance(100);CHECK(!a.map_window.visible&&a.game.gold==gold&&a.game.world_relics.count==relics);unchanged();
 CHECK(!a.game.treasure_state&&a.game.world.room_open==1);a.render();CHECK(exists(HitKind::UiAction,int(UiAction::TreasureOpen)));
}

static void opening_and_reload(){
 begin_room(WorldRoom::Treasure);const int gold=a.game.gold;const unsigned relics=a.game.world_relics.count;click(UiAction::TreasureOpen);
 const uint32_t presentation_random=a.treasure_random;CHECK(presentation_random&&a.treasure_holder_delay_us>=200000&&a.treasure_holder_delay_us<400000);
 CHECK(a.game.phase==Phase::Treasure&&a.game.treasure_state==1&&a.game.world.treasure_gold_claimed&&a.game.gold>gold&&a.treasure_presentation==2&&!a.treasure_can_proceed());
 CHECK(a.treasure_room_seed==world_run_seed(a.game));
 const unsigned start=a.treasure_open_tick;CHECK(a.treasure_banner_tick==start);treasure_alpha(255,0,0,10);const uint16_t first=scene_pixel();snapshot();a.ui_action(UiAction::TreasureOpen);a.input(Key::EndTurn);unchanged();
 // Independent source samples: chest 1 -> 0.5 linear, collection 0 -> 1
 // linear, banner EXPO_OUT, and the projected BACK_OUT position.
 advance_to(start+1);treasure_alpha(244,21,111,7);advance_to(start+2);treasure_alpha(234,42,174,4);unchanged();
 blocked_actions(UiAction::TreasureTake,false);CHECK(a.treasure_presentation==2&&a.treasure_open_tick==start);treasure_alpha(234,42,174,4);
 a.input(Key::Detail);CHECK(a.detail_item==int(a.game.world.treasure_relic));a.render();a.pointer_scroll(1);a.ui_action(UiAction::TreasureTake);a.input(Key::Back);CHECK(a.detail_item<0);unchanged();
 CHECK(a.treasure_random==presentation_random);
 advance_to(start+6);treasure_alpha(192,126,247,-1);advance_to(start+12);treasure_alpha(129,252,255);advance_to(start+13);treasure_alpha(128,255,255);unchanged();
 advance_to(start+34);blocked_actions(UiAction::TreasureTake);uint16_t last_open=scene_pixel();capture("treasure-opening");advance_to(start+35);uint16_t first_shine=scene_pixel();capture("treasure-shine");advance_to(start+50);uint16_t finished=scene_pixel();capture("treasure-open");
 if(!actual_profile)CHECK(first!=last_open&&last_open!=first_shine&&first_shine!=finished);unchanged();
 advance_to(start+75);CHECK(!a.treasure_can_proceed());advance_to(start+76);CHECK(a.treasure_can_proceed());unchanged();
 // A stale presentation clock from another point is never reused, even if
 // a host changes rooms without calling its normal reset helper.
 a.treasure_room_point=0xffff;a.sync_treasure_presentation();cleared_treasure();treasure_alpha(128,255,255);unchanged();
 // Emulate a stale clock from another native run with the same bootstrap
 // seed, act and point. Only App presentation changes; the valid State,
 // its named RNG streams and every encoded byte remain untouched.
 a.treasure_room_point=a.game.world.current_point;a.treasure_room_act=a.game.world.act_index;
 a.treasure_room_seed=world_run_seed(a.game)^(uint64_t(1)<<32);a.treasure_presentation=2;a.treasure_open_tick=a.animation;
 a.sync_treasure_presentation();cleared_treasure();CHECK(a.treasure_can_proceed());unchanged();
 CHECK(a.save_current_run());a.treasure_presentation=1;a.treasure_open_tick=a.treasure_banner_tick=a.treasure_close_tick=a.animation;a.treasure_chest_from=123;a.treasure_banner_from=456;a.treasure_banner_y=7;
 CHECK(a.load_saved_run());cleared_treasure();treasure_alpha(128,255,255);CHECK(a.treasure_can_proceed()&&a.treasure_pointer_can_take());CHECK(a.treasure_holder_opacity()==255&&a.treasure_holder_brightness()==255&&!a.treasure_holder_offset());CHECK(a.treasure_random==presentation_random);CHECK(scene_pixel()==finished);unchanged();
 const int opened_gold=a.game.gold;a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Treasure&&a.game.world.room_open==2&&a.map_window.is_open&&a.game.gold==opened_gold&&a.game.world_relics.count==relics);cleared_treasure();snapshot();
}
static uint16_t expected_mix(uint16_t back,uint16_t front,unsigned alpha){
 const unsigned r=((back>>11)*(255-alpha)+(front>>11)*alpha+127)/255;
 const unsigned g=(((back>>5)&63)*(255-alpha)+((front>>5)&63)*alpha+127)/255;
 const unsigned b=((back&31)*(255-alpha)+(front&31)*alpha+127)/255;
 return uint16_t((r<<11)|(g<<5)|b);
}
static void holder_pixels(unsigned child_alpha,unsigned brightness){
 if(actual_profile)return;
 const int id=a.relic_face_image(WorldRelicVajra);const auto image=a.content.image(id);
 CHECK(image.w==2&&image.h==2&&image.alpha&&!image.palette&&image.alpha[0]==255&&image.alpha[1]==128&&!image.alpha[2]);
 const unsigned opacity=(a.treasure_collection_opacity()*child_alpha+127)/255;
 const uint16_t color=uint16_t((((31*brightness+127)/255)<<11)|(((63*brightness+127)/255)<<5)|((31*brightness+127)/255));
 const int x[]={141,166,141,166},y[]={87,87,112,112};
 for(unsigned pixel=0;pixel<4;pixel++){
  const int yy=y[pixel]+a.treasure_holder_offset(),xx=x[pixel];
  const uint16_t background=a.pixels[yy*ScreenW+(xx<160?5:315)];
  const unsigned alpha=(unsigned(image.alpha[pixel])*opacity+127)/255;
  CHECK(a.pixels[yy*ScreenW+xx]==expected_mix(background,color,alpha));
 }
}
static void holder_entry(){
 begin_room(WorldRoom::Treasure);a.game.world.treasure_relic=WorldRelicVajra;
 // Compare the controller-only result with the real App opening. Cosmetic
 // random sampling must not consume any named/legacy run RNG or save byte.
 static State controller;static SaveData controller_save;controller=a.game;
 CHECK(open_treasure(controller)&&save(controller,controller_save));
 const uint32_t previous_random=a.treasure_random;click(UiAction::TreasureOpen);CHECK(save(a.game,after));
 CHECK(after.size==controller_save.size&&!std::memcmp(after.bytes,controller_save.bytes,after.size));
 const uint32_t presentation_random=a.treasure_random,start=a.treasure_open_tick;
 CHECK(presentation_random&&presentation_random!=previous_random&&a.treasure_holder_delay_us>=200000&&a.treasure_holder_delay_us<400000);
 snapshot();
 // Independent source tween samples: child transparentBlack -> White in
 // 250ms; delayed holder Black -> White in 200ms; 600ms BACK_OUT motion.
 // App-only delay fixtures cover both range endpoints and a midpoint.
 struct Sample{uint32_t delay,tick;unsigned alpha,brightness;int y;bool pointer;};
 const Sample samples[]={
  {200000,0,0,0,30,false},{200000,2,67,0,30,false},{200000,7,236,37,23,false},
  {200000,12,255,250,2,false},{200000,15,255,255,-3,false},{200000,24,255,255,0,false},{200000,25,255,255,0,true},
  {300000,6,202,0,30,false},{300000,10,255,38,24,false},{300000,13,255,164,8,false},
  {300000,18,255,255,-3,false},{300000,27,255,255,0,false},{300000,28,255,255,0,true},
  {399999,12,255,0,30,false},{399999,13,255,37,24,false},{399999,18,255,247,2,false},
  {399999,21,255,255,-3,false},{399999,30,255,255,0,false},{399999,31,255,255,0,true}
 };
 for(const auto&sample:samples){
  a.treasure_holder_delay_us=sample.delay;a.animation=start+sample.tick;a.pointer.visible=false;a.render();
  CHECK(a.treasure_holder_opacity()==sample.alpha&&a.treasure_holder_brightness()==sample.brightness&&a.treasure_holder_offset()==sample.y);
  CHECK(a.treasure_pointer_can_take()==sample.pointer&&exists(HitKind::UiAction,int(UiAction::TreasureTake),false)==sample.pointer);
  holder_pixels(sample.alpha,sample.brightness);CHECK(a.treasure_random==presentation_random);unchanged();
  a.render();CHECK(a.treasure_random==presentation_random);unchanged();
 }
 // A press that began while MouseFilter was IGNORE must not acquire on
 // release after the hit appears. A new press after the same boundary does.
 a.treasure_holder_delay_us=300000;a.animation=start+27;a.render();CHECK(!a.treasure_pointer_can_take());
 a.pointer_move(160,110);a.pointer_button(true);CHECK(a.pointer.held&&a.pointer.captured.kind!=HitKind::UiAction);
 advance_to(start+28);CHECK(a.treasure_pointer_can_take());a.pointer_button(false);a.pointer.visible=false;unchanged();
 CHECK(a.game.treasure_state==1&&a.treasure_random==presentation_random);capture("treasure-holder-settled");click(UiAction::TreasureTake);
 CHECK(a.game.treasure_state==2&&has_world_relic(a.game,WorldRelicVajra)&&a.treasure_random==presentation_random);
 // A later room takes one further draw from the existing presentation
 // stream, rather than re-seeding from the run or the new room clock.
 begin_room(WorldRoom::Treasure);CHECK(a.treasure_random==presentation_random);click(UiAction::TreasureOpen);
 CHECK(a.treasure_random!=presentation_random&&a.treasure_holder_delay_us>=200000&&a.treasure_holder_delay_us<400000);
}
static void actual_claim(){
 begin_room(WorldRoom::Treasure);
 // Choose a fixed supported model to isolate its actual acquisition/choice
 // path; room entry and the opening gold still use the native controller.
 a.game.world.treasure_relic=WorldRelicVajra;click(UiAction::TreasureOpen);const unsigned start=a.treasure_open_tick;advance_to(start+2);treasure_alpha(234,42,174,4);
 const unsigned before_relics=a.game.world_relics.count;const uint16_t before_take=scene_pixel();CHECK(!a.treasure_pointer_can_take()&&!exists(HitKind::UiAction,int(UiAction::TreasureTake),false));a.input(Key::Confirm);
 CHECK(a.game.treasure_state==2&&a.game.phase==Phase::Treasure&&has_world_relic(a.game,WorldRelicVajra)&&a.game.world_relics.count==before_relics+1&&a.treasure_can_proceed());
 CHECK(a.treasure_presentation==3&&a.treasure_open_tick==start&&a.treasure_close_tick==start+2);treasure_alpha(234,255,174,4);CHECK(scene_pixel()==before_take);
 CHECK(!exists(HitKind::UiAction,int(UiAction::TreasureTake),false));
 // The awarded holder is immediately hidden, not retained for the parent
 // fade. Both points lie in the same uniform synthetic chest image quadrant.
 if(!actual_profile)CHECK(a.pixels[100*ScreenW+149]==a.pixels[100*ScreenW+5]);
 snapshot();a.ui_action(UiAction::TreasureTake);CHECK(a.treasure_close_tick==start+2);unchanged();
 advance_to(start+3);CHECK(a.treasure_chest_opacity()>234&&a.treasure_chest_opacity()<255);CHECK(a.treasure_collection_opacity()==227&&a.treasure_banner_opacity()<174&&a.treasure_banner_offset()==4);
 advance_to(start+11);CHECK(a.treasure_collection_opacity()==3);advance_to(start+12);CHECK(a.treasure_collection_opacity()==0&&a.treasure_chest_opacity()==255);advance_to(start+15);treasure_alpha(255,0,0,4);unchanged();
 // Closing only changes opacity: opening -> shine -> final still uses the
 // original open_tick, even after a very early focused keyboard claim.
 advance_to(start+34);const uint16_t last_open=scene_pixel();advance_to(start+35);const uint16_t first_shine=scene_pixel();advance_to(start+50);const uint16_t finished=scene_pixel();
 if(!actual_profile)CHECK(last_open!=first_shine&&first_shine!=finished);CHECK(a.treasure_open_tick==start);unchanged();capture("treasure-claimed");a.input(Key::Confirm);CHECK(a.game.phase==Phase::Treasure&&a.game.world.room_open==2&&a.map_window.is_open);
 begin_room(WorldRoom::Treasure);a.game.world.treasure_relic=WorldRelicDollysMirror;click(UiAction::TreasureOpen);advance_to(a.treasure_open_tick+31);const unsigned deck=a.game.deck_count;CHECK(a.treasure_pointer_can_take());click(UiAction::TreasureTake);
 CHECK(a.game.treasure_state==2&&a.game.phase==Phase::WorldRelicChoice&&world_relic_pending(a.game)&&a.treasure_presentation==3);const unsigned close=a.treasure_close_tick;treasure_alpha(128,255,255);snapshot();a.ui_action(UiAction::TreasureLeave);a.ui_action(UiAction::TreasureTake);unchanged();
 advance_to(close+10);CHECK(a.treasure_chest_opacity()==255&&a.treasure_collection_opacity()==0&&a.treasure_banner_opacity()==1);advance_to(close+13);treasure_alpha(255,0,0);CHECK(a.treasure_close_tick==close);unchanged();
 click(HitKind::WorldChoice,0);a.ui_action(UiAction::ConfirmWorldChoice);CHECK(a.game.phase==Phase::Treasure&&a.game.treasure_state==2&&a.game.deck_count==deck+1&&!world_relic_pending(a.game));a.render();CHECK(a.cursor==1&&a.treasure_can_proceed()&&a.treasure_close_tick==close);treasure_alpha(255,0,0);snapshot();a.input(Key::Confirm);CHECK(a.game.phase==Phase::Treasure&&a.game.world.room_open==2&&a.map_window.is_open);
}
static void restored_claim(){
 begin_room(WorldRoom::Treasure);a.game.world.treasure_relic=WorldRelicVajra;click(UiAction::TreasureOpen);CHECK(a.save_current_run());snapshot();CHECK(a.load_saved_run());cleared_treasure();treasure_alpha(128,255,255);unchanged();
 const uint16_t opened=scene_pixel();click(UiAction::TreasureTake);CHECK(a.treasure_presentation==4&&!a.treasure_open_tick);treasure_alpha(128,255,255);CHECK(scene_pixel()==opened);const unsigned close=a.treasure_close_tick;snapshot();
 advance_to(close+10);CHECK(a.treasure_chest_opacity()==255&&a.treasure_collection_opacity()==0);const uint16_t finished=scene_pixel();advance_to(close+13);treasure_alpha(255,0,0);advance_to(close+35);CHECK(scene_pixel()==finished);unchanged();
 CHECK(a.save_current_run());CHECK(a.load_saved_run());cleared_treasure();treasure_alpha(255,0,0);CHECK(scene_pixel()==finished);unchanged();a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Treasure&&a.game.world.room_open==2&&a.map_window.is_open);
}
static void empty_chest(){
 const RelicId relics[]={WorldRelicSilverCrucible};begin_room(WorldRoom::Treasure,reference::ActKind::Overgrowth,0,relics,1);CHECK(a.game.world.treasure_relic==NoWorldRelic);
 const uint32_t presentation_random=a.treasure_random;const int gold=a.game.gold;const unsigned count=a.game.world_relics.count,calls=world_rng(a.game,WorldRng::Rewards).calls;click(UiAction::TreasureOpen);CHECK(a.treasure_random==presentation_random&&!a.treasure_holder_delay_us);
 CHECK(a.game.phase==Phase::Treasure&&a.game.treasure_state==1&&a.game.gold==gold&&a.game.world_relics.count==count&&!a.game.world.treasure_gold_claimed&&world_rng(a.game,WorldRng::Rewards).calls==calls);a.render();CHECK(!exists(HitKind::UiAction,int(UiAction::TreasureTake),false)&&!a.treasure_can_proceed()&&a.treasure_empty_waiting()&&a.cursor==1);treasure_alpha(255,0,0,10);
 const unsigned start=a.treasure_open_tick;const uint32_t collection_start=a.treasure_collection_start_ms;
 snapshot();a.ui_action(UiAction::TreasureTake);a.ui_action(UiAction::TreasureLeave);a.input(Key::EndTurn);unchanged();advance_to(start+13);treasure_alpha(128,255,255);capture("treasure-empty");advance_to(start+30);CHECK(a.treasure_presentation==2&&a.treasure_empty_waiting());
 // Empty collection completion is a real 1s timer, independent of the
 // renderer's 33ms ticks. Assert both sides before another visual tick.
 a.advance(9);a.render();CHECK(a.animation==start+30&&uint32_t(a.map_clock_ms-collection_start)==999&&a.treasure_presentation==2&&a.treasure_empty_waiting());unchanged();
 a.advance(1);a.render();CHECK(a.animation==start+30&&uint32_t(a.map_clock_ms-collection_start)==1000&&a.treasure_presentation==3&&!a.treasure_empty_waiting()&&a.treasure_close_tick==a.animation&&a.treasure_open_tick==start);treasure_alpha(128,255,255);
 const unsigned close=a.treasure_close_tick;
 advance_to(close+9);CHECK(a.treasure_collection_opacity()==3);advance_to(close+10);CHECK(a.treasure_collection_opacity()==0&&a.treasure_chest_opacity()==255);advance_to(close+13);treasure_alpha(255,0,0);unchanged();CHECK(a.treasure_can_proceed());
 CHECK(a.save_current_run());CHECK(a.load_saved_run());cleared_treasure();treasure_alpha(255,0,0);CHECK(a.treasure_random==presentation_random);unchanged();a.input(Key::Confirm);CHECK(a.game.phase==Phase::Treasure&&a.game.world.room_open==2&&a.map_window.is_open);
}
static void treasure_clock_wrap(){
 begin_room(WorldRoom::Treasure);a.animation=0xfffffffbu;a.reset_presentation();click(UiAction::TreasureOpen);const uint32_t start=a.treasure_open_tick;CHECK(start==0xfffffffbu);snapshot();
 const uint32_t presentation_random=a.treasure_random;
 for(unsigned i=0;i<6;i++)a.advance(33);a.render();CHECK(a.animation==1&&a.treasure_open_tick==start);treasure_alpha(192,126,247,-1);CHECK(a.treasure_holder_opacity()==202&&!a.treasure_holder_brightness()&&a.treasure_holder_offset()==30&&!a.treasure_pointer_can_take());unchanged();
 // Large elapsed values are clamped before converting ticks to ms. This
 // delta used to wrap multiplication by 33 back into the opening frames.
 a.animation=start+130150525u;a.render();treasure_alpha(128,255,255);CHECK(!a.treasure_can_proceed()&&a.treasure_pointer_can_take());CHECK(a.treasure_holder_opacity()==255&&a.treasure_holder_brightness()==255&&!a.treasure_holder_offset()&&a.treasure_random==presentation_random);const uint16_t finished=scene_pixel();unchanged();
 // Advancing only the legacy visual clock cannot enable Skip. Its raw-ms
 // timer is independent and completes without replaying the opening art.
 a.map_clock_ms+=2500;a.advance(0);CHECK(a.treasure_can_proceed()&&a.treasure_skip_ready);CHECK(scene_pixel()==finished);unchanged();
 a.reset_presentation();cleared_treasure();treasure_alpha(128,255,255);CHECK(scene_pixel()==finished);unchanged();
}
static void region_captures(){
 struct Room{reference::ActKind start;unsigned act;const char*name;const char*lit;const char*dark;};
 const Room entries[]={{reference::ActKind::Underdocks,0,"underdocks","UNDERDOCKS","UNDERDOCKS_DARK"},{reference::ActKind::Overgrowth,1,"hive","HIVE","HIVE_DARK"},{reference::ActKind::Overgrowth,2,"glory","GLORY","GLORY_DARK"}};
 for(const auto&e:entries){
  begin_room(WorldRoom::Rest,e.start,e.act);rest_scene_pixel(e.lit);char name[80]{};std::snprintf(name,sizeof name,"rest-%s-choices",e.name);capture(name);snapshot();a.render();a.advance(66);a.render();unchanged();click(HitKind::Choice,0);std::snprintf(name,sizeof name,"rest-%s-completed",e.name);capture(name);CHECK(!a.camp_options(nullptr,0));rest_scene_pixel(e.dark);
  begin_room(WorldRoom::Treasure,e.start,e.act);std::snprintf(name,sizeof name,"treasure-%s-closed",e.name);capture(name);click(UiAction::TreasureOpen);advance_to(a.treasure_open_tick+50);std::snprintf(name,sizeof name,"treasure-%s-open",e.name);capture(name);snapshot();a.render();unchanged();
 }
}
static void rest_motion(){
 begin_room(WorldRoom::Rest);a.room_presentation.rest={};a.pointer_move(310,35);a.render();snapshot();
 CHECK(!hit(HitKind::Choice,0).enabled&&!a.room_presentation.rest.alpha(a.map_clock_ms));
 click(HitKind::Choice,0);unchanged();room_clock(250);
 CHECK(a.room_presentation.rest.alpha(a.map_clock_ms)==223&&!hit(HitKind::Choice,0).enabled);capture("rest-entry-half");
 room_clock(250);CHECK(hit(HitKind::Choice,0).enabled);unchanged();
 const auto h=hit(HitKind::Choice,0);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.render();room_clock(50);
 CHECK(a.room_presentation.rest.buttons[0].sample(a.map_clock_ms).scale==4506);
 CHECK(a.room_presentation.rest.description==0&&a.room_presentation.rest.description_alpha(a.map_clock_ms)==255);
 capture("rest-original-hover");unchanged();
 // A captured press visibly shrinks; releasing off the icon cannot heal.
 a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.render();room_clock(250);
 CHECK(a.room_presentation.rest.buttons[0].sample(a.map_clock_ms).scale<4096);
 a.pointer_move(310,35);a.pointer_button(false);a.render();unchanged();room_clock(500);
 CHECK(a.room_presentation.rest.description_alpha(a.map_clock_ms)>0&&a.room_presentation.rest.description_alpha(a.map_clock_ms)<16);
 room_clock(500);CHECK(!a.room_presentation.rest.description_alpha(a.map_clock_ms));unchanged();
}
static void begin_event(OriginalEventId id,Character character=Character::Ironclad){
 new_run(a.game,0x7195c3,false,character);CHECK(world_set_seed(a.game,"NSPIRE-ROOM-UI7"));CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
 if(id!=OriginalEventId::Neow){a.game.original_events={};CHECK(world_finish_room(a.game));CHECK(original_event_begin(a.game,id,19,1));}
 present();a.render();CHECK(a.game.phase==Phase::Event);snapshot();
}
static void event_motion(){
 begin_event(OriginalEventId::AromaOfChaos);const auto&motion=a.room_presentation.event;
 CHECK(!motion.title_alpha(a.map_clock_ms)&&!motion.body_reveal(a.map_clock_ms));
 CHECK(choices()==2&&!hit(HitKind::Choice,0).enabled&&!hit(HitKind::Choice,1).enabled);
 a.input(Key::Confirm);unchanged();room_clock(500);CHECK(!hit(HitKind::Choice,0).enabled);
 CHECK(hit(HitKind::Choice,0).x==117);room_clock(250);
 CHECK(motion.title_alpha(a.map_clock_ms)>0&&motion.title_alpha(a.map_clock_ms)<255&&!motion.body_reveal(a.map_clock_ms));
 capture("event-options-entering");unchanged();room_clock(250);
 CHECK(motion.title_alpha(a.map_clock_ms)==255&&hit(HitKind::Choice,0).enabled&&!hit(HitKind::Choice,1).enabled);
 CHECK(motion.body_reveal(a.map_clock_ms)>motion.body_progress(a.map_clock_ms));
 room_clock(200);CHECK(hit(HitKind::Choice,1).enabled);unchanged();room_clock(550);
 CHECK(motion.body_reveal(a.map_clock_ms)==4096);capture("event-typewriter-complete");
 const uint32_t entered=motion.start;a.input(Key::Detail);CHECK(a.event_reader);room_clock(125);a.input(Key::Back);a.render();
 CHECK(!a.event_reader&&motion.start==entered);unchanged();
 // Single-player release selects immediately: the source's .3 s flash is
 // only awaited for shared multiplayer choices, which this port excludes.
 click(HitKind::Choice,1);CHECK(a.game.phase==Phase::OriginalEventChoice);
 begin_event(OriginalEventId::AromaOfChaos);a.fast_mode=true;a.room_presentation.event={};a.render();snapshot();
 room_clock(500);CHECK(hit(HitKind::Choice,0).enabled&&!hit(HitKind::Choice,1).enabled);
 room_clock(100);CHECK(hit(HitKind::Choice,1).enabled);unchanged();a.fast_mode=false;
 begin_event(OriginalEventId::Neow);CHECK(a.room_presentation.event.ancient);a.input(Key::Confirm);unchanged();room_clock(800);
 CHECK(a.game.phase==Phase::Event&&a.room_presentation.event.start+800==a.map_clock_ms);room_clock(200);
 CHECK(choices()>=2);capture("ancient-source-dialogue-layout");unchanged();
}
static void merchant_motion_and_pickups(){
 begin_room(WorldRoom::Shop);a.game.gold=9999;a.render();snapshot();click(UiAction::OpenShop);CHECK(a.shop_open);
 const uint32_t began=a.map_clock_ms;CHECK(a.room_presentation.merchant.y(began)==-222);CHECK(!choices());unchanged();
 room_clock(350);CHECK(a.room_presentation.merchant.y(a.map_clock_ms)>0&&a.room_presentation.merchant.y(a.map_clock_ms)<18);capture("merchant-rug-half-open");unchanged();
 room_clock(350);CHECK(a.room_presentation.merchant.y(a.map_clock_ms)==18);
 constexpr int centers[]={44,102,160,218,277,58,119};
 for(int i=0;i<ShopCardCount;++i){const auto h=hit(HitKind::Choice,i);CHECK(h.x+h.w/2==centers[i]&&h.w==54&&h.h==85&&h.y+h.h<=235);}
 room_clock(300);CHECK(a.room_presentation.merchant.alpha(a.map_clock_ms)==204);capture("merchant-source-scale");unchanged();
 auto h=hit(HitKind::Choice,4);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.render();room_clock(250);room_clock(250);
 CHECK(a.room_presentation.merchant.focused==4&&a.room_presentation.merchant.scale(4,a.map_clock_ms)==800);
 CHECK(a.room_presentation.merchant.hand_x/16>240);capture("merchant-pointing-hand");unchanged();
 a.pointer_move(310,35);a.render();room_clock(250);CHECK(a.room_presentation.merchant.scale(4,a.map_clock_ms)>650&&a.room_presentation.merchant.scale(4,a.map_clock_ms)<660);
 room_clock(250);CHECK(a.room_presentation.merchant.scale(4,a.map_clock_ms)==650);unchanged();
 // The actual purchase creates one bounded card pickup; nested key/pointer
 // observation cannot duplicate it or change saved rules/RNG.
 static State oracle;static SaveData expected;CHECK(load(oracle,before.bytes,before.size));CHECK(buy(oracle,0));CHECK(save(oracle,expected));
 const auto count=a.game.deck_count;click(HitKind::Choice,0);CHECK(a.game.deck_count==count+1);
 CHECK(save(a.game,after)&&after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,expected.size));
 unsigned flights=0;for(const auto&f:a.reward_presentation.flights)flights+=f.kind==RewardPickupKind::Card;CHECK(flights==1&&a.cursor!=0);
 snapshot();room_clock(150);capture("merchant-card-pickup");unchanged();
 click(UiAction::CloseShop);CHECK(!a.shop_open);a.render();CHECK(!choices());room_clock(500);
 CHECK(a.room_presentation.merchant.y(a.map_clock_ms)==-222);CHECK(a.room_presentation.merchant.alpha(a.map_clock_ms)>0);room_clock(300);
 CHECK(!a.room_presentation.merchant.visible(a.map_clock_ms)&&!a.room_presentation.merchant.alpha(a.map_clock_ms));unchanged();
}
static void fake_merchant_layout(){
 begin_event(OriginalEventId::FakeMerchant);a.game.gold=9999;a.render();snapshot();
 CHECK(!a.shop_open&&!choices()&&exists(HitKind::UiAction,int(UiAction::OpenShop))&&exists(HitKind::UiAction,int(UiAction::LeaveShop)));
 capture("fake-merchant-closed");click(UiAction::OpenShop);CHECK(a.shop_open);room_clock(1000);CHECK(choices()==6);
 constexpr int x[]={166,227,137,197,272,156},y[]={66,82,132,147,128,206};
 for(int i=0;i<6;++i){const auto h=hit(HitKind::Choice,i);CHECK(h.x+h.w/2==x[i]&&h.y+17==y[i]&&h.enabled);}
 CHECK(!exists(HitKind::UiAction,int(UiAction::LeaveShop)));capture("fake-merchant-inventory");unchanged();
 const unsigned relics=ui_owned_relic_count(a.game);click(HitKind::Choice,0);CHECK(ui_owned_relic_count(a.game)==relics+1&&a.game.original_events.offers[0]==NoWorldRelic);a.render();
 CHECK(!hit(HitKind::Choice,0).enabled&&a.room_presentation.merchant.dialogue==0);
 snapshot();click(UiAction::CloseShop);CHECK(!a.shop_open);room_clock(800);unchanged();click(UiAction::LeaveShop);
 CHECK(a.map_window.is_open&&!a.game.original_events.active);
}
static void direct_removal_preview(){
 begin_event(OriginalEventId::SlipperyBridge);const unsigned index=unsigned(a.game.original_events.values[1]);CHECK(index<a.game.deck_count);
 const auto enchant=card_type(a.game,a.game.deck[index],-1,int(index))==CardType::Attack?EventEnchantment::Sharp:EventEnchantment::Nimble;
 CHECK(event_card_enchant(a.game,int(index),enchant,7));a.render();room_clock(1800);
 const Card removed=a.game.deck[index];const EventCardFacts facts=a.game.event_cards.deck[index];char description[1024]{};
 a.card_description(removed,-1,description,sizeof description,-1,int(index),&facts);snapshot();
 static State oracle;static SaveData expected;CHECK(load(oracle,before.bytes,before.size));CHECK(original_event_choose(oracle,0));CHECK(save(oracle,expected));
 click(HitKind::Choice,0);CHECK(a.reward_presentation.removed.active&&card_token(a.reward_presentation.removed.card)==card_token(removed));
 CHECK(a.reward_presentation.removed.facts.enchantment_amount==7&&!std::strcmp(a.reward_presentation.removed.description,description));
 CHECK(save(a.game,after)&&after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,expected.size));
 snapshot();room_clock(250);capture("event-direct-card-removal");unchanged();room_clock(1450);
 CHECK(!a.reward_presentation.removed.alive(a.map_clock_ms));unchanged();
}
int main(int argc,char**argv){
 std::vector<uint8_t>data;actual_profile=argc>1;if(actual_profile){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=synthetic_profile();
 if(argc>2)capture_directory=argv[2];CHECK(a.init(data.data(),data.size(),{memory_save,memory_load}));CHECK(a.content.current_profile());
 ordinary_rest();rest_actor_layers();combat_backgrounds();paged_tent();closed_chest();opening_and_reload();holder_entry();actual_claim();restored_claim();empty_chest();treasure_clock_wrap();region_captures();
 rest_motion();event_motion();merchant_motion_and_pickups();fake_merchant_layout();direct_removal_preview();
 const uint32_t presentation_random=a.treasure_random;
 a.treasure_presentation=3;a.treasure_open_tick=17;a.treasure_banner_tick=19;a.treasure_close_tick=23;a.treasure_chest_from=123;a.treasure_banner_from=456;a.treasure_banner_y=7;a.treasure_holder_delay_us=333333;CHECK(a.init(data.data(),data.size(),{memory_save,memory_load}));cleared_treasure();CHECK(a.treasure_random==presentation_random);
 std::printf("world room UI: %u checks passed; %s profile; stable rest choices and real treasure acquisition with presentation-only timing\n",checks,actual_profile?"actual":"synthetic");
}
