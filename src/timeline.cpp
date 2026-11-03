#include "timeline.hpp"
#include "reference_rng.hpp"
namespace spire {
// Retained only to read/write old NSHB metadata without discarding history.
void timeline_initialize(TimelineProgress&p,bool legacy){
    for(unsigned i=0;i<EpochCount;++i){p.states[i]=EpochState::None;p.obtained_at[i]=0;}
    p.score=0;p.ancients_seen=p.characters_known=0;
    p.flags=TimelineInitialized|(legacy?TimelineLegacyAccess:0);
}
bool timeline_valid(const TimelineProgress&p){
    if(p.flags&~3u||p.characters_known&~31u)return false;
    for(unsigned i=0;i<EpochCount;++i)if(unsigned(p.states[i])>5||(unsigned(p.states[i])<3&&p.obtained_at[i]))return false;
    return true;
}
uint8_t timeline_first_act(const TimelineUnlocks&unlocks,uint8_t seen,reference::Random&rng){
    // Preserve the original first-act selection/discovery RNG order. New
    // runs use all regions, except the first tutorial run's Overgrowth.
    const bool underdocks=unlocks.has(Epoch::Underdocks);
    const uint8_t first=underdocks&&!(seen&2)?1:uint8_t(rng.below(underdocks?2:1));
    (void)rng.below(1);(void)rng.below(1);return first;
}
} // namespace spire
