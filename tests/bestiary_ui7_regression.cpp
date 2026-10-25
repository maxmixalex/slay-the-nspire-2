#include "app.hpp"
#include "content_fixture.hpp"
#include "rich_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks,preferences_saved,run_writes;
#include "fixtures/bestiary_order_ui7.inc"
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"bestiary UI7:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool store_preferences(const UiPreferences&){++preferences_saved;return true;}
static bool store_run(const State&){++run_writes;return true;}
static bool load_run(State&){++run_writes;return false;}
static int row(unsigned model){for(int i=0;i<app.bestiary.row_count;++i)if(app.bestiary_entry_model(i)==model)return i;return -1;}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));CHECK(!run_writes);}
static void settle(unsigned frames=20){for(unsigned i=0;i<frames;++i){app.advance(33);app.render();}}
static HitRegion find(HitKind kind,int value=-1){for(unsigned i=0;i<unsigned(app.hit_count);++i)if(app.hits[i].kind==kind&&(value<0||app.hits[i].value==value))return app.hits[i];return {};}
static void key(uint16_t code){CHECK(app.physical_key(code,true));CHECK(app.physical_key(code,false));}
static void click(HitRegion hit){CHECK(hit.kind!=HitKind::None);app.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);app.pointer_button(true);app.pointer_button(false);app.render();}
static void capture(const char*folder,const char*name){if(!folder)return;std::string path=std::string(folder)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto pixel:app.pixels){const unsigned char rgb[3]={static_cast<unsigned char>((pixel>>11)*255/31),static_cast<unsigned char>(((pixel>>5)&63)*255/63),static_cast<unsigned char>((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void original_unknown_labels(){
 for(const char*key:{"LOCKED.monsterName","UNSEEN.monsterName"}){
  const char*text=app.content.world_string("bestiary",key);CHECK(std::strstr(text,"[lb]")&&std::strstr(text,"[rb]"));
  RichReader reader(text);RichChar ch{};unsigned count=0,left=0,right=0;uint32_t first=0,last=0;
  while(reader.next(ch)){if(!count)first=ch.codepoint;last=ch.codepoint;++count;left+=ch.codepoint=='[';right+=ch.codepoint==']';}
  CHECK(count>2&&first=='['&&last==']'&&left==1&&right==1);
 }
}
static void ordered_catalog(){
 new_run(app.game,0x41cef1ea,false,Character::Ironclad);app.title=true;app.reset_presentation();
 clear_ui_progress(app.progress);app.progress_loaded=true;app.progress.discovered_acts=15;CHECK(save(app.game,before));
 CHECK(app.open_bestiary(true)&&app.bestiary.row_count==106);
 const unsigned lengths[]={29,22,22,23,5};const auto&order=expected_bestiary_order[app.content.english()?0:1];
 unsigned row_index=0,index=0;bool seen[101]{};
 for(unsigned region=0;region<5;++region){CHECK(app.bestiary.rows[row_index++]==-1-int(region));
  for(unsigned i=0;i<lengths[region];++i){const auto entry=app.bestiary.rows[row_index++];
   CHECK(entry==order[index++]&&entry>=0&&entry<101&&!seen[entry]);seen[entry]=true;
  }
 }
 CHECK(row_index==106&&index==101);unchanged();app.input(Key::Back);CHECK(!app.bestiary.open);
}
static void source_layout(){
 BestiaryPresentation list{};list.row_count=4;list.rows[0]=-1;list.rows[1]=0;list.rows[2]=1;list.rows[3]=-2;list.layout_rows();
 CHECK(list.Top==23&&list.Bottom==217&&list.Left==244&&list.Right==297);
 CHECK(list.row_offsets[0]==0&&list.row_offsets[1]==68&&list.row_offsets[2]==108&&list.row_offsets[3]==148&&list.row_offsets[4]==212);
 CHECK(list.row_y(0)==23&&list.row_height(0)==12&&list.row_y(1)==36&&list.row_height(1)==7&&list.row_y(2)==44);
 CHECK(list.ModeX==18&&list.ModeY==216&&list.ModeWidth==50&&list.ModeHeight==11);
 // Source50ms focus and300ms release use the same glyph band throughout.
 BestiaryPresentation::MoveMotion move{};move.focus(true,100);CHECK(move.focus_scale(100)==1024&&move.focus_scale(150)==1075);
 move.press(true,150);CHECK(move.press_scale(150)==1024&&move.press_scale(350)==922);
 move.press(false,350);CHECK(move.press_scale(350)==922&&move.press_scale(650)==1024);
 BestiaryPresentation::ModeMotion mode{};mode.observe(true,false,100);CHECK(mode.scale(100)==1024&&mode.scale(150)==1075);
 mode.observe(true,true,150);CHECK(mode.scale(150)==1075&&mode.scale(400)==973);
 mode.observe(true,false,400);CHECK(mode.scale(1000)==973);
 mode.observe(false,false,1000);CHECK(mode.scale(1500)==1024);
 for(unsigned i=0;i<18;++i){CHECK(bestiary_kaiser::directory(bestiary_kaiser::Sources[i])==int(BestiaryKaiserAnimationFirst+i));
  if(i)CHECK(bestiary_kaiser::Sources[i-1]<bestiary_kaiser::Sources[i]);}
 CHECK(bestiary_kaiser::directory(0)<0&&bestiary_kaiser::directory(1765)<0);
}
static bool stored_glyph(const std::vector<uint8_t>&pack,unsigned cp,unsigned face,unsigned size){
 const auto u32=[&](unsigned at){return unsigned(pack[at])|unsigned(pack[at+1])<<8|unsigned(pack[at+2])<<16|unsigned(pack[at+3])<<24;};
 const unsigned images=u32(16),count=u32(20),offset=40+images*16,packed=size|((face&3)<<6)|((face&4)<<3);
 const uint64_t key=uint64_t(packed)<<24|cp;unsigned low=0,high=count;
 while(low<high){const unsigned middle=(low+high)/2,at=offset+middle*16;const uint64_t candidate=uint64_t(pack[at+8]|unsigned(pack[at+9])<<8)<<24|u32(at);
  if(candidate<key)low=middle+1;else high=middle;}
 if(low==count)return false;const unsigned at=offset+low*16;return u32(at)==cp&&(pack[at+8]|unsigned(pack[at+9])<<8)==packed;
}
static void actual_glyphs(const std::vector<uint8_t>&pack){
 if(pack.empty())return;
 const auto text=[&](const char*value,unsigned face,unsigned size){RichReader reader(value);RichChar ch{};
  while(reader.next(ch))if(ch.codepoint>=32)CHECK(stored_glyph(pack,ch.codepoint,face,size));};
 clear_ui_progress(app.progress);app.progress.discovered_acts=15;CHECK(app.open_bestiary(true));
 for(int row_index=0;row_index<app.bestiary.row_count;++row_index){const unsigned model=app.bestiary_entry_model(row_index);if(model!=255)text(app.monster_title(EnemyId(model)),4,10);}
 text(app.content.world_title("encounters","DECIMILLIPEDE_ELITE"),4,10);text(app.content.world_string("bestiary","LOCKED.monsterTitle"),4,10);
 for(const char*label:{"ACTIONS.header","MODE.viewActions","MODE.viewStats"})text(app.content.world_string("bestiary",label),5,5);
 text(app.content.world_string("bestiary","EVENTS.title"),5,6);
 for(const char*act:{"OVERGROWTH","UNDERDOCKS","HIVE","GLORY"})text(app.content.world_title("acts",act),5,6);
 text(app.content.world_string("bestiary","QUOTE_PLACEHOLDER"),6,5);
 app.input(Key::Back);
}
static void flow(const char*folder){
 new_run(app.game,0x41cef1ea,false,Character::Silent);app.title=true;app.bestiary_actions_preferred=true;app.reset_presentation();
 app.callbacks.save=store_run;app.callbacks.load=load_run;app.callbacks.save_preferences=store_preferences;
 clear_ui_progress(app.progress);app.progress_loaded=true;CHECK(save(app.game,before));CHECK(!app.open_bestiary());
 app.progress.discovered_acts=1;CHECK(!app.open_bestiary());
 auto&nibbit=app.progress.enemies[unsigned(EnemyId::Nibbit)];nibbit[0]={4,2,1};nibbit[1]={2,2,0};nibbit[2]={1,0,1};
 auto&wurm=app.progress.enemies[unsigned(EnemyId::FuzzyWurmCrawler)];wurm[0]={1,1,0};wurm[2]={1,1,0};
 CHECK(app.bestiary_available()&&app.open_bestiary());CHECK(app.bestiary.open&&app.bestiary.actions);
 CHECK(app.bestiary.row_count==36);settle();CHECK(app.bestiary.selected>=0);
 const int first=app.bestiary.selected,first_row=app.bestiary.selected_row;
 const int nr=row(unsigned(EnemyId::Nibbit)),wr=row(unsigned(EnemyId::FuzzyWurmCrawler));CHECK(nr>=0&&wr>=0&&nr!=wr);
 // LouseProgenitor belongs to the Hive. An undiscovered act must remain
 // absent even if some other act already unlocked the compendium.
 CHECK(row(unsigned(EnemyId::LouseProgenitor))<0);
 const int alternate=first_row==nr?wr:nr;app.bestiary.position=app.bestiary.target=0;app.bestiary.reveal(alternate);settle();
 auto item=find(HitKind::BestiaryEntry,alternate);CHECK(item.enabled&&item.kind==HitKind::BestiaryEntry);
 app.pointer_move(item.x+2,item.y+2);app.render();CHECK(app.bestiary.selected==first);click(item);CHECK(app.bestiary.selected_row==alternate);
 // A covered title's selection and all simulation bytes stay unchanged.
 const int menu=app.menu;app.pointer_move(50,55);CHECK(app.menu==menu);unchanged();
 app.input(Key::Down);CHECK(app.bestiary.selected_row==alternate&&app.bestiary.focused!=alternate);app.input(Key::Confirm);CHECK(app.bestiary.selected_row==app.bestiary.focused);
 app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(nr),true});settle();CHECK(app.bestiary.selected_row==nr);
 // Nibbit.ShouldShowMoveInBestiary explicitly hides SLICE_MOVE. The
 // original buttons are BUTT, HISS, Hurt and Die; no fifth Slice button.
 CHECK(app.bestiary_move_count()==4);app.input(Key::Left);
 const char*clips[]={"attack","hiss","hurt","die"};
 for(unsigned i=0;i<4;++i){
  CHECK(app.bestiary.action==i);app.input(Key::Confirm);CHECK(app.bestiary.actors[0].count==1);
  const auto*clip=world_enemy_clip_directory(unsigned(app.bestiary.actors[0].clips[0]));
  CHECK(clip&&!std::strcmp(clip->native_clip,clips[i]));
  if(i!=3)app.input(Key::Down);
 }
 app.input(Key::Down);CHECK(!app.bestiary.action);app.input(Key::Confirm);settle(3);capture(folder,"bestiary-nibbit-action");unchanged();
 app.input(Key::EndTurn);CHECK(!app.bestiary.actions&&app.bestiary_actions_preferred&&preferences_saved==0);settle();
 CHECK(find(HitKind::BestiaryFilter,1).enabled&&find(HitKind::BestiaryFilter,2).enabled&&!find(HitKind::BestiaryFilter,3).enabled);
 click(find(HitKind::BestiaryFilter,2));settle();CHECK(app.bestiary.filter==2);capture(folder,"bestiary-silent-loss-statistics");
 const unsigned filter=app.bestiary.filter;click(find(HitKind::BestiaryFilter,3));CHECK(app.bestiary.filter==filter);unchanged();
 // Continuous wheel and thumb routing consume pointer input without
 // selecting the entries that happen to pass beneath the dragged thumb.
 const int selected=app.bestiary.selected;app.pointer_scroll(12);settle();CHECK(app.bestiary.position>0);
 auto thumb=find(HitKind::BestiaryScroll);CHECK(thumb.kind==HitKind::BestiaryScroll);app.pointer_move(thumb.x+thumb.w/2,thumb.y+5);app.pointer_button(true);
 app.pointer_move(thumb.x+thumb.w/2,thumb.y+thumb.h-4);app.pointer_button(false);settle();CHECK(app.bestiary.selected==selected&&app.bestiary.position>0);unchanged();
 app.input(Key::Save);app.input(Key::Load);unchanged();app.input(Key::Back);CHECK(!app.bestiary.open&&!app.bestiary_actions_preferred&&preferences_saved==1);
 CHECK(app.open_bestiary()&&!app.bestiary.actions);settle();
 const auto back=find(HitKind::UiAction,int(UiAction::Back));CHECK(back.x==0&&back.y==173&&back.w==30&&back.h==21);
 CHECK(find(HitKind::Close).kind==HitKind::None);click(back);CHECK(!app.bestiary.open&&preferences_saved==2);unchanged();
 // Special encounter entries use encounter wins and own three actors;
 // defeating one segment alone cannot unlock the complete encounter.
 app.progress.discovered_acts|=4;app.progress.encounters[unsigned(EncounterId::DecimillipedeElite)][0]={1,1,0};
 app.progress.enemies[unsigned(EnemyId::Crusher)][0]={1,1,0};app.progress.enemies[unsigned(EnemyId::Rocket)][0]={1,1,0};
 CHECK(app.open_bestiary());CHECK(row(unsigned(EnemyId::LouseProgenitor))>=0);
 int decimi=-1;for(int i=0;i<app.bestiary.row_count;++i)if(app.bestiary.rows[i]>=0&&app.bestiary_entry_model(i)==255)decimi=i;
 CHECK(decimi>=0);app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(decimi),true});CHECK(app.bestiary.actor_count==3);settle();capture(folder,"bestiary-decimillipede");
 app.input(Key::EndTurn);app.input(Key::Left);app.input(Key::Confirm);CHECK(app.bestiary.actors[0].count&&app.bestiary.actors[1].count&&app.bestiary.actors[2].count);settle(3);unchanged();
 for(EnemyId kind:{EnemyId::Crusher,EnemyId::Rocket}){const int r=row(unsigned(kind));CHECK(r>=0);app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(r),true});
  CHECK(app.bestiary.actor_count==1);app.input(Key::Left);app.input(Key::Confirm);CHECK(app.bestiary.actors[0].count);settle(4);capture(folder,kind==EnemyId::Crusher?"bestiary-crusher":"bestiary-rocket");unchanged();}
 app.input(Key::Back);CHECK(!app.bestiary.open);
}
static void physical_actions(const char*folder){
 new_run(app.game,0x41cef1ea,false,Character::Silent);app.title=true;app.reset_presentation();app.bestiary_actions_preferred=true;
 clear_ui_progress(app.progress);app.progress_loaded=true;app.progress.discovered_acts=15;
 app.progress.enemies[unsigned(EnemyId::Nibbit)][0]={1,1,0};app.progress.enemies[unsigned(EnemyId::Crusher)][0]={1,1,0};
 app.input_bindings=default_input_bindings();app.keyboard_only=false;CHECK(save(app.game,before));CHECK(app.open_bestiary());
 const int nibbit=row(unsigned(EnemyId::Nibbit)),crusher=row(unsigned(EnemyId::Crusher));CHECK(nibbit>=0&&crusher>=0);
 app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(nibbit),true});settle();
 const char*source[]={"attack","hiss","hurt","die"};
 for(unsigned i=0;i<4;++i){const auto snapshot=app.bestiary.actors[0];CHECK(app.physical_key(uint16_t('1'+i),true));
  CHECK(app.bestiary.actors[0].start==snapshot.start&&app.bestiary.actors[0].count==snapshot.count);
  CHECK(app.physical_key(uint16_t('1'+i),true));CHECK(app.bestiary.actors[0].start==snapshot.start);
  app.advance(17);CHECK(app.physical_key(uint16_t('1'+i),false));CHECK(app.bestiary.action==i&&app.bestiary.actors[0].count==1);
  const auto*clip=world_enemy_clip_directory(unsigned(app.bestiary.actors[0].clips[0]));CHECK(clip&&!std::strcmp(clip->native_clip,source[i]));
  CHECK(app.bestiary.actors[0].start==app.map_clock_ms);unchanged();
 }
 CHECK(app.bestiary.actors[0].hold);const auto held_start=app.bestiary.actors[0].start;key('9');CHECK(app.bestiary.actors[0].start==held_start);
 // A previous entry's held action cannot operate on the newly selected one.
 CHECK(app.physical_key('1',true));app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(crusher),true});
 const auto replacement=app.bestiary.actors[0];CHECK(app.physical_key('1',false));CHECK(app.bestiary.actors[0].start==replacement.start&&app.bestiary.actors[0].count==replacement.count);
 settle();CHECK(app.bestiary_move_count()==7);
 const char*crab_clips[]={"left/attack_heavy","left/attack_med","left/attack_double","left/buff","left/attack_med","left/hurt","left/die"};
 for(unsigned i=0;i<7;++i){key(uint16_t('1'+i));CHECK(app.bestiary.action==i&&app.bestiary.actors[0].count);
  const auto*clip=world_enemy_clip_directory(unsigned(app.bestiary.actors[0].clips[0]));CHECK(clip&&!std::strcmp(clip->native_clip,crab_clips[i]));unchanged();}
 key('1');settle(3);capture(folder,"bestiary-source-seven-action-layout");
 // Directional mode uses sourceDeck/Exhaust/Discard/Draw/TopPanel. It must
 // not confuse the fifth command with the legacy Potions key.
 app.keyboard_only=true;app.pointer.visible=false;
 const uint16_t directional[]={'Q','F','D','A','W'};
 for(unsigned i=0;i<5;++i){key(directional[i]);CHECK(app.bestiary.action==i&&app.bestiary.actors[0].count);
  const auto*clip=world_enemy_clip_directory(unsigned(app.bestiary.actors[0].clips[0]));CHECK(clip&&!std::strcmp(clip->native_clip,crab_clips[i]));}
 const auto fifth=app.bestiary.actors[0];key('P');CHECK(app.bestiary.action==4&&app.bestiary.actors[0].start==fifth.start);
 // This keyboard/calculator host has no controller alternate stick. Every
 // remaining original action stays reachable through real focus+Select.
 app.input(Key::Left);key(InputDown);CHECK(app.bestiary.action==5);key(' ');
 CHECK(!std::strcmp(world_enemy_clip_directory(unsigned(app.bestiary.actors[0].clips[0]))->native_clip,"left/hurt"));
 key(InputDown);CHECK(app.bestiary.action==6);key(' ');
 CHECK(!std::strcmp(world_enemy_clip_directory(unsigned(app.bestiary.actors[0].clips[0]))->native_clip,"left/die"));
 CHECK(app.physical_key('Q',true));const auto no_release=app.bestiary.actors[0];app.physical_keys_cancel();CHECK(!app.physical_key('Q',false));
 CHECK(app.bestiary.actors[0].start==no_release.start&&app.bestiary.moves[0].phase!=1);
 CHECK(app.physical_key(13,true));CHECK(app.bestiary.actions);CHECK(app.physical_key(13,true));CHECK(app.bestiary.actions);
 CHECK(app.physical_key(13,false));CHECK(!app.bestiary.actions);const auto stats=app.bestiary.actors[0];key('Q');CHECK(app.bestiary.actors[0].start==stats.start);
 CHECK(app.physical_key(27,true));CHECK(app.bestiary.open);CHECK(app.physical_key(27,false));CHECK(!app.bestiary.open);unchanged();
 app.keyboard_only=false;
}
template<class Visitor>static void sparse_pixels(Picture picture,Visitor visit){
 CHECK(picture.pixels&&picture.format==4);const uint8_t*run=picture.pixels;
 for(unsigned y=0;y<picture.h;++y){const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
  for(unsigned span=0;span<spans;++span){const unsigned x=run[0]|unsigned(run[1])<<8,count=run[2]|unsigned(run[3])<<8,stride=run[4]?3:2;run+=5;
   for(unsigned i=0;i<count;++i){const auto*pixel=run+i*stride;visit(x+i,y,uint16_t(pixel[0]|unsigned(pixel[1])<<8),stride==3?unsigned(pixel[2]):255u);}
   run+=count*stride;
  }
 }
}
static void actual_supplements(const char*folder){
 if(app.content.version()<BestiaryKaiserContentVersion)return;
 unsigned frames=0,pixels=0;
 for(unsigned i=0;i<18;++i){const unsigned source=bestiary_kaiser::Sources[i];const auto original=app.content.clip(int(source)),patch=app.content.clip(int(BestiaryKaiserAnimationFirst+i));
  CHECK(patch.count==original.count&&patch.frame_ms==original.frame_ms&&patch.sprite_units==source+1);
  CHECK(patch.pivot_x<=0&&patch.pivot_y<=0);const int left=bestiary_kaiser::left(source<800?1:2);
  for(unsigned frame=0;frame<patch.count;++frame){++frames;const Picture picture=app.content.image(int(patch.first+frame));
   CHECK(int(picture.w)-patch.pivot_x<=320&&int(picture.h)-patch.pivot_y<=240);
   sparse_pixels(picture,[&](unsigned x,unsigned y,uint16_t,unsigned alpha){if(!alpha)return;++pixels;
    const int screen_x=int(x)-patch.pivot_x,screen_y=int(y)-patch.pivot_y;
    // Supplemental alpha never paints over the retained world's rectangle.
    CHECK(screen_x<left||screen_x>=left+bestiary_kaiser::Width||screen_y<bestiary_kaiser::Top||screen_y>=bestiary_kaiser::Top+bestiary_kaiser::Height);
   });
  }
 }
 CHECK(frames==344&&pixels>0&&app.content.healthy());
 new_run(app.game,0x41cef1ea,false,Character::Silent);app.title=true;app.reset_presentation();app.bestiary_actions_preferred=true;
 clear_ui_progress(app.progress);app.progress_loaded=true;app.progress.discovered_acts=15;
 app.progress.enemies[unsigned(EnemyId::Crusher)][0]={1,1,0};app.progress.enemies[unsigned(EnemyId::Rocket)][0]={1,1,0};
 CHECK(save(app.game,before)&&app.open_bestiary());app.pointer.visible=false;
 for(unsigned source:{310u,1009u}){const bool crusher=source==310;const int r=row(unsigned(crusher?EnemyId::Crusher:EnemyId::Rocket));CHECK(r>=0);
  app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(r),true});settle();
  auto&actor=app.bestiary.actors[0];actor.current=0;actor.count=1;actor.clips[0]=int16_t(source);actor.start=app.map_clock_ms;actor.hold=true;
  app.render();const auto patch=app.content.clip(bestiary_kaiser::directory(source));unsigned painted=0;
  sparse_pixels(app.content.image(int(patch.first)),[&](unsigned x,unsigned y,uint16_t color,unsigned alpha){
   const int sx=int(x)-patch.pivot_x,sy=int(y)-patch.pivot_y;
   if(alpha!=255||sy<40||sy>=200||sx<(crusher?160:190)||sx>=(crusher?173:230))return;
   for(int i=0;i<app.hit_count;++i){const auto&hit=app.hits[i];if(sx>=hit.x&&sx<hit.x+hit.w&&sy>=hit.y&&sy<hit.y+hit.h)return;}
   CHECK(app.pixels[sy*320+sx]==color);++painted;
  });
  CHECK(painted>0);capture(folder,crusher?"bestiary-crusher-complete-crop":"bestiary-rocket-complete-crop");unchanged();
 }
 app.input(Key::Back);CHECK(!app.bestiary.open&&app.content.healthy());
}
int main(int argc,char**argv){
 std::vector<uint8_t> pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));}
 if(argc>1){original_unknown_labels();actual_glyphs(pack);}source_layout();ordered_catalog();flow(argc>2?argv[2]:nullptr);physical_actions(argc>2?argv[2]:nullptr);actual_supplements(argc>2?argv[2]:nullptr);
 std::printf("bestiary UI7: %u checks passed; presentation=%zu bytes\n",checks,sizeof(BestiaryPresentation));
}
