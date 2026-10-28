#include "world_map_transition.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

static unsigned checks=0;
static void check(bool condition){
    ++checks;
    if(!condition){std::fprintf(stderr,"FAIL map transition check %u\n",checks);std::exit(1);}
}

// Independent binary32 reference for Godot 4.5.1 GradientTexture2D generation:
// closest point projection, signed length ratio, repeated-endpoint cubic,
// and the non-HDR uint8 truncation. Runtime uses constexpr integer arithmetic.
static unsigned reference_texel(unsigned row){
    const float from_x=.529915f,from_y=1.f,to_x=.540598f,to_y=.574786f;
    const float x=0.f,y=float(row)/127.f,dx=to_x-from_x,dy=to_y-from_y;
    const float projection=(dx*(x-from_x)+dy*(y-from_y))/(dx*dx+dy*dy);
    const float closest_x=from_x+dx*projection,closest_y=from_y+dy*projection;
    const float a=closest_x-from_x,b=closest_y-from_y;
    float t=std::sqrt(a*a+b*b)/std::sqrt(dx*dx+dy*dy);
    if(a*dx+b*dy<0.f)t=-t;
    if(t<0.f)t=0.f;
    if(t>1.f)t=1.f;
    const float alpha=.5f*(t+3.f*(t*t)-2.f*(t*t*t));
    const double byte=double(alpha)*255.;
    return byte<=0.?0u:byte>=255.?255u:unsigned(byte);
}
static double reference_gradient(double progress,int y,int height){
    static unsigned texels[128]{};
    static bool initialized=false;
    if(!initialized){for(unsigned i=0;i<128;++i)texels[i]=reference_texel(i);initialized=true;}
    const double global_top=-60.+(-2564.+2564.*progress);
    const double screen_center=(double(y)+.5)*1080./height;
    const double v=(screen_center-global_top)/2400.;
    if(v<0.||v>=1.)return 0.;
    const double sample=v*128.-.5;
    if(sample<=0.)return texels[0]/255.;
    if(sample>=127.)return texels[127]/255.;
    const unsigned index=unsigned(std::floor(sample));
    const double fraction=sample-index;
    return (texels[index]*(1.-fraction)+texels[index+1]*fraction)/255.;
}
static unsigned rounded_byte(double value){
    if(value<=0.)return 0;
    if(value>=1.)return 255;
    return unsigned(std::floor(value*255.+.5));
}
static unsigned reference_out(spire::world_map_transition::Speed speed,unsigned elapsed,int y,int height){
    using spire::world_map_transition::Speed;
    if(speed==Speed::Fast){
        if(elapsed<=300)return 0;
        if(elapsed>=600)return 255;
        const double p=(elapsed-300)/300.;
        return rounded_byte(1.-(1.-p)*(1.-p));
    }
    if(elapsed<=500)return 0;
    if(elapsed>=1100)return 255;
    const double p=(elapsed-500)/600.;
    return rounded_byte(p+reference_gradient(p,y,height)*(1.-p));
}

