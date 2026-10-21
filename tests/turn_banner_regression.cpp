#include "turn_banner_presentation.hpp"
#include <cstdio>
#include <cstdlib>
using spire::TurnBannerPresentation;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"turn banner:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    TurnBannerPresentation b;CHECK(b.sample(0).kind==b.None);
    b.begin_player(1,100);CHECK(b.sample(99).kind==b.None&&b.sample(100).alpha==0);
    auto f=b.sample(131);CHECK(f.kind==b.Player&&f.alpha==255&&f.offset>=7&&f.number==1&&!f.extra);
    CHECK(b.sample(158).alpha<255&&b.sample(167).kind==b.None);
    b.begin_enemy(200);CHECK(b.sample(200).scale==8192&&b.sample(200).alpha==0);
    b.begin_enemy(205);CHECK(b.enemy_start==200); // One banner for all enemy moves.
    f=b.sample(223);CHECK(f.kind==b.Enemy&&f.scale==4096&&f.alpha>200);
    f=b.sample(245);CHECK(f.kind==b.Enemy&&f.red>0&&f.alpha<255);
    b.begin_player(2,220);CHECK(b.player_start==270);CHECK(b.sample(269).kind==b.Enemy);
    f=b.sample(271);CHECK(f.kind==b.Player&&f.number==2);
    b.begin_enemy(400);b.begin_player(3,550);CHECK(b.sample(475).kind==b.None&&b.sample(551).kind==b.Player);
    b.begin_player(4,700,true);CHECK(b.sample(710).extra);
    // Unsigned clocks may wrap once during a long foreground session.
    b.reset();b.begin_enemy(0xfffffff0u);b.begin_player(9,10);CHECK(b.player_start==54);
    CHECK(b.sample(9).kind==b.Enemy&&b.sample(55).kind==b.Player);
    std::printf("turn banner: %u checks passed\n",checks);
}
