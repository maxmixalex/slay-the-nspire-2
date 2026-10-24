#include "app.hpp"
#include "ui_core_observer.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstring>
using namespace spire;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"ui progress line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static App app;
static UiProgress restored;
static uint8_t bytes[UiProgressCapacity+1];
static size_t saved_size;
static unsigned writes;
static bool storage_ok=true;
static bool write_history(const UiProgress&value){if(!storage_ok)return false;++writes;saved_size=encode_ui_progress(value,bytes,UiProgressCapacity);return saved_size!=0;}
static bool read_history(UiProgress&value){return saved_size&&decode_ui_progress(bytes,saved_size,value);}
static uint32_t wall(){return 1792838400;}
static SaveData before,after;
int main(){
    app.callbacks.save_progress=write_history;app.callbacks.load_progress=read_history;app.callbacks.wall_clock=wall;
    new_run(app.game,0x7317,false,Character::Ironclad);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.title=false;
    CHECK(save(app.game,before));app.begin_progress();CHECK(app.progress.current.serial==1&&app.progress.count==0&&writes>0);
    CHECK(app.progress.current.started_at==wall());CHECK(app.progress.current.floor_count==1);
    for(unsigned i=0;i<8;++i)app.advance(250);CHECK(app.progress.current.seconds==2);
    app.paused=true;app.advance(250);app.paused=false;app.settings_open=true;app.advance(250);app.settings_open=false;
    CHECK(app.progress.current.seconds==2);
    app.open_compendium();app.open_statistics();app.statistics_open=app.compendium_open=false;
    CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    // The final potion model is a valid slot; Empty is not a discovered model.
    app.discover_potion(Potion(PotionKinds));app.discover_potion(Potion::Empty);
    CHECK(app.progress.discovered_potions[(PotionKinds-1)/8]&(1u<<((PotionKinds-1)%8)));
    // Three actual initial creatures of one model must not collapse to one
    // victory, and an already observed identity must not count as a summon.
    ++app.game.world.combat_count;
    {UiCoreObserver observer(app);CHECK(encounter_setup(app.game,EncounterId::CorpseSlugsNormal,0));app.game.phase=Phase::Combat;}
    const unsigned model=unsigned(EnemyId::CorpseSlug);CHECK(app.game.enemy_count==3);CHECK(app.enemy_statistics(model).seen==3);
    app.observe_enemy_spawned(app.game,model,app.game.enemy_world.models[0].combat_id);CHECK(app.enemy_statistics(model).seen==3);
    app.game.world.combat_ending=1;app.sync_progress();CHECK(app.enemy_statistics(model).wins==3);
    CHECK(app.enemy_statistics(model,int(Character::Ironclad)).wins==3&&app.enemy_statistics(model,int(Character::Silent)).wins==0);
    CHECK(app.encounter_statistics(unsigned(EncounterId::CorpseSlugsNormal)).wins==1);CHECK(app.bestiary_available());
    app.sync_progress();CHECK(app.enemy_statistics(model).wins==3);
    app.game.world.combat_ending=0;++app.game.world.combat_count;
    {UiCoreObserver observer(app);CHECK(encounter_setup(app.game,EncounterId::CorpseSlugsNormal,0));app.game.phase=Phase::Combat;}
    CHECK(app.enemy_statistics(model).seen==6);
    {UiCoreObserver observer(app);player_kill(app.game,true);resolve_pending(app.game);}
    CHECK(app.progress.count==1&&app.history_at(0)&&app.history_at(0)->outcome==2);
    CHECK(app.enemy_statistics(model).wins==3&&app.enemy_statistics(model).losses==3);
    CHECK(app.progress.characters[0].losses==1&&app.progress.characters[0].play_seconds==2);
    const unsigned end_writes=writes;app.finish_progress();CHECK(app.progress.count==1&&writes==end_writes);
    CHECK(app.persist_progress());CHECK(saved_size>0&&saved_size<UiProgressCapacity);
    CHECK(decode_ui_progress(bytes,saved_size,restored));CHECK(restored.history[0].deck_count==app.game.deck_count);
    CHECK(restored.enemies[model][0].seen==6&&restored.enemies[model][0].losses==3);
    const size_t size=saved_size;CHECK(!decode_ui_progress(bytes,size-1,restored));bytes[size]=0;CHECK(!decode_ui_progress(bytes,size+1,restored));
    bytes[0]^=1;CHECK(!decode_ui_progress(bytes,size,restored));bytes[0]^=1;
    // Old run saves can resume without invented earlier kills or timings.
    clear_ui_progress(app.progress);app.progress_loaded=true;app.progress_dirty=false;
    new_run(app.game,0x7327,false,Character::Silent);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.game.world.combat_count=7;app.title=false;
    app.begin_progress(true);CHECK(app.progress.current.partial&&app.progress.current.seconds==0&&app.progress.current.finished_battle==7);
    CHECK(!app.bestiary_available());app.progress_time(2000);app.finish_progress(true);
    CHECK(app.history_at(0)->partial&&app.history_at(0)->abandoned&&app.progress.characters[1].losses==1);
    // Failed optional persistence keeps a dirty retry and cannot erase a run.
    storage_ok=false;app.progress_dirty=true;CHECK(!app.persist_progress()&&app.progress_dirty);
    storage_ok=true;CHECK(app.persist_progress()&&!app.progress_dirty);
    // Bounded ring ordering survives an encode/decode, including full capacity.
    for(unsigned i=0;i<UiHistoryCapacity+3;++i){new_run(app.game,1000+i,false);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.begin_progress();app.finish_progress(true);}
    CHECK(app.progress.count==UiHistoryCapacity);const uint32_t newest=app.history_at(0)->seed,oldest=app.history_at(UiHistoryCapacity-1)->seed;
    CHECK(newest==1000+UiHistoryCapacity+2&&oldest==1003);
    CHECK(decode_ui_progress(bytes,saved_size,restored));app.progress=restored;
    CHECK(app.history_at(0)->seed==newest&&app.history_at(UiHistoryCapacity-1)->seed==oldest);
    std::printf("Observed UI history: instances, outcomes, persistence, old-save boundary and ring ordering passed (%zu bytes).\n",saved_size);
    return 0;
}
