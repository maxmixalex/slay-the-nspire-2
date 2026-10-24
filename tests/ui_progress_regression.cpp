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
static uint32_t now_seconds=1792838400;
static uint32_t wall(){return now_seconds;}
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
    ++app.game.world.combat_count;app.game.world.room_type=unsigned(WorldRoom::Monster);
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
    UiProgressSummary summary{};CHECK(decode_ui_progress_summary(bytes,saved_size,summary));CHECK(summary.exists&&summary.play_seconds==2&&summary.last_saved_at==wall());
    const size_t size=saved_size;CHECK(!decode_ui_progress(bytes,size-1,restored));bytes[size]=0;CHECK(!decode_ui_progress(bytes,size+1,restored));
    bytes[0]^=1;CHECK(!decode_ui_progress(bytes,size,restored));bytes[0]^=1;
    // Old run saves can resume without invented earlier kills or timings.
    clear_ui_progress(app.progress);app.progress_loaded=true;app.progress_dirty=false;
    new_run(app.game,0x7327,false,Character::Silent);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.game.world.combat_count=7;app.title=false;
    app.begin_progress(true);CHECK(app.progress.current.partial&&app.progress.current.seconds==0&&app.progress.current.finished_battle==7);
    CHECK(!app.bestiary_available());app.progress_time(2000);app.finish_progress(true);
    CHECK(app.history_at(0)->partial&&app.history_at(0)->abandoned&&app.progress.characters[1].losses==1);
    // Failed optional persistence keeps a dirty retry and cannot erase a run.
    const uint32_t last_saved=app.progress.last_saved_at;now_seconds+=99;
    storage_ok=false;app.progress_dirty=true;CHECK(!app.persist_progress()&&app.progress_dirty&&app.progress.last_saved_at==last_saved);
    storage_ok=true;CHECK(app.persist_progress()&&!app.progress_dirty&&app.progress.last_saved_at==now_seconds);
    // Card-library Victories is per distinct model in the final winning deck,
    // and never inferred from the retained ring or from duplicate copies.
    new_run(app.game,999,false);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.begin_progress();
    app.game.deck[0]={Strike,0};app.game.deck[1]={Strike,1};app.game.phase=Phase::Complete;app.finish_progress();
    CHECK(app.progress.card_wins[Strike]==1);app.finish_progress();CHECK(app.progress.card_wins[Strike]==1);
    CHECK(decode_ui_progress(bytes,saved_size,restored)&&restored.card_wins[Strike]==1);
    CHECK(decode_ui_progress_summary(bytes,saved_size,summary)&&summary.last_saved_at==now_seconds);
    // NSH1 used the same remaining schema but had neither the timestamp nor
    // per-card wins. Preserve its real history/discoveries, with new fields 0.
    restored=app.progress;restored.current.facts={};for(auto&r:restored.history)r.facts={};
    restored.current_discoveries={};restored.architect_damage=0;restored.journal={};
    saved_size=encode_ui_progress(restored,bytes,UiProgressCapacity);CHECK(saved_size&&bytes[3]=='2');
    constexpr size_t discovery_bytes=(CardKinds+7)/8+(WorldRelicKinds+7)/8+(PotionKinds+7)/8+(OriginalEventKinds+7)/8;
    const size_t tail=28+discovery_bytes+CardKinds*4,old_size=saved_size-4-CardKinds*4;
    std::memmove(bytes+24,bytes+28,discovery_bytes);std::memmove(bytes+24+discovery_bytes,bytes+tail,saved_size-tail);
    bytes[3]='1';for(unsigned i=0;i<4;++i)bytes[4+i]=uint8_t(old_size>>(i*8));
    CHECK(decode_ui_progress(bytes,old_size,restored));CHECK(restored.card_wins[Strike]==0&&restored.last_saved_at==0&&restored.count==app.progress.count);
    CHECK(decode_ui_progress_summary(bytes,old_size,summary)&&summary.exists&&summary.last_saved_at==0);
    CHECK(!decode_ui_progress_summary(bytes,old_size-1,summary)&&!summary.exists);
    app.progress_dirty=true;CHECK(app.persist_progress());
    // Bounded ring ordering survives an encode/decode, including full capacity.
    for(unsigned i=0;i<UiHistoryCapacity+3;++i){new_run(app.game,1000+i,false);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.begin_progress();app.finish_progress(true);}
    CHECK(app.progress.count==UiHistoryCapacity);const uint32_t newest=app.history_at(0)->seed,oldest=app.history_at(UiHistoryCapacity-1)->seed;
    CHECK(newest==1000+UiHistoryCapacity+2&&oldest==1003);
    CHECK(decode_ui_progress(bytes,saved_size,restored));app.progress=restored;
    CHECK(app.history_at(0)->seed==newest&&app.history_at(UiHistoryCapacity-1)->seed==oldest);
    std::printf("Observed UI history: instances, outcomes, persistence, old-save boundary and ring ordering passed (%zu bytes).\n",saved_size);
    return 0;
}
