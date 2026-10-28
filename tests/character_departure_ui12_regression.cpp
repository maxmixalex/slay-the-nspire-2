#include "../src/app.cpp"
#include "world_run.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after,resume_disk;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"character_departure_ui12:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void step(unsigned ms){while(ms){const unsigned dt=ms>50?50:ms;app.advance(dt);ms-=dt;}app.render();}
static void select(const char*seed){
    app.reset_presentation();app.title=app.character_select=true;app.practice_entry=false;
    app.run_mode={};app.progress.tutorial_flags=FtueFresh|FtueSeenMask;
    app.selected_character=Character::Ironclad;std::strcpy(app.selected_seed,seed);
}
static void black_blend_contract(){
    constexpr unsigned levels[]={0,1,25,64,127,128,192,254,255};
    for(unsigned keep:levels){const CharacterDepartureShade lookup(keep);for(unsigned color=0;color<65536;++color){
        const unsigned red=((color>>11)*keep+127)/255;
        const unsigned green=(((color>>5)&63)*keep+127)/255;
        const unsigned blue=((color&31)*keep+127)/255;
        CHECK(character_departure_black(uint16_t(color),keep)==uint16_t((red<<11)|(green<<5)|blue));
        CHECK(lookup.apply(uint16_t(color))==uint16_t((red<<11)|(green<<5)|blue));
    }}
    Canvas canvas{app};app.character_departure.phase=2;app.character_departure.started=app.map_clock_ms;
    for(auto&p:app.pixels)p=0xffff;character_departure_screen(canvas);
    for(auto p:app.pixels)CHECK(p==0); // Black, rather than the UI's blue Ink.
    app.map_clock_ms+=400;for(auto&p:app.pixels)p=0xffff;character_departure_screen(canvas);
    CHECK(app.pixels[0]==uint16_t((4<<11)|(8<<5)|4)); // Source cubic-in at t=.5 retains 1/8.
    app.character_departure={};
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
static bool restore_run(State&state){return load(state,resume_disk.bytes,resume_disk.size);}
static void continue_transition(){
    select("CONTINUE UI14");app.selected_character=Character::Silent;CHECK(app.start_selected_run({}));
    app.game.gold+=7;CHECK(validate(app.game)&&save(app.game,resume_disk));
    app.callbacks.load=restore_run;app.return_to_title();CHECK(app.has_save&&app.title);
    const unsigned serial=app.run_number;app.input(Key::Confirm);app.render();
    CHECK(app.title&&app.character_departure.phase==1&&app.character_departure.continuing);
    CHECK(app.character_departure.character==unsigned(Character::Silent)&&!app.character_departure.reveal_ms&&app.run_save_linked);
    CHECK(!app.hit_count&&app.run_number==serial);
    app.input(Key::Right);app.input(Key::Confirm);app.input(Key::Back);
    step(799);CHECK(app.title&&app.character_departure.phase==1);
    step(1);CHECK(!app.title&&!app.character_select&&app.character_departure.phase==2);
    step(800);CHECK(!app.character_departure.active()&&app.run_number==serial);
    CHECK(save(app.game,after)&&after.size==resume_disk.size&&!std::memcmp(after.bytes,resume_disk.bytes,after.size));
    app.return_to_title();app.input(Key::Load);CHECK(app.character_departure.phase==1&&app.character_departure.continuing);
    step(1600);CHECK(!app.title&&!app.character_departure.active());
    CHECK(save(app.game,after)&&after.size==resume_disk.size&&!std::memcmp(after.bytes,resume_disk.bytes,after.size));
}
static void actual_masks(const char*path){
    FILE*file=std::fopen(path,"rb");CHECK(file);
    std::vector<uint8_t> bytes(spire_test::pack_size(file));CHECK(std::fread(bytes.data(),1,bytes.size(),file)==bytes.size());CHECK(!std::fclose(file));
    CHECK(app.init(bytes.data(),bytes.size(),{}));
    select("PREFETCH UI14");app.character_random=true;app.render();
    for(unsigned i=0;i<3;++i)CHECK(app.content.image_cached(app.content.ui_image(I12_CharacterTransitionFirst+i)));
    for(Character model:{Character::Ironclad,Character::Silent,Character::Necrobinder}){
        const unsigned index=model==Character::Silent?1:model==Character::Necrobinder?2:0;
        select("PREFETCH UI14");app.selected_character=model;app.render();
        const int id=app.content.ui_image(I12_CharacterTransitionFirst+index);CHECK(app.content.image_cached(id));
        app.character_select=false;app.has_save=true;app.saved_character=model;app.render();CHECK(app.content.image_cached(id));
        const auto mask=app.content.image(id);CHECK(mask.pixels&&mask.w==320&&mask.h==240);
        const water_reflection::Texture samples(mask);
        std::vector<uint16_t> expected(320*240);
        for(unsigned phase:{1u,2u})for(unsigned elapsed:{0u,1u,100u,400u,600u,799u,800u}){
            app.character_departure={};app.character_departure.phase=uint8_t(phase);app.character_departure.character=uint8_t(model);app.map_clock_ms=elapsed;
            const unsigned progress=elapsed>=800?4096:elapsed*4096/800;
            const unsigned cubic=unsigned((uint64_t(progress)*progress*progress+8388608u)>>24);
            const unsigned alpha=phase==1?(progress*progress*255u+8388608u)>>24:255*(4096-cubic)/4096;
            const unsigned keep=255-alpha;const int cutoff=(255*(11*4096-12*int(progress))+40959)/40960;
            const uint16_t threshold=cutoff>0&&cutoff<=255?rgb(cutoff,cutoff,cutoff):0;
            for(unsigned i=0;i<expected.size();++i){
                const uint16_t base=uint16_t(i*61+173);app.pixels[i]=base;
                const bool covered=phase==1&&progress&&(cutoff<=0||(cutoff<=255&&samples.at(i%320,i/320).color>=threshold));
                const unsigned red=((base>>11)*keep+127)/255,green=(((base>>5)&63)*keep+127)/255,blue=((base&31)*keep+127)/255;
                expected[i]=covered?0:uint16_t((red<<11)|(green<<5)|blue);
            }
            Canvas canvas{app};character_departure_screen(canvas);
            CHECK(!std::memcmp(expected.data(),app.pixels,expected.size()*sizeof(uint16_t)));
        }
    }
    app.character_departure={};
}
int main(int argc,char**argv){
    black_blend_contract();source_order();
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
    continue_transition();
    if(argc>1)actual_masks(argv[1]);
    std::printf("Random selection, saved-run transition, canonical state and source departure clocks: %u checks\n",checks);
}
