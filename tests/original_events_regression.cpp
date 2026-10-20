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
static void reset(){new_run(game,0x71ef23,false,Character::Ironclad);game.phase=Phase::Map;game.hp=50;game.max_hp=80;game.original_events=OriginalEventState{};}
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

 // The five decipher prices are 3, 6, 12, 24, and current max HP minus one.
 // The final decipher upgrades the whole surviving deck and stops RNG draws.
 reset();game.hp=game.max_hp=80;require(original_event_begin(game,OriginalEventId::TabletOfTruth,19,0),"begin tablet");
 for(unsigned i=0;i<4;i++){require(original_event_choose(game,0),"continue decipher");persisted();}
 require(game.max_hp==35&&game.original_events.values[0]==34,"tablet exponential costs");
 unsigned draws=game.original_events.rng_calls;require(original_event_choose(game,0),"final decipher");
 require(game.hp==1&&game.max_hp==1&&game.original_events.finished,"final decipher leaves one maximum HP");
 require(game.original_events.rng_calls==draws,"final whole-deck upgrade does not draw randomness");
 for(unsigned i=0;i<game.deck_count;i++)require(!card_upgradeable(game.deck[i]),"all eligible cards upgraded");persisted();
 reset();require(original_event_begin(game,OriginalEventId::TinkerTime,73,1),"begin Tinker Time");
 require(original_event_choose(game,0),"open two card types");require(game.original_events.option_count==2,"two of three card types offered");persisted();
 int type=game.original_events.values[1];require(original_event_choose(game,1),"pick offered card type");persisted();
 int rider=game.original_events.values[0];unsigned deck=game.deck_count;require(original_event_choose(game,0),"pick offered rider");
 require(game.deck_count==deck+1&&game.deck[deck].kind==EventCardMadScience,"Mad Science enters permanent deck");
 require(game.event_cards.deck[deck].science_type==type&&game.event_cards.deck[deck].rider==rider,"custom type and rider survive card acquisition");persisted();
 reset();require(original_event_begin(game,OriginalEventId::ColorfulPhilosophers,5,0),"begin colorful philosophers");
 require(original_event_choose(game,0),"choose another character's cards");
 require(game.original_events.reward_active&&game.original_events.reward_count==3&&game.original_events.selection==OriginalEventSelection::Rewards,"three independent card rewards remain a set");persisted();
 require(original_event_reward(game,2),"open last reward before first");require(game.original_events.selection==OriginalEventSelection::OfferCard,"card reward opens choices");
 uint16_t pick=0;require(original_event_select(game,&pick,1),"claim one offered card");
 require(game.original_events.rewards[2].claimed&&!game.original_events.rewards[0].claimed,"claiming one reward keeps siblings available");persisted();
 require(original_event_reward(game,-1),"skip remainder of reward set");require(!game.original_events.reward_active&&game.original_events.finished,"reward group returns to completed event");
 reset();game.gold=250;require(original_event_begin(game,OriginalEventId::CrystalSphere,31,1),"begin crystal sphere");
 require(original_event_choose(game,1),"take six crystal charges and debt");
 require(game.original_events.selection==OriginalEventSelection::Crystal&&game.original_events.crystal_charges==6,"crystal starts at six charges");
 require(game.original_events.crystal_clear[0]&&game.original_events.crystal_clear[10]&&game.original_events.crystal_clear[110]&&game.original_events.crystal_clear[120],"crystal corner visibility");persisted();
 require(original_event_crystal_tool(game,false),"switch to one-cell tool");
 for(unsigned n=0;n<6;n++){unsigned cell=0;while(cell<CrystalCells&&game.original_events.crystal_clear[cell])++cell;require(cell<CrystalCells,"unrevealed cell exists");require(original_event_crystal_click(game,cell%11,cell/11),"consume crystal click");persisted();}
 require(game.original_events.crystal_charges==0,"all six clicks consumed once");
 require(game.original_events.finished,"crystal finish page reached after last click");
 before=game;game.original_events.reward_count=MaxOriginalEventRewards+1;require(!valid_original_event_state(game),"excess reward count rejected");game=before;
 reset();game.gold=1000;require(original_event_begin(game,OriginalEventId::FakeMerchant,23,1),"begin fake merchant");
 require(game.original_events.offer_count==6&&game.original_events.option_count==7,"six distinct fake wares and leave");
 for(unsigned i=0;i<6;i++){require(game.original_events.choice_prices[i]>=42&&game.original_events.choice_prices[i]<=58,"fake wares use shop random price range");for(unsigned j=0;j<i;j++)require(game.original_events.offers[i]!=game.original_events.offers[j],"fake inventory draws without replacement");}
 persisted();
 // Capacity fixture, not a claim that every item fits a generated board:
 // the constructor's ten append/subscription passes provide these strict
 // upper bounds. Persist all callbacks/rewards and select beyond byte/deck
 // indexes so neither the codec nor reward navigation can truncate them.
 reset();require(original_event_begin(game,OriginalEventId::CrystalSphere,51,1),"begin capacity fixture");
 auto&full=game.original_events;full.finished=1;full.selection=OriginalEventSelection::Rewards;full.select_min=0;full.select_max=1;full.select_cancelable=1;game.phase=Phase::OriginalEventChoice;
 full.crystal_item_count=MaxCrystalItems;full.crystal_revealed_count=0;
 for(unsigned attempt=0;attempt<10;attempt++)for(unsigned item=0;item<15;item++){
  unsigned at=attempt*15+item;auto&v=full.crystal_items[at];v.kind=2;v.w=v.h=1;v.amount=10;v.subscriptions=uint8_t(10-attempt);v.revealed=1;
  for(unsigned n=0;n<v.subscriptions;n++)full.crystal_revealed[full.crystal_revealed_count++]=uint8_t(at);
 }
 require(full.crystal_revealed_count==MaxCrystalReveals,"all 825 repeated reveal callbacks retained");
 full.reward_active=1;full.reward_count=MaxOriginalEventRewards;full.reward_card_count=MaxOriginalEventRewardCards;
 for(unsigned i=0;i<MaxOriginalEventRewardCards;i++)full.reward_cards[i]=uint16_t(card_token({Strike,0}));
 for(unsigned i=0;i<MaxOriginalEventRewards;i++){auto&r=full.rewards[i];if(i<165){r.kind=4;r.first=uint16_t(i*3);r.count=3;}else{r.kind=1;r.amount=1;}}
 persisted();unsigned far=MaxOriginalEventRewards-1;require(far>MaxDeck&&far>255,"fixture exceeds both ordinary choice and byte indexes");
 int gold=game.gold;require(original_event_reward(game,int(far)),"claim reward beyond ordinary deck capacity");require(game.original_events.rewards[far].claimed&&game.gold==gold+1,"high reward index is not truncated");persisted();
 require(original_event_reward(game,164),"open final card reward beyond byte-offset storage");require(game.original_events.reward_current==164&&game.original_events.offer_count==3,"495-card buffer resolves its final three cards");persisted();
 require(original_event_select(game,&pick,1),"take card from final reward range");persisted();
 reset();require(original_event_begin(game,OriginalEventId::Amalgamator,7,0),"begin dynamic-name fixture");OriginalTextReference reference;
 require(original_event_description_reference(game,"Card1",0,true,reference)&&reference.kind==OriginalTextReferenceKind::Card&&reference.model==card_token({UltimateStrike,0}),"event card title references canonical token");
 reset();potion_set(game,0,Potion::Strength);require(original_event_begin(game,OriginalEventId::StoneOfAllTime,7,1),"begin potion title fixture");
 require(original_event_description_reference(game,"DrinkRandomPotion",0,true,reference)&&reference.model==unsigned(Potion::Strength),"event remembers selected potion title");
 require(original_event_choose(game,0),"consume named potion");require(original_event_description_reference(game,"DrinkRandomPotion",-1,true,reference)&&reference.model==unsigned(Potion::Strength),"consumed potion remains named on outcome page");persisted();
 std::printf("original event continuation checks: %u\n",checks);return 0;
}
