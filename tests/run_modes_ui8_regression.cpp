#include "app.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static State restored;
static UiProgress history;
static uint8_t history_bytes[UiProgressCapacity];
static uint32_t now=1792800000u; // 2026-10-24, UTC.
static uint32_t clock_now(){return now;}
static unsigned checks;
static const char*captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"run_modes_ui8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void settle(){for(unsigned i=0;i<40;++i)a.advance(33);a.render();}
static void capture(const char*name){if(!captures)return;a.render();FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");for(auto p:a.pixels){const uint8_t pixel[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(pixel,1,3,f)==3);}CHECK(!std::fclose(f));}
static HitRegion hit(HitKind kind,int value){a.render();for(int i=a.hit_count-1;i>=0;--i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){const auto h=hit(kind,value);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);}
int main(int argc,char**argv){
    std::vector<uint8_t> bytes;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());std::fclose(f);CHECK(a.init(bytes.data(),bytes.size(),{}));}if(argc>2)captures=argv[2];
    a.callbacks.wall_clock=clock_now;a.progress_loaded=true;
    a.input(Key::Confirm);CHECK(a.title&&!a.character_select&&a.run_mode.screen==RunMenu::Singleplayer);settle();capture("singleplayer");
    a.ui_action(UiAction::OpenCustomRun);CHECK(a.run_mode.screen==RunMenu::Custom&&a.run_mode.pending.mode==RunMode::Custom);
    a.ui_action(UiAction::SelectRegent);CHECK(a.selected_character==Character::Regent);
    a.ui_action(UiAction::RunAscensionUp);CHECK(a.selected_ascension==1);
    a.ui_action(UiAction::RunSeed);CHECK(a.map_preview.editing&&a.map_preview.open&&a.run_seed_editing);
    for(char c:std::string(" mode i o "))a.reference_text(c);a.reference_text(13);
    CHECK(!std::strcmp(a.selected_seed,"M0DE 1 0")&&!a.map_preview.open&&!a.map_preview.generated);
    a.run_modifier_select(unsigned(RunModifier::Draft));CHECK(a.run_mode.pending.modifiers==run_modifier_bit(RunModifier::Draft));
    a.run_modifier_select(unsigned(RunModifier::SealedDeck));CHECK(a.run_mode.pending.modifiers==run_modifier_bit(RunModifier::SealedDeck));
    a.run_modifier_select(unsigned(RunModifier::Insanity));CHECK(a.run_mode.pending.modifiers==run_modifier_bit(RunModifier::Insanity));
    a.run_modifier_select(unsigned(RunModifier::AllStar));CHECK(run_modifier_count(a.run_mode.pending)==2);settle();capture("custom-modifiers");
    a.progress_page_scroll.target=0;settle();const auto h=hit(HitKind::RunModifier,0);const uint32_t before=a.run_mode.pending.modifiers;
    a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_move(h.x+h.w/2,h.y+h.h/2-18);a.pointer_button(false);CHECK(a.run_mode.pending.modifiers==before);
    a.input(Key::Back);CHECK(a.run_mode.screen==RunMenu::Singleplayer);a.ui_action(UiAction::OpenDailyRun);
    CHECK(a.run_mode.pending.mode==RunMode::Daily&&run_modifier_count(a.run_mode.pending)==3);
    RunOptions expected{};Character character{};uint8_t ascension{};char seed[65]{};
    prepare_daily_run(now/86400,Character::Ironclad,expected,character,ascension,seed);
    CHECK(a.selected_character==character&&a.selected_ascension==ascension&&!std::strcmp(seed,a.selected_seed));
    CHECK(a.run_mode.pending.modifiers==expected.modifiers&&a.run_mode.pending.setup_flags==expected.setup_flags);
    const auto old=a.run_mode.pending;now+=86399-now%86400;a.refresh_daily_run();CHECK(a.run_mode.pending.modifiers==old.modifiers);
    ++now;prepare_daily_run(now/86400,a.selected_character,expected,character,ascension,seed);a.refresh_daily_run();
    CHECK(a.run_mode.pending.daily_day==now/86400&&a.run_mode.pending.modifiers==expected.modifiers&&a.selected_character==character);
    settle();capture("daily-offline");
    a.input(Key::Back);a.ui_action(UiAction::OpenCustomRun);a.run_modifier_select(unsigned(RunModifier::Flight));
    a.ui_action(UiAction::StartConfiguredRun);CHECK(!a.title&&a.game.world.active&&a.game.run_options.mode==RunMode::Custom&&has_run_modifier(a.game,RunModifier::Flight));
    CHECK(a.game.original_events.finished&&original_event_option_count(a.game)==1); // No ordinary Neow relic for modifiers without a Neow option.
    SaveData saved{};CHECK(save(a.game,saved));CHECK(load(restored,saved.bytes,saved.size));
    CHECK(restored.run_options.mode==RunMode::Custom&&restored.run_options.modifiers==a.game.run_options.modifiers);
    a.progress_time(2200);a.game.phase=Phase::Complete;a.finish_progress();CHECK(a.progress.count&&a.history_at(0)->mode==unsigned(RunMode::Custom));
    CHECK(a.progress.characters[unsigned(a.game.character)].wins==0&&a.progress.characters[unsigned(a.game.character)].highest_ascension==0);
    const size_t n=encode_ui_progress(a.progress,history_bytes,sizeof history_bytes);CHECK(n&&history_bytes[3]=='6');CHECK(decode_ui_progress(history_bytes,n,history));
    CHECK(history.history[0].mode==unsigned(RunMode::Custom)&&history.history[0].modifiers==run_modifier_bit(RunModifier::Flight));
    UiProgressSummary summary{};CHECK(decode_ui_progress_summary(history_bytes,n,summary)&&summary.play_seconds>=2);
    // Build the actual older NSH3 shape, rather than stamping a new facts
    // tail with its former version. Mode and modifier data survive both.
    history.current.facts={};for(auto&r:history.history)r.facts={};history.current_discoveries={};history.architect_damage=0;
    const size_t legacy_size=encode_ui_progress(history,history_bytes,sizeof history_bytes);CHECK(legacy_size&&history_bytes[3]=='3');
    CHECK(decode_ui_progress(history_bytes,legacy_size,history)&&history.history[0].mode==unsigned(RunMode::Custom)&&history.history[0].modifiers==run_modifier_bit(RunModifier::Flight)&&!history.current.facts.version);
    std::printf("Singleplayer hierarchy, custom/daily seed/modifiers, drag, save/resume and unranked statistics: %u checks\n",checks);
}
