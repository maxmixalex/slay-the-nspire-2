#include "timeline.hpp"
#include "core.hpp"
#include <cstdio>
#include <cstdlib>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Timeline line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
int main(){
 TimelineProgress p;timeline_initialize(p);CHECK(timeline_valid(p));
 auto start=timeline_unlocks(p);CHECK(timeline_character_allowed(start,0));
 for(unsigned i=1;i<5;++i)CHECK(!timeline_character_allowed(start,i));
 CHECK(!timeline_model_allowed(start,TimelineKind::Card,MoltenFist));
 CHECK(timeline_model_allowed(start,TimelineKind::Card,Strike));
 CHECK(timeline_obtain(p,Epoch::Ironclad2,101));CHECK(p.states[unsigned(Epoch::Ironclad2)]==EpochState::ObtainedNoSlot);
 CHECK(!timeline_revealable(p,Epoch::Ironclad2));CHECK(!timeline_reveal(p,Epoch::Ironclad2,102));
 CHECK(timeline_obtain(p,Epoch::Neow,103));CHECK(timeline_revealable(p,Epoch::Ironclad2));
 CHECK(timeline_reveal(p,Epoch::Neow,104));CHECK(p.states[unsigned(Epoch::Silent1)]==EpochState::Obtained);
 CHECK(p.obtained_at[unsigned(Epoch::Silent1)]==104);
 CHECK(!timeline_model_allowed(timeline_unlocks(p),TimelineKind::Card,MoltenFist));
 CHECK(timeline_reveal(p,Epoch::Ironclad2,105));CHECK(p.obtained_at[unsigned(Epoch::Ironclad2)]==101);
 CHECK(timeline_model_allowed(timeline_unlocks(p),TimelineKind::Card,MoltenFist));
 CHECK(!timeline_model_allowed(start,TimelineKind::Card,MoltenFist)); // frozen run stays frozen
 CHECK(!timeline_obtain(p,Epoch::Ironclad2,999));CHECK(p.obtained_at[unsigned(Epoch::Ironclad2)]==101);
 CHECK(timeline_reveal(p,Epoch::Silent1,106));CHECK(timeline_character_allowed(timeline_unlocks(p),1));
 CHECK(timeline_add_score(p,199,107)==Epoch::Count&&p.score==199);
 CHECK(timeline_add_score(p,10000,108)==Epoch::Colorless1&&p.score==499);
 CHECK(timeline_obtained_count(p,true)==1);CHECK(!timeline_model_allowed(timeline_unlocks(p),TimelineKind::Card,Automation));
 CHECK(timeline_add_score(p,1,109)==Epoch::Relic1&&p.score==0);
 CHECK(timeline_obtained_count(p,true)==2);
 for(unsigned i=2;i<TimelineAgnosticCount;++i){CHECK(timeline_add_score(p,UINT32_MAX,110+i)==timeline_agnostic[i].epoch);CHECK(timeline_obtained_count(p,true)==i+1);}
 CHECK(p.score==0);CHECK(timeline_add_score(p,100,200)==Epoch::Count);
 TimelineProgress legacy;timeline_initialize(legacy,true);CHECK(timeline_obtained_count(legacy)==0&&legacy.score==0);
 for(unsigned i=0;i<EpochCount;++i){CHECK(legacy.obtained_at[i]==0);CHECK(timeline_unlocks(legacy).has(Epoch(i)));}
 // All 57 graph nodes are obtainable/revealable through the complete source graph.
 TimelineProgress all;timeline_initialize(all);
 for(unsigned i=0;i<EpochCount;++i)timeline_obtain(all,Epoch(i),300+i);
 for(unsigned pass=0;pass<EpochCount;++pass)for(unsigned i=0;i<EpochCount;++i)if(timeline_revealable(all,Epoch(i)))timeline_reveal(all,Epoch(i),400);
 for(unsigned i=0;i<EpochCount;++i){CHECK(all.states[i]==EpochState::Revealed);CHECK(all.obtained_at[i]==300+i);}
 CHECK(timeline_valid(all));all.states[0]=EpochState(6);CHECK(!timeline_valid(all));
 std::printf("Timeline source lifecycle: %u checks passed\n",checks);
}
