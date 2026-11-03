#pragma once
#include <stdint.h>

namespace spire {
namespace reference {struct Random;}
// Historical NSHB/v18 compatibility vocabulary; no player-facing Timeline.
enum class EpochState:uint8_t {None,NoSlot,NotObtained,ObtainedNoSlot,Obtained,Revealed};
enum class Epoch:uint8_t {
#include "timeline_ids.inc"
 Count
};
constexpr unsigned EpochCount=unsigned(Epoch::Count),TimelineAgnosticCount=18;
struct TimelineUnlocks {
    // v18 save compatibility only. New runs never gate playable models by it.
    uint32_t words[2]{UINT32_MAX,0x01ffffffu};
    bool has(Epoch e)const{const unsigned n=unsigned(e);return n<EpochCount&&(words[n/32]&(uint32_t(1)<<(n%32)));}
    void set(Epoch e){const unsigned n=unsigned(e);if(n<EpochCount)words[n/32]|=uint32_t(1)<<(n%32);}
    bool valid()const{return !(words[1]&~0x01ffffffu);}
};
enum TimelineFlags:uint8_t {TimelineInitialized=1,TimelineLegacyAccess=2};
struct TimelineProgress {
    uint32_t obtained_at[EpochCount]{};
    uint32_t score=0;
    EpochState states[EpochCount]{};
    // Compatibility access does not assert any historic obtain/reveal date.
    uint8_t flags=0,ancients_seen=0,characters_known=0;
};
static_assert(EpochCount==57&&sizeof(TimelineProgress)==292,"Bounded source epoch progress");
// Historical progress has no runtime obtain/reveal or model-lock operations.
void timeline_initialize(TimelineProgress&,bool legacy_access=false);
bool timeline_valid(const TimelineProgress&);
uint8_t timeline_first_act(const TimelineUnlocks&,uint8_t discovered_acts,reference::Random&);
} // namespace spire
