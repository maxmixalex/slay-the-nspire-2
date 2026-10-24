#include "app.hpp"
#include "content_fixture.hpp"
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
static void click(HitRegion hit){CHECK(hit.kind!=HitKind::None);app.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);app.pointer_button(true);app.pointer_button(false);app.render();}
static void capture(const char*folder,const char*name){if(!folder)return;std::string path=std::string(folder)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto pixel:app.pixels){const unsigned char rgb[3]={static_cast<unsigned char>((pixel>>11)*255/31),static_cast<unsigned char>(((pixel>>5)&63)*255/63),static_cast<unsigned char>((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
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
 app.input(Key::EndTurn);CHECK(!app.bestiary.actions&&!app.bestiary_actions_preferred&&preferences_saved==1);settle();
 CHECK(find(HitKind::BestiaryFilter,1).enabled&&find(HitKind::BestiaryFilter,2).enabled&&!find(HitKind::BestiaryFilter,3).enabled);
 click(find(HitKind::BestiaryFilter,2));settle();CHECK(app.bestiary.filter==2);capture(folder,"bestiary-silent-loss-statistics");
 const unsigned filter=app.bestiary.filter;click(find(HitKind::BestiaryFilter,3));CHECK(app.bestiary.filter==filter);unchanged();
 // Continuous wheel and thumb routing consume pointer input without
 // selecting the entries that happen to pass beneath the dragged thumb.
 const int selected=app.bestiary.selected;app.pointer_scroll(12);settle();CHECK(app.bestiary.position>0);
 auto thumb=find(HitKind::BestiaryScroll);CHECK(thumb.kind==HitKind::BestiaryScroll);app.pointer_move(thumb.x+thumb.w/2,thumb.y+5);app.pointer_button(true);
 app.pointer_move(thumb.x+thumb.w/2,thumb.y+thumb.h-4);app.pointer_button(false);settle();CHECK(app.bestiary.selected==selected&&app.bestiary.position>0);unchanged();
 app.input(Key::Save);app.input(Key::Load);unchanged();app.input(Key::Back);CHECK(!app.bestiary.open);
 CHECK(app.open_bestiary()&&!app.bestiary.actions);settle();click(find(HitKind::Close));CHECK(!app.bestiary.open);unchanged();
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
int main(int argc,char**argv){
 std::vector<uint8_t> pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));}
 ordered_catalog();flow(argc>2?argv[2]:nullptr);std::printf("bestiary UI7: %u checks passed\n",checks);
}
