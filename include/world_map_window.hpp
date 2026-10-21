#pragma once
#include <stdint.h>

namespace spire {

struct MapWindowLayer {uint16_t rgb=0,alpha=0;};
struct MapWindowSample {
    bool visible=false;
    MapWindowLayer map{},legend{},drawing_tools{};
    uint16_t points_alpha=0,backstop_alpha=0;
    int32_t map_y_q12=0,legend_x_q12=0;
};

namespace world_map_window {
constexpr unsigned One=4096;
constexpr unsigned BackstopTarget=(One*85+50)/100;
constexpr uint32_t OpenDuration=450,CloseDuration=350;
constexpr uint8_t to_byte(unsigned q12){
    return uint8_t(q12>=One?255:(q12*255+One/2)/One);
}
// The authored black ColorRect has color.a=.85098 in addition to its
// modulate.a tween. Quantize their product only at the final byte boundary.
constexpr uint8_t backstop_coverage(unsigned q12){
    return uint8_t((uint64_t(q12>One?One:q12)*85098*255+uint64_t(One)*50000)/
                   (uint64_t(One)*100000));
}

namespace detail {
constexpr uint16_t linear(unsigned from,unsigned to,uint32_t elapsed,uint32_t duration){
    return elapsed>=duration?uint16_t(to):
        uint16_t((from*(duration-elapsed)+to*elapsed+duration/2)/duration);
}
constexpr uint16_t delayed_linear(unsigned from,unsigned to,uint32_t elapsed,
                                  uint32_t delay,uint32_t duration){
    return elapsed<delay?uint16_t(from):linear(from,to,elapsed-delay,duration);
}
constexpr int32_t cubic_out(int32_t from,int32_t to,uint32_t elapsed){
    if(elapsed>=250)return to;
    const uint32_t remaining=250-elapsed;
    const uint64_t denominator=uint64_t(250)*250*250;
    const uint64_t weight=denominator-uint64_t(remaining)*remaining*remaining;
    const int32_t distance=to-from;
    const uint32_t magnitude=distance<0?uint32_t(-int64_t(distance)):uint32_t(distance);
    const int32_t movement=int32_t((uint64_t(magnitude)*weight+denominator/2)/denominator);
    return distance<0?from-movement:from+movement;
}
// Godot 4.5.1 BACK_OUT with s=1.70158. For a 120 -> 0 translation,
// u=1-t gives 120*u^2*((s+1)*u-s), including the negative overshoot.
// All division operands are unsigned; Ndless supplies __aeabi_uldivmod.
constexpr int32_t legend_open_x(uint32_t elapsed){
    if(elapsed>=250)return 0;
    const uint32_t remaining=250-elapsed;
    const int32_t factor=int32_t(270158*remaining)-170158*250;
    const uint32_t magnitude=factor<0?uint32_t(-factor):uint32_t(factor);
    const uint64_t numerator=uint64_t(120)*One*remaining*remaining*magnitude;
    const uint64_t denominator=uint64_t(100000)*250*250*250;
    const int32_t result=int32_t((numerator+denominator/2)/denominator);
    return factor<0?-result:result;
}
}
}

// Ordinary NMapScreen Open/Close only. Act introduction, scroll targeting,
// marker placement, capstone visibility and interaction belong to the App.
// Colors use Q12 modulation, and translations use Q12 source pixels. Parent
// modulation must be applied per drawn child, not to a flattened framebuffer.
// Time is the unscaled wrapping host millisecond clock, independent of Fast.
struct MapWindowState {
    bool is_open=false,visible=false;
    uint32_t start=0;

    bool opening()const{return mode_==Opening;}
    bool closing()const{return mode_==Closing;}
    void reset(){*this=MapWindowState{};}
    void settle(bool opened=true){
        reset();if(!opened)return;
        is_open=visible=true;mode_=Steady;
        from_.visible=true;from_.map=from_.legend=from_.drawing_tools={4096,4096};
        from_.points_alpha=4096;from_.backstop_alpha=world_map_window::BackstopTarget;
    }
    bool open(uint32_t now){
        if(is_open)return false;
        const int32_t old_legend_x=sample(now).legend_x_q12;
        from_={};from_.legend_x_q12=old_legend_x;
        start=now;is_open=visible=true;mode_=Opening;
        return true;
    }
    bool close(uint32_t now,bool animate=true){
        if(!is_open)return false;
        from_=sample(now);start=now;is_open=false;visible=animate;
        mode_=animate?Closing:Hidden;from_.visible=animate;
        return true;
    }
    void update(uint32_t now){
        const uint32_t elapsed=uint32_t(now-start);
        if(mode_==Opening&&elapsed>=world_map_window::OpenDuration){
            from_=sample(now);mode_=Steady;
        }else if(mode_==Closing&&elapsed>=world_map_window::CloseDuration){
            from_=sample(now);mode_=Hidden;visible=false;
        }
    }
    MapWindowSample sample(uint32_t now)const{
        using namespace world_map_window;
        if(mode_==Steady||mode_==Hidden){MapWindowSample out=from_;out.visible=visible;return out;}
        const uint32_t elapsed=uint32_t(now-start);
        MapWindowSample out{};
        if(mode_==Opening){
            out.visible=true;
            const uint16_t map=detail::linear(0,One,elapsed,250);
            const uint16_t points=detail::delayed_linear(0,One,elapsed,100,250);
            const uint16_t drawing=detail::delayed_linear(0,One,elapsed,200,250);
            out.map={map,map};out.legend={points,points};out.drawing_tools={drawing,drawing};
            out.points_alpha=points;
            // Keep .85 exact until Q12 rounding rather than interpolating a
            // previously rounded terminal value at every intermediate sample.
            out.backstop_alpha=elapsed>=250?BackstopTarget:
                uint16_t((One*85*elapsed+12500)/25000);
            // Godot From stores a tween initial value without writing it.
            // Before its delay elapses the invisible legend keeps its old x.
            out.legend_x_q12=elapsed<100?from_.legend_x_q12:detail::legend_open_x(elapsed-100);
            return out;
        }
        out=from_;out.visible=elapsed<CloseDuration;
        out.backstop_alpha=detail::linear(from_.backstop_alpha,0,elapsed,150);
        out.points_alpha=detail::linear(from_.points_alpha,0,elapsed,150);
        out.legend.alpha=detail::linear(from_.legend.alpha,0,elapsed,150);
        out.drawing_tools.alpha=detail::linear(from_.drawing_tools.alpha,0,elapsed,150);
        out.map.rgb=detail::delayed_linear(from_.map.rgb,0,elapsed,100,250);
        out.map.alpha=detail::delayed_linear(from_.map.alpha,0,elapsed,100,250);
        out.map_y_q12=detail::cubic_out(from_.map_y_q12,from_.map_y_q12+200*int32_t(One),elapsed);
        out.legend_x_q12=detail::cubic_out(from_.legend_x_q12,120*int32_t(One),elapsed);
        return out;
    }

private:
    enum Mode : uint8_t {Hidden,Opening,Steady,Closing};
    Mode mode_=Hidden;
    MapWindowSample from_{};
};

}
