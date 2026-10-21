#pragma once
#include <stdint.h>

namespace spire { namespace world_map_transition {

enum class Speed : uint8_t { Normal, Fast };

constexpr uint32_t fade_out_duration_ms(Speed speed){
    return speed==Speed::Fast?600u:1100u;
}
constexpr uint32_t fade_in_duration_ms(Speed speed){
    return speed==Speed::Fast?300u:800u;
}
// RoomFadeIn's Normal callback releases both gates before its visual fade
// finishes. Fast releases MouseFilter immediately but InTransition only at
// completion; NInputManager checks the latter for all mapped key/controller
// inputs. These delays are independent of the still-visible black overlay.
constexpr uint32_t fade_in_keyboard_delay_ms(Speed speed){
    return speed==Speed::Fast?300u:200u;
}
constexpr uint32_t fade_in_pointer_delay_ms(Speed speed){
    return speed==Speed::Fast?0u:200u;
}

namespace detail {
// Original scenes/game.tscn, Gradient_1kice / GradientTexture2D_5newe:
// width=1, height=128; fill_from=(.529915,1), fill_to=(.540598,.574786).
// Godot 4.5.1 projects each (0,row/127) onto the fill segment. Its two-stop
// cubic interpolation repeats each endpoint, then truncates non-HDR alpha
// to RGBA8. The constexpr table is evaluated from those equations; no
// external texture, floating-point runtime or dynamic initialization is used.
constexpr uint32_t GradientQ=1u<<18;
constexpr uint32_t multiply_gradient(uint32_t a,uint32_t b){
    return uint32_t((uint64_t(a)*b+GradientQ/2)/GradientQ);
}
constexpr uint8_t gradient_texel(unsigned row){
    if(row>=128)return 0;
    const int64_t numerator=int64_t(127-row)*425214*1000000-
        int64_t(529915)*10683*127;
    const int64_t denominator=(int64_t(10683)*10683+
        int64_t(425214)*425214)*127;
    if(numerator<=0)return 0;
    if(numerator>=denominator)return 255;
    const uint32_t t=uint32_t((numerator*GradientQ+denominator/2)/denominator);
    const uint32_t square=multiply_gradient(t,t);
    const uint32_t cube=multiply_gradient(square,t);
    const uint32_t twice_alpha=t+3*square-2*cube;
    return uint8_t(uint64_t(twice_alpha)*255/(2*GradientQ));
}
struct GradientSamples {
    uint8_t alpha[128]{};
    constexpr GradientSamples(){
        for(unsigned i=0;i<128;++i)alpha[i]=gradient_texel(i);
    }
};
constexpr GradientSamples Gradient{};
constexpr uint32_t SampleQ=1u<<16;

// Return alpha in byte units with 16 fractional bits. A virtual 1080-high
// viewport preserves the original parent top=-60 and gradient height=2400.
// Its local y moves -2564 -> 0. Calculator pixels sample at their centers;
// this vertical projection and final 8-bit coverage replace GPU rasterization.
// The 128 RGBA8 texels use clamp-to-edge linear filtering (v*128-.5).
inline uint32_t normal_gradient_alpha(uint32_t movement_ms,int y,int height){
    // The public caller validated y/height. Keep these divisions unsigned:
    // the freestanding ARM runtime supplies __aeabi_uldivmod, not ldivmod.
    const uint64_t screen_y=((uint64_t(unsigned(y))*2+1)*1080*(SampleQ/2))/unsigned(height);
    const uint64_t origin_y=screen_y+uint64_t(2624)*SampleQ;
    const uint64_t movement=(uint64_t(2564)*SampleQ*movement_ms+300)/600;
    if(movement>origin_y)return 0;
    const uint64_t local_y=origin_y-movement;
    if(local_y>=uint64_t(2400)*SampleQ)return 0;
    const uint64_t texel_center=local_y*128/2400;
    if(texel_center<=SampleQ/2)return uint32_t(Gradient.alpha[0])*SampleQ;
    const uint64_t sample=texel_center-SampleQ/2;
    if(sample>=uint64_t(127)*SampleQ)return uint32_t(Gradient.alpha[127])*SampleQ;
    const unsigned index=unsigned(sample/SampleQ);
    const unsigned fraction=unsigned(sample%SampleQ);
    return Gradient.alpha[index]*(SampleQ-fraction)+Gradient.alpha[index+1]*fraction;
}
}

// Elapsed time is already real presentation milliseconds. Callers own clock
// wrap, lifecycle and room submission. Invalid scanlines contribute no mask.
// NTransition.RoomFadeOut uses 500+600 ms linear movement/opacity in Normal,
// or 300+300 ms QUAD_OUT SimpleTransition opacity in Fast. The normal black
// gradient is drawn first and the full-screen black simple layer second:
// alpha = simple + gradient*(1-simple), rounded only after composition.
inline uint8_t fade_out_alpha(Speed speed,uint32_t elapsed_ms,int pixel_y,
                             int viewport_height=240){
    if(viewport_height<=0||pixel_y<0||pixel_y>=viewport_height)return 0;
    if(speed==Speed::Fast){
        if(elapsed_ms<=300)return 0;
        if(elapsed_ms>=600)return 255;
        const uint32_t t=elapsed_ms-300;
        return uint8_t((255u*t*(600u-t)+45000u)/90000u);
    }
    if(elapsed_ms<=500)return 0;
    if(elapsed_ms>=1100)return 255;
    const uint32_t t=elapsed_ms-500;
    const uint32_t gradient=detail::normal_gradient_alpha(t,pixel_y,viewport_height);
    const uint64_t numerator=uint64_t(255)*t*detail::SampleQ+
        uint64_t(600-t)*gradient;
    return uint8_t((numerator+300*detail::SampleQ)/(600*detail::SampleQ));
}

// RoomFadeIn removes the gradient immediately and fades SimpleTransition
// linearly: 800 ms Normal / 300 ms Fast. Input-unlock timing is owned by App.
constexpr uint8_t fade_in_alpha(Speed speed,uint32_t elapsed_ms){
    const uint32_t duration=fade_in_duration_ms(speed);
    return elapsed_ms>=duration?0:
        uint8_t((255*(duration-elapsed_ms)+duration/2)/duration);
}

}}
