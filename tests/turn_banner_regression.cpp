#include "turn_banner_presentation.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using spire::TurnBannerPresentation;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"turn banner:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    TurnBannerPresentation b;CHECK(b.sample(0).kind==b.None);
    b.begin_player(1,100);CHECK(b.sample(99).kind==b.None&&b.sample(100).alpha==0);
    auto f=b.sample(131);CHECK(f.kind==b.Player&&f.alpha==255&&f.offset>=7&&f.number==1&&!f.extra);
    CHECK(b.sample(158).alpha<255&&b.sample(167).kind==b.None);
    CHECK(b.sample_player(146).offset==10);
    // NPlayerTurnBanner's300ms exit uses SINE_OUT after the1.5s movement
    // and0.4s hold. Compare with the independent scalar source trajectory.
    for(unsigned tick=158;tick<167;++tick){const unsigned ms=(tick-100)*33;
        const double fraction=double(ms-1900)/300;
        const int expected=int(std::lround(255*(1-std::sin(fraction*1.5707963267948966))));
        CHECK(std::abs(int(b.sample_player(tick).alpha)-expected)<=1);
    }
    b.begin_enemy(200);CHECK(b.sample(200).scale==8192&&b.sample(200).alpha==0);
    b.begin_enemy(205);CHECK(b.enemy_start==200); // One banner for all enemy moves.
    f=b.sample(223);CHECK(f.kind==b.Enemy&&f.scale==4096&&f.alpha>200);
    f=b.sample(245);CHECK(f.kind==b.Enemy&&f.red>0&&f.alpha<255);
    b.begin_player(2,220);CHECK(b.player_start==220);CHECK(b.sample_enemy(269).kind==b.Enemy);
    f=b.sample_player(221);CHECK(f.kind==b.Player&&f.number==2); // Independent original scene instances overlap.
    CHECK(b.sample_enemy(221).kind==b.Enemy);
    CHECK(TurnBannerPresentation::EnemyLeadTicks+TurnBannerPresentation::IntentLeadTicks<TurnBannerPresentation::EnemyTicks);
    CHECK(TurnBannerPresentation::enemy_lead_ticks(false)==25&&TurnBannerPresentation::intent_lead_ticks(false)==13);
    CHECK(TurnBannerPresentation::enemy_lead_ticks(true)==16&&TurnBannerPresentation::intent_lead_ticks(true)==8);
    CHECK(TurnBannerPresentation::wait_ticks(350,175,true)==6&&TurnBannerPresentation::wait_ticks(600,275,true)==9);
    b.begin_enemy(400);b.begin_player(3,550);CHECK(b.sample(475).kind==b.None&&b.sample(551).kind==b.Player);
    b.begin_player(4,700,true);CHECK(b.sample(710).extra);
    b.begin_enemy(715);CHECK(b.sample_player(720).kind==b.Player&&b.sample_enemy(720).kind==b.Enemy);
    // Unsigned clocks may wrap once during a long foreground session.
    b.reset();b.begin_enemy(0xfffffff0u);b.begin_player(9,10);CHECK(b.player_start==10);
    CHECK(b.sample_enemy(9).kind==b.Enemy&&b.sample_player(11).kind==b.Player);
    CHECK(TurnBannerPresentation::outline_pixels(24)==5);
    CHECK(TurnBannerPresentation::outline_pixels(24,8192)==9);
    CHECK(TurnBannerPresentation::outline_pixels(16)==3);
    std::printf("turn banner: %u checks passed\n",checks);
}
