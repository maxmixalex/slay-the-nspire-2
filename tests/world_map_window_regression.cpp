#include "world_map_window.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map window: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static int q(double value){return int(std::lround(value*4096.));}
static double ramp(unsigned elapsed,unsigned delay,unsigned duration){
    if(elapsed<=delay)return 0.;
    return elapsed>=delay+duration?1.:double(elapsed-delay)/duration;
}
// Independent continuous equations from the pinned NMapScreen property
// tweens. Q12 is only the calculator's storage/rounding adapter.
static void opening(uint32_t start){
    MapWindowState state;CHECK(!state.visible&&!state.is_open);
    CHECK(state.open(start));CHECK(!state.open(start+10));
    for(unsigned t=0;t<=500;++t){
        const auto s=state.sample(start+t);const double m=ramp(t,0,250),p=ramp(t,100,250),d=ramp(t,200,250);
        CHECK(s.visible&&state.is_open);CHECK(std::abs(int(s.map.rgb)-q(m))<=1&&s.map.alpha==s.map.rgb);
        CHECK(std::abs(int(s.points_alpha)-q(p))<=1);
        CHECK(s.legend.rgb==s.points_alpha&&s.legend.alpha==s.points_alpha);
        CHECK(std::abs(int(s.drawing_tools.rgb)-q(d))<=1&&s.drawing_tools.alpha==s.drawing_tools.rgb);
        CHECK(std::abs(int(s.backstop_alpha)-q(.85*m))<=1);
        CHECK(std::abs(int(world_map_window::backstop_coverage(s.backstop_alpha))-int(std::lround(.85098*.85*m*255.)))<=1);
        CHECK(s.map_y_q12==0);
        if(t<100)CHECK(s.legend_x_q12==0);
        else{
            const double u=1.-p;
            const double x=120.*u*u*(2.70158*u-1.70158);
            CHECK(std::abs(s.legend_x_q12-q(x))<=1);
        }
        state.update(start+t);
    }
    CHECK(!state.opening()&&!state.closing()&&state.visible&&state.is_open);
}
static void closing(uint32_t start,unsigned open_time){
    MapWindowState state;CHECK(state.open(start));state.update(start+open_time);
    const auto before=state.sample(start+open_time);CHECK(state.close(start+open_time));
    CHECK(!state.is_open&&state.visible&&state.closing());
    // Repeated Close, including Close(false), is an original IsOpen no-op.
    CHECK(!state.close(start+open_time+1,false));
    for(unsigned t=0;t<=400;++t){
        const auto s=state.sample(start+open_time+t);
        const double early=1.-ramp(t,0,150),late=1.-ramp(t,100,250);
        CHECK(s.visible==(t<350));
        CHECK(std::abs(int(s.backstop_alpha)-int(std::lround(before.backstop_alpha*early)))<=1);
        CHECK(std::abs(int(s.points_alpha)-int(std::lround(before.points_alpha*early)))<=1);
        CHECK(std::abs(int(s.map.rgb)-int(std::lround(before.map.rgb*late)))<=1&&s.map.alpha==s.map.rgb);
        CHECK(s.legend.rgb==before.legend.rgb&&std::abs(int(s.legend.alpha)-int(std::lround(before.legend.alpha*early)))<=1);
        const double u=1.-ramp(t,0,250),weight=1.-u*u*u;
        CHECK(std::abs(s.map_y_q12-int(std::lround(before.map_y_q12+200.*4096.*weight)))<=1);
        CHECK(std::abs(s.legend_x_q12-int(std::lround(before.legend_x_q12+(120.*4096.-before.legend_x_q12)*weight)))<=1);
    }
    state.update(start+open_time+350);CHECK(!state.visible&&!state.is_open&&!state.closing());
    CHECK(state.open(start+open_time+351));CHECK(state.sample(start+open_time+351).map.alpha==0);
    CHECK(state.close(start+open_time+360,false));CHECK(!state.visible&&!state.is_open);
    state.settle();CHECK(state.is_open&&state.visible&&state.sample(0).map.alpha==4096);
    state.reset();CHECK(!state.is_open&&!state.visible&&state.sample(0).map.alpha==0);
}
int main(){
    for(uint32_t start:{0u,12345u,0xffffff00u}){
        opening(start);
        for(unsigned elapsed:{0u,1u,99u,100u,125u,249u,250u,300u,350u,450u,10000u})closing(start,elapsed);
    }
    std::printf("world map window: %u independent tween checks passed\n",checks);
}
