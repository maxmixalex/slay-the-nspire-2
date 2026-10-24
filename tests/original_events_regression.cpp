#include "core.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
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
static void potion_reward_inventory(){
 for(unsigned sozu=0;sozu<2;sozu++){
  reset();require(gain_world_relic(game,WorldRelicBeltBuckle),"prepare event potion listener");
  if(sozu)require(gain_world_relic(game,WorldRelicSozu),"prepare event procurement prohibition");
  require(add_potion_slots(game,MaxPotionSlots-potion_slot_count(game)),"expand event potion belt");
  for(unsigned i=0;i<MaxPotionSlots;i++)potion_set(game,i,Potion::Strength);
  require(original_event_begin(game,OriginalEventId::ColorfulPhilosophers,5,0)&&original_event_choose(game,0),"open a real event reward set");
  auto&e=game.original_events;for(auto&r:e.rewards)r=OriginalEventReward{};e.reward_count=2;e.reward_card_count=0;
  e.rewards[0].kind=2;e.rewards[0].model=uint16_t(Potion::Blood);e.rewards[1].kind=1;e.rewards[1].amount=5;persisted();
  if(!sozu){
   before=game;require(!original_event_reward(game,0)&&!std::memcmp(&game,&before,sizeof game),"full event potion reward is rejected without mutating any saved state");
   potion_set(game,0,Potion::Blood);game.hp=40;
   require(can_use_potion(game,0,-1)&&use_potion(game,0,-1)&&game.hp==56,"a noncombat potion is usable while event rewards remain open");
   require(e.selection==OriginalEventSelection::Rewards&&e.reward_active&&!e.rewards[0].claimed&&!world_relic_pending(game),"drinking preserves event reward ownership without pending hooks");
   require(procure_potion(game,Potion::Strength),"restore the first slot before exercising the last slot");potion_set(game,15,Potion::FairyInABottle);
   require(can_discard_potion(game,15)&&discard_potion(game,15)&&e.selection==OriginalEventSelection::Rewards,"discard automatic bottle from slot sixteen without leaving event rewards");persisted();
  }
  require(original_event_reward(game,0)&&e.rewards[0].claimed&&e.selection==OriginalEventSelection::Rewards&&e.reward_current==NoOriginalEventReward,"event potion claims directly and returns to the remaining reward");
  for(unsigned i=0;i<MaxPotionSlots;i++)require(potion_at(game,i)==(!sozu&&i==15?Potion::Blood:Potion::Strength),"event procurement changes only a real empty slot and respects Sozu");persisted();
  require(original_event_reward(game,1)&&!e.reward_active&&game.phase==Phase::Event&&e.finished,"last event reward resumes the original event");persisted();
 }
}
int main(){
 potion_reward_inventory();
 for(unsigned i=1;i<unsigned(OriginalEventId::Count);i++){
  auto id=OriginalEventId(i);require(original_event_key(id)[0],"canonical event key exists");
  require(original_event_id(original_event_key(id))==id,"canonical key roundtrip");
 }
 require(original_event_id("missing-event")==OriginalEventId::None,"unknown event rejected");
 // Enter every reachable model through the real dispatcher, with inventories
 // satisfying the finite event dependencies. This is an initial-state/save
 // integration check; individual outcome assertions remain below.
 for(unsigned i=1;i<unsigned(OriginalEventId::Count);i++){
  reset();game.gold=1000;potion_set(game,0,Potion::Blood);potion_set(game,1,Potion::Strength);
  const RelicId tradable[]={WorldRelicAnchor,WorldRelicVajra,WorldRelicOddlySmoothStone,WorldRelicStrawberry,WorldRelicPear};
  for(auto relic:tradable)require(gain_world_relic(game,relic),"prepare event dependency inventory");
  auto id=OriginalEventId(i);if(id==OriginalEventId::WarHistorianRepy)require(event_add_deck_card(game,{EventCardLanternKey,0})>=0,"prepare forced historian dependency");
  const unsigned act=id==OriginalEventId::Neow?0:1;
  if(!original_event_begin(game,id,0x71ef23,act)||!validate(game)){std::fprintf(stderr,"initial event dispatch/save failed: %s\n",original_event_key(id));return 1;}
  if(id==OriginalEventId::BattlewornDummy){int hp=0;require(original_event_description_value(game,"Setting1Hp",hp)&&hp==75&&original_event_description_value(game,"Setting2Hp",hp)&&hp==150&&original_event_description_value(game,"Setting3Hp",hp)&&hp==300,"dummy option HP uses the actual single-player monster models");}
  unsigned enabled=0;for(unsigned option=0;option<original_event_option_count(game);option++)if(original_event_option_enabled(game,int(option)))++enabled;
  require(enabled>0,"every entered event provides a usable initial option");persisted();
 }
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
 require(game.original_events.reward_active&&game.original_events.selection==OriginalEventSelection::Rewards&&!game.original_events.finished,"last crystal click awaits revealed rewards before completing the event");persisted();
 require(original_event_reward(game,-1),"close crystal rewards through the public reward action");
 require(game.original_events.finished&&!std::strcmp(game.original_events.page_key,"FINISH"),"crystal finish page reached after reward continuation");persisted();
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
 reset();for(unsigned i=0;i<potion_slot_count(game);i++)potion_set(game,i,Potion::Strength);
 require(original_event_begin(game,OriginalEventId::DrowningBeacon,9,0)&&original_event_choose(game,0),"beacon offers Glowwater to full potion inventory");persisted();
 require(original_event_potion(game,1),"replace one potion through discard and procure stages");require(potion_at(game,1)==Potion::GlowwaterPotion&&potion_at(game,0)==Potion::Strength&&game.original_events.finished,"potion replacement completes before the queued event outcome");persisted();
 reset();require(gain_world_relic(game,WorldRelicDriftwood)&&gain_world_relic(game,WorldRelicLastingCandy),"obtain event-reward modifier fixtures");
 require(original_event_begin(game,OriginalEventId::ColorfulPhilosophers,5,0)&&original_event_choose(game,0),"create restricted event card rewards with Driftwood");
 for(unsigned i=0;i<game.world_relics.count;i++)if(game.world_relics.entries[i].id==WorldRelicLastingCandy)require(game.world_relics.entries[i].values[0]==0,"event rewards do not advance Candy's combat-only cadence");
 require(original_event_reward(game,0)&&original_event_can_reroll_reward(game),"open rerollable event card reward");persisted();
 require(original_event_reroll_reward(game)&&game.original_events.reward_current==0&&!game.original_events.rewards[0].claimed,"event reroll keeps the reward unclaimed and selected");
 for(unsigned i=0;i<game.original_events.offer_count;i++)require(card_rarity(card_from_token(game.original_events.offers[i]).kind)==2,"Colorful reroll retains the original rarity predicate");require(!original_event_reroll_reward(game),"event reward reroll is single use");persisted();
 reset();require(gain_world_relic(game,WorldRelicDriftwood),"obtain future-potion reroll fixture");potion_set(game,0,Potion::Strength);
 require(original_event_begin(game,OriginalEventId::TheFutureOfPotions,3,1)&&original_event_choose(game,0),"trade potion for typed upgraded reward");require(original_event_reward(game,0),"open future-potion card reward");persisted();
 require(original_event_reroll_reward(game),"reroll future-potion reward");unsigned type_mask=game.original_events.rewards[0].type_mask;
 for(unsigned i=0;i<game.original_events.offer_count;i++){Card card=card_from_token(game.original_events.offers[i]);require((type_mask&(1u<<unsigned(card_type(game,card))))&&!card_upgradeable(card),"future-potion reroll preserves card type and after-generation upgrade");}persisted();

 // Ordinary event transforms append in the player's selected order.
 reset();game.deck_count=4;game.deck[0]={Bash,0};game.deck[1]={Strike,0};game.deck[2]={Defend,0};game.deck[3]={Defend,1};
 require(original_event_begin(game,OriginalEventId::MorphicGrove,123,0)&&original_event_choose(game,0),"open two-card transform");uint16_t transforms[]={2,0};persisted();
 require(original_event_select(game,transforms,2),"transform in unsorted selection order");
 require(game.deck_count==4&&game.deck[0].kind==Strike&&game.deck[1].kind==Defend&&game.deck[1].upgraded,"sequential event transforms preserve untouched original order");persisted();
 reset();game.hp=game.max_hp=3;potion_set(game,0,Potion::FairyInABottle);require(original_event_begin(game,OriginalEventId::TabletOfTruth,19,0),"begin insufficient-maximum tablet");unsigned prior_rng=game.original_events.rng_calls;
 require(original_event_choose(game,0)&&game.phase==Phase::Event&&game.hp==1&&game.max_hp==1,"tablet's explicit Kill allows normal death prevention");
 require(potion_at(game,0)==Potion::Empty&&game.original_events.page==1&&game.original_events.rng_calls==prior_rng,"revived decipher advances dialogue without the skipped upgrade roll");persisted();
 const OriginalEventId locked[]={OriginalEventId::StoneOfAllTime,OriginalEventId::RanwidTheElder,OriginalEventId::TheFutureOfPotions};
 for(auto id:locked){reset();potion_set(game,0,Potion::Blood);require(original_event_begin(game,id,7,1),"begin event with locked potion inventory");
  before=game;require(original_event_blocks_potions(game)&&!can_use_potion(game,0,-1)&&!use_potion(game,0,-1),"event potion lock reaches the public use API");require(!can_discard_potion(game,0)&&!discard_potion(game,0),"event potion lock also reaches the public discard API");require(!std::memcmp(&game,&before,sizeof game),"blocked potion actions change no state");}
 reset();potion_set(game,0,Potion::Blood);require(original_event_begin(game,OriginalEventId::StoneOfAllTime,7,1)&&original_event_choose(game,0),"event may consume its own offered potion while ordinary use is locked");
 require(potion_at(game,0)==Potion::Empty&&game.original_events.finished&&!original_event_blocks_potions(game),"finished event releases potion lock");
 reset();require(world_begin_run(game,reference::ActKind::Overgrowth,0),"start native world for event combat fixture");game.original_events=OriginalEventState{};potion_set(game,0,Potion::FoulPotion);
 require(original_event_begin(game,OriginalEventId::FakeMerchant,23,1)&&original_event_can_use_foul(game)&&can_use_potion(game,0,-1),"Foul Potion is usable on the original Fake Merchant");
 require(use_potion(game,0,-1)&&game.original_events.combat_pending&&game.enemy_world.encounter==EncounterId::FakeMerchantEventEncounter,"public potion use enters the exact event encounter");
 require(potion_at(game,0)==Potion::Empty&&original_event_combat_reward_count(game)==7,"Foul consumes one bottle and retains all seven extra rewards");uint8_t reward_kind=0;uint16_t reward_value=0;
 require(original_event_combat_reward(game,0,reward_kind,reward_value)&&reward_kind==2&&reward_value==WorldRelicFakeMerchantsRug,"Fake Merchant extra rewards begin with the genuine rug");persisted();
 for(unsigned i=0;i<game.enemy_count;i++)game.enemies[i].hp=0;resolve_pending(game);
 auto& rewards=game.world_relics;
 require(game.phase==Phase::WorldRelicChoice&&rewards.choice==WorldRelicChoice::OfferReward&&!game.world.reward_count&&game.reward_claimed==7,"main and event combat rewards have one saved owner");
 const unsigned group=rewards.choice_group;unsigned kinds[6]{};
 for(unsigned i=0;i<rewards.offer_count;i++){const auto& reward=rewards.rewards[rewards.offers[i]];require(reward.group==group&&reward.kind<6,"every combat reward belongs to the same group");++kinds[reward.kind];}
 require(kinds[1]==1&&kinds[2]==7&&kinds[3]<=1&&kinds[4]==1,"one menu contains the main card and gold, potion roll, rug and all unbought fake wares");persisted();
 auto reward_position=[&](unsigned kind,unsigned value=65535){for(unsigned i=0;i<rewards.offer_count;i++){const auto& reward=rewards.rewards[rewards.offers[i]];if(reward.kind==kind&&(value==65535||reward.value==value))return uint16_t(i);}require(false,"requested combat reward is offered");return uint16_t(0);};
 uint16_t position=reward_position(1);const unsigned main_reward=rewards.offers[position];
 require(world_relic_select(game,&position,1)&&rewards.choice==WorldRelicChoice::OfferCard,"open the main card before claiming event extras");persisted();
 require(world_relic_select(game,nullptr,0)&&rewards.choice==WorldRelicChoice::OfferReward&&!(rewards.rewards[main_reward].flags&1),"back leaves the same main card reward unclaimed");
 position=reward_position(2,WorldRelicFakeMerchantsRug);
 require(world_relic_select(game,&position,1)&&has_world_relic(game,WorldRelicFakeMerchantsRug)&&rewards.choice_group==group&&rewards.choice==WorldRelicChoice::OfferReward,"claim an event relic and return to the combined reward menu");persisted();
 position=reward_position(1);require(world_relic_select(game,&position,1)&&rewards.choice==WorldRelicChoice::OfferCard,"return to the original main card after the event relic");
 const unsigned deck_before_reward=game.deck_count;const Card chosen=card_from_token(rewards.offers[0]);position=0;
 require(world_relic_select(game,&position,1)&&game.deck_count==deck_before_reward+1&&card_token(game.deck[deck_before_reward])==card_token(chosen),"main card is obtained once from the combined group");
 require(rewards.choice==WorldRelicChoice::OfferReward&&rewards.choice_group==group&&game.original_events.combat_pending&&!game.original_events.finished,"claiming the main card does not finish the event while extras remain");persisted();
 require(world_relic_select(game,nullptr,0)&&game.phase==Phase::Reward&&!world_relic_pending(game),"skip the remaining combined combat rewards");persisted();
 require(world_finish_room(game),"proceed after the combined combat reward closes");
 require(game.original_events.finished&&!game.original_events.reward_active&&original_event_combat_reward_count(game)==0,"event continuation cannot offer the combat extras a second time");persisted();
 std::printf("original event continuation checks: %u\n",checks);return 0;
}
