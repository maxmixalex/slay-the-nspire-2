#include "timeline.hpp"
#include "reference_rng.hpp"
#include "core.hpp"
namespace spire {
#include "timeline_data.inc"
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
bool timeline_obtain(TimelineProgress&p,Epoch e,uint32_t now){
    const unsigned n=unsigned(e);if(n>=EpochCount||unsigned(p.states[n])>=3)return false;
    p.states[n]=p.states[n]==EpochState::None?EpochState::ObtainedNoSlot:EpochState::Obtained;
    p.obtained_at[n]=now;return true;
}
bool timeline_revealable(const TimelineProgress&p,Epoch e){
    unsigned n=unsigned(e);if(n>=EpochCount||unsigned(p.states[n])<3||p.states[n]==EpochState::Revealed)return false;
    // Every source expansion has one parent. The source queue starts at Neow
    // and extant slots, traversing obtained ancestors even before reveal.
    for(unsigned depth=0;depth<EpochCount;++depth){
        if(n==unsigned(Epoch::Neow)||p.states[n]==EpochState::Obtained||p.states[n]==EpochState::Revealed||p.states[n]==EpochState::NoSlot)return true;
        n=unsigned(timeline_epochs[n].parent);
        if(n>=EpochCount||unsigned(p.states[n])<3)return false;
    }return false;
}
bool timeline_reveal(TimelineProgress&p,Epoch e,uint32_t now){
    if(!timeline_revealable(p,e))return false;
    p.states[unsigned(e)]=EpochState::Revealed;
    if(e==Epoch::Neow)timeline_obtain(p,Epoch::Silent1,now);
    for(unsigned i=0;i<EpochCount;++i)if(timeline_epochs[i].parent==e){
        if(p.states[i]==EpochState::ObtainedNoSlot)p.states[i]=EpochState::Obtained;
        else if(unsigned(p.states[i])<3)p.states[i]=EpochState::NotObtained;
    }return true;
}
TimelineUnlocks timeline_unlocks(const TimelineProgress&p){
    TimelineUnlocks result;
    if(!(p.flags&TimelineInitialized)||(p.flags&TimelineLegacyAccess))return result;
    result.words[0]=result.words[1]=0;
    for(unsigned i=0;i<EpochCount;++i)if(p.states[i]==EpochState::Revealed)result.set(Epoch(i));
    return result;
}
unsigned timeline_obtained_count(const TimelineProgress&p,bool agnostic){
    unsigned n=0;
    if(agnostic){for(const auto&r:timeline_agnostic)n+=unsigned(p.states[unsigned(r.epoch)])>=3;}
    else for(auto state:p.states)n+=unsigned(state)>=3;
    return n;
}
unsigned timeline_next_threshold(const TimelineProgress&p){
    const unsigned n=timeline_obtained_count(p,true);return n<TimelineAgnosticCount?timeline_agnostic[n].threshold:0;
}
Epoch timeline_add_score(TimelineProgress&p,uint32_t score,uint32_t now){
    const unsigned threshold=timeline_next_threshold(p);if(!threshold)return Epoch::Count;
    const uint32_t total=score>UINT32_MAX-p.score?UINT32_MAX:score+p.score;
    if(total<threshold){p.score=total;return Epoch::Count;}
    Epoch next=Epoch::Count;
    for(const auto&r:timeline_agnostic)if(unsigned(p.states[unsigned(r.epoch)])<3){next=r.epoch;break;}
    timeline_obtain(p,next,now);const unsigned cap=timeline_next_threshold(p);
    p.score=cap?(total-threshold<cap?total-threshold:cap-1):0;return next;
}
bool timeline_model_allowed(const TimelineUnlocks&u,TimelineKind kind,unsigned model){
    unsigned epoch=EpochCount;
    switch(kind){
    case TimelineKind::Card:if(model<CardKinds)epoch=timeline_card_epochs.values[model];break;
    case TimelineKind::Relic:if(model<WorldRelicKinds)epoch=timeline_relic_epochs.values[model];break;
    case TimelineKind::Potion:if(model<=PotionKinds)epoch=timeline_potion_epochs.values[model];break;
    case TimelineKind::Event:if(model<OriginalEventKinds+1)epoch=timeline_event_epochs.values[model];break;
    default:break;
    }return epoch==EpochCount||u.has(Epoch(epoch));
}
Epoch timeline_character_epoch(unsigned character,unsigned chapter){
    static constexpr uint8_t first[]={unsigned(Epoch::Ironclad2)-1,unsigned(Epoch::Silent1),unsigned(Epoch::Defect1),unsigned(Epoch::Regent1),unsigned(Epoch::Necrobinder1)};
    if(character>=5||chapter<1||chapter>7||(character==0&&chapter==1))return Epoch::Count;
    return Epoch(first[character]+chapter-1);
}
bool timeline_character_allowed(const TimelineUnlocks&u,unsigned character){
    return character<5&&(character==0||u.has(timeline_character_epoch(character,1)));
}
uint8_t timeline_first_act(const TimelineUnlocks&unlocks,uint8_t seen,reference::Random&rng){
    // ActModel.GetRandomList forces an unlocked, undiscovered non-default
    // act without drawing. The two other pinned act buckets have one item.
    const bool underdocks=unlocks.has(Epoch::Underdocks);
    const uint8_t first=underdocks&&!(seen&2)?1:uint8_t(rng.below(underdocks?2:1));
    (void)rng.below(1);(void)rng.below(1);return first;
}

} // namespace spire
