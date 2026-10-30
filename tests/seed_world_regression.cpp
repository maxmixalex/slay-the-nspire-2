#include "app.hpp"
#include "world_run.hpp"
#include "host_text_input.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace spire;
static App app;
static State original,loaded;
static SaveData encoded,again,disk;
static UiProgress progress;
static uint8_t progress_bytes[UiProgressCapacity];
static unsigned checks=0,entropy_calls=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"seed world:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool store(const State&s){return save(s,disk);}
static bool restore(State&s){return load(s,disk.bytes,disk.size);}
static uint32_t entropy(){++entropy_calls;return 0x12345678u;}
static void same(const SaveData&a,const SaveData&b){CHECK(a.size==b.size);CHECK(!std::memcmp(a.bytes,b.bytes,a.size));}
static uint32_t bootstrap(uint64_t seed){const uint32_t value=uint32_t(seed)^uint32_t(seed>>32);return value?value:1;}

static void raw_roundtrips(){
    for(const char*seed:{"oldA","olda","old",u8"old𝔞😀",u8"o\u00adldA",u8"\U000e0100\U000e0100oldA",u8"old\u0301I o",u8"é中𐐨",""}){
        uint64_t value=0;CHECK(reference::seed_hash(seed,value));
        new_run(original,bootstrap(value),false,Character::Ironclad);
        CHECK(world_set_seed_raw(original,seed));CHECK(original.world.run_seed==value&&!std::strcmp(original.world.seed_text,seed));
        CHECK(world_begin_run(original)&&validate(original));CHECK(save(original,encoded));
        new_run(loaded,17,true,Character::Silent);CHECK(load(loaded,encoded.bytes,encoded.size)&&validate(loaded));
        CHECK(loaded.world.run_seed==value&&!std::strcmp(loaded.world.seed_text,seed));CHECK(save(loaded,again));same(encoded,again);
        for(unsigned i=0;i<WorldRngCount;++i){auto&before=world_rng(original,WorldRng(i));auto&after=world_rng(loaded,WorldRng(i));
            CHECK(before.calls==after.calls);for(unsigned word=0;word<4;++word)CHECK(before.words[word]==after.words[word]);
            CHECK(before.next()==after.next());}
        CHECK(save(original,encoded));CHECK(save(loaded,again));same(encoded,again);
        CHECK(!world_set_seed_raw(original,"old\xed\xa0\x80"));CHECK(save(original,again));same(encoded,again);
        CHECK(!world_set_seed_raw(original,std::string(65,'a').c_str()));CHECK(save(original,again));same(encoded,again);
    }
}
static void start(const char*input){
    app.forget_run_checkpoint();app.character_departure={};app.reset_presentation();
    app.title=app.character_select=true;app.practice_entry=app.paused=app.help=app.library=false;
    app.selected_character=Character::Ironclad;app.selected_ascension=0;app.selected_region=reference::ActKind::Overgrowth;
    app.progress_loaded=true;app.progress.tutorial_flags=0;app.callbacks.save=store;app.callbacks.load=restore;app.callbacks.seed_entropy=entropy;
    CHECK(std::strlen(input)<sizeof app.selected_seed);std::strcpy(app.selected_seed,input);
    CHECK(app.start_selected_run(RunOptions{},false));CHECK(!app.title&&validate(app.game));
}
static void ordinary_run_and_continue(){
    for(const char*input:{"oldA",u8" \u00a0éio中\u3000",u8"ıiİIſßﬃᾀ",u8"e\u0301",u8"𐐨𞤢","   "}){
        char canonical[65]{};CHECK(reference::canonicalize_seed(input,canonical,sizeof canonical));uint64_t value=0;
        CHECK(reference::seed_hash(canonical,value));const unsigned before_entropy=entropy_calls;
        start(input);CHECK(entropy_calls==before_entropy);CHECK(app.game.world.run_seed==value&&!std::strcmp(app.game.world.seed_text,canonical));
        CHECK(app.game.seed==bootstrap(value));CHECK(app.progress.current.run_seed==value&&!std::strcmp(app.progress.current.seed_text,canonical));
        CHECK(app.save_current_run());CHECK(save(app.game,encoded));
        const size_t size=encode_ui_progress(app.progress,progress_bytes,sizeof progress_bytes);CHECK(size);
        clear_ui_progress(progress);CHECK(decode_ui_progress(progress_bytes,size,progress));
        CHECK(progress.current.run_seed==value&&!std::strcmp(progress.current.seed_text,canonical));
        new_run(app.game,1,true,Character::Silent);CHECK(app.load_saved_run());CHECK(entropy_calls==before_entropy);
        CHECK(save(app.game,again));same(encoded,again);
        CHECK(app.map_preview.set_seed(input));CHECK(!std::strcmp(app.map_preview.seed,canonical)&&app.map_preview.generated);
    }
    const unsigned calls=entropy_calls;start("");CHECK(entropy_calls==calls+1&&*app.game.world.seed_text);
}
static void editor_and_legacy_console(){
    app.title=app.character_select=true;app.run_seed_editing=true;app.map_preview.open=true;app.map_preview.editing=true;app.map_preview.draft[0]=0;
    const char*text=u8" é中😀 oldA ";host_text_utf8(text,std::strlen(text),[](int cp){app.reference_text(cp);});
    CHECK(!std::strcmp(app.map_preview.draft,text));app.reference_text(13);
    CHECK(!app.run_seed_editing&&!app.map_preview.open&&!std::strcmp(app.selected_seed,text));
    // The literal entered value remains on the setup screen until departure,
    // matching NCustomRunScreen -> StartRunLobby. Old is ordinary input here.
    start("oldA");CHECK(app.game.world.run_seed==UINT64_C(0xec8011cfff0dfcc3));
    CHECK(app.debug_command("new IRONCLAD raw:oldA"));CHECK(validate(app.game)&&app.game.world.run_seed==UINT64_C(0xcdcab7dd));
    CHECK(!std::strcmp(app.game.world.seed_text,"oldA")&&app.game.world.active);
    CHECK(app.save_current_run());CHECK(save(app.game,encoded));new_run(app.game,5,true,Character::Silent);
    CHECK(app.load_saved_run()&&app.game.world.run_seed==UINT64_C(0xcdcab7dd));CHECK(save(app.game,again));same(encoded,again);
    CHECK(app.debug_command("new IRONCLAD raw:olda")&&app.game.world.run_seed==UINT64_C(0xcdcab7bd));
    CHECK(app.debug_command("new IRONCLAD raw:")&&app.game.world.run_seed==UINT64_C(0xef46db3751d8e999));
    CHECK(app.debug_command("new IRONCLAD 1104089578")&&app.game.seed==1104089578u);
    CHECK(save(app.game,encoded));CHECK(!app.debug_command(("new IRONCLAD raw:"+std::string(65,'a')).c_str()));CHECK(save(app.game,again));same(encoded,again);
}
int main(){
    raw_roundtrips();ordinary_run_and_continue();editor_and_legacy_console();
    std::printf("Seed world / UI / save: %u checks\n",checks);return 0;
}
