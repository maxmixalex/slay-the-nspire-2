#include "app.hpp"
#include "debug_commands.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace spire;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#x,app.debug_reply);return 1;}}while(0)
static App app;
static State snapshot;
static unsigned saves=0;
static bool save_host(const State&){++saves;return true;}
int main(int argc,char**argv){
    std::vector<uint8_t> content;
    if(argc>1){
        FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));
        const long size=std::ftell(f);CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));
        content.resize(size);CHECK(std::fread(content.data(),1,content.size(),f)==content.size());CHECK(!std::fclose(f));
        CHECK(app.init(content.data(),content.size(),{}));
    }
    const auto capture=[&](const char*name){
        app.render();if(argc<3)return true;
        char path[512];if(std::snprintf(path,sizeof path,"%s/%s.ppm",argv[2],name)>=int(sizeof path))return false;
        FILE*f=std::fopen(path,"wb");if(!f)return false;std::fprintf(f,"P6\n320 240\n255\n");
        unsigned char row[ScreenW*3];bool ok=true;
        for(int y=0;y<ScreenH;++y){for(int x=0;x<ScreenW;++x){const unsigned p=app.pixels[y*ScreenW+x];
            row[x*3]=(p>>11)*255/31;row[x*3+1]=((p>>5)&63)*255/63;row[x*3+2]=(p&31)*255/31;}
            ok&=std::fwrite(row,1,sizeof row,f)==sizeof row;}
        return std::fclose(f)==0&&ok;
    };
    app.callbacks.save=save_host;
    CHECK(!app.debug_command("room treasure EXTRA")&&app.title);
    CHECK(!app.debug_command("new unknown")&&app.title);
    CHECK(app.debug_command("new silent 12345"));CHECK(!app.title&&app.game.character==Character::Silent&&validate(app.game));
    snapshot=app.game;CHECK(!app.debug_command("room event NOT_AN_EVENT"));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    CHECK(!app.debug_command("energy 2147483648"));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    CHECK(!app.debug_command("act 0"));CHECK(!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    char command[128];std::snprintf(command,sizeof command,"room monster %s",encounter_definition(EncounterId::BowlbugsWeak)->key);
    CHECK(app.debug_command(command)&&app.game.phase==Phase::Combat&&validate(app.game));
    if(argc>1){
        snapshot=app.game;app.input(Key::Debug);CHECK(app.debug_open);CHECK(capture("debug-combat-menu"));
        const HitRegion*close=nullptr;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Close)close=&app.hits[i];
        CHECK(close);app.pointer_move(close->x+close->w/2,close->y+close->h/2);app.pointer_button(true);app.pointer_button(false);
        CHECK(!app.debug_open&&!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    }
    CHECK(app.debug_command("energy 77")&&app.game.energy==77);
    CHECK(app.debug_command("kill all")&&app.game.phase==Phase::WorldRelicChoice&&app.game.world_relics.choice==WorldRelicChoice::OfferReward&&app.game.world_relics.return_phase==uint8_t(Phase::Reward)&&app.game.world_relics.offer_count>0&&validate(app.game));
    CHECK(app.debug_command("room shop")&&app.game.phase==Phase::Shop);
    const auto gold=app.game.gold;CHECK(app.debug_command("gold 1000")&&app.game.gold>=gold);
    CHECK(app.debug_command("act 3")&&app.game.world.act_index==2&&validate(app.game));
    snapshot=app.game;app.input(Key::Debug);CHECK(app.debug_open);app.input(Key::Save);app.advance(100);CHECK(!saves&&!std::memcmp(&snapshot,&app.game,sizeof snapshot));
    app.debug_selection=10;app.debug_variant=0;app.debug_preset();app.input(Key::Right);CHECK(app.debug_variant==1&&std::strstr(app.debug_line,"room event "));
#if SPIRE_DEBUG_TEXT_INPUT
    app.input(Key::Detail);CHECK(app.debug_editing);std::strcpy(app.debug_line,"room res");app.debug_text('t');app.debug_text(13);
#else
    app.input(Key::Detail);CHECK(app.debug_open&&!app.debug_editing);
    char menu_line[sizeof app.debug_line];std::strcpy(menu_line,app.debug_line);
    app.debug_text('x');app.debug_text(8);CHECK(!std::strcmp(menu_line,app.debug_line));
    app.render();
    for(int i=0;i<app.hit_count;++i)CHECK(app.hits[i].kind!=HitKind::Command||app.hits[i].value!=int(Key::Detail));
    app.pointer_move(160,175);app.pointer_button(true);app.pointer_button(false);
    CHECK(app.debug_open&&!app.debug_editing&&!std::strcmp(menu_line,app.debug_line));
    app.input(Key::Left);CHECK(app.debug_variant==0&&!std::strcmp(app.debug_line,"room event"));
    app.input(Key::Up);CHECK(app.debug_selection==9);
    app.input(Key::Down);CHECK(app.debug_selection==10);
    app.input(Key::Up);app.input(Key::Up);CHECK(app.debug_selection==8&&!std::strcmp(app.debug_line,"room rest"));
    app.input(Key::Confirm);
#endif
    CHECK(!app.debug_open&&app.game.phase==Phase::Camp&&validate(app.game));
    if(argc>1){CHECK(capture("debug-rest-result"));app.input(Key::Debug);CHECK(capture("debug-room-menu"));app.input(Key::Back);}
    CHECK(saves==0&&!app.run_save_linked);
    std::printf("debug UI regression: %s profile, %s input passed\n",argc>1?(app.content.english()?"actual ENG":"actual ZHS"):"synthetic",SPIRE_DEBUG_TEXT_INPUT?"text":"menu-only");
    return 0;
}
