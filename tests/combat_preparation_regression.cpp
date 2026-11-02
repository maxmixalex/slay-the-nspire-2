#define main existing_content_tiles_suite
#include "content_tiles_regression.cpp"
#undef main
#include "resource_transfer.hpp"
#include "ui7_assets.hpp"
#include "framed_animation_ui22.hpp"
#include "content_animation_profile.hpp"

namespace {
App app;
SaveData initial,current;
unsigned polls=0,saves=0;
uint32_t clock_value=0,clock_step=0;
uint32_t preparation_clock(){const auto now=clock_value;clock_value+=clock_step;return now;}
void poll_input(){++polls;}
bool save_run(const State&){++saves;return true;}
Package native_pack(unsigned images=160,unsigned auxiliary=InspectionAuxiliaryCount,unsigned version=PredictiveIndexContentVersion){
    Package p=basic(2);p.images=images;p.encode(false,auxiliary,version,Ui7StringCount,BestiaryKaiserAnimationKinds);
    const unsigned strings=40+p.images*16+p.glyphs*16,clips=strings+Ui7StringCount*8;
    put(p.bytes,strings+LocaleMarkerString*8,unsigned(p.bytes.size()));put(p.bytes,strings+LocaleMarkerString*8+4,3);
    p.bytes.insert(p.bytes.end(),{'e','n','g',0});seal(p.bytes);
    for(unsigned i=0;i<BestiaryKaiserAnimationKinds;++i)put(p.bytes,clips+i*16,1);
    return p;
}
void battle(Provider&provider,bool timed,Character character=Character::Ironclad){
    Callbacks cb{};cb.save=save_run;cb.foreground_poll=poll_input;cb.foreground_clock_ticks=timed?preparation_clock:nullptr;
    CHECK(app.init(provider.source(),cb));new_run(app.game,73,false,character);auto&s=app.game;
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
void literal(Package&p,unsigned image,const Bytes&pixels,unsigned width,unsigned height=1){
    const unsigned at=unsigned(p.bytes.size());p.bytes.insert(p.bytes.end(),pixels.begin(),pixels.end());
    const size_t row=p.image_entry(image);put(p.bytes,row,at);put(p.bytes,row+4,width,2);put(p.bytes,row+6,height,2);
    put(p.bytes,row+8,0);put(p.bytes,row+12,unsigned(pixels.size()));
}
unsigned clip_row(const Package&p,unsigned clip){return 40+p.images*16+p.glyphs*16+Ui7StringCount*8+clip*16;}
void set_clip(Package&p,unsigned clip,unsigned first,unsigned count){
    const unsigned row=clip_row(p,clip);put(p.bytes,row,first);put(p.bytes,row+4,count,2);put(p.bytes,row+6,83,2);
}
void run_player_attacks(){
    constexpr unsigned attacks[]={AnimHeroAttack,AnimSilentAttack,AnimDefectAttack,AnimRegentAttack,AnimNecrobinderAttack};
    for(bool reduced:{false,true}){
        Package p=native_pack(177,FramedUi22AuxiliaryCount,46);
        for(unsigned id=160;id<177;++id){Bytes pixels;for(unsigned i=0;i<16;++i)append(pixels,id*193+i,2);literal(p,id,pixels,4,4);}
        for(unsigned character=0;character<CharacterKinds;++character){
            const unsigned first=160+character*3;set_clip(p,attacks[character],first,3);
            if(reduced)for(unsigned i=1;i<3;++i)std::memcpy(p.bytes.data()+p.image_entry(first+i),p.bytes.data()+p.image_entry(first),16);
        }
        // The last attack uses registration, including a fourth piece well
        // outside the LCD and outside its contiguous directory frame range.
        Bytes table;const unsigned words[]={FramedUi20Magic,1,3,83,1024,0,1024,0,28,3,
            16,1,19,1,22,2,172,0,0,173,0,0,174,0,0,176,1000,1000};
        for(unsigned word:words)append(table,word,2);literal(p,175,table,28);
        put(p.bytes,p.base-FramedUi22AuxiliaryCount*4+(FramedUi22BaseAuxiliary+AnimNecrobinderAttack)*4,175);
        if(reduced){put(p.bytes,36,content_animation_profile::ReducedPoses);std::memcpy(p.bytes.data()+p.image_entry(176),p.bytes.data()+p.image_entry(172),16);}
        seal(p.bytes);Provider provider{&p.bytes};
        for(unsigned character=0;character<CharacterKinds;++character){
            label=reduced?"five-character reduced attack aliases keep original clocks":"five-character cold attack frames and registered pieces";
            battle(provider,false,Character(character));const auto clip=app.content.clip(attacks[character]);
            CHECK(clip.count==3&&clip.frame_ms==83&&app.content.reduced_animations()==reduced);
            CHECK(app.combat_preparation.count<=CombatPreparation::Capacity&&app.combat_preparation.pages<=CombatPreparation::PageBudget);
            const unsigned animation=app.animation;unsigned steps=0;
            while(app.combat_preparation.active){app.advance(33);CHECK(++steps<2000);CHECK(app.animation==animation);same_game();}
            app.render();const unsigned first=160+character*3;
            for(unsigned i=0;i<3;++i)CHECK(app.content.image_cached(first+i));
            if(character==unsigned(Character::Necrobinder))CHECK(app.content.image_cached(175)&&app.content.image_cached(176));
            const auto before=app.content.read_stats();
            for(unsigned i=0;i<3;++i)CHECK(app.content.image(first+i).pixels);
            if(character==unsigned(Character::Necrobinder))CHECK(app.content.image(175).pixels&&app.content.image(176).pixels);
            const auto after=app.content.read_stats();CHECK(before.callbacks==after.callbacks&&before.callback_bytes==after.callback_bytes);
            CHECK(app.content.healthy()&&!resource_transfer_uses&&!provider.bad_range&&!saves);same_game();
        }
    }
}
void run_attack_budget(){
    // A full existing enemy window wins over the optional player attack.
    // Compare the entire old plan against the same pack with attack enabled.
    Package p=native_pack(463);for(unsigned id=160;id<463;++id){Bytes pixels;append(pixels,id,2);literal(p,id,pixels,1);}
    set_clip(p,AnimEnemyIdle,160,300);set_clip(p,AnimHeroAttack,460,0);seal(p.bytes);
    Provider provider{&p.bytes};label="optional attack does not displace a saturated enemy preparation window";
    battle(provider,false);const auto existing=app.combat_preparation;CHECK(existing.bounded&&existing.count==CombatPreparation::Capacity);
    set_clip(p,AnimHeroAttack,460,3);battle(provider,false);const auto&added=app.combat_preparation;
    CHECK(added.bounded&&added.count==existing.count&&added.pages==existing.pages&&added.first_count==existing.first_count);
    CHECK(!std::memcmp(added.images,existing.images,sizeof added.images)&&!std::memcmp(added.first,existing.first,sizeof added.first));
    CHECK(added.pages<=CombatPreparation::PageBudget);same_game();app.reset_presentation();
}
}
int main(){run_preparation();run_transition_art();run_player_attacks();run_attack_budget();std::printf("combat preparation: %u checks; time/work budgets, unchanged game/labels, cancellation, announced art, five-character attacks, reduced aliases, saturated priorities and resource faults\n",checks);}
