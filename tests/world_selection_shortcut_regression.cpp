#include "core.hpp"
#include "original_events.hpp"
#include "inplace_reset.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static State s,loaded;
static SaveData encoded,again;
static unsigned checks;
static const char* scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL [%s] %d: %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static void reset(std::initializer_list<Card> cards){
 reset_in_place(s);new_run(s,0x41cef1ea,false);init_world_relics(s);
 s.phase=Phase::Map;s.deck_count=0;s.relics=s.relic_count=s.relic_flash=0;s.world_relics.count=0;
 for(auto&r:s.world_relics.entries)clear_zero_object(r);
 for(Card card:cards)CHECK(event_add_deck_card(s,card)>=0);
 CHECK(validate(s));
}
static void roundtrip(){
 CHECK(validate(s));CHECK(save(s,encoded));CHECK(load(loaded,encoded.bytes,encoded.size));
 CHECK(save(loaded,again));CHECK(encoded.size==again.size&&!std::memcmp(encoded.bytes,again.bytes,encoded.size));s=loaded;
}
static void acquire(RelicId id){
 CHECK(gain_world_relic(s,id));CHECK(s.phase==Phase::Map&&!world_relic_pending(s));
 CHECK(!s.world_relics.step_count&&!s.world_relics.running);roundtrip();
}
struct RemovalTrace {
 unsigned kinds[5]{},count=0;CoreObserver previous{};
 RemovalTrace(){CoreObserver observer{};observer.context=this;observer.on_history=[](void*opaque,const State&,const RunHistoryNotice&notice){
  auto&self=*static_cast<RemovalTrace*>(opaque);if(notice.kind==RunHistoryEvent::CardRemoved&&self.count<5)self.kinds[self.count++]=card_from_token(notice.model).kind;
 };previous=set_core_observer(observer);}
 ~RemovalTrace(){set_core_observer(previous);}
};
static void upgrades_and_duplicates(){
 scenario="Pomander automatically upgrades its only eligible original card";
 reset({{Strike,0},{Defend,1}});acquire(WorldRelicPomander);CHECK(s.deck[0].upgraded&&s.deck[1].upgraded);
 scenario="Yummy Cookie returns fewer than its requested four upgrades";
 reset({{Strike,0},{Defend,0},{Bash,0}});acquire(WorldRelicYummyCookie);
 CHECK(s.deck_count==3);for(unsigned i=0;i<s.deck_count;++i)CHECK(s.deck[i].upgraded);
 scenario="Dolly singleton keeps permanent metadata and awaited Bing Bong clones";
 reset({{DefectGeneticAlgorithm,1}});s.defect.genetic_deck[0]=7;acquire(WorldRelicBingBong);acquire(WorldRelicDollysMirror);
 CHECK(s.deck_count==3);for(unsigned i=0;i<3;++i)CHECK(s.deck[i].kind==DefectGeneticAlgorithm&&s.deck[i].upgraded&&s.defect.genetic_deck[i]==7);
}
static void removals(){
 scenario="Empty Cage all-candidate removal follows original curse-first order";
 reset({{EventCardCurseOfTheBell,0},{Strike,0},{EventCardDoubt,0}});{
  RemovalTrace trace;acquire(WorldRelicEmptyCage);
  CHECK(s.deck_count==1&&s.deck[0].kind==EventCardCurseOfTheBell&&trace.count==2&&trace.kinds[0]==EventCardDoubt&&trace.kinds[1]==Strike);
 }
 scenario="Biiig Hug removes a whole eligible deck below the requested count";
 reset({{EventCardCurseOfTheBell,0},{Strike,0},{Defend,0},{Bash,0}});acquire(WorldRelicBiiigHug);CHECK(s.deck_count==1&&s.deck[0].kind==EventCardCurseOfTheBell);
 scenario="Precise Scissors handles its sole removable card";
 reset({{EventCardCurseOfTheBell,0},{Strike,0}});acquire(WorldRelicPreciseScissors);CHECK(s.deck_count==1&&s.deck[0].kind==EventCardCurseOfTheBell);
 scenario="Precarious Shears still resolves its queued acquisition payment";
 reset({{EventCardCurseOfTheBell,0},{Strike,0},{Defend,0}});const int hp=s.hp;acquire(WorldRelicPrecariousShears);CHECK(s.deck_count==1&&s.deck[0].kind==EventCardCurseOfTheBell&&s.hp==hp-16);
 scenario="Preserved Fog adds Folly after returning its complete removal set";
 reset({{Strike,0}});acquire(WorldRelicPreservedFog);CHECK(s.deck_count==1&&s.deck[0].kind==EventCardFolly);
 scenario="Pael's Tooth preserves the automatic selection snapshots";
 reset({{Strike,0},{Defend,0},{Bash,1}});acquire(WorldRelicPaelsTooth);
 CHECK(s.deck_count==1&&s.deck[0].kind==Bash&&s.world_relics.tooth_count==2);roundtrip();
}
static void transformations_and_enchantments(){
 scenario="Astrolabe automatically transforms and upgrades all three originals";
 reset({{Strike,0},{Defend,0},{Bash,0}});acquire(WorldRelicAstrolabe);
 CHECK(s.deck_count==3);for(unsigned i=0;i<3;++i)CHECK(s.deck[i].upgraded&&s.deck[i].kind!=Strike&&s.deck[i].kind!=Defend&&s.deck[i].kind!=Bash);
 scenario="New Leaf completes its sole transform without an overlay";
 reset({{Strike,0}});acquire(WorldRelicNewLeaf);CHECK(s.deck_count==1&&s.deck[0].kind!=Strike);
 struct EnchantCase {RelicId relic;CardKind card;EventEnchantment enchant;};
 const EnchantCase cases[]={{WorldRelicElectricShrymp,Defend,EventEnchantment::Imbued},
  {WorldRelicPaelsGrowth,Strike,EventEnchantment::Clone},{WorldRelicPunchDagger,Strike,EventEnchantment::Momentum},
  {WorldRelicRoyalStamp,Strike,EventEnchantment::RoyallyApproved},{WorldRelicTriBoomerang,Strike,EventEnchantment::Instinct}};
 for(const auto&test:cases){
  scenario=world_relic_definitions[test.relic].key;
  // The power at index zero is ineligible; Royal Stamp must finish writing
  // its shuffled candidate list before the shortcut reads index one.
  reset({{Inflame,0},{test.card,0}});
  if(test.enchant==EventEnchantment::Clone){
   s.event_cards.deck[0].enchantment=EventEnchantment::Glam;s.event_cards.deck[0].enchantment_amount=1;
  }
  CHECK(!event_card_can_enchant(s,0,test.enchant)&&event_card_can_enchant(s,1,test.enchant));
  acquire(test.relic);CHECK(s.event_cards.deck[1].enchantment==test.enchant&&s.event_cards.deck[0].enchantment!=test.enchant);
 }
 scenario="Tri-Boomerang also automatically returns a multi-card set below three";
 reset({{Strike,0},{Bash,0},{Defend,0}});acquire(WorldRelicTriBoomerang);
 CHECK(s.event_cards.deck[0].enchantment==EventEnchantment::Instinct&&s.event_cards.deck[1].enchantment==EventEnchantment::Instinct&&s.event_cards.deck[2].enchantment==EventEnchantment::None);
}
static void manual_and_legacy_boundaries(){
 scenario="Optional relic commands retain confirmation with one candidate";
 for(RelicId relic:{WorldRelicClaws,WorldRelicGnarledHammer,WorldRelicKifuda}){
  reset({{relic==WorldRelicKifuda?Defend:Strike,0}});CHECK(gain_world_relic(s,relic));
  CHECK(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice_min==0);roundtrip();
  CHECK(world_relic_select(s,nullptr,0));CHECK(s.phase==Phase::Map&&s.deck_count==1);roundtrip();
 }
 scenario="More candidates than the fixed request retain a real selection";
 reset({{Strike,0},{Defend,0}});CHECK(gain_world_relic(s,WorldRelicDollysMirror));
 CHECK(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice==WorldRelicChoice::Duplicate);roundtrip();
 uint16_t card=1;CHECK(world_relic_select(s,&card,1));CHECK(s.deck_count==3&&s.deck[2].kind==Defend);roundtrip();
 scenario="An old singleton continuation does not auto-resolve on load or later acquisition";
 reset({{Strike,0},{Defend,0}});CHECK(gain_world_relic(s,WorldRelicNewLeaf));CHECK(s.phase==Phase::WorldRelicChoice);
 s.deck_count=1;roundtrip();CHECK(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice==WorldRelicChoice::Transform);
 CHECK(gain_world_relic(s,WorldRelicAnchor));CHECK(s.phase==Phase::WorldRelicChoice&&s.deck[0].kind==Strike);roundtrip();
 card=0;CHECK(world_relic_select(s,&card,1));CHECK(s.phase==Phase::Map&&s.deck_count==1&&s.deck[0].kind!=Strike&&has_world_relic(s,WorldRelicAnchor));roundtrip();
}
int main(){upgrades_and_duplicates();removals();transformations_and_enchantments();manual_and_legacy_boundaries();std::printf("World source fixed selection shortcuts: %u checks passed\n",checks);}
