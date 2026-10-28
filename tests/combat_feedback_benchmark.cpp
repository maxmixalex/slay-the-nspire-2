#include "app.hpp"
#include "ui_core_observer.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
using namespace spire;
using Clock=std::chrono::steady_clock;
static App app;static State baseline,direct;static SaveData expected,actual;
static uint64_t nanos,effects,hash=1469598103934665603ULL;
static void mix(uint32_t value){hash^=value;hash*=1099511628211ULL;}
static void observe(void*context,const State&s,Effect effect){
 const auto start=Clock::now();UiCoreObserver::receive_effect(context,s,effect);
 nanos+=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();++effects;
}
#ifdef SPIRE_BENCH_BEFORE_EFFECT
static void before(void*context,const State&s,Effect effect){
 const auto start=Clock::now();UiCoreObserver::before_effect(context,s,effect);
 nanos+=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start).count();
}
#endif
int main(int argc,char**argv){
 const unsigned repeat=argc>1?unsigned(std::atoi(argv[1])):500;
 for(unsigned two=0;two<2;++two){
  new_run(app.game,0x41cef1ea,false);app.game.phase=Phase::Combat;app.game.current_node=app.game.floor=app.game.turn=1;
  app.game.hp=app.game.max_hp=999;app.game.energy=9;
  if(!encounter_setup(app.game,two?EncounterId::NibbitsNormal:EncounterId::NibbitsWeak))return 1;
  for(unsigned i=0;i<app.game.enemy_count;++i)app.game.enemies[i].hp=app.game.enemies[i].max_hp=999;
  const int id=app.game.card_count;queue_extension(app.game,XOp::EnemyGenerateCard,two?DaggerSpray:TwinStrike,1025);resolve_pending(app.game);
  const unsigned hand=app.game.hand.count-1;baseline=app.game;direct=baseline;
  if(!play(direct,int(hand),0)||!save(direct,expected))return 2;
  nanos=effects=0;
  for(unsigned i=0;i<repeat;++i){
   app.game=baseline;app.title=false;app.reset_presentation();app.animation=0;app.enemy_visual_clock=0;app.begin_combat_feedback(id);
   CoreObserver observer{&app,UiCoreObserver::receive,UiCoreObserver::receive_animation,observe};
#ifdef SPIRE_BENCH_BEFORE_EFFECT
   observer.before_effect=before;
#endif
   const auto previous=set_core_observer(observer);const bool played=play(app.game,int(hand),0);set_core_observer(previous);
   app.finish_combat_feedback();if(!played)return 3;
  }
  if(!save(app.game,actual)||expected.size!=actual.size||std::memcmp(expected.bytes,actual.bytes,actual.size))return 4;
  hash=1469598103934665603ULL;for(unsigned i=0;i<actual.size;++i)mix(actual.bytes[i]);const auto save_hash=hash;
  hash=1469598103934665603ULL;const auto&f=app.combat_feedback;
  for(unsigned i=0;i<f.count;++i){const auto&q=f.cues[i];mix(q.actor);mix(q.at);mix(q.hp);mix(q.block);mix(q.max_hp);}
  const auto&v=app.combat_status;for(unsigned i=0;i<v.count;++i){const auto&q=v.cues[i];mix(q.cell);mix(q.at);mix(q.value.amount);mix(q.value.auxiliary);mix(q.value.source);mix(q.value.visible);}
  printf("actors=%u iterations=%u effects=%llu capture_ns_per_effect=%.3f save_hash=%016llx cue_hash=%016llx\n",app.game.enemy_count+1,repeat,(unsigned long long)effects,double(nanos)/double(effects),(unsigned long long)save_hash,(unsigned long long)hash);
 }
}
