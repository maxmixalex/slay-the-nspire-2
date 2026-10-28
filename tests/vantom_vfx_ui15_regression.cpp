// Requires the owner's complete UI15 pack. Exercise real combat transitions,
// original texture decoding, death events, world/local particles and restore.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
using namespace spire;
static App a;
static State direct,before;
static SaveData first,second,restore_point;
static unsigned checks=0;
static const char*scenario="complete private resources";
static std::string output;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Vantom UI15 [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static std::vector<uint8_t> read(const std::string&path){
 FILE*f=std::fopen(path.c_str(),"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);
 CHECK(n>0&&!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>bytes(size_t(n),0);
 CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size()&&!std::fclose(f));return bytes;
}
static void same(){CHECK(save(direct,first)&&save(a.game,second));CHECK(first.size==second.size&&!std::memcmp(first.bytes,second.bytes,first.size));}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof before));}
static void capture(const char*name){
 if(output.empty())return;FILE*f=std::fopen((output+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(const auto pixel:a.pixels){const uint8_t rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};
  CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));
}
static void paint(){before=a.game;a.render();unchanged();CHECK(a.content.healthy());}
static void advance(unsigned ms){a.advance(ms);paint();}
static void fresh(){
 new_run(a.game,0x41cef1ea,false);a.game.phase=Phase::Combat;a.game.current_node=1;a.game.floor=a.game.turn=1;
 enter_combat(a.game);CHECK(a.game.phase==Phase::Combat&&encounter_setup(a.game,EncounterId::VantomBoss));
 a.game.hp=a.game.max_hp=999;a.game.block=0;
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.animation=300;a.map_clock_ms=9900;a.tick_remainder=0;a.reset_presentation();a.pointer.visible=false;a.fast_mode=false;
 a.sync_combat_preparation();a.combat_preparation.shown=true;
 for(unsigned i=0;i<1024&&a.combat_preparation.active;++i)a.step_combat_preparation();CHECK(!a.combat_preparation.active);
 a.sync_combat_feedback();a.sync_enemy_feedback();CHECK(validate(a.game));
 for(unsigned i=0;i<100;++i)advance(33);
 CHECK(a.game.enemy_count==1&&a.game.enemy_world.models[0].kind==EnemyId::Vantom);
}
static unsigned alive(unsigned kind){
 static constexpr unsigned offsets[]={0,80,130,180},amounts[]={80,50,50,30};unsigned count=0;
 const auto&s=Canvas::vantom_effects()[0];const uint32_t now=a.animation*33u;
 for(unsigned i=offsets[kind];i<offsets[kind]+amounts[kind];++i)count+=s.particles[i].active&&now-s.particles[i].born<s.particles[i].lifetime;
 return count;
}
static unsigned particle_ink(unsigned kind){
 Canvas c{a};const auto pose=a.combat_enemy_pose(0);const auto&state=Canvas::vantom_effects()[0];
 const unsigned units=a.content.clip(state.clip).sprite_units;CHECK(units);
 const int scale=int(int64_t(pose.scale)*units/creature_sprite_units(EnemyId::Vantom));
 std::memset(a.pixels,0,sizeof a.pixels);before=a.game;
 c.vantom_draw_particles(1,kind,pose.sprite_x,pose.sprite_y,scale,units);unchanged();
 unsigned ink=0;for(const auto pixel:a.pixels)ink+=pixel!=0;return ink;
}
static void original_textures(){
 scenario="five exact source textures render through the affine particle path";
 Canvas c{a};
 for(unsigned key=vantom_vfx::BasicParticle;key<vantom_vfx::AuxiliaryCount;++key){
  const int image=a.content.ui_image(key);CHECK(image>=0);const auto p=a.content.image(image);
  CHECK(p.pixels&&p.format==1&&p.alpha&&p.w==(key==vantom_vfx::BasicParticle?9:48)&&p.h==(key==vantom_vfx::BasicParticle?20:48));
  std::memset(a.pixels,0,sizeof a.pixels);c.picture(image,80,40);
  const std::vector<uint16_t>expected(a.pixels,a.pixels+ScreenW*ScreenH);
  std::memset(a.pixels,0,sizeof a.pixels);
  c.vantom_particle_image(image,80*4096+int(p.w)*2048,40*4096+int(p.h)*2048,1048576,0,0,1048576,255,255,255);
  CHECK(!std::memcmp(a.pixels,expected.data(),sizeof a.pixels));
  unsigned ink=0;for(const auto pixel:a.pixels)ink+=pixel!=0;CHECK(ink>0);
 }
}
static void actual_attack(bool delayed){
 scenario=delayed?"delayed frames retain every actual intermediate idle stop event":"actual Ink Blot starts and stops its original spray";
 fresh();direct=a.game;CHECK(end_turn(direct));
 {UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(end_turn(a.game));}same();
 unsigned peak=0,ink=0;bool captured=false;
 const uint32_t end=a.animation+240;
 while(a.animation<end){const unsigned ms=mn(delayed?250u:33u,(end-a.animation)*33u);advance(ms);
  peak=mx(peak,alive(vantom_vfx::Spray));
  if(alive(vantom_vfx::Spray)){if(!captured){capture(delayed?"spray-delayed-frame":"spray-attack");captured=true;}
   ink=mx(ink,particle_ink(vantom_vfx::Spray));}
 }
 CHECK(peak>0&&peak<=80&&ink>0);CHECK(!Canvas::vantom_effects()[0].clock.streams[vantom_vfx::Spray].active);
 CHECK(!alive(vantom_vfx::Spray));same();
}
static void actual_death(bool shrunk){
 using namespace vantom_vfx;scenario=shrunk?"real Shrink removal restores Vantom during its source death animation":"Vantom real death retains dissolve and both world sprays";
 fresh();CHECK(save(a.game,restore_point));
 if(shrunk){potion_set(a.game,0,Potion::BeetleJuice);direct=a.game;CHECK(use_potion(direct,0,0));
  {UiCoreObserver observer(a);a.begin_combat_feedback();CHECK(use_potion(a.game,0,0));}same();
  for(unsigned i=0;i<60;++i)advance(33);Canvas c{a};CHECK(c.vantom_scale(1)==512);
 }
 // Original Slippery caps each HP loss at1 and consumes one layer. Execute
 // those real damage/power callbacks first; a large hit cannot bypass it.
 a.game.enemies[0].block=0;const unsigned slippery=unsigned(enemy_power_value(a.game,0,EnemyPower::Slippery));
 CHECK(slippery==8);const int protected_hp=a.game.enemies[0].hp;direct=a.game;
 for(unsigned i=0;i<slippery;++i){damage_unpowered(direct,0,1);resolve_pending(direct);}
 {UiCoreObserver observer(a);a.begin_combat_feedback();for(unsigned i=0;i<slippery;++i){damage_unpowered(a.game,0,1);resolve_pending(a.game);}}
 CHECK(!enemy_power_value(a.game,0,EnemyPower::Slippery)&&a.game.enemies[0].hp==protected_hp-int(slippery));same();
 const uint32_t settled=mx(a.animation+100u,a.combat_feedback.end_at+33u);
 while(a.animation<settled)advance(33);CHECK(a.combat_health(1).hp==a.game.enemies[0].hp);
 direct=a.game;damage_unpowered(direct,0,9999);resolve_pending(direct);
 {UiCoreObserver observer(a);a.begin_combat_feedback();damage_unpowered(a.game,0,9999);resolve_pending(a.game);}same();CHECK(a.game.enemies[0].hp==0);
 unsigned peak[EmitterCount]{},ink[EmitterCount]{};bool dissolution=false,completed=false,restoring_scale=false,tail_captured=false,captured[EmitterCount]{};
 uint32_t die_started=0;
 for(unsigned i=0;i<190;++i){advance(33);auto&effect=Canvas::vantom_effects()[0];
  if(effect.clock.current!=Die)continue;
  if(!die_started)die_started=effect.clock.started;CHECK(effect.clock.started==die_started);
  const unsigned age=a.animation*33u-die_started;
  Canvas canvas{a};const int scale=canvas.vantom_scale(1);
  if(shrunk){const auto&motion=Canvas::vantom_motions()[0].scale;
   // CreatureCmd.Kill starts death, then removes the creature's powers.
   // ShrinkPower.AfterRemoved calls Vantom.ScaleTo(1,.75), so the2133ms
   // local burst is correctly back at full scale, never frozen at512.
   CHECK(motion.from==512&&motion.to==1024&&motion.duration==750);
   CHECK(motion.started>=die_started&&motion.started-die_started<=33);
   restoring_scale|=scale>512&&scale<1024;if(age>=783)CHECK(scale==1024);
  }else CHECK(scale==1024);
  const unsigned elapsed=age<33?0:age-33;
  const unsigned expected=elapsed>=1000?4096u:unsigned(uint64_t(elapsed)*elapsed*4096u/1000000u);
  CHECK(effect.clock.dissolve(a.animation*33u)==expected);dissolution|=expected>0&&expected<4096;
  if(!tail_captured&&age>=533&&age<1033){capture(shrunk?"tail-dissolve-after-shrink":"tail-dissolve");tail_captured=true;}
  if(age>=1033)CHECK(effect.restarts==1&&effect.restart_at==die_started+1033);
  if(age>=3133){for(unsigned kind=0;kind<EmitterCount;++kind)CHECK(!alive(kind));completed=true;}
  for(unsigned kind=DeathFront;kind<EmitterCount;++kind){const unsigned amount=alive(kind);peak[kind]=mx(peak[kind],amount);
   if(amount){if(!captured[kind]&&age>=(kind==Explosion?2233u:733u)){paint();
     capture(kind==Explosion?(shrunk?"death-explosion-after-shrink":"death-explosion"):
      kind==DeathFront?(shrunk?"death-spray-front-after-shrink":"death-spray-front"):(shrunk?"death-spray-back-after-shrink":"death-spray-back"));captured[kind]=true;}
    ink[kind]=mx(ink[kind],particle_ink(kind));}}
 }
 CHECK(die_started&&dissolution&&completed&&(!shrunk||restoring_scale));CHECK(peak[DeathFront]>0&&peak[DeathFront]<=50&&peak[DeathBack]>0&&peak[DeathBack]<=50&&peak[Explosion]==30);
 CHECK(ink[DeathFront]>0&&ink[DeathBack]>0&&ink[Explosion]>0);same();
 scenario="Continue of the same run discards all previous Vantom particles and scale motion";
 CHECK(load(a.game,restore_point.bytes,restore_point.size)&&validate(a.game));a.reset_presentation();
 CHECK(!Canvas::vantom_effects()[0].owner&&!Canvas::vantom_motions()[0].owner);
 for(const auto&p:Canvas::vantom_effects()[0].particles)CHECK(!p.active);
 Canvas c{a};CHECK(c.vantom_scale(1)==1024);a.sync_combat_feedback();a.sync_enemy_feedback();paint();
 CHECK(Canvas::vantom_effects()[0].clock.current!=Die&&!Canvas::vantom_effects()[0].restarts);
 for(unsigned kind=0;kind<EmitterCount;++kind)CHECK(!alive(kind));
}
int main(int argc,char**argv){
 CHECK(argc==2||argc==3);const std::string root=argv[1];
 auto common=read(root+"/data/nSpire2.dat.tns"),language=read(root+"/data/nSpire2.eng.dat.tns");SplitResourceSource split;
 CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));
 CHECK(a.content.ui_image(VantomUi15BaseAuxiliary)>=0&&a.content.ui_image(vantom_vfx::AuxiliaryCount-1)>=0);
 if(argc==3){output=argv[2];std::filesystem::create_directories(output);}
 original_textures();actual_attack(false);actual_attack(true);actual_death(false);actual_death(true);
 std::printf("Vantom UI15: %u checks passed; complete private pack and actual combat callbacks\n",checks);
}
