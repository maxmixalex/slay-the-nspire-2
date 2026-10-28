#include "app.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"bestiary phase:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static int fact(const char*name,unsigned actor=0){for(unsigned i=0;i<world_enemy_fact_count();++i)if(!std::strcmp(name,world_enemy_fact_name(i)))return app.bestiary.actors[actor].facts[i];return -1;}
static void pure(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static const char*clip(int id){const auto*c=world_enemy_clip_directory(unsigned(id));CHECK(c);return c->native_clip;}
static void select(unsigned model){for(int i=0;i<app.bestiary.row_count;++i)if(app.bestiary.rows[i]>=0&&app.bestiary_entry_model(i)==model){app.bestiary_pointer({0,0,0,0,HitKind::BestiaryEntry,int16_t(i),true});return;}CHECK(false);}
static void move(unsigned action){app.bestiary_pointer({0,0,0,0,HitKind::BestiaryMove,int16_t(action),true});CHECK(app.bestiary.actors[0].count);pure();}
int main(int argc,char**argv){
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);std::vector<unsigned char>pack(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);
  // The streaming source below outlives the calls in the remainder of main.
  static std::vector<unsigned char>data;data.swap(pack);CHECK(app.init(data.data(),data.size(),{}));}
 new_run(app.game,0x41cef1ea,false,Character::Silent);app.title=true;app.reset_presentation();clear_ui_progress(app.progress);app.progress_loaded=true;app.progress.discovered_acts=15;
 for(auto&model:app.progress.enemies)model[0]={1,1,0};for(auto&encounter:app.progress.encounters)encounter[0]={1,1,0};
 CHECK(save(app.game,before));CHECK(app.open_bestiary(true));app.pointer.visible=false;
 select(unsigned(EnemyId::TestSubject));move(0);CHECK(fact("Respawns")==1);CHECK(!std::strcmp(clip(app.bestiary.actors[0].clips[0]),"regenerate1"));
 CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"idle_loop2"));move(1);CHECK(!std::strcmp(clip(app.bestiary.actors[0].clips[0]),"attack_double2"));move(2);CHECK(!std::strcmp(clip(app.bestiary.actors[0].clips[0]),"attack_big2"));
 move(0);CHECK(fact("Respawns")==2);CHECK(!std::strcmp(clip(app.bestiary.actors[0].clips[0]),"regenerate2"));CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"idle_loop3"));
 select(unsigned(EnemyId::CubexConstruct));move(0);CHECK(app.bestiary.actors[0].count==2);CHECK(!std::strcmp(clip(app.bestiary.actors[0].clips[0]),"unburrow"));CHECK(!std::strcmp(clip(app.bestiary.actors[0].clips[1]),"charge_start"));CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"charge_loop"));CHECK(fact("IsCharging")==1&&fact("IsBurrowed")==0);
 select(unsigned(EnemyId::BowlbugRock));move(1);CHECK(fact("IsOffBalance")==1);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"stunned_loop"));move(2);CHECK(fact("IsOffBalance")==0);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"idle_loop"));
 select(unsigned(EnemyId::LagavulinMatriarch));move(0);CHECK(fact("IsAwake")==0);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"sleep_loop"));move(1);CHECK(fact("IsAwake")==1);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"idle_loop"));
 select(unsigned(EnemyId::ToughEgg));move(0);CHECK(fact("IsHatched")==1);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"idle_loop"));
 select(unsigned(EnemyId::Tunneler));move(1);CHECK(fact("Burrowed")==1);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"hidden_loop"));move(3);CHECK(fact("IsStunned")==1&&fact("Burrowed")==0);CHECK(!std::strcmp(clip(app.bestiary.actors[0].idle),"stunned_loop"));
 select(unsigned(EnemyId::SewerClam));move(0);CHECK(fact("coral_growth_step")==1);move(0);CHECK(fact("coral_growth_step")==2);
 select(unsigned(EnemyId::TheInsatiable));move(0);CHECK(fact("HasLiquified")==1);select(unsigned(EnemyId::TheObscura));move(0);CHECK(fact("HasSummoned")==1);
 select(255);CHECK(app.bestiary.actor_count==3);move(2);
 for(unsigned i=0;i<3;++i){CHECK(fact("EnemyDown",i)==1);CHECK(app.bestiary.actors[i].start==app.map_clock_ms+150*i);CHECK(!std::strcmp(clip(app.bestiary.actors[i].idle),"dead_loop"));}
 move(0);for(auto&actor:app.bestiary.actors)CHECK(!std::strcmp(clip(actor.clips[0]),"dead_loop"));move(1);for(unsigned i=0;i<3;++i){CHECK(fact("EnemyDown",i)==0);CHECK(!std::strcmp(clip(app.bestiary.actors[i].idle),"idle_loop"));}
 pure();std::printf("bestiary phase: %u checks passed, detached presentation %zu bytes\n",checks,sizeof(BestiaryPresentation));
}
