#define main existing_content_tiles_suite
#include "content_tiles_regression.cpp"
#undef main
#include "../src/app.cpp"

namespace {
App app;
SaveData initial,current;
unsigned polls=0,saves=0;
uint32_t clock_value=0,clock_step=0;
uint32_t preparation_clock(){const auto now=clock_value;clock_value+=clock_step;return now;}
void poll_input(){++polls;}
bool save_run(const State&){++saves;return true;}
Package native_pack(){
    Package p=basic(2);p.encode(false,InspectionAuxiliaryCount,PredictiveIndexContentVersion,Ui7StringCount,BestiaryKaiserAnimationKinds);
    const unsigned strings=40+p.images*16+p.glyphs*16,clips=strings+Ui7StringCount*8;
    put(p.bytes,strings+LocaleMarkerString*8,unsigned(p.bytes.size()));put(p.bytes,strings+LocaleMarkerString*8+4,3);
    p.bytes.insert(p.bytes.end(),{'e','n','g',0});seal(p.bytes);
    for(unsigned i=0;i<BestiaryKaiserAnimationKinds;++i)put(p.bytes,clips+i*16,1);
    return p;
}
void battle(Provider&provider,bool timed){
    Callbacks cb{};cb.save=save_run;cb.foreground_poll=poll_input;cb.foreground_clock_ticks=timed?preparation_clock:nullptr;
    CHECK(app.init(provider.source(),cb));new_run(app.game,73,false);auto&s=app.game;
    s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=s.enemy_count=1;s.enemies[0]={46,46,0,0,0,0};
    s.deck_count=s.card_count=s.hand.count=0;
    for(int i=0;i<5;++i){s.deck[s.deck_count++]={i%2?Defend:Strike,0};s.cards[s.card_count]=s.deck[i];s.zones[s.card_count]=Zone::Hand;s.hand.ids[s.hand.count++]=s.card_count++;}
    app.title=app.library=app.paused=app.help=app.character_select=app.map_preview.open=false;
    app.reset_presentation();app.pointer.visible=false;CHECK(validate(s));CHECK(save(s,initial));
    app.render();CHECK(app.combat_preparation.active&&app.combat_preparation.shown);saves=0;
}
void same_game(){CHECK(save(app.game,current));CHECK(initial.size==current.size&&!std::memcmp(initial.bytes,current.bytes,initial.size));}
void run_preparation(){
    const Package p=native_pack();Provider provider{&p.bytes};
    for(bool timed:{false,true}){
        label=timed?"clock-bounded battle preparation":"deterministic offline battle preparation";
        clock_step=timed?100:0;clock_value=UINT32_MAX-300;polls=0;battle(provider,timed);
        const unsigned animation=app.animation;const auto labels=app.rich_label_clocks,overflow=app.overflow_label_clocks;
        const auto progress=app.progress;unsigned steps=0;
        while(app.combat_preparation.active){
            const auto before=app.combat_preparation.cold_images,clock_before=clock_value;
            app.advance(33);CHECK(++steps<2000);CHECK(app.content.healthy()&&app.animation==animation);same_game();
            CHECK(app.combat_preparation.cold_images-before<=CombatPreparation::ImagesPerUpdate);
            if(timed)CHECK(uint32_t(clock_value-clock_before)<=CombatPreparation::UpdateTicks+200);
            CHECK(!std::memcmp(&labels,&app.rich_label_clocks,sizeof labels));CHECK(!std::memcmp(&overflow,&app.overflow_label_clocks,sizeof overflow));
            // A preload is not a displayed card binding or a save event.
            CHECK(!std::memcmp(progress.discovered_cards,app.progress.discovered_cards,sizeof progress.discovered_cards));CHECK(!saves);
            if(app.combat_preparation.active){app.render();for(auto pixel:app.pixels)CHECK(pixel==0);}
        }
        CHECK(polls&&app.combat_preparation.ui_stage==4);CHECK(!resource_transfer_uses);app.render();CHECK(app.content.healthy());same_game();
    }
    label="pause and presentation reset safely cancel a partial image";
    clock_step=400;battle(provider,true);app.advance(33);CHECK(app.combat_preparation.active);
    app.input(Key::Back);CHECK(app.paused);const unsigned updates=app.combat_preparation.updates;app.advance(33);CHECK(app.combat_preparation.updates==updates);same_game();
    app.paused=false;app.reset_presentation();CHECK(!app.combat_preparation.active&&!resource_transfer_uses);
    app.render();unsigned steps=0;while(app.combat_preparation.active){app.advance(33);CHECK(++steps<2000);}CHECK(app.content.healthy());same_game();
    label="a failed cold preparation stops before exposing a partial frame";
    battle(provider,false);provider.fail_first=p.group_offsets[0];provider.fail_last=provider.fail_first+1;
    steps=0;while(app.combat_preparation.active){app.advance(33);CHECK(++steps<2000);}
    CHECK(!app.content.healthy()&&!resource_transfer_uses&&!saves);same_game();
    provider.fail_first=provider.fail_last=0;battle(provider,false);steps=0;
    while(app.combat_preparation.active){app.advance(33);CHECK(++steps<2000);}CHECK(app.content.healthy());same_game();CHECK(!provider.bad_range);
}
}
int main(){run_preparation();std::printf("combat preparation: %u checks; time/work budgets, unchanged game/labels, cancellation and resource faults\n",checks);}
