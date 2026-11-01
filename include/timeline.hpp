#pragma once
#include <stdint.h>

namespace spire {
// Source EpochState, not the unrelated four-value NEpochSlot visual state.
enum class EpochState:uint8_t {None,NoSlot,NotObtained,ObtainedNoSlot,Obtained,Revealed};
enum class Epoch:uint8_t {
#include "timeline_ids.inc"
 Count
};
constexpr unsigned EpochCount=unsigned(Epoch::Count),TimelineAgnosticCount=18;
struct TimelineUnlocks {
    // Default construction retains the historical all-open simulation API.
    // Normal new runs replace this with the current profile's revealed mask.
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
    uint8_t flags=0,ancients_seen=0,characters_finished=0;
};
static_assert(EpochCount==57&&sizeof(TimelineProgress)==292,"Bounded source epoch progress");
enum class TimelineKind:uint8_t {None,Card,Relic,Potion,Event};
struct TimelineEpoch {
    int16_t era;
    uint16_t models[3];
    Epoch parent;
    uint8_t position,story,kind,count;
};
struct TimelineAgnostic {Epoch epoch;uint16_t threshold;};
extern const TimelineEpoch timeline_epochs[EpochCount];
extern const TimelineAgnostic timeline_agnostic[TimelineAgnosticCount];
void timeline_initialize(TimelineProgress&,bool legacy_access=false);
bool timeline_valid(const TimelineProgress&);
bool timeline_obtain(TimelineProgress&,Epoch,uint32_t now);
bool timeline_revealable(const TimelineProgress&,Epoch);
bool timeline_reveal(TimelineProgress&,Epoch,uint32_t now);
TimelineUnlocks timeline_unlocks(const TimelineProgress&);
unsigned timeline_obtained_count(const TimelineProgress&,bool agnostic=false);
unsigned timeline_next_threshold(const TimelineProgress&);
// Original score bar grants at most one epoch per completed standard run.
Epoch timeline_add_score(TimelineProgress&,uint32_t score,uint32_t now);
bool timeline_model_allowed(const TimelineUnlocks&,TimelineKind,unsigned model);
bool timeline_character_allowed(const TimelineUnlocks&,unsigned character);
Epoch timeline_character_epoch(unsigned character,unsigned chapter);
} // namespace spire
