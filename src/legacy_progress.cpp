#include "legacy_progress.hpp"
namespace spire {
// Retained only to read/write old NSHB metadata without discarding history.
void legacy_progress_initialize(TimelineProgress&p,bool legacy){
    for(unsigned i=0;i<EpochCount;++i){p.states[i]=EpochState::None;p.obtained_at[i]=0;}
    p.score=0;p.ancients_seen=p.characters_known=0;
    p.flags=TimelineInitialized|(legacy?TimelineLegacyAccess:0);
}
bool legacy_progress_valid(const TimelineProgress&p){
    if(p.flags&~3u||p.characters_known&~31u)return false;
    for(unsigned i=0;i<EpochCount;++i)if(unsigned(p.states[i])>5||(unsigned(p.states[i])<3&&p.obtained_at[i]))return false;
    return true;
}
} // namespace spire
