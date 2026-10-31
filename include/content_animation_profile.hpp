#pragma once
#include <stdint.h>

namespace spire::content_animation_profile {
// NSP2/46 retired the integrity-only checksum at header byte 36. NAF1 is
// the optional reduced-pose presentation profile; all clip clocks remain
// unchanged. Older checksum-bearing formats must never select this mode.
constexpr uint32_t ReducedPoses=0x3146414eu;
constexpr unsigned MinimumVersion=46;
inline bool reduced(const uint8_t*header,unsigned version){
    return header&&version>=MinimumVersion&&
        (uint32_t(header[36])|(uint32_t(header[37])<<8)|
         (uint32_t(header[38])<<16)|(uint32_t(header[39])<<24))==ReducedPoses;
}
}
