#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static SaveData snapshot(const State&s){SaveData b{};CHECK(validate(s));CHECK(save(s,b)&&b.bytes[4]==12);return b;}
static void same(const State&s,const SaveData&before){auto b=snapshot(s);CHECK(b.size==before.size&&!std::memcmp(b.bytes,before.bytes,b.size));}
static void roundtrip(const State&s){auto b=snapshot(s);State t{};CHECK(load(t,b.bytes,b.size));same(t,b);}
static State event(EventId id,Character ch=Character::Ironclad){State s{};new_run(s,140,false,ch);s.phase=Phase::Event;s.event_id=id;s.current_node=3;s.next_row=1;s.floor=2;s.map[3].room=Room::Event;s.map[3].event=(uint8_t)id;CHECK(validate(s));return s;}
static void aroma(){
 for(auto character:{Character::Ironclad,Character::Silent}){
    State s=event(EventId::AromaOfChaos,character);int gold=s.gold,node=s.current_node;unsigned n=s.deck_count;auto rng=s.rng_world;
    CHECK(event_choice(s,0));CHECK(s.phase==Phase::Transform&&s.event_remaining==1&&s.event_deck_action==EventDeckAction::Transform);roundtrip(s);
    auto pending=snapshot(s);CHECK(!transform_card(s,-1)&&!transform_card(s,n)&&!event_choice(s,1)&&!remove_card(s,0));same(s,pending);
    CHECK(cancel_event_selection(s));CHECK(s.phase==Phase::Event&&s.event_page==0&&s.gold==gold&&s.rng_world==rng&&s.deck_count==n);CHECK(s.event_deck_action==EventDeckAction::None);roundtrip(s);
    CHECK(load(s,pending.bytes,pending.size));State copy=s;int old=s.deck[2].kind;CHECK(transform_card(s,2)&&transform_card(copy,2));same(copy,snapshot(s));
    CHECK(s.phase==Phase::Event&&s.event_page==2&&s.deck[2].kind!=old&&card_in_pool(s.deck[2].kind,character)&&!s.deck[2].upgraded);
    CHECK(s.gold==gold&&s.deck_count==n&&s.current_node==node&&!s.map[node].visited&&s.event_remaining==0);roundtrip(s);
    auto done=snapshot(s);CHECK(!transform_card(s,2)&&!cancel_event_selection(s)&&!event_choice(s,1));same(s,done);CHECK(event_choice(s,0));CHECK(s.phase==Phase::Map&&s.next_row==2);roundtrip(s);
    s=event(EventId::AromaOfChaos,character);rng=s.rng_world;CHECK(event_choice(s,1));CHECK(s.phase==Phase::Upgrade&&s.event_deck_action==EventDeckAction::Upgrade);roundtrip(s);
    pending=snapshot(s);CHECK(upgrade_card(s,4));CHECK(s.phase==Phase::Event&&s.event_page==3&&s.deck[4].upgraded&&s.rng_world==rng&&s.gold==gold);roundtrip(s);
    CHECK(!upgrade_card(s,4));CHECK(load(s,pending.bytes,pending.size));CHECK(cancel_event_selection(s));CHECK(!s.deck[4].upgraded);roundtrip(s);
    for(unsigned i=0;i<s.deck_count;i++)s.deck[i].upgraded=1;
    CHECK(event_choice(s,1));CHECK(s.phase==Phase::Event&&s.event_page==3&&s.event_deck_action==EventDeckAction::None);CHECK(event_choice(s,0));CHECK(s.phase==Phase::Map);roundtrip(s);
 }
}
static void doors(){
 for(unsigned eligible=0;eligible<=10;eligible++)for(unsigned seed=1;seed<=80;seed++){
    State s=event(EventId::DoorsOfLightAndDark);s.rng_world=seed;for(unsigned i=0;i<s.deck_count;i++)s.deck[i].upgraded=i>=eligible;
    auto copy=s;CHECK(event_choice(s,0)&&event_choice(copy,0));same(copy,snapshot(s));unsigned upgraded=0;
    for(unsigned i=0;i<eligible;i++)upgraded+=s.deck[i].upgraded;
    CHECK(upgraded==(eligible<2?eligible:2));CHECK(s.phase==Phase::Event&&s.event_page==2&&!s.map[3].visited&&s.gold==99);roundtrip(s);
 }
 State s=event(EventId::DoorsOfLightAndDark);s.gold=0;s.removals=4;s.shop_sold=17;s.shop_prices[7]=150;int price=remove_price(s);auto rng=s.rng_world;unsigned count=s.deck_count;
 CHECK(event_option_enabled(s,1));CHECK(event_choice(s,1));CHECK(s.phase==Phase::Remove&&s.event_deck_action==EventDeckAction::Remove);auto pending=snapshot(s);roundtrip(s);
 CHECK(!remove_card(s,count));same(s,pending);CHECK(remove_card(s,0));CHECK(s.phase==Phase::Event&&s.event_page==3&&s.gold==0&&s.deck_count==count-1&&remove_price(s)==price&&s.removals==4&&s.shop_sold==17&&s.rng_world==rng);roundtrip(s);
 auto done=snapshot(s);CHECK(!remove_card(s,0));same(s,done);CHECK(load(s,pending.bytes,pending.size));CHECK(cancel_event_selection(s));CHECK(s.deck_count==count&&s.gold==0);roundtrip(s);
 s=event(EventId::DoorsOfLightAndDark);s.deck_count=1;auto one=snapshot(s);CHECK(!event_option_enabled(s,1)&&!event_choice(s,1));same(s,one);CHECK(event_choice(s,0));roundtrip(s);
 // A normal shop removal still charges and returns to the inventory, not an event.
 s=event(EventId::DoorsOfLightAndDark);s.phase=Phase::Shop;s.gold=500;price=remove_price(s);CHECK(buy(s,ShopRemove));CHECK(remove_card(s,0));CHECK(s.phase==Phase::Shop&&s.gold==500-price&&s.removals==1);roundtrip(s);
}
static void boundaries(){
 auto s=event(EventId::AromaOfChaos);CHECK(event_choice(s,0));auto b=snapshot(s);State bad=s;bad.phase=Phase::Shop;CHECK(!validate(bad));bad=s;bad.event_remaining=2;CHECK(!validate(bad));bad=s;bad.event_deck_action=EventDeckAction::Remove;CHECK(!validate(bad));
 for(size_t n=0;n<b.size;n+=4){State copy=s;CHECK(!load(copy,b.bytes,n));same(copy,b);}
 auto corrupt=b;corrupt.bytes[b.size-8]=255;State copy=s;CHECK(!load(copy,corrupt.bytes,corrupt.size));same(copy,b);
 // Event identities are materialized in new maps. Viewing old maps cannot reroll them.
 unsigned seen=0;for(unsigned seed=1;seed<100;seed++){new_run(s,seed);for(const auto&node:s.map)if(node.room==Room::Event)seen|=1u<<node.event;roundtrip(s);}
 CHECK(seen==(1u<<EventKinds)-1);
}
static void legacy(const char*path){
 FILE*f=fopen(path,"r");CHECK(f);unsigned v;std::vector<uint8_t>b;while(fscanf(f,"%2x",&v)==1)b.push_back((uint8_t)v);fclose(f);CHECK(b[4]==9);
 State s{};CHECK(load(s,b.data(),b.size()));CHECK(s.event_id==EventId::Grove&&s.phase==Phase::Transform&&s.event_deck_action==EventDeckAction::None&&s.event_remaining==2&&s.gold==0);roundtrip(s);
 CHECK(!cancel_event_selection(s));CHECK(transform_card(s,0));CHECK(s.phase==Phase::Transform&&s.event_remaining==1);CHECK(!transform_card(s,0));CHECK(transform_card(s,1));CHECK(s.phase==Phase::Event&&s.event_page==2);roundtrip(s);
}
int main(int argc,char**argv){aroma();doors();boundaries();if(argc>1)legacy(argv[1]);printf("events: %u checks passed\n",checks);}
