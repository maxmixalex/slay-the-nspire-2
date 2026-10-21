#include "app.hpp"
#include "status.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
static App a;
static SaveData before,after,disk;
static uint32_t entropy_value=0;
static unsigned entropy_calls=0,checks=0;
static const char* screenshot_directory;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"device feedback:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t entropy(){++entropy_calls;return entropy_value;}
static bool store(const State&s){return save(s,disk);}
static bool restore(State&s){return load(s,disk.bytes,disk.size);}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void capture(const char* name){
    if(!screenshot_directory)return;
    const std::string path=std::string(screenshot_directory)+"/"+name+".ppm";
    FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
    unsigned char row[ScreenW*3];
    for(int y=0;y<ScreenH;++y){
        for(int x=0;x<ScreenW;++x){const unsigned p=a.pixels[y*ScreenW+x];row[x*3]=(p>>11)*255/31;row[x*3+1]=((p>>5)&63)*255/63;row[x*3+2]=(p&31)*255/31;}
        CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);
    }
    CHECK(!std::fclose(f));
}
static void fresh(uint32_t sample,const char*seed=""){
    a.game=State{};a.callbacks={store,restore,nullptr,entropy};a.run_number=a.map_clock_ms=0;entropy_value=sample;
    a.title=a.character_select=true;a.paused=a.help=a.library=a.settings_open=a.practice_entry=false;
    a.frontend_confirm=0;a.selected_character=Character::Ironclad;a.selected_ascension=0;a.selected_region=reference::ActKind::Overgrowth;
    std::strncpy(a.selected_seed,seed,sizeof a.selected_seed-1);a.selected_seed[sizeof a.selected_seed-1]=0;
    a.reset_presentation();a.ui_action(UiAction::StartCharacter);CHECK(!a.title&&!a.character_select&&validate(a.game));
}
static void seeds(){
    fresh(0x12345678u);CHECK(*a.game.world.seed_text&&!*a.selected_seed&&entropy_calls==1);
    const uint32_t first=a.game.seed;const uint64_t world=a.game.world.run_seed;
    char replay[65]{};std::strcpy(replay,a.game.world.seed_text);snapshot();
    // Re-entering the displayed auto seed reproduces the entire opening,
    // including the legacy seed, named streams, map, deck and Neow state.
    fresh(0x87654321u,replay);CHECK(entropy_calls==1&&a.game.seed==first&&a.game.world.run_seed==world);unchanged();
    fresh(0x87654321u);CHECK(a.game.seed!=first&&a.game.world.run_seed!=world&&entropy_calls==2);
    CHECK(a.save_current_run());snapshot();const unsigned calls=entropy_calls;
    a.game=State{};entropy_value=0xaabbccddu;CHECK(a.load_saved_run());CHECK(entropy_calls==calls);unchanged();
    // Invalid text must fail before consuming entropy or replacing the run.
    a.character_select=a.title=true;a.selected_seed[0]=char(0xff);a.selected_seed[1]=0;
    const unsigned serial=a.run_number;a.ui_action(UiAction::StartCharacter);
    CHECK(a.character_select&&a.title&&entropy_calls==calls&&a.run_number==serial);unchanged();
    fresh(0);CHECK(a.game.seed==0x20261003u&&!*a.game.world.seed_text);
    // Even a coarse, repeated RTC sample produces different starts in one
    // process; zero/absent entropy remains an explicit replay fixture mode.
    a.run_number=0;entropy_value=0x10203040u;bool random=false;
    const auto seed=a.next_run_seed(&random);CHECK(random);CHECK(a.next_run_seed()!=seed);
    a.callbacks.seed_entropy=nullptr;a.run_number=0;CHECK(a.next_run_seed(&random)==0x20261003u&&!random);
}
static void statuses(){
    new_run(a.game,73,true,Character::Ironclad);a.title=a.character_select=a.paused=a.help=a.library=false;
    auto&s=a.game;s.strength=-3;s.dexterity=123;s.vigor=7;s.thorns=2;s.pain=3;s.embrace=4;s.no_draw=1;s.no_block=1;
    s.noxious_fumes=1;s.accuracy=2;s.demon_form=3;s.infinite_blades=1;s.flame_barrier=6;s.block_next_turn=9;s.draw_next_turn=2;
    s.enemies[0].strength=-2;s.enemies[0].weak=4;s.enemies[0].vulnerable=5;s.enemies[0].poison=123;
    a.reset_presentation();a.pointer.visible=false;a.render();snapshot();
    unsigned loading_steps=0;const unsigned frozen_animation=a.animation;
    while(a.combat_preparation.active&&loading_steps++<1024){a.advance(33);a.render();CHECK(a.animation==frozen_animation);}
    CHECK(!a.combat_preparation.active);unchanged();
    // Inspect actual localized glyph widths and dense icon wrapping, as well
    // as the existing synthetic/no-content layout and canonical state checks.
    a.advance(250);a.render();capture("player-turn-banner");unchanged();
    for(unsigned i=0;i<10;++i)a.advance(250);a.render();capture("dense-statuses");unchanged();
    for(int actor=0;actor<=s.enemy_count;++actor){
        int ids[StatusKinds],count=collect_statuses(s,actor,ids),found=0;
        for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::BattleStatus&&a.hits[i].value/StatusKinds==actor)++found;
        CHECK(found==count);
        for(int i=0;i<count;++i){const int encoded=actor*StatusKinds+ids[i];const HitRegion*hit=nullptr;
            for(int h=0;h<a.hit_count;++h)if(a.hits[h].kind==HitKind::BattleStatus&&a.hits[h].value==encoded)hit=&a.hits[h];
            CHECK(hit&&hit->enabled&&hit->x>=0&&hit->y>=24&&hit->x+hit->w<=320&&hit->y+hit->h<=190);
        }
    }
    CHECK(status_amount(s,0,0)==-3&&status_amount(s,0,1)==123&&status_amount(s,1,6)==123);
    a.open_status_icon(StatusKinds+6);CHECK(a.status_actor==1);
    int ids[StatusKinds];collect_statuses(s,1,ids);CHECK(ids[a.status_selected]==6);unchanged();
    a.input(Key::Back);CHECK(a.status_actor<0);a.render();unchanged();
    for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::BattleStatus){
        const HitRegion h=a.hits[i];a.pointer_move(h.x+1,h.y+1);a.render();capture("status-tooltip");unchanged();break;
    }
}
int main(int argc,char**argv){
    CHECK(argc<=3);seeds();std::vector<uint8_t>data;
    if(argc>1){
        FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);
        CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));data.resize(size);
        CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));CHECK(a.init(data.data(),data.size(),{}));
    }
    if(argc>2)screenshot_directory=argv[2];statuses();
    std::printf("device feedback: %u checks passed; %s content\n",checks,argc>1?"actual localized":"fallback");
}
