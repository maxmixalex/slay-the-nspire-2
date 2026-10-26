#pragma once
#include <stdint.h>

namespace spire {
// A departure starts when its first frame reaches the LCD. Time spent
// creating a run or preparing that first frame precedes the visible tween.
// This only rebases the host's animation delta; diagnostic frame intervals
// continue measuring all input, resource, rendering and presentation work.
struct DepartureFrameClock {
    uint8_t phase=0;
    bool preparing=false;
    bool presented(uint8_t next_phase,bool next_preparing){
        const bool restart=next_phase&&(next_phase!=phase||
            (next_phase==2&&preparing&&!next_preparing));
        phase=next_phase;preparing=next_preparing;
        return restart;
    }
};
}