int main(){
    using namespace spire::world_map_transition;
    check(fade_out_duration_ms(Speed::Normal)==1100);
    check(fade_out_duration_ms(Speed::Fast)==600);
    check(fade_in_duration_ms(Speed::Normal)==800);
    check(fade_in_duration_ms(Speed::Fast)==300);
    check(fade_in_keyboard_delay_ms(Speed::Normal)==200);
    check(fade_in_keyboard_delay_ms(Speed::Fast)==300);
    check(fade_in_pointer_delay_ms(Speed::Normal)==200);
    check(fade_in_pointer_delay_ms(Speed::Fast)==0);
    // An App caller must use each input gate's own deadline, not the full
    // opacity duration. Mock both input channels at the original boundaries.
    for(unsigned elapsed:{0u,199u,200u,299u,300u,799u,800u}){
        check((elapsed<fade_in_keyboard_delay_ms(Speed::Normal))==(elapsed<200));
        check((elapsed<fade_in_pointer_delay_ms(Speed::Normal))==(elapsed<200));
        check((elapsed<fade_in_keyboard_delay_ms(Speed::Fast))==(elapsed<300));
        check(!(elapsed<fade_in_pointer_delay_ms(Speed::Fast)));
    }
    check(fade_in_alpha(Speed::Normal,200)>0);
    check(fade_in_alpha(Speed::Fast,0)==255);
    for(unsigned i=0;i<128;++i){
        check(detail::Gradient.alpha[i]==reference_texel(i));
        if(i)check(detail::Gradient.alpha[i]<=detail::Gradient.alpha[i-1]);
    }
    check(detail::Gradient.alpha[0]==255);
    check(detail::Gradient.alpha[127]==0);
    // Every millisecond and every scanline at calculator, original and small
    // boundary sizes. This checks the moving rectangle's actual pixel-center
    // sampling and both layers; fixed-point coverage may differ by at most one
    // final alpha level at a rounding threshold from the independent oracle.
    unsigned maximum_error=0,rounded_differences=0;
    for(int height:{1,2,239,240,1080})for(int y=0;y<height;++y){
        unsigned previous=0;
        for(unsigned t=0;t<=1100;++t){
            const unsigned actual=fade_out_alpha(Speed::Normal,t,y,height);
            const unsigned expected=reference_out(Speed::Normal,t,y,height);
            const unsigned error=actual>expected?actual-expected:expected-actual;
            if(error>maximum_error)maximum_error=error;
            if(error)++rounded_differences;
            check(error<=1);
            check(actual>=previous);
            previous=actual;
        }
        for(unsigned t:{0u,499u,500u})check(fade_out_alpha(Speed::Normal,t,y,height)==0);
        for(unsigned t:{1100u,1101u,std::numeric_limits<uint32_t>::max()})
            check(fade_out_alpha(Speed::Normal,t,y,height)==255);
    }
    // At the midpoint the source gradient darkens the top before the bottom;
    // a uniform screen fade or dropping the simple layer cannot satisfy this.
    check(fade_out_alpha(Speed::Normal,800,0)>fade_out_alpha(Speed::Normal,800,239));
    check(fade_out_alpha(Speed::Normal,800,239)>=127);
    for(unsigned t=0;t<=700;++t){
        check(fade_out_alpha(Speed::Fast,t,0)==reference_out(Speed::Fast,t,0,240));
        check(fade_out_alpha(Speed::Fast,t,0)==fade_out_alpha(Speed::Fast,t,239));
    }
    for(Speed speed:{Speed::Normal,Speed::Fast}){
        unsigned previous=255;
        const unsigned duration=fade_in_duration_ms(speed);
        for(unsigned t=0;t<=duration+1;++t){
            const unsigned actual=fade_in_alpha(speed,t);
            const unsigned expected=t>=duration?0:rounded_byte(1.-double(t)/duration);
            const unsigned error=actual>expected?actual-expected:expected-actual;
            check(error<=1);
            check(actual<=previous);previous=actual;
        }
        check(fade_in_alpha(speed,0)==255);
        check(fade_in_alpha(speed,duration)==0);
        check(fade_in_alpha(speed,std::numeric_limits<uint32_t>::max())==0);
        check(fade_out_alpha(speed,0,-1)==0);
        check(fade_out_alpha(speed,1100,240)==0);
        check(fade_out_alpha(speed,1100,0,0)==0);
        check(fade_out_alpha(speed,1100,0,-1)==0);
        // Valid extreme dimensions must not overflow before their division.
        const int height=std::numeric_limits<int>::max();
        for(int y:{0,height/2,height-1})for(unsigned t:{501u,750u,1099u}){
            const unsigned actual=fade_out_alpha(speed,t,y,height);
            const unsigned expected=reference_out(speed,t,y,height);
            const unsigned error=actual>expected?actual-expected:expected-actual;
            check(error<=1);
        }
    }
    std::printf("%u map transition checks passed; maximum alpha error=%u, rounded differences=%u\n",
        checks,maximum_error,rounded_differences);
}
