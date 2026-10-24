#include "core.hpp"
#include "world_run.hpp"
#include "run_options.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"history acquisition %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static State state,plain,observed;
static std::vector<RunHistoryNotice> notices;
static void capture(void*,const State&s,const RunHistoryNotice&notice){CHECK(&s==&observed);notices.push_back(notice);}
template<class Action> static bool command(Action action){
    plain=state;observed=state;notices.clear();
    CoreObserver none{};const auto previous=set_core_observer(none);const bool expected=action(plain);
    CoreObserver listener{};listener.on_history=capture;set_core_observer(listener);const bool actual=action(observed);
    set_core_observer(previous);CHECK(actual==expected);
    // The journal is presentation data: RNGs, serialized continuations,
    // canonical logs and every other State byte must stay identical.
    CHECK(!std::memcmp(&plain,&observed,sizeof(State)));state=observed;return actual;
}
static unsigned count(RunHistoryEvent kind,int flags=-1){unsigned n=0;for(const auto&v:notices)if(v.kind==kind&&(flags<0||v.flags==unsigned(flags)))++n;return n;}
static const RunHistoryNotice& one(RunHistoryEvent kind){CHECK(count(kind)==1);for(const auto&v:notices)if(v.kind==kind)return v;std::abort();}
static void reset(){
    new_run(state,0x41cef1ea,false,Character::Ironclad);init_world_relics(state);state.phase=Phase::Map;
    state.world_relics.count=0;state.relic_count=0;state.relics=0;
    for(auto&r:state.world_relics.entries)r={};for(auto&p:state.potions)p=Potion::Empty;notices.clear();
}
static void cards(){
    reset();const int at=state.deck_count;
    CHECK(command([](State&s){return event_add_deck_card(s,{Strike,1})>=0;}));
    CHECK(one(RunHistoryEvent::CardObtained).model==card_token({Strike,1}));CHECK(notices.size()==1);
    CHECK(command([=](State&s){return event_card_enchant(s,at,EventEnchantment::Sharp,4);}));
    const auto enchanted=one(RunHistoryEvent::CardEnchanted);CHECK(enchanted.model==card_token({Strike,1})&&enchanted.detail==4&&enchanted.flags==unsigned(EventEnchantment::Sharp));
    CHECK(!command([=](State&s){return event_card_enchant(s,at,EventEnchantment::Sharp,4);}));CHECK(notices.empty());
    CHECK(command([=](State&s){return event_remove_deck_card(s,at);}));CHECK(one(RunHistoryEvent::CardRemoved).model==card_token({Strike,1}));CHECK(notices.size()==1);
    CHECK(!command([](State&s){return event_remove_deck_card(s,-1);}));CHECK(notices.empty());
    state.deck[0]={Strike,1};
    CHECK(command([](State&s){return event_replace_deck_card(s,0,{Defend,0});}));
    const auto transformed=one(RunHistoryEvent::CardTransformed);CHECK(transformed.model==card_token({Strike,1})&&transformed.detail==card_token({Defend,0})&&!transformed.flags);CHECK(notices.size()==1);
    // Internal snapshot operations also implement transformations; their
    // caller determines whether a real RemoveFromDeck/Add command occurred.
    CHECK(command([](State&s){EventDeckSnapshot card;return event_take_deck_card(s,0,card)&&event_restore_deck_card(s,card)>=0;}));CHECK(notices.empty());
}
static void clones(){
    reset();CHECK(gain_world_relic(state,WorldRelicBingBong));state.world.active=1;state.run_options.mode=RunMode::Custom;state.run_options.modifiers=run_modifier_bit(RunModifier::Hoarder);
    const unsigned before=state.deck_count;
    CHECK(command([](State&s){return event_add_deck_card(s,{Bash,1})>=0;}));
    CHECK(state.deck_count==before+4&&count(RunHistoryEvent::CardObtained)==4);
    for(const auto&v:notices)CHECK(v.kind==RunHistoryEvent::CardObtained&&v.model==card_token({Bash,1}));
}
static void relics(){
    reset();CHECK(command([](State&s){return gain_world_relic(s,WorldRelicStrawberry);}));
    CHECK(one(RunHistoryEvent::RelicObtained).model==WorldRelicStrawberry);CHECK(one(RunHistoryEvent::MaxHpGained).model==7);
    const unsigned serial=state.world_relics.entries[0].serial;
    CHECK(command([=](State&s){return world_relic_replace(s,serial,WorldRelicVajra);}));
    CHECK(one(RunHistoryEvent::RelicRemoved).model==WorldRelicStrawberry&&one(RunHistoryEvent::RelicObtained).model==WorldRelicVajra);
    CHECK(command([](State&s){return lose_world_relic(s,WorldRelicVajra);}));CHECK(one(RunHistoryEvent::RelicRemoved).model==WorldRelicVajra);
    CHECK(!command([](State&s){return lose_world_relic(s,WorldRelicVajra);}));CHECK(notices.empty());
}
static void potions(){
    reset();CHECK(command([](State&s){return procure_potion_at(s,Potion::Fruit,0);}));CHECK(one(RunHistoryEvent::PotionObtained).model==unsigned(Potion::Fruit));
    CHECK(!command([](State&s){return procure_potion_at(s,Potion::Blood,0);}));CHECK(notices.empty());
    CHECK(command([](State&s){return use_potion(s,0,-1);}));CHECK(one(RunHistoryEvent::PotionUsed).model==unsigned(Potion::Fruit));CHECK(one(RunHistoryEvent::MaxHpGained).model==5);
    CHECK(!command([](State&s){return use_potion(s,0,-1);}));CHECK(notices.empty());
    CHECK(procure_potion_at(state,Potion::Blood,0));CHECK(command([](State&s){return discard_potion(s,0);}));CHECK(one(RunHistoryEvent::PotionDiscarded).model==unsigned(Potion::Blood));
    reset();CHECK(gain_world_relic(state,WorldRelicSozu));CHECK(!command([](State&s){return procure_potion(s,Potion::Blood);}));CHECK(notices.empty());
    reset();CHECK(procure_potion_at(state,Potion::FairyInABottle,0));state.hp=1;
    CHECK(command([](State&s){player_hp_loss(s,9999,false,false,-1);resolve_pending(s);return true;}));
    CHECK(state.hp>0&&state.potions[0]==Potion::Empty&&one(RunHistoryEvent::PotionUsed).model==unsigned(Potion::FairyInABottle));
    CHECK(!count(RunHistoryEvent::PotionDiscarded));
    reset();enter_combat(state);resolve_pending(state);CHECK(state.phase==Phase::Combat);CHECK(procure_potion_at(state,Potion::Forge,0));
    CHECK(command([](State&s){return use_potion(s,0,-1);}));CHECK(count(RunHistoryEvent::PotionUsed)==1&&!count(RunHistoryEvent::CardUpgraded));
}
static void maximum_hp(){
    reset();state.hp=state.max_hp=998;
    CHECK(command([](State&s){increase_max_hp(s,20);return true;}));CHECK(state.max_hp==999&&one(RunHistoryEvent::MaxHpGained).model==1);
    CHECK(command([](State&s){increase_max_hp(s,20);return true;}));CHECK(notices.empty());
    reset();enter_combat(state);resolve_pending(state);CHECK(state.phase==Phase::Combat);const int old=state.max_hp;
    CHECK(command([](State&s){player_lose_max_hp(s,2);return true;}));CHECK(!count(RunHistoryEvent::MaxHpLost));
    CHECK(command([](State&s){resolve_pending(s);return true;}));CHECK(state.max_hp==old-2&&one(RunHistoryEvent::MaxHpLost).model==2);
}
static void batch_transform(){
    reset();state.deck_count=3;state.deck[0]={Strike,0};state.deck[1]={Strike,1};state.deck[2]={Defend,1};
    CHECK(command([](State&s){return gain_world_relic(s,WorldRelicPandorasBox);}));
    CHECK(state.deck_count==3&&count(RunHistoryEvent::CardTransformed,1)==3&&count(RunHistoryEvent::CardTransformed,2)==3);
    CHECK(!count(RunHistoryEvent::CardObtained)&&!count(RunHistoryEvent::CardRemoved));
    unsigned pending=0;const unsigned originals[]={unsigned(card_token({Strike,0})),unsigned(card_token({Strike,1})),unsigned(card_token({Defend,1}))};
    for(const auto&v:notices)if(v.kind==RunHistoryEvent::CardTransformed&&v.flags==1){CHECK(v.model==originals[pending++]);CHECK(card_from_token(v.detail).kind<CardKinds&&card_token(card_from_token(v.detail))==v.detail);}
    CHECK(pending==3);
}
static void quest_completion(){
    reset();const int index=event_add_deck_card(state,{EventCardSpoilsMap,0});CHECK(index>=0);
    const int gold=state.gold;
    CHECK(command([=](State&s){return event_card_spoils_complete(s,index);}));
    CHECK(state.gold==gold+600&&one(RunHistoryEvent::QuestCompleted).model==EventCardSpoilsMap);
    CHECK(one(RunHistoryEvent::CardRemoved).model==card_token({EventCardSpoilsMap,0}));
    CHECK(!command([=](State&s){return event_card_spoils_complete(s,index);}));CHECK(notices.empty());
}
int main(){cards();clones();relics();potions();maximum_hp();batch_transform();quest_completion();std::printf("history acquisition: %u checks; actual commands preserve every canonical State byte\n",checks);}
