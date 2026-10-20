#include "core.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
static unsigned checks=0;
static void require(bool value,const char*message){++checks;if(!value){std::fprintf(stderr,"original events: %s\n",message);std::exit(1);}}
static State game,before,loaded;
static SaveData bytes,again;
static void reset(){new_run(game,0x71ef23,true,Character::Ironclad);game.phase=Phase::Map;game.hp=50;game.max_hp=80;game.original_events={};}
static void persisted(){
 require(validate(game),"valid event state");require(save(game,bytes),"save pending event");
 require(load(loaded,bytes.bytes,bytes.size),"load pending event");require(save(loaded,again),"resave pending event");
 require(bytes.size==again.size&&!std::memcmp(bytes.bytes,again.bytes,bytes.size),"event save is byte exact");game=loaded;
}
int main(){
 for(unsigned i=1;i<unsigned(OriginalEventId::Count);i++){
  auto id=OriginalEventId(i);require(original_event_key(id)[0],"canonical event key exists");
  require(original_event_id(original_event_key(id))==id,"canonical key roundtrip");
 }
 require(original_event_id("missing-event")==OriginalEventId::None,"unknown event rejected");
 reset();before=game;require(!original_event_begin(game,OriginalEventId::None,123,0),"invalid event cannot begin");
 require(!std::memcmp(&game,&before,sizeof game),"rejected begin leaves every state byte intact");
 require(original_event_begin(game,OriginalEventId::AbyssalBaths,123,0),"begin baths");
 require(original_event_option_count(game)==2,"baths initial options");
 before=game;require(!original_event_choose(game,3),"invalid option rejected");require(!std::memcmp(&game,&before,sizeof game),"invalid option is atomic");
 require(original_event_choose(game,0),"immerse");require(game.max_hp==82&&game.hp==49,"max HP and heal happen before 3 damage");
 require(game.original_events.values[0]==4&&!std::strcmp(game.original_events.page_key,"IMMERSE"),"next dose increments");persisted();
 require(original_event_choose(game,0),"linger");require(game.max_hp==84&&game.hp==47,"linger uses next increasing damage");
 require(game.original_events.values[0]==5&&game.original_events.values[1]==1,"damage and narrative counters are distinct");
 require(original_event_choose(game,1),"exit baths");require(game.original_events.finished&&!std::strcmp(game.original_events.page_key,"EXIT_BATHS"),"exit does not grant abstain heal");
 reset();require(original_event_begin(game,OriginalEventId::AbyssalBaths,9,0),"begin alternate baths");
 require(original_event_choose(game,1),"abstain");require(game.hp==60&&game.max_hp==80&&game.original_events.finished,"abstain heals 10 only");persisted();
 reset();require(original_event_begin(game,OriginalEventId::AbyssalBaths,9,0),"begin selection fixture");
 auto&e=game.original_events;e.step_count=3;e.steps[0]={OriginalEventOp::Select,int16_t(OriginalEventSelection::Remove),int16_t(OriginalEventFilter::Removable),2,2,0};
 e.steps[1]={OriginalEventOp::Heal,6};e.steps[2]={OriginalEventOp::Finish};
 original_event_resume(game);require(game.phase==Phase::OriginalEventChoice&&e.step_cursor==1,"selection suspends the remaining queue");
 unsigned original_count=game.deck_count;uint16_t eligible[MaxDeck];require(original_event_eligible(game,eligible,MaxDeck)>=2,"two removable instances available");
 uint16_t repeated[]={eligible[0],eligible[0]};before=game;require(!original_event_select(game,repeated,2),"duplicate selection rejected");require(!std::memcmp(&game,&before,sizeof game),"bad batch consumes no cards or random draws");
 persisted();uint16_t selected[]={eligible[1],eligible[0]};require(original_event_select(game,selected,2),"non-sorted selection accepted");
 require(game.deck_count==original_count-2&&game.hp==56&&game.original_events.finished,"selected batch finishes before queued heal and finish");persisted();
 before=game;game.original_events.option_count=MaxOriginalEventOptions+1;require(!valid_original_event_state(game),"bad option count rejected");game=before;
 game.original_events.step_cursor=MaxOriginalEventSteps+1;require(!valid_original_event_state(game),"bad continuation cursor rejected");game=before;
 game.original_events.crystal_owner[0]=1;require(!valid_original_event_state(game),"missing crystal owner rejected");game=before;
 require(!original_event_crystal_click(game,500,500),"unsupported crystal action cannot mutate a different event");
 std::printf("original event continuation checks: %u\n",checks);return 0;
}
