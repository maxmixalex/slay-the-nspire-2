#include "app.hpp"
#include "debug_commands.hpp"
#include <cstdio>
#include <cstring>
using namespace spire;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#x,app.debug_reply);return 1;}}while(0)
static App app;
static State snapshot;
static unsigned saves=0;
static bool save_host(const State&){++saves;return true;}
int main(){
    app.callbacks.save=save_host;
    CHECK(!app.debug_command("room treasure EXTRA")&&app.title);
    CHECK(!app.debug_command("new unknown")&&app.title);
    CHECK(app.debug_command("new silent 12345"));CHECK(!app.title&&app.game.character==Character::Silent&&validate(app.game));
    snapshot=app.game;CHECK(!app.debug_command("room event NOT_AN_EVENT"));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    CHECK(!app.debug_command("energy 2147483648"));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    CHECK(!app.debug_command("act 0"));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    char command[128];std::snprintf(command,sizeof command,"room monster %s",encounter_definition(EncounterId::BowlbugsWeak)->key);
    CHECK(app.debug_command(command)&&app.game.phase==Phase::Combat&&validate(app.game));
    CHECK(app.debug_command("energy 77")&&app.game.energy==77);
    CHECK(app.debug_command("kill all")&&app.game.phase==Phase::Reward&&validate(app.game));
    CHECK(app.debug_command("room shop")&&app.game.phase==Phase::Shop);
    const auto gold=app.game.gold;CHECK(app.debug_command("gold 1000")&&app.game.gold>=gold);
    CHECK(app.debug_command("act 3")&&app.game.world.act_index==2&&validate(app.game));
    snapshot=app.game;app.input(Key::Debug);CHECK(app.debug_open);app.input(Key::Save);app.advance(100);CHECK(!saves&&!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    app.debug_selection=10;app.debug_variant=0;app.debug_preset();app.input(Key::Right);CHECK(app.debug_variant==1&&std::strstr(app.debug_line,"room event "));
    app.input(Key::Detail);CHECK(app.debug_editing);std::strcpy(app.debug_line,"room res");app.debug_text('t');app.debug_text(13);
    CHECK(!app.debug_open&&app.game.phase==Phase::Camp&&validate(app.game));
    CHECK(saves==0&&!app.run_save_linked);
    return 0;
}
