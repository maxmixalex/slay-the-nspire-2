#include "app.hpp"
#include "debug_commands.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace spire;
static App app,other;
static State original;
static SaveData disk,before,after;
static unsigned checks=0,reads=0;
static bool fail_read=false,reenter=false;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"state workspace:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool read_run(State&s){
    ++reads;
    if(reenter){
        CHECK(!state_workspace::saved_probe(&app));
        CHECK(!state_workspace::begin_saved_read(&other));
        CHECK(!debug_detail::begin(app.game));
        CHECK(!debug_detail::transaction_active());
    }
    if(fail_read){s.hp=-1;s.rng_cards^=0x12345678;return false;}
    return load(s,disk.bytes,disk.size);
}
static void unchanged(){
    CHECK(save(app.game,after));
    CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
int main(int argc,char**argv){
    CHECK(argc<=2);std::vector<uint8_t> pack;
    if(argc==2){
        FILE*f=std::fopen(argv[1],"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));
        const long n=std::ftell(f);CHECK(n>0);std::rewind(f);pack.resize(size_t(n));
        CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());CHECK(!std::fclose(f));
        CHECK(app.init(pack.data(),pack.size(),{}));
    }
    new_run(app.game,20261029,false,Character::Silent);
    CHECK(world_begin_run(app.game));CHECK(debug_enter_room(app.game,WorldRoom::Shop));
    CHECK(validate(app.game)&&save(app.game,disk));
    app.callbacks.load=other.callbacks.load=read_run;
    app.progress.tutorial_flags=0;app.return_to_title();
    CHECK(app.title&&app.has_save&&app.saved_character==Character::Silent);
    const State*const shared_address=state_workspace::saved_probe(&app);
    CHECK(shared_address&&shared_address->phase==Phase::Shop);
    CHECK(!state_workspace::saved_probe(&other));
    original=app.game;CHECK(save(app.game,before));
    // Console entry invalidates the retained stock; either close path reloads
    // exactly once. Neither path modifies the active game or writes a save.
    for(unsigned i=0;i<24;++i){
        const unsigned prior=reads;app.input(Key::Debug);
        CHECK(app.debug_open&&!state_workspace::saved_probe(&app));
        app.advance(33);if(!pack.empty())app.render();unchanged();
        app.input(i&1?Key::Back:Key::Debug);
        CHECK(!app.debug_open&&reads==prior+1);
        CHECK(state_workspace::saved_probe(&app)==shared_address);
        CHECK(state_workspace::saved_probe(&app)->phase==Phase::Shop);unchanged();
    }
    // While a rollback owns the only State, neither App can decode over it.
    const unsigned prior=reads;CHECK(debug_detail::begin(app.game));
    CHECK(!state_workspace::saved_probe(&app));
    CHECK(!app.refresh_save()&&!other.refresh_save()&&reads==prior);
    CHECK(!state_workspace::begin_saved_read(&app));
    app.game.gold+=17;app.game.rng_cards^=0x19ad11;
    CHECK(!debug_detail::finish(app.game,false));
    CHECK(!debug_detail::transaction_active()&&!state_workspace::saved_probe(&app));
    CHECK(!std::memcmp(&original,&app.game,sizeof original));unchanged();
    app.return_to_title();CHECK(reads==prior+1&&state_workspace::saved_probe(&app)==shared_address);
    // A real room command rejected after abandoning its old room must keep
    // the complete saved simulation, including all named RNG streams.
    app.game.world.combat_count=255;original=app.game;
    CHECK(!debug_enter_room(app.game,WorldRoom::Monster,EncounterId::NibbitsWeak));
    CHECK(!std::memcmp(&original,&app.game,sizeof original));
    CHECK(!state_workspace::saved_probe(&app));
    app.game.world.combat_count=0;CHECK(save(app.game,before));app.return_to_title();
    // Failed/partial reads are never exposed as valid merchant stock.
    fail_read=true;CHECK(!app.refresh_save()&&!app.has_save);
    CHECK(!state_workspace::saved_probe(&app));unchanged();fail_read=false;
    reenter=true;CHECK(app.refresh_save());reenter=false;
    CHECK(state_workspace::saved_probe(&app)==shared_address);unchanged();
    // The owner prevents another App/profile's preview from being reused.
    CHECK(other.refresh_save()&&state_workspace::saved_probe(&other)==shared_address);
    CHECK(!state_workspace::saved_probe(&app));
    app.advance(33);CHECK(!state_workspace::saved_probe(&app));unchanged();
    app.return_to_title();CHECK(state_workspace::saved_probe(&app)==shared_address);
    CHECK(!state_workspace::saved_probe(&other));
    if(!pack.empty()){
        CHECK(app.init(pack.data(),pack.size(),app.callbacks));
        CHECK(state_workspace::saved_probe(&app)==shared_address);
        CHECK(app.reload_language(ResourceSource::memory(pack.data(),pack.size())));
        CHECK(state_workspace::saved_probe(&app)==shared_address);
        for(unsigned i=0;i<30;++i){app.advance(33);app.render();}
        CHECK(app.content.healthy());
    }
    CHECK(app.load_saved_run());CHECK(save(app.game,after));
    CHECK(after.size==disk.size&&!std::memcmp(after.bytes,disk.bytes,disk.size));
    std::printf("state workspace: %u checks; console close, rollback, reentrant/failed reads, owner, init, language reload and Continue pass\n",checks);
}
