#include "app.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"character_departure_ui12:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void step(unsigned ms){while(ms){const unsigned dt=ms>50?50:ms;app.advance(dt);ms-=dt;}app.render();}
static void select(const char*seed){
    app.reset_presentation();app.title=app.character_select=true;app.practice_entry=false;
    app.run_mode={};app.progress.tutorial_flags=FtueFresh|FtueSeenMask;
    app.selected_character=Character::Ironclad;std::strcpy(app.selected_seed,seed);
}
static void source_order(){
    // Independent source order: NCharacterSelectScreen adds ModelDb's five
    // models then Random. Runtime serialized enum order is intentionally
    // different and must not be used as a portrait or keyboard position.
    constexpr Character expected[]={Character::Ironclad,Character::Silent,Character::Regent,Character::Necrobinder,Character::Defect};
    constexpr UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectDefect,UiAction::SelectRandomCharacter};
    select("SOURCE ORDER");const unsigned serial=app.run_number;app.render();
    for(unsigned i=0;i<=CharacterKinds;++i){
        const auto*hit=app.hit_at(55+int(i)*36+15,218);CHECK(hit&&hit->kind==HitKind::UiAction&&hit->value==int(actions[i]));
        app.pointer_move(55+int(i)*36+15,218);app.pointer_button(true);app.pointer_button(false);
        CHECK(app.character_random==(i==CharacterKinds));if(i<CharacterKinds)CHECK(app.selected_character==expected[i]);
    }
    app.ui_action(UiAction::SelectIronclad);
    for(unsigned i=1;i<CharacterKinds;++i){app.input(Key::Right);CHECK(!app.character_random&&app.selected_character==expected[i]);}
    app.input(Key::Right);CHECK(app.character_random);app.input(Key::Right);CHECK(!app.character_random&&app.selected_character==expected[0]);
    app.input(Key::Left);CHECK(app.character_random);
    for(int i=int(CharacterKinds)-1;i>=0;--i){app.input(Key::Left);CHECK(!app.character_random&&app.selected_character==expected[i]);}
    CHECK(app.run_number==serial&&!std::strcmp(app.selected_seed,"SOURCE ORDER"));
}
int main(){
    source_order();
    select("WORLD UI12");const unsigned serial=app.run_number;app.render();
    app.ui_action(UiAction::SelectRandomCharacter);CHECK(app.character_random);
    app.ui_action(UiAction::StartCharacter);
    CHECK(app.run_number==serial+1&&app.character_departure.phase==1&&app.character_departure.reveal_ms==1000);
    CHECK(!app.character_random&&app.title&&app.character_select&&app.game.world.active);
    const Character resolved=app.selected_character;CHECK(resolved==app.game.character&&unsigned(resolved)<CharacterKinds);
    CHECK(validate(app.game)&&save(app.game,before));
    app.input(Key::Right);app.input(Key::Confirm);app.ui_action(UiAction::SelectSilent);app.ui_action(UiAction::StartCharacter);
    CHECK(app.run_number==serial+1&&app.selected_character==resolved);
    step(1799);CHECK(app.title&&app.character_departure.phase==1&&app.hit_count==0);
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    step(1);CHECK(!app.title&&!app.character_select&&app.character_departure.phase==2);
    step(799);CHECK(app.character_departure.active());step(1);CHECK(!app.character_departure.active());
    select("WORLD UI12");app.ui_action(UiAction::SelectRandomCharacter);CHECK(app.start_selected_run({}));
    CHECK(app.game.character==resolved&&!app.character_departure.active()&&!app.title);
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    select("NORMAL UI12");app.ui_action(UiAction::SelectNecrobinder);app.ui_action(UiAction::StartCharacter);
    CHECK(!app.character_departure.reveal_ms&&app.game.character==Character::Necrobinder);step(799);CHECK(app.title);step(1);CHECK(!app.title);step(800);CHECK(!app.character_departure.active());
    // Sampling several independent seeded starts must actually include all
    // playable models, and navigating Random itself consumes no run seed.
    unsigned models=0;
    for(unsigned i=0;i<64;++i){char seed[24]{};std::snprintf(seed,sizeof seed,"RANDOM %u",i);select(seed);const unsigned n=app.run_number;
        app.input(Key::Left);CHECK(app.character_random&&app.run_number==n);CHECK(app.start_selected_run({}));models|=1u<<unsigned(app.game.character);CHECK(validate(app.game));}
    CHECK(models==(1u<<CharacterKinds)-1);
    std::printf("Random character selection, deterministic run, locked controls and source departure clocks: %u checks\n",checks);
}
