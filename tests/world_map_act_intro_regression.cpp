#include "world_map_act_intro.hpp"
#include "world_map_window.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

using namespace spire;
static unsigned checks;
static int maximum_camera_error;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map act intro: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static int q(double value){return int(std::lround(value*4096.));}
static double ramp(unsigned t,unsigned start,unsigned duration){return t<=start?0.:t-start>=duration?1.:double(t-start)/duration;}
// Independent continuous Godot 4.5.1 EXPO_IN_OUT equation, including its
// unequal .0005 / 1.0005 endpoint correction. Do not use the production table.
static double expo(unsigned t,unsigned duration){
    if(!t)return 0.;if(t>=duration)return 1.;
    const double u=2.*t/duration;
    return u<1.?std::pow(2.,10.*(u-1.))/2.-.0005:
        1.0005*(2.-std::pow(2.,-10.*(u-1.)))/2.;
}
static void equations(){
    for(unsigned duration:{1500u,3000u})for(int from:{-600,0,200,2325,2400})for(unsigned t=0;t<=duration;++t){
        const int expected=q(from*(1.-expo(t,duration)));
        const int actual=world_map_act_intro::camera_y(q(from),t,duration);
        const int error=std::abs(expected-actual);if(error>maximum_camera_error)maximum_camera_error=error;
        CHECK(error<=512); // <= 0.125 original source pixel, before projection.
    }
    CHECK(world_map_act_intro::camera_y(q(2400),1,3000)>q(2400));
    CHECK(world_map_act_intro::camera_y(q(2400),2999,3000)<0);
    for(bool fast:{false,true})for(unsigned t=0;t<=4800;++t){
        MapActBanner banner{t,0,0,fast};const auto s=banner.sample();
        const unsigned fade=fast?2250u:3750u,end=fade+1000;
        const double root=t<=fade?1.:t>=end?0.:std::pow((end-t)/1000.,2.);
        CHECK(std::abs(int(s.background_alpha)-q(.25*ramp(t,500,500)*root))<=1);
        CHECK(std::abs(int(s.name_alpha)-q(ramp(t,250,1000)*root))<=1);
        CHECK(std::abs(int(s.number_alpha)-q(ramp(t,500,1000)*root))<=1);
        const double y=t<500||t>=1750?0.:10.*std::pow((1750-t)/1250.,2.);
        CHECK(std::abs(s.number_y_q12-q(y))<=1);
    }
}
static void camera_lifecycle(bool fast,uint32_t now){
    MapActIntroState s;s.reset(fast);CHECK(s.delay()==(fast?500u:1000u));
    const unsigned hold=s.delay(),quarter=s.interrupt_at();s.open_intro(true,1,2,false);
    CHECK(s.played&&s.camera_running&&s.banner_count==1&&s.y_q12==q(2400));
    CHECK(!s.can_scroll()&&!s.cancel(now));
    s.update(hold-1,now+hold-1,true);CHECK(s.camera_elapsed==hold-1&&!s.property_started&&s.y_q12==q(2400));
    s.snap(q(200)); // Close/reopen can write position during the hold.
    s.update(1,now+hold,true);CHECK(s.property_started&&s.camera_from_q12==q(200)&&s.y_q12==q(200));
    s.update(quarter-hold-1,now+quarter-1,true);CHECK(!s.interruptible&&!s.can_scroll());
    s.update(1,now+quarter,true);CHECK(s.interruptible&&s.can_scroll());
    const unsigned elapsed=s.camera_elapsed,banner_elapsed=s.banners[0].elapsed;
    const int y=s.y_q12;s.update(1000,now+quarter+1000,false);
    CHECK(s.camera_elapsed==elapsed&&s.banners[0].elapsed==banner_elapsed&&s.y_q12==y);
    s.install_map();CHECK(!s.played&&s.interruptible&&s.camera_running&&s.banner_count==1);
    CHECK(s.cancel(now+quarter+1000)&&!s.camera_running&&!s.interruptible&&!s.can_scroll());
    CHECK(s.target_q12==0&&s.y_q12==y&&s.banner_count==1);
    s.update(199,now+quarter+1199,false);CHECK(s.input_guard&&!s.can_scroll());
    s.update(1,now+quarter+1200,false);CHECK(!s.input_guard&&s.can_scroll());
    s.update(33,now+quarter+1233,true);CHECK(std::abs(s.y_q12)<std::abs(y)&&s.banners[0].elapsed==banner_elapsed+33);
    s.open_intro(true,1,2,true);CHECK(s.camera_running&&s.delay()==hold&&s.banner_count==2);
    s.update(s.camera_duration(),now+10000,true);CHECK(!s.camera_running&&s.interruptible&&s.y_q12==0);
    s.install_map();s.open_intro(true,2,3,false);CHECK(s.can_scroll()); // original interruptible latch survives SetMap.
    CHECK(s.cancel(now+10001));
    s.reset(!fast);CHECK(!s.played&&!s.camera_running&&!s.banner_count&&!s.input_guard&&!s.position_valid);
}
static void hidden_and_duplicate_banners(){
    MapActIntroState s;s.open_intro(true,1,2,false);CHECK(s.add_banner(1,2,false));
    s.update(2000,2000,true);CHECK(s.banner_count==2&&s.banners[0].elapsed==2000&&s.banners[1].elapsed==2000);
    CHECK(s.cancel(2000));const auto one=s.banners[0].sample();CHECK(one.background_alpha==1024&&one.name_alpha==4096);
    s.update(200,2200,false);CHECK(s.banner_count==2&&s.banners[0].elapsed==2000&&!s.input_guard);
    s.open_intro(false,2,3,true);CHECK(s.banner_count==3&&!s.camera_running&&s.y_q12==0);
    s.update(2750,4950,true);CHECK(s.banner_count==1&&s.banners[0].act==2&&s.banners[0].elapsed==2750);
    s.update(499,5449,true);CHECK(s.banner_count==1);s.update(1,5450,true);CHECK(!s.banner_count);
    for(unsigned i=0;i<MapActIntroState::MaxBanners;++i)CHECK(s.add_banner(i%3,i%4,i%2));
    CHECK(!s.add_banner(0,0,false)&&s.banner_count==MapActIntroState::MaxBanners);
    s.update(0xffffffffu,0xffffffffu,true);CHECK(!s.banner_count); // clamp before elapsed addition.
    s.reset();s.open_intro(true,0,0,false);s.update(1800,1800,true);
    const auto elapsed=s.camera_elapsed;s.open_intro(false,0,0,true);
    CHECK(s.camera_running&&s.camera_elapsed==elapsed&&s.y_q12==0); // skip branch does not kill an old tween.
    s.update(1,1801,true);CHECK(s.y_q12>0);
}
static void window_position(){
    MapWindowState w;CHECK(w.open(100));w.update(600);
    CHECK(w.close(600,true,q(1800)));CHECK(w.sample(600).map_y_q12==q(1800));
    CHECK(w.sample(850).map_y_q12==q(2000));CHECK(w.sample(949).visible);w.update(950);CHECK(!w.visible);
    CHECK(w.open(1000)&&w.sample(1000).map_y_q12==0);
    CHECK(w.close(1000,false,q(-100)));CHECK(!w.visible);
}
int main(){
    equations();for(bool fast:{false,true})for(uint32_t start:{0u,12345u,0xffffff00u})camera_lifecycle(fast,start);
    hidden_and_duplicate_banners();window_position();
    std::printf("world map act intro: %u independent checks passed; max camera error %d/4096 source pixel\n",checks,maximum_camera_error);
}
