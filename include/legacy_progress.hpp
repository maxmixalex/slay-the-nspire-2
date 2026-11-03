#pragma once
#include <stdint.h>

namespace spire {
// Historical NSHB/v18 compatibility vocabulary; no player-facing Timeline.
enum class EpochState:uint8_t {None,NoSlot,NotObtained,ObtainedNoSlot,Obtained,Revealed};
// Only indices used by migration are named. The remaining serialized slots
// stay opaque and round-trip unchanged; they have no model-unlock behavior.
enum class Epoch:uint8_t {Defect1=10,Necrobinder1=26,Neow=33,Regent1=37,Silent1=49,Count=57};
constexpr unsigned EpochCount=unsigned(Epoch::Count);
struct TimelineUnlocks {
    // v18 save compatibility only. New runs never gate playable models by it.
    uint32_t words[2]{UINT32_MAX,0x01ffffffu};
    bool has(Epoch e)const{const unsigned n=unsigned(e);return n<EpochCount&&(words[n/32]&(uint32_t(1)<<(n%32)));}
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
void legacy_progress_initialize(TimelineProgress&,bool legacy_access=false);
bool legacy_progress_valid(const TimelineProgress&);
} // namespace spire
