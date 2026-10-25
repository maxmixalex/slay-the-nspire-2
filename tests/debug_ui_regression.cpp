#include "app.hpp"
#include "debug_commands.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace spire;
static App app;static State snapshot;static UiProgress progress_before;static unsigned checks=0,saves=0,progress_saves=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"console line %d: %s (%s)\n",__LINE__,#x,app.debug_reply);return 1;}}while(0)
static bool save_host(const State&){++saves;return true;}
static bool save_progress_host(const UiProgress&){++progress_saves;return true;}
static void text(const char*s){while(*s)app.debug_text(*s++);}
int main(int argc,char**argv){
    std::vector<uint8_t> content;
    if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);std::rewind(f);content.resize(size_t(n));CHECK(std::fread(content.data(),1,content.size(),f)==content.size());std::fclose(f);CHECK(app.init(content.data(),content.size(),{}));}
    app.callbacks.save=save_host;
    CHECK(!app.debug_command("room treasure EXTRA")&&app.title);CHECK(!app.debug_command("new unknown")&&app.title);
    CHECK(app.debug_command("new silent 12345"));CHECK(!app.title&&app.game.character==Character::Silent&&validate(app.game));
    CHECK(app.debug_command("room monster BOWLBUGS_WEAK")&&app.game.phase==Phase::Combat&&validate(app.game));
    snapshot=app.game;
    for(const char*bad:{"energy -1","stars -2","energy 2147483648","act 0","room event NOT_AN_EVENT","relic add NOT_A_RELIC","card NOT_A_CARD","power NOT_A_POWER 2 0","godmode extra","heal -1","potion 1","kill -1"}){
        CHECK(!app.debug_command(bad));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    }
    const int energy=app.game.energy;CHECK(app.debug_command("energy 77")&&app.game.energy==energy+77);
    CHECK(app.debug_command("stars 12")&&app.game.expansion.stars==12);
    const int gold=app.game.gold;CHECK(app.debug_command("gold -20")&&app.game.gold==gold-20);
    CHECK(app.debug_command("block 10")&&app.game.block>=10);const int hp=app.game.hp;CHECK(app.debug_command("damage 3 0")&&app.game.hp==hp);
    CHECK(app.debug_command("power STRENGTH_POWER 3 0")&&app.game.strength==3);
    CHECK(app.debug_command("power STRENGTH_POWER -1 0")&&app.game.strength==2);
    CHECK(app.debug_command("power NOXIOUS_FUMES_POWER 2 0")&&app.game.noxious_fumes==2);
    CHECK(app.debug_command("power VULNERABLE_POWER 2 1")&&app.game.enemies[0].vulnerable==2);
    CHECK(app.debug_command("card BASH Hand"));CHECK(app.game.cards[app.game.hand.ids[app.game.hand.count-1]].kind==Bash);
    char command[128];std::snprintf(command,sizeof command,"upgrade %u",unsigned(app.game.hand.count-1));CHECK(app.debug_command(command));CHECK(app.game.cards[app.game.hand.ids[app.game.hand.count-1]].upgraded);
    CHECK(app.debug_command("remove_card BASH Hand"));const unsigned deck=app.game.deck_count;
    CHECK(app.debug_command("card BASH Deck")&&app.game.deck_count==deck+1);CHECK(app.debug_command("remove_card BASH Deck")&&app.game.deck_count==deck);
    CHECK(app.debug_command("potion FIRE_POTION"));bool found=false;for(unsigned i=0;i<potion_slot_count(app.game);++i)found|=potion_at(app.game,i)==Potion::Fire;CHECK(found);
    CHECK(app.debug_command("relic add ANCHOR")&&has_world_relic(app.game,world_relic_id("ANCHOR")));
    CHECK(app.debug_command("relic remove ANCHOR")&&!has_world_relic(app.game,world_relic_id("ANCHOR")));
    CHECK(app.debug_command("godmode")&&debug_godmode(app.game)&&app.game.strength==9999&&power_value(app.game,XPower::Buffer)==9999);
    CHECK(app.debug_command("room monster BOWLBUGS_WEAK")&&app.game.strength==9999&&power_value(app.game,XPower::Regen)==9999);
    CHECK(app.debug_command("godmode")&&!debug_godmode(app.game)&&app.game.strength==0&&power_value(app.game,XPower::Buffer)==0);
    CHECK(app.game.enemy_count>1);const int second_hp=app.game.enemies[1].hp;CHECK(app.debug_command("kill")&&app.game.enemies[0].hp==0&&app.game.enemies[1].hp==second_hp);
    CHECK(app.debug_command("win")&&validate(app.game)&&!combat_phase(app.game.phase));
    CHECK(app.debug_command("room shop")&&app.game.phase==Phase::Shop);
    CHECK(app.debug_command("ancient NEOW GOLDEN_PEARL")&&app.game.original_events.offers[0]==WorldRelicGoldenPearl&&validate(app.game));
    snapshot=app.game;CHECK(!app.debug_command("ancient NEOW PAELS_WING")&&!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    CHECK(app.debug_command("act HIVE")&&app.game.world.act_index==0&&app.game.world.rooms[0].region==unsigned(reference::ActKind::Hive)&&validate(app.game));
    CHECK(app.debug_command("travel")&&debug_travel(app.game));const auto&map=app.game.world.maps[app.game.world.act_index];const unsigned destination=map.boss;
    CHECK(world_reachable(app.game,destination));CHECK(world_choose_point(app.game,destination)&&validate(app.game));
    CHECK(app.debug_command("travel")&&!debug_travel(app.game));
    CHECK(app.debug_command("act 3")&&app.game.world.act_index==2&&validate(app.game));
    // Unlock's Bestiary contract is the source's empty-FightStats Ironclad
    // win, with complete argument validation before any profile mutation.
    app.progress_loaded=true;app.callbacks.save_progress=save_progress_host;
    constexpr unsigned egg=unsigned(EnemyId::BowlbugEgg),nectar=unsigned(EnemyId::BowlbugNectar);
    for(auto&stat:app.progress.enemies[egg])stat={};
    progress_before=app.progress;snapshot=app.game;const unsigned saves_before_unlock=progress_saves;
    CHECK(!app.debug_command("unlock monsters BOWLBUG_EGG NOT_A_MONSTER"));
    CHECK(!std::memcmp(&progress_before,&app.progress,sizeof progress_before)&&progress_saves==saves_before_unlock);
    CHECK(app.debug_command("unlock monsters BOWLBUG_EGG"));
    CHECK(app.enemy_statistics(egg).wins==1&&app.enemy_statistics(egg,int(Character::Ironclad)).wins==1);
    CHECK(app.enemy_statistics(egg,int(Character::Silent)).wins==0&&app.enemy_statistics(egg).seen==1);
    CHECK(app.debug_command("unlock monsters BOWLBUG_EGG")&&app.enemy_statistics(egg).wins==1);
    for(auto&stat:app.progress.enemies[egg])stat={};
    app.progress.enemies[egg][0].seen=2;app.progress.enemies[egg][unsigned(Character::Ironclad)+1].seen=2;
    CHECK(app.debug_command("unlock monsters BOWLBUG_EGG")&&app.enemy_statistics(egg).wins==1);
    CHECK(app.enemy_statistics(egg).seen==2&&app.enemy_statistics(egg,int(Character::Ironclad)).seen==2);
    for(auto&stat:app.progress.enemies[nectar])stat={};
    app.progress.enemies[nectar][0]={1,0,1};app.progress.enemies[nectar][unsigned(Character::Silent)+1]={1,0,1};
    CHECK(app.debug_command("unlock monsters BOWLBUG_NECTAR"));
    CHECK(app.enemy_statistics(nectar).wins==0&&app.enemy_statistics(nectar).losses==1);
    progress_before=app.progress;const unsigned previous_profile_saves=progress_saves;
    CHECK(!app.debug_command("unlock cards BASH NOT_A_CARD"));
    CHECK(!app.debug_command("unlock relics ANCHOR NOT_A_RELIC"));
    CHECK(!app.debug_command("unlock ascensions IRONCLAD"));
    CHECK(!std::memcmp(&progress_before,&app.progress,sizeof progress_before)&&progress_saves==previous_profile_saves);
    CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    app.input(Key::Debug);CHECK(app.debug_open&&app.debug_editing);snapshot=app.game;app.input(Key::Save);app.advance(100);CHECK(!saves&&!std::memcmp(&snapshot,&app.game,sizeof snapshot));
#if SPIRE_DEBUG_TEXT_INPUT
    app.debug_text(21);text("ene");app.debug_text(9);CHECK(!std::strcmp(app.debug_line,"energy ")&&!app.debug_completing);
    app.debug_text(21);text("room res");app.debug_text(9);CHECK(!std::strcmp(app.debug_line,"room RestSite "));app.debug_text(13);CHECK(app.debug_open&&app.game.phase==Phase::Camp&&validate(app.game));
    app.input(Key::Up);CHECK(!std::strcmp(app.debug_line,"room RestSite "));app.input(Key::Down);CHECK(!*app.debug_line);
    text("gold 12");app.input(Key::Left);app.debug_text(8);text("3");CHECK(!std::strcmp(app.debug_line,"gold 32"));const int before=app.game.gold;app.debug_text(13);CHECK(app.debug_open&&app.game.gold==before+32);
    text("relic ");app.debug_text(9);CHECK(app.debug_completing&&app.debug_completion_total>2);app.input(Key::Down);app.debug_text(13);CHECK(!app.debug_completing&&!std::strcmp(app.debug_line,"relic remove "));
    app.debug_text(21);text("power ");app.debug_text(9);CHECK(app.debug_completing);app.debug_text(27);CHECK(app.debug_open&&!app.debug_completing);
    app.debug_text(21);text("help");app.debug_text(13);CHECK(app.debug_output_count>4&&app.debug_open);
#endif
    if(argc>1){app.render();const HitRegion*close=nullptr;for(unsigned i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Close)close=&app.hits[i];CHECK(close);const int x=close->x+close->w/2,y=close->y+close->h/2;app.pointer_move(x,y);app.pointer_button(true);app.pointer_button(false);CHECK(!app.debug_open);}
    else app.input(Key::Back);
    CHECK(saves==0&&!app.run_save_linked&&validate(app.game));CHECK(app.debug_command("die")&&app.game.phase==Phase::Dead&&validate(app.game));
    std::printf("developer console: %u checks; transaction rollback, original argument semantics, model hooks, history, completion, editing and no autosave pass\n",checks);return 0;
}
