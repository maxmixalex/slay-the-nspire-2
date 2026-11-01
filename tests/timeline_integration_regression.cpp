#include "app.hpp"
#include "world_run.hpp"
#include "world_card_factory.hpp"
#include "ui_core_observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static App app;
static State restored;
static UiProgress decoded;
static uint8_t bytes[UiProgressCapacity];
static SaveData saved;
static uint32_t now=1793552400;
static unsigned checks,writes;
static uint32_t wall(){return now;}
static bool persist(const UiProgress&p){++writes;return encode_ui_progress(p,bytes,sizeof bytes)!=0;}
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"timeline integration:%d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void fresh(){
    app=App{};app.progress_loaded=true;app.callbacks.wall_clock=wall;app.callbacks.save_progress=persist;
    clear_ui_progress(app.progress);timeline_initialize(app.progress.timeline);
}
static void profile_roundtrip(){
    fresh();auto&t=app.progress.timeline;
    CHECK(timeline_obtain(t,Epoch::Neow,now));CHECK(timeline_reveal(t,Epoch::Neow,now+10));
    CHECK(timeline_reveal(t,Epoch::Silent1,now+20));
    CHECK(timeline_add_score(t,5000,now+30)==Epoch::Colorless1);
    const size_t n=encode_ui_progress(app.progress,bytes,sizeof bytes);CHECK(n&&bytes[3]=='B');
    CHECK(decode_ui_progress(bytes,n,decoded));
    CHECK(decoded.timeline.states[unsigned(Epoch::Neow)]==EpochState::Revealed);
    CHECK(decoded.timeline.obtained_at[unsigned(Epoch::Neow)]==now);
    CHECK(decoded.timeline.obtained_at[unsigned(Epoch::Silent1)]==now+10);
    CHECK(decoded.timeline.score==499); // One score grant, clamp below next 500.
    CHECK(!decode_ui_progress(bytes,n-1,decoded));
    clear_ui_progress(decoded);decoded.characters_started=3;decoded.characters[3].wins=1;
    const size_t old=encode_ui_progress(decoded,bytes,sizeof bytes);CHECK(old&&bytes[3]=='2');
    CHECK(decode_ui_progress(bytes,old,decoded));
    CHECK((decoded.timeline.flags&TimelineLegacyAccess)&&decoded.timeline.characters_known==11);
    CHECK(timeline_obtained_count(decoded.timeline)==0&&decoded.timeline.score==0);
    for(unsigned i=0;i<EpochCount;++i){CHECK(!decoded.timeline.obtained_at[i]);CHECK(timeline_unlocks(decoded.timeline).has(Epoch(i)));}
}
static void fresh_run(){
    fresh();CHECK(app.timeline_character_available(0));
    for(unsigned i=1;i<=CharacterKinds;++i)CHECK(!app.timeline_character_available(i));
    CHECK(!app.timeline_mode_available(RunMode::Custom)&&!app.timeline_mode_available(RunMode::Daily));
    app.selected_character=Character::Silent;CHECK(!app.start_selected_run({},false));
    app.selected_character=Character::Ironclad;CHECK(app.start_selected_run({},false));
    CHECK(app.game.world.first_run&&app.game.phase==Phase::Map&&validate(app.game));
    CHECK(app.game.world.rooms[0].ancient==0&&app.game.world.total_floor==0);
    CHECK(app.game.world.rooms[0].encounters[0]==EncounterId::NibbitsWeak);
    CHECK(app.game.world.rooms[0].encounters[1]==EncounterId::SlimesWeak);
    CHECK(app.game.world.rooms[0].bosses[0]==EncounterId::VantomBoss);
    CHECK(app.game.world.rooms[1].bosses[0]==EncounterId::TheInsatiableBoss);
    CHECK(app.game.world.rooms[2].bosses[0]==EncounterId::QueenBoss);
    CHECK(app.game.world.rooms[0].events[0]==unsigned(OriginalEventId::ByrdonisNest));
    const auto*map=world_current_map(app.game);CHECK(map&&!world_reachable(app.game,map->root));
    uint8_t choices[reference::Columns];const unsigned n=world_available_points(app.game,choices,sizeof choices);CHECK(n);
    CHECK(save(app.game,saved)&&load(restored,saved.bytes,saved.size));
    CHECK(restored.world.first_run&&!restored.world.unlocks.has(Epoch::Neow)&&restored.phase==Phase::Map);
    CHECK(!timeline_model_allowed(restored.world.unlocks,TimelineKind::Card,MoltenFist));
    CHECK(timeline_model_allowed(restored.world.unlocks,TimelineKind::Card,Bash));
    CHECK(world_choose_point(app.game,choices[0]));CHECK(app.game.enemy_world.encounter==EncounterId::NibbitsWeak);
    {UiCoreObserver observation(app);player_kill(app.game,true);resolve_pending(app.game);}
    CHECK(app.progress.timeline.states[unsigned(Epoch::Neow)]==EpochState::ObtainedNoSlot);
    CHECK(app.progress.timeline.obtained_at[unsigned(Epoch::Neow)]==now);
    const auto score=app.progress.timeline.score;const unsigned finished=app.progress.characters[0].losses;
    app.finish_progress();CHECK(app.progress.timeline.score==score&&app.progress.characters[0].losses==finished);
    app.return_to_title();app.open_timeline();CHECK(app.timeline_open&&!app.timeline_detail);
    app.timeline_input(Key::Confirm);CHECK(app.timeline_detail);
    app.timeline_input(Key::Confirm);CHECK(app.progress.timeline.states[unsigned(Epoch::Neow)]==EpochState::Revealed);
    CHECK(!app.game.world.unlocks.has(Epoch::Neow)); // Frozen old run, even after reveal.
    app.timeline_activate(unsigned(Epoch::Silent1));app.timeline_input(Key::Confirm);
    CHECK(app.timeline_character_available(unsigned(Character::Silent))&&!app.timeline_character_available(CharacterKinds));
    app.timeline_input(Key::Back);app.timeline_input(Key::Back);CHECK(!app.timeline_open);
    app.selected_character=Character::Silent;CHECK(app.start_selected_run({},false));
    CHECK(!app.game.world.first_run&&app.game.world.unlocks.has(Epoch::Neow));
    CHECK(app.game.original_events.id==OriginalEventId::Neow);
}
static void frozen_and_legacy_run(){
    fresh();timeline_initialize(app.progress.timeline,true);app.selected_character=Character::Necrobinder;
    CHECK(app.start_selected_run({},false));CHECK(save(app.game,saved));
    const unsigned original_size=saved.size;
    // v17 is exactly the old payload followed by its reserved trailer.
    std::memmove(saved.bytes+saved.size-16,saved.bytes+saved.size-4,4);saved.size-=12;saved.bytes[4]=17;
    CHECK(load(restored,saved.bytes,saved.size));CHECK(!restored.world.first_run);
    for(unsigned i=0;i<EpochCount;++i)CHECK(restored.world.unlocks.has(Epoch(i)));
    CHECK(save(restored,saved)&&saved.size==original_size);
    const auto before=app.progress.timeline.score;app.game.run_options.mode=RunMode::Custom;
    app.begin_progress();app.finish_progress(true);CHECK(app.progress.timeline.score==before);
    CHECK(!timeline_obtained_count(app.progress.timeline));
}
static void source_obtain_boundaries(){
    fresh();new_run(app.game,16,false,Character::Ironclad);
    auto&t=app.progress.timeline;auto&r=app.progress.current;
    r.facts.last_battle_room=unsigned(WorldRoom::Boss);
    for(unsigned act=0;act<3;++act){app.game.world.act_index=uint8_t(act);app.timeline_win_battle(app.game);
        CHECK(t.states[unsigned(timeline_character_epoch(0,act+2))]==EpochState::ObtainedNoSlot);}
    app.progress.encounters[unsigned(EncounterId::ByrdonisElite)][1].wins=14;
    app.progress.encounters[unsigned(EncounterId::VantomBoss)][1].wins=14;
    app.progress.encounters[unsigned(EncounterId::ByrdonisElite)][2].wins=100;
    app.timeline_win_battle(app.game);CHECK(t.states[unsigned(Epoch::Ironclad5)]==EpochState::None&&t.states[unsigned(Epoch::Ironclad6)]==EpochState::None);
    app.progress.encounters[unsigned(EncounterId::ByrdonisElite)][1].wins=15;
    app.progress.encounters[unsigned(EncounterId::VantomBoss)][1].wins=15;
    app.timeline_win_battle(app.game);CHECK(t.states[unsigned(Epoch::Ironclad5)]==EpochState::ObtainedNoSlot&&t.states[unsigned(Epoch::Ironclad6)]==EpochState::ObtainedNoSlot);
    r.character=unsigned(Character::Silent);r.outcome=2;app.timeline_finish_run(app.game);
    CHECK(t.states[unsigned(Epoch::Regent1)]==EpochState::ObtainedNoSlot);
    r.character=unsigned(Character::Regent);app.timeline_finish_run(app.game);CHECK(t.states[unsigned(Epoch::Necrobinder1)]==EpochState::ObtainedNoSlot);
    r.character=unsigned(Character::Necrobinder);app.timeline_finish_run(app.game);CHECK(t.states[unsigned(Epoch::Defect1)]==EpochState::ObtainedNoSlot);
    t.characters_known=31;r.character=0;r.outcome=2;app.timeline_finish_run(app.game);
    CHECK(t.states[unsigned(Epoch::Orobas)]==EpochState::ObtainedNoSlot&&t.states[unsigned(Epoch::DailyRun)]==EpochState::None);
    r.outcome=1;r.ascension=2;app.progress.characters[0].wins=2;app.timeline_finish_run(app.game);
    CHECK(t.states[unsigned(Epoch::Ironclad7)]==EpochState::None&&t.states[unsigned(Epoch::CustomAndSeeds)]==EpochState::None);
    CHECK(t.states[unsigned(Epoch::DailyRun)]==EpochState::ObtainedNoSlot);
    r.ascension=1;app.progress.characters[1].wins=1;t.ancients_seen=0xfdu;app.timeline_finish_run(app.game);
    CHECK(t.states[unsigned(Epoch::Ironclad7)]==EpochState::ObtainedNoSlot&&t.states[unsigned(Epoch::CustomAndSeeds)]==EpochState::ObtainedNoSlot);
    CHECK(t.states[unsigned(Epoch::Darv)]==EpochState::ObtainedNoSlot);
    fresh();app.run_mode.screen=RunMenu::Custom;app.timeline_choose_character(Character::Defect);
    CHECK(app.selected_character==Character::Ironclad); // Disabled Custom portraits cannot select locked models.
    for(auto mode:{RunMode::Custom,RunMode::Daily}){app.progress.current.mode=unsigned(mode);app.progress.current.outcome=1;app.timeline_finish_run(app.game);CHECK(!timeline_obtained_count(app.progress.timeline));}
}
static void entered_act_persists_before_boss(){
    fresh();timeline_initialize(app.progress.timeline,true);std::strcpy(app.selected_seed,"oldA");
    CHECK(app.start_selected_run({},false));CHECK(app.game.world.rooms[0].region==1);
    CHECK(app.progress.discovered_acts==2&&!app.game.world.combat_count);
    {UiCoreObserver observation(app);player_kill(app.game,true);resolve_pending(app.game);}
    CHECK(app.progress.characters[0].losses==1);
    const size_t n=encode_ui_progress(app.progress,bytes,sizeof bytes);CHECK(n&&decode_ui_progress(bytes,n,decoded));
    CHECK(decoded.discovered_acts==2&&decoded.encounters[unsigned(EncounterId::WaterfallGiantBoss)][0].seen==0);
    app.progress=decoded;app.return_to_title();std::strcpy(app.selected_seed,"oldA");
    CHECK(app.start_selected_run({},false));
    // Original-DLL vector: after the first Underdocks entry, oldA consumes
    // its first bucket draw and chooses Overgrowth. A boss is not required.
    CHECK(app.game.world.rooms[0].region==0&&app.progress.discovered_acts==3);
}
int main(){profile_roundtrip();fresh_run();frozen_and_legacy_run();source_obtain_boundaries();entered_act_persists_before_boss();
    std::printf("Timeline integration: %u checks; profile migration, normal reveal, frozen pools, first run and idempotent settlement.\n",checks);
}
