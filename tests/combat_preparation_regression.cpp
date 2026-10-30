#define main existing_content_tiles_suite
#include "content_tiles_regression.cpp"
#undef main
#include "resource_transfer.hpp"
#include "ui7_assets.hpp"

namespace {
App app;
SaveData initial,current;
unsigned polls=0,saves=0;
uint32_t clock_value=0,clock_step=0;
uint32_t preparation_clock(){const auto now=clock_value;clock_value+=clock_step;return now;}
void poll_input(){++polls;}
bool save_run(const State&){++saves;return true;}
Package native_pack(unsigned images=160){
    Package p=basic(2);p.images=images;p.encode(false,InspectionAuxiliaryCount,PredictiveIndexContentVersion,Ui7StringCount,BestiaryKaiserAnimationKinds);
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
        const unsigned animation=app.animation;
        const auto progress=app.progress;unsigned steps=0;
        while(app.combat_preparation.active){
            const auto before=app.combat_preparation.cold_images,clock_before=clock_value;
            // A real black render still advances Frame bookkeeping. Only
            // the preparation update must leave these clocks untouched.
            const auto labels=app.rich_label_clocks,overflow=app.overflow_label_clocks;
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
void run_transition_art(){
    // Distinct literal pictures make cache residency observable instead of
    // aliasing every synthetic auxiliary slot to image zero.
    Package p=native_pack(162);constexpr unsigned speck=160,status_frame=161;
    const unsigned auxiliary=p.base-InspectionAuxiliaryCount*4;
    for(unsigned image:{speck,status_frame}){
        const unsigned at=unsigned(p.bytes.size());
        for(unsigned pixel=0;pixel<16;++pixel)append(p.bytes,uint16_t(image*257+pixel),2);
        const size_t record=p.image_entry(image);
        put(p.bytes,record,at);put(p.bytes,record+4,4,2);put(p.bytes,record+6,4,2);
        put(p.bytes,record+8,0);put(p.bytes,record+12,32);
    }
    put(p.bytes,auxiliary+I7_GlowSpeck*4,speck);put(p.bytes,auxiliary+U_ColorlessSkill*4,status_frame);seal(p.bytes);
    Provider provider{&p.bytes};
    struct Case{EncounterId encounter;unsigned move;bool status;};
    const Case cases[]={{EncounterId::HauntedShipNormal,0,false},{EncounterId::HauntedShipNormal,2,true},
        {EncounterId::MechaKnightElite,1,true},{EncounterId::HauntedShipNormal,31,false}};
    for(const auto&test:cases){
        label="current card transitions and announced status-card frames";
        battle(provider,false);CHECK(encounter_setup(app.game,test.encounter));
        app.game.enemy_world.models[0].node=uint8_t(test.move);
        CHECK(bool(enemy_intent(app.game,0).flags&EnemyIntentStatus)==test.status);
        app.reset_presentation();CHECK(save(app.game,initial));app.render();
        unsigned steps=0;while(app.combat_preparation.active){app.advance(33);CHECK(++steps<2000);same_game();}
        CHECK(app.content.healthy()&&app.content.image_cached(speck));
        CHECK(app.content.image_cached(status_frame)==test.status);
        const auto before=app.content.read_stats();
        CHECK(app.content.image(speck).pixels);if(test.status)CHECK(app.content.image(status_frame).pixels);
        const auto after=app.content.read_stats();
        CHECK(before.callbacks==after.callbacks&&before.callback_bytes==after.callback_bytes);
        CHECK(!resource_transfer_uses&&!provider.bad_range&&!saves);same_game();
    }
}
}
int main(){run_preparation();run_transition_art();std::printf("combat preparation: %u checks; time/work budgets, unchanged game/labels, cancellation, announced transition art and resource faults\n",checks);}
