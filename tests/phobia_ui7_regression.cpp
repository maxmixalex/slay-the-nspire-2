#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "ui_core_observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"phobia UI7:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Bytes=std::vector<uint8_t>;
static void put(Bytes&b,unsigned at,uint32_t value,unsigned count=4){CHECK(at+count<=b.size());for(unsigned i=0;i<count;++i)b[at+i]=uint8_t(value>>(8*i));}
static Bytes fixture(bool alternate){
 const unsigned ni=A_Count+47,ng=1,ns=Ui7StringCount,nc=alternate?PhobiaAnimationKinds:Ui7AnimationKinds;
 const unsigned nu=alternate?unsigned(PhobiaAuxiliaryCount):unsigned(Ui7AuxiliaryCount);
 const unsigned ga=40+ni*16,sa=ga+16,ca=sa+ns*8,ua=ca+nc*16+CardKinds*16,base=ua+nu*4;
 const unsigned table=base+64,costs=table+16,descriptors=costs+((ni+1)&~1u);
 Bytes b(descriptors+16);put(b,0,0x3250534e);put(b,4,alternate?PhobiaContentVersion:Ui7ContentVersion);put(b,8,0x41cef1ea);
 put(b,16,ni);put(b,20,ng);put(b,24,ns);put(b,28,nc);put(b,32,CardKinds);
 const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,descriptors,table,770,1,524288,617323,costs,ni};
 for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
 for(unsigned i=0;i<ni;++i){const unsigned at=unsigned(b.size());b.push_back(uint8_t(i+1));b.push_back(uint8_t(i+2));
  put(b,40+i*16,at);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 const unsigned glyph=unsigned(b.size());b.push_back(255);put(b,ga,'?');put(b,ga+4,glyph);put(b,ga+8,8,2);b[ga+10]=b[ga+11]=1;put(b,ga+12,2,2);
 const unsigned empty=unsigned(b.size());b.push_back(0);const unsigned marker=unsigned(b.size());b.insert(b.end(),{'e','n','g',0});
 for(unsigned i=0;i<ns;++i)put(b,sa+i*8,empty);put(b,sa+LocaleMarkerString*8,marker);put(b,sa+LocaleMarkerString*8+4,3);
 for(unsigned i=0;i<nc;++i){put(b,ca+i*16,i>=Ui7AnimationKinds?A_Count+i-Ui7AnimationKinds:0);put(b,ca+i*16+4,1,2);put(b,ca+i*16+6,33,2);}
 if(alternate)for(unsigned i=0;i<4;++i)put(b,ua+(Ui7AuxiliaryCount+i)*4,A_Count+43+i);
 put(b,12,unsigned(b.size()));return b;
}
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void capture(const char*directory,const char*model,const char*mode){
 if(!directory)return;const std::string name=std::string(directory)+"/phobia-"+model+"-"+mode+".ppm";
 FILE*f=std::fopen(name.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(auto pixel:app.pixels){const uint8_t rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));
}
static int setup(EncounterId encounter,EnemyId model){
 int slot=-1;
 // Flyconid legitimately chooses one of two slime partners. Find a seeded
 // original composition, rather than assigning a model to an invalid slot.
 for(unsigned seed=0;slot<0&&seed<32;++seed){new_run(app.game,0x41cef1ea+seed,false,Character::Ironclad);
  app.game.phase=Phase::Combat;app.game.current_node=1;app.game.floor=app.game.turn=1;app.game.hp=app.game.max_hp=999;
  CHECK(encounter_setup(app.game,encounter));for(int i=0;i<app.game.enemy_count;++i)if(app.game.enemy_world.models[i].kind==model){slot=i;break;}}
 CHECK(slot>=0&&validate(app.game));app.title=app.library=app.paused=app.help=app.character_select=false;
 app.phobia_mode=false;app.reset_presentation();app.sync_enemy_feedback();snapshot();app.render();
 for(unsigned i=0;app.combat_preparation.active&&i<1024;++i){app.advance(33);app.render();}
 CHECK(!app.combat_preparation.active);unchanged();return slot;
}
static void same_pose(const CreaturePose&a,const CreaturePose&b){CHECK(a.x==b.x&&a.feet==b.feet&&a.width==b.width&&a.scale==b.scale&&a.intent==b.intent&&a.hp==b.hp&&a.status==b.status);}
static void models(const char*directory){
 struct Case {EnemyId model;EncounterId encounter;bool skin,segment;};
 const Case cases[]={
  {EnemyId::FossilStalker,EncounterId::FossilStalkerNormal,true,false},
  {EnemyId::HauntedShip,EncounterId::HauntedShipNormal,true,false},
  {EnemyId::SkulkingColony,EncounterId::SkulkingColonyElite,true,false},
  {EnemyId::TheObscura,EncounterId::TheObscuraNormal,true,false},
  {EnemyId::TwigSlimeM,EncounterId::FlyconidNormal,true,false},
  {EnemyId::DecimillipedeSegmentBack,EncounterId::DecimillipedeElite,false,true},
  {EnemyId::DecimillipedeSegmentFront,EncounterId::DecimillipedeElite,false,true},
  {EnemyId::DecimillipedeSegmentMiddle,EncounterId::DecimillipedeElite,false,true},
  {EnemyId::Entomancer,EncounterId::EntomancerElite,false,false},
  {EnemyId::PhrogParasite,EncounterId::PhrogParasiteElite,false,false},
  {EnemyId::TerrorEel,EncounterId::TerrorEelElite,false,false},
  {EnemyId::TheInsatiable,EncounterId::TheInsatiableBoss,false,false},
  {EnemyId::Wriggler,EncounterId::DenseVegetationEventEncounter,false,false}};
 unsigned skin_clips=0;
 for(const auto&test:cases){const int slot=setup(test.encounter,test.model);const auto*visual=world_enemy_visual(unsigned(test.model));CHECK(visual);
  int normal=-1,alternate=-1;uint32_t normal_ms=0,alternate_ms=0;bool normal_loop=false,alternate_loop=false;
  CHECK(app.enemy_frame(slot,normal,normal_ms,normal_loop));const auto pose=app.combat_enemy_pose(slot);
  app.render();capture(directory,visual->name,"normal");const std::vector<uint16_t> pixels(app.pixels,app.pixels+ScreenW*ScreenH);
  app.phobia_mode=true;app.render();CHECK(app.enemy_frame(slot,alternate,alternate_ms,alternate_loop));
  CHECK(alternate>=int(Ui7AnimationKinds)&&alternate<int(PhobiaAnimationKinds));
  CHECK(alternate!=normal);same_pose(pose,app.combat_enemy_pose(slot));
  CHECK(std::memcmp(pixels.data(),app.pixels,sizeof app.pixels));capture(directory,visual->name,"enabled");
  if(test.skin)CHECK(normal_ms==alternate_ms&&normal_loop==alternate_loop);
  else CHECK(alternate_ms==0&&alternate_loop);
  unchanged();app.phobia_mode=false;app.render();int restored=-1;uint32_t restored_ms=0;bool restored_loop=false;
  CHECK(app.enemy_frame(slot,restored,restored_ms,restored_loop)&&restored==normal&&restored_ms==normal_ms&&restored_loop==normal_loop);
  CHECK(!std::memcmp(pixels.data(),app.pixels,sizeof app.pixels));same_pose(pose,app.combat_enemy_pose(slot));unchanged();
  if(test.skin){const auto original=app.enemy_presentation[slot];
   for(unsigned i=0;i<visual->clip_count;++i){const auto*row=world_enemy_clip_row(unsigned(test.model),i);CHECK(row);
    auto&m=app.enemy_presentation[slot];m.clip=int16_t(row->directory);m.start=app.animation;m.until=app.animation+1000;m.loop=false;m.count=0;
    app.phobia_mode=true;CHECK(app.enemy_frame(slot,alternate,alternate_ms,alternate_loop));
    CHECK(alternate==app.content.world_clip("phobia",row->key)&&alternate>=int(Ui7AnimationKinds));++skin_clips;
   }app.enemy_presentation[slot]=original;unchanged();
  }
  if(test.segment){app.phobia_mode=true;
   {UiCoreObserver observer(app);damage_unpowered(app.game,slot,9999);resolve_pending(app.game);}snapshot();
   for(unsigned i=0;i<60&&app.combat_health(slot+1).hp>0;++i){app.advance(33);app.render();}
   CHECK(app.game.phase==Phase::Combat&&app.combat_health(slot+1).hp==0);
   CHECK(app.enemy_frame(slot,alternate,alternate_ms,alternate_loop));
   const std::string key=std::string(visual->name)+".dead_body";
   CHECK(alternate==app.content.world_clip("phobia",key.c_str()));same_pose(pose,app.combat_enemy_pose(slot));
   app.render();capture(directory,visual->name,"dead");unchanged();
  }
 }
 CHECK(skin_clips==32);
 // An unaffected monster keeps the exact clip and render when the setting
 // changes; the switch is not a global replacement of creature visuals.
 const int slot=setup(EncounterId::NibbitsWeak,EnemyId::Nibbit);int normal,alternate;uint32_t ms;bool loop;
 CHECK(app.enemy_frame(slot,normal,ms,loop));app.phobia_mode=true;CHECK(app.enemy_frame(slot,alternate,ms,loop)&&normal==alternate);unchanged();
}
static void backgrounds(){
 struct Case {const char*key;unsigned auxiliary;};const Case cases[]={{"FACTORY",PhobiaFactory},{"HIVE",PhobiaHive},
  {"KNOWLEDGE_DEMON_BOSS",PhobiaKnowledgeDemon},{"THE_INSATIABLE_BOSS",PhobiaInsatiable}};
 for(const auto&test:cases)CHECK(app.content.world_image("phobia_backgrounds",test.key)==app.content.ui_image(test.auxiliary));
 setup(EncounterId::KnowledgeDemonBoss,EnemyId::KnowledgeDemon);app.phobia_mode=true;CHECK(app.combat_background()==app.content.ui_image(PhobiaKnowledgeDemon));unchanged();
 setup(EncounterId::TheInsatiableBoss,EnemyId::TheInsatiable);app.phobia_mode=true;CHECK(app.combat_background()==app.content.ui_image(PhobiaInsatiable));unchanged();
 // Region fallback follows the active original act. Factory is an authored
 // background catalog item with no normal-run route in the pinned models.
 app.game.enemy_world.active=0;app.game.world.active=1;app.game.world.act_index=0;app.game.world.rooms[0].region=2;
 CHECK(app.combat_background()==app.content.ui_image(PhobiaHive));
}
static void legacy(){auto bytes=fixture(false);CHECK(app.init(bytes.data(),bytes.size(),{}));
 const int slot=setup(EncounterId::FossilStalkerNormal,EnemyId::FossilStalker);int normal,alternate;uint32_t ms;bool loop;
 CHECK(app.enemy_frame(slot,normal,ms,loop));app.phobia_mode=true;
 CHECK(app.content.world_clip("phobia","FossilStalker.idle_loop")<0&&app.content.world_image("phobia_backgrounds","HIVE")<0);
 CHECK(app.enemy_frame(slot,alternate,ms,loop)&&normal==alternate);unchanged();
}
static void swarm_samples(){
 bool ages[100]{};unsigned changed=0;int64_t radius_sum=0;
 for(unsigned slot=0;slot<100;++slot){const auto p=phobia_swarm::sample(73,slot,0),again=phobia_swarm::sample(73,slot,0),later=phobia_swarm::sample(73,slot,660);
  CHECK(p.age<2000&&p.age%20==0&&!ages[p.age/20]);ages[p.age/20]=true;
  CHECK(p.x==again.x&&p.y==again.y&&p.scale==again.scale&&p.age==again.age);changed+=p.x!=later.x||p.y!=later.y;
  for(const auto q:{p,later})CHECK(q.x>=-803*256&&q.x<=803*256&&q.y>=-803*256&&q.y<=803*256&&q.scale>=0&&q.scale<=1550);
 }
 CHECK(changed>95);
 // At birth no velocity has accumulated. Check the original area-uniform
 //50..400 ring independently, including its expected squared-radius mean.
 for(unsigned seed=0;seed<1024;++seed){const auto p=phobia_swarm::sample(seed,0,0);
  const int64_t squared=(int64_t(p.x)*p.x+int64_t(p.y)*p.y)/65536;
  CHECK(squared>=2450&&squared<=161000);radius_sum+=squared;
 }
 CHECK(radius_sum/1024>76000&&radius_sum/1024<86000);
 const auto end=phobia_swarm::sample(73,21,419),reborn=phobia_swarm::sample(73,21,420);
 CHECK(end.age==1999&&reborn.age==0&&end.scale<4&&reborn.scale>100);
}
static void swarm(const char*directory){
 if(!app.content.has_phobia_swarm_assets())return;
 CHECK(app.content.has_startup_logo_assets()&&app.content.current_profile());
 const auto image=app.content.image(app.content.ui_image(I12_PhobiaSwarm));CHECK(image.w==15&&image.h==15&&image.pixels);
 const int slot=setup(EncounterId::EntomancerElite,EnemyId::Entomancer);app.phobia_mode=true;snapshot();
 // Render the real source texture on an isolated surface. A fixed clock is
 // stable, the warmed first frame is populated, and only the proper enabled
 // body emits. This also catches texture/auxiliary-directory mismatches.
 const auto paint=[&](bool enabled,unsigned model,uint32_t clock,unsigned opacity=255){
  std::memset(app.pixels,0,sizeof app.pixels);app.phobia_mode=enabled;Canvas c{app};c.opacity=uint8_t(opacity);
  phobia_swarm_body(c,model,160,190,1024,clock,73);unsigned ink=0;for(auto pixel:app.pixels)ink+=pixel!=0;return ink;
 };
 CHECK(paint(true,unsigned(EnemyId::Entomancer),0)>25);
 const std::vector<uint16_t> first(app.pixels,app.pixels+ScreenW*ScreenH);
 CHECK(paint(true,unsigned(EnemyId::Entomancer),0)>25&&!std::memcmp(first.data(),app.pixels,sizeof app.pixels));
 CHECK(paint(true,unsigned(EnemyId::Entomancer),660)>25&&std::memcmp(first.data(),app.pixels,sizeof app.pixels));
 CHECK(!paint(false,unsigned(EnemyId::Entomancer),660));CHECK(!paint(true,unsigned(EnemyId::Nibbit),660));CHECK(!paint(true,unsigned(EnemyId::Entomancer),660,0));unchanged();
 app.phobia_mode=true;app.render();capture(directory,"Entomancer","swarm-warm");
 const auto p=app.combat_enemy_pose(slot);const int cx=p.sprite_x,cy=p.sprite_y-175*p.scale/3906;
 uint16_t region[81*81]{};for(int y=0;y<81;++y)for(int x=0;x<81;++x){const int px=cx+x-40,py=cy+y-40;if(px>=0&&px<320&&py>=22&&py<240)region[y*81+x]=app.pixels[py*320+px];}
 // Keep the room clock fixed: the alternate body is static, while its
 // independent particles follow the combat clock.
 app.animation+=20;app.render();unsigned differences=0;
 for(int y=0;y<81;++y)for(int x=0;x<81;++x){const int px=cx+x-40,py=cy+y-40;if(px>=0&&px<320&&py>=22&&py<240)differences+=region[y*81+x]!=app.pixels[py*320+px];}
 CHECK(differences>20);capture(directory,"Entomancer","swarm-later");unchanged();
 app.title=true;app.progress.discovered_acts=15;app.progress.enemies[unsigned(EnemyId::Entomancer)][0]={1,0,0};CHECK(app.open_bestiary());
 int entry=-1;for(int row=0;row<app.bestiary.row_count;++row)if(app.bestiary_entry_model(row)==unsigned(EnemyId::Entomancer))entry=row;CHECK(entry>=0);
 app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(entry),true});for(unsigned i=0;i<60;++i)app.advance(33);app.render();
 CHECK(app.bestiary.actors[0].model==unsigned(EnemyId::Entomancer));capture(directory,"Entomancer","swarm-bestiary");unchanged();app.input(Key::Back);
}
static Bytes read_bytes(const std::string&path){FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);Bytes out(spire_test::pack_size(f));CHECK(std::fread(out.data(),1,out.size(),f)==out.size());std::fclose(f);return out;}
int main(int argc,char**argv){Bytes bytes,common,locale;SplitResourceSource split;const char*directory=nullptr;
 if(argc>2&&!std::strcmp(argv[1],"--split")){
  common=read_bytes(std::string(argv[2])+"/nSpire2.common.tns");locale=read_bytes(std::string(argv[2])+"/nSpire2.eng.tns");
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage::English));
  CHECK(app.init(split.source(),{}));directory=argc>3?argv[3]:nullptr;
 }else{bytes=argc>1?read_bytes(argv[1]):fixture(true);CHECK(app.init(bytes.data(),bytes.size(),{}));directory=argc>2?argv[2]:nullptr;}
 CHECK(app.content.version()>=PhobiaContentVersion);swarm_samples();models(directory);backgrounds();swarm(directory);
 if(!common.empty())for(unsigned language=1;language<=LocaleLanguageCount;++language){
  const auto info=locale_language_info(language);locale=read_bytes(std::string(argv[2])+"/"+info.file);
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage(language)));
  CHECK(boot_probe_language(split.source())==BootLanguage(language));CHECK(app.init(split.source(),{})&&app.content.current_profile());
  CHECK(app.content.has_phobia_swarm_assets()&&app.content.has_startup_logo_assets()&&app.content.has_sleeping_vfx_assets());
  for(unsigned id:{I12_SleepingZEnglish,I12_SleepingZJapanese}){const auto image=app.content.image(app.content.ui_image(id));CHECK(image.pixels&&image.w==32&&image.h==32);}
  for(const char*key:{"SKIP_INTRO_LOGO_HEADER","SKIP_INTRO_LOGO_DESCRIPTION","TOAST_SKIP_INTRO_LOGO_ON","TOAST_SKIP_INTRO_LOGO_OFF"})CHECK(*app.content.world_string("settings_ui",key));
  CHECK(app.content.healthy());unchanged();
 }
 legacy();
 std::printf("phobia UI7: %u checks passed\n",checks);
}
