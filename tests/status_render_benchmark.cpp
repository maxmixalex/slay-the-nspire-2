// Native CPU and pixel transcript for the real Haunted Ship Haunt command.
// Fixture setup is controlled; this is not a natural-run or device-FPS test.
#include "app.hpp"
#include "ui_core_observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static uint64_t cpu(){timespec now{};clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&now);return uint64_t(now.tv_sec)*1000000000u+uint64_t(now.tv_nsec);}
static uint64_t fingerprint(){uint64_t hash=1469598103934665603ULL;for(uint16_t pixel:app.pixels){hash=(hash^uint8_t(pixel))*1099511628211ULL;hash=(hash^uint8_t(pixel>>8))*1099511628211ULL;}return hash;}
static bool setup(){
 new_run(app.game,0x41cef1ea,false);app.game.phase=Phase::Combat;app.game.current_node=app.game.floor=app.game.turn=1;
 enter_combat(app.game);if(!encounter_setup(app.game,EncounterId::HauntedShipNormal))return false;
 app.game.hp=app.game.max_hp=999;
 app.title=app.paused=app.help=app.library=app.character_select=app.settings_open=app.profiles_open=app.compendium_open=false;
 app.statistics_open=app.history_open=app.map_preview.open=false;app.run_mode={};app.frontend_confirm=0;app.progress.tutorial_flags=0;
 app.map_clock_ms=1000;app.animation=30;app.reset_presentation();app.pointer.visible=false;
 app.sync_combat_preparation();app.combat_preparation.shown=true;
 for(unsigned n=0;n<1024&&app.combat_preparation.active;++n)app.step_combat_preparation();
 if(app.combat_preparation.active)return false;
 app.sync_combat_feedback();app.sync_enemy_feedback();app.render();
 const auto count=app.game.card_count;
 {UiCoreObserver observer(app);app.begin_combat_feedback();
  app.game.enemy_world.actor=0;app.game.enemy_world.models[0].node=2;record(app.game,LogKind::Enemy,0,2);
  queue_extension(app.game,XOp::EnemyMove,0,2);queue_extension(app.game,XOp::EnemyMoveFinish,0,2);resolve_pending(app.game);
 }
 return app.game.card_count==count+5&&app.combat_feedback.active&&save(app.game,before);
}
int main(int argc,char**argv){
 if(argc<2||argc>4){std::fprintf(stderr,"Usage: status_render_benchmark LOGICAL_PACK [REPEATS] [TRANSCRIPT]\n");return 2;}
 const unsigned repeats=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):10;if(!repeats||repeats>60)return 2;
 FILE*file=std::fopen(argv[1],"rb");if(!file||std::fseek(file,0,SEEK_END))return 3;
 const long size=std::ftell(file);if(size<=0||size>256*1024*1024||std::fseek(file,0,SEEK_SET))return 3;
 std::vector<uint8_t> data(size);if(std::fread(data.data(),1,data.size(),file)!=data.size()||std::fclose(file))return 3;
 FILE*transcript=argc>3?std::fopen(argv[3],"wb"):nullptr;if(argc>3&&!transcript)return 3;
 if(!app.init(data.data(),unsigned(data.size()),{}))return 4;
 uint64_t total=0,peak=0;unsigned frames=0,previews=0;
 for(unsigned repeat=0;repeat<repeats;++repeat){
  if(!setup())return 5;
  for(unsigned frame=0;frame<182;++frame){
   const auto start=cpu();app.advance(33);app.render();const auto elapsed=cpu()-start;
   total+=elapsed;if(elapsed>peak)peak=elapsed;++frames;
   for(const auto&sprite:app.combat_cards.sprites)previews+=sprite.mode==CombatCardMotion::Preview&&sprite.card.kind==Dazed;
   if(transcript&&std::fprintf(transcript,"%u %u %016llx\n",repeat,frame,(unsigned long long)fingerprint())<0)return 6;
  }
  if(!save(app.game,after)||before.size!=after.size||std::memcmp(before.bytes,after.bytes,before.size))return 7;
 }
 std::printf("{\"frames\":%u,\"cpu_ms\":%.6f,\"max_ms\":%.6f,\"dazed_preview_frames\":%u,\"canonical_unchanged\":true,\"native_cpu_not_device_fps\":true}\n",frames,total/1000000.,peak/1000000.,previews);
 return previews&&app.content.healthy()&&(!transcript||!std::fclose(transcript))?0:8;
}
