#include "core.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "world_card_factory.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <cstddef>
using namespace spire;
// The frozen v3-v14 writer uses four bytes per field. Independently maximizing
// every variable count gives 109116 bytes, including its header/checksum. The
// v15 fields in the new world tail are compact and no wider than their C++
// storage; counting the whole tail also includes padding. Add the six extended
// legacy enemy slots, extra potion slots, saved Feral result and ordered
// result-location listeners/captures, plus alignment.
constexpr size_t LegacyEncodedCeiling=109116;
constexpr size_t SharedV15Bytes=1+MaxPotionSlots-LegacyPotionSlots+
 (MaxEnemies-LegacyEnemies)*(19+8+4+4+8+1+NDebuffCount+2*NTempStrengthCount+4*(MaxAutoFrames+1))+
 (MaxAutoFrames+1)+4+(MaxAutoFrames+1);
constexpr size_t EncodedCapacityCeiling=LegacyEncodedCeiling+SharedV15Bytes+
 sizeof(State)-offsetof(State,world)+3;
static_assert(EncodedCapacityCeiling<=SaveCapacity,"Every bounded v15 state must fit the save buffer");
static State s,loaded,original;
static SaveData encoded,roundtrip;
static unsigned checks=0;
static void require(bool okay,const char*message){++checks;if(!okay){std::fprintf(stderr,"world relics: %s\n",message);std::exit(1);}}
static WorldRelicInstance& relic(RelicId id){for(unsigned i=0;i<s.world_relics.count;i++)if(s.world_relics.entries[i].id==id)return s.world_relics.entries[i];std::abort();}
static void reset(){new_run(s,0x5a8317,false,Character::Ironclad);init_world_relics(s);s.phase=Phase::Map;s.world_relics.count=0;s.relic_count=0;s.relics=0;for(auto&r:s.world_relics.entries)r=WorldRelicInstance{};}
static void add(RelicId id){require(gain_world_relic(s,id),"obtain native relic");require(!world_relic_pending(s),"fixture relic has no unresolved acquisition");}
static void combat(){enter_combat(s);resolve_pending(s);require(s.phase==Phase::Combat,"fixture combat starts");for(unsigned i=0;i<s.enemy_count;i++)s.enemies[i].hp=s.enemies[i].max_hp=2000;}
static void hook(WorldRelicHook h,int a=0,int b=0,int c=0){world_relic_hook(s,h,a,b,c);resolve_pending(s);require(s.phase!=Phase::Error,"listener did not overflow or reject continuation");}
static void save_pending(){require(validate(s),"pending world relic state validates");require(save(s,encoded),"serialize pending world relic");require(load(loaded,encoded.bytes,encoded.size),"load pending world relic");require(save(loaded,roundtrip),"reserialize pending world relic");require(encoded.size==roundtrip.size&&!std::memcmp(encoded.bytes,roundtrip.bytes,encoded.size),"pending hook serials and counters are byte exact");s=loaded;}
static unsigned card_in_hand(CardType type){for(unsigned i=0;i<s.hand.count;i++)if(card_type(s,s.cards[s.hand.ids[i]],s.hand.ids[i])==type)return s.hand.ids[i];std::abort();}
static void prepared_hand(std::initializer_list<CardKind> kinds){
 s.phase=Phase::Combat;s.floor=s.turn=1;s.current_node=0;s.enemy_count=2;
 for(auto&enemy:s.enemies)enemy={2000,2000,0,0,0,2};
 s.card_count=s.hand.count=0;
 for(auto kind:kinds){unsigned id=s.card_count++;s.cards[id]={kind,0};s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=uint16_t(id);}
}
static void orbit_fixture(unsigned remainder=0){
 s.regent.powers[unsigned(RPower::Orbit)]=1;s.regent.power_order[s.regent.power_count++]=uint8_t(RPower::Orbit);
 s.regent.orbit[0]=1;s.regent.orbit_remainder[0]=uint8_t(remainder);
}
static void single_reward(unsigned kind,unsigned value=0){auto&w=s.world_relics;
 w.group_count=w.reward_count=1;w.group_active[0]=w.group_skip[0]=1;w.rewards[0].group=1;w.rewards[0].kind=uint8_t(kind);w.rewards[0].value=uint16_t(value);
 w.choice=WorldRelicChoice::OfferReward;w.choice_group=1;w.return_phase=uint8_t(Phase::Map);w.choice_max=1;w.choice_cancelable=1;w.offer_count=1;w.offers[0]=0;s.phase=Phase::WorldRelicChoice;
}
// Advance one already-queued relic callback through its production handler so
// a save can be taken between resource payment and its awaited continuation.
static void relic_front(){
 require(s.effect_count&&s.effects[0].kind==EffectKind::Extended&&s.effects[0].target==int(XOp::WRelicHook),"queued relic callback exists");
 Effect effect=s.effects[0];for(unsigned i=1;i<s.effect_count;i++)s.effects[i-1]=s.effects[i];s.effects[--s.effect_count]=Effect{};
 world_relic_effect(s,effect.value);
}
static void reward_capacity(){
 reset();single_reward(4,1);auto&w=s.world_relics;
 w.reward_count=MaxWorldRelicRewards;w.offer_count=MaxWorldRelicRewards;
 for(unsigned i=0;i<MaxWorldRelicRewards;i++){
  auto&r=w.rewards[i];r=WorldRelicReward{};r.group=1;r.kind=4;r.value=uint16_t(i+1);w.offers[i]=uint16_t(i);
 }
 // The enemy queue is serialized independently of the populated reward set.
 // Fill both so the boundary save also retains the last extra descriptor.
 auto&enemy=s.enemy_world;enemy.returned_gold_count=MaxEnemyExtraRewards;
 for(unsigned i=0;i<MaxEnemyExtraRewards;i++)enemy.returned_gold[i]=uint16_t(i+1);
 save_pending();require(encoded.size<=EncodedCapacityCeiling,"reward boundary save stays below the conservative complete-state ceiling");
 require(w.reward_count==128&&enemy.returned_gold_count==96&&enemy.returned_gold[95]==96,"all bounded reward and source entries survive the codec");
 uint16_t last=MaxWorldRelicRewards-1;int gold=s.gold;
 require(world_relic_select(s,&last,1)&&s.gold==gold+int(MaxWorldRelicRewards)&&w.offer_count==MaxWorldRelicRewards-1,"last reward index can be claimed without truncation");save_pending();
 require(world_relic_select(s,nullptr,0)&&!world_relic_pending(s)&&!w.reward_count,"skipping the remaining full group releases every entry");save_pending();
 reset();single_reward(4,1);w.reward_count=MaxWorldRelicRewards;
 for(auto&r:w.rewards){r=WorldRelicReward{};r.group=1;r.kind=4;r.value=1;}
 w.choice=WorldRelicChoice::None;w.choice_group=w.choice_reward=0;w.offer_count=0;s.phase=Phase::Map;
 require(!gain_world_relic(s,WorldRelicCauldron)&&s.phase==Phase::Error&&w.reward_count==MaxWorldRelicRewards,"overflow of a full reward store fails explicitly instead of dropping new rewards");
}
int main(){
 reset();add(WorldRelicAnchor);add(WorldRelicVajra);combat();require(s.block==10&&s.strength==1,"native first-33 relics are not also executed by legacy hooks");
 reset();add(WorldRelicShuriken);add(WorldRelicRuinedHelmet);add(WorldRelicNunchaku);combat();unsigned attack=card_in_hand(CardType::Attack);
 for(unsigned i=0;i<3;i++)hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0);
 require(s.strength==2&&relic(WorldRelicRuinedHelmet).values[0]==1,"first positive Strength gain is doubled once");
 for(unsigned i=0;i<3;i++)hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0);
 require(s.strength==3,"later Strength gains keep their original amount");
 require(relic(WorldRelicNunchaku).values[0]==6,"attack counter advances for repeated CardPlay listeners");
 hook(WorldRelicHook::AfterCombatEnd);require(relic(WorldRelicNunchaku).values[0]==6&&relic(WorldRelicShuriken).values[0]==0&&relic(WorldRelicRuinedHelmet).values[0]==0,"combat cleanup preserves run counters and clears combat counters");
 reset();add(WorldRelicGalacticDust);combat();int block=s.block;hook(WorldRelicHook::AfterStarsSpent,25);require(s.block==block+20&&relic(WorldRelicGalacticDust).values[0]==5,"large star payments award every full threshold and retain remainder");
 reset();add(WorldRelicSelfFormingClay);add(WorldRelicCentennialPuzzle);combat();int initial=s.hand.count;
 hook(WorldRelicHook::AfterHpLost,3,4,-1);require(s.world_relics.self_forming_clay==3&&relic(WorldRelicCentennialPuzzle).values[0]==1,"HP loss creates delayed block and one puzzle draw");
 hook(WorldRelicHook::AfterHpLost,2,4,-1);require(s.world_relics.self_forming_clay==6&&s.hand.count>=initial,"further losses stack delayed block without rearming puzzle");
 reset();add(WorldRelicToastyMittens);add(WorldRelicBurningSticks);
 // Enter without opening AfterPlayerTurnStart, then dispatch the explicit hook.
 s.phase=Phase::Combat;s.turn=1;s.hp=s.max_hp=80;s.enemy_count=1;s.enemies[0].hp=s.enemies[0].max_hp=100;
 s.playing=-1;s.card_count=2;s.cards[0]={Defend,0};s.cards[1]={Strike,0};s.zones[0]=s.zones[1]=Zone::Hand;s.hand.count=2;s.hand.ids[0]=0;s.hand.ids[1]=1;
 hook(WorldRelicHook::AfterHandDraw);require(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice==WorldRelicChoice::Hand,"Toasty pauses its ordered hook at a real hand selection");save_pending();
 uint16_t chosen=0;require(world_relic_select(s,&chosen,1),"resolve Toasty selection");resolve_pending(s);
 require(s.strength==1&&s.exhaust.count==1&&s.hand.count==2&&s.hand.ids[0]==1&&s.cards[s.hand.ids[1]].kind==Defend,"Burning Sticks exhaust callback runs before Toasty's Strength continuation");
 require(!s.world_relics.card_is_dupe[s.hand.ids[1]],"Burning Sticks creates an ordinary clone rather than a replay dupe");
 for(const auto&f:s.world_relics.hook_frames)require(!f.active,"finished nested hook frame released");
 reset();add(WorldRelicToastyMittens);add(WorldRelicMrStruggles);
 s.phase=Phase::Combat;s.turn=1;s.enemy_count=1;s.enemies[0].hp=s.enemies[0].max_hp=100;s.playing=-1;s.card_count=2;s.cards[0]={Defend,0};s.cards[1]={Strike,0};s.zones[0]=s.zones[1]=Zone::Hand;s.hand.count=2;s.hand.ids[0]=0;s.hand.ids[1]=1;
 hook(WorldRelicHook::AfterHandDraw);require(s.phase==Phase::WorldRelicChoice,"listener snapshot pauses");
 require(lose_world_relic(s,WorldRelicMrStruggles),"remove later listener while paused");require(gain_world_relic(s,WorldRelicMercuryHourglass),"append another listener while paused");
 require(world_relic_select(s,&chosen,1),"resume saved listener snapshot");resolve_pending(s);
 require(s.enemies[0].hp==100,"removed serial is skipped and newly acquired listener is excluded from old snapshot");
 reset();add(WorldRelicBeltBuckle);combat();require(s.dexterity==2,"empty potion inventory grants buckle Dexterity");
 require(procure_potion(s,Potion::Strength),"procure during combat");resolve_pending(s);require(s.dexterity==0,"procurement removes exactly buckle's Dexterity");
 require(discard_potion(s,0),"discard last potion");resolve_pending(s);require(s.dexterity==2,"discarding final potion restores buckle Dexterity");
 reset();add(WorldRelicBrilliantScarf);combat();attack=card_in_hand(CardType::Attack);
 for(unsigned i=0;i<4;i++)hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0);
 require(cost(s,int(attack))==0,"fifth manual play has zero energy cost");
 hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0x10000);require(relic(WorldRelicBrilliantScarf).values[0]==4,"automatic CardPlay does not consume Scarf's manual-play counter");
 hook(WorldRelicHook::AfterCardPlayed,int(attack),0,0);require(cost(s,int(attack))>0,"Scarf discount ends after fifth manual play");
 reset();add(WorldRelicBeltBuckle);combat();unsigned old_slots=potion_slot_count(s);require(gain_world_relic(s,WorldRelicAlchemicalCoffer),"obtain Coffer in combat");
 require(potion_slot_count(s)==old_slots+4&&world_relic_pending(s),"Coffer saves the pending per-bottle continuation");
 for(unsigned i=0;i<potion_slot_count(s);i++)require(potion_at(s,i)==Potion::Empty,"no bottle jumps ahead of acquisition continuation");save_pending();resolve_pending(s);
 require(!world_relic_pending(s)&&s.dexterity==0,"first Coffer procurement updates Buckle before completing pickup");
 for(unsigned i=0;i<potion_slot_count(s);i++)require((potion_at(s,i)!=Potion::Empty)==(i>=old_slots),"Coffer fills only the four newly added slots");
 reset();add(WorldRelicSozu);old_slots=potion_slot_count(s);add(WorldRelicAlchemicalCoffer);require(potion_slot_count(s)==old_slots+4,"Sozu does not prevent added potion capacity");for(unsigned i=0;i<potion_slot_count(s);i++)require(potion_at(s,i)==Potion::Empty,"Sozu blocks every generated Coffer bottle");
 reset();int genetic=event_add_deck_card(s,{DefectGeneticAlgorithm,1});require(genetic>=0,"create permanent Genetic Algorithm");s.defect.genetic_deck[genetic]=7;add(WorldRelicBingBong);
 require(gain_world_relic(s,WorldRelicDollysMirror)&&s.world_relics.choice==WorldRelicChoice::Duplicate,"Dolly opens duplicate choice");save_pending();unsigned old_deck=s.deck_count;uint16_t source=uint16_t(genetic);require(world_relic_select(s,&source,1),"duplicate stored genetic card");
 require(s.deck_count==old_deck+2&&s.defect.genetic_deck[old_deck]==7&&s.defect.genetic_deck[old_deck+1]==7,"BingBong sees complete permanent metadata before its clone callback");
 reset();s.deck_count=MaxDeck;for(unsigned i=0;i<s.deck_count;i++)s.deck[i]={i&1?Defend:Strike,0};add(WorldRelicPandorasBox);
 for(unsigned i=0;i<s.deck_count;i++)require(s.deck[i].kind!=Strike&&s.deck[i].kind!=Defend,"Pandora transforms beyond the continuation queue capacity");require(!s.world_relics.step_count,"completed batch acquisition releases every queued step");
 reset();add(WorldRelicNunchaku);add(WorldRelicNunchaku);s.world_relics.entries[0].values[0]=3;s.world_relics.entries[1].values[0]=8;int amount=0;
 require(world_relic_display(s,0,amount)&&amount==3&&world_relic_display(s,1,amount)&&amount==8,"display counters use the requested owned copy");s.world_relics.entries[1].flags|=WorldRelicMelted;require(!world_relic_display(s,1,amount),"melted owned copy does not show an active counter");
 reset();add(WorldRelicDriftwood);require(gain_world_relic(s,WorldRelicGlassEye)&&s.world_relics.choice==WorldRelicChoice::OfferReward,"Glass Eye exposes its five independent rewards");uint16_t first_reward=0;
 require(world_relic_select(s,&first_reward,1)&&world_relic_can_reroll_reward(s),"Driftwood adds a reroll to the opened card reward");save_pending();unsigned current_reward=s.world_relics.choice_reward;
 require(world_relic_reroll_reward(s)&&s.world_relics.choice==WorldRelicChoice::OfferCard&&s.world_relics.choice_reward==current_reward,"reroll remains in the same unclaimed reward");require(!world_relic_can_reroll_reward(s)&&!world_relic_reroll_reward(s),"one reward cannot be rerolled twice");
 for(unsigned i=0;i<s.world_relics.offer_count;i++)require(card_rarity(card_from_token(s.world_relics.offers[i]).kind)==2,"reroll keeps Glass Eye's original common-rarity restriction");save_pending();
 original=s;s.world_relics.hook_frames[0].active=1;s.world_relics.hook_frames[0].count=1;s.world_relics.hook_frames[0].serials[0]=uint16_t(s.world_relics.last_serial+1);require(!valid_world_relic_state(s),"forged listener serial rejected");s=original;

 // Transforming one card removes it, then appends its replacement. A batch
 // removes all originals before the first replacement triggers any listener.
 reset();s.deck_count=4;s.deck[0]={Bash,0};s.deck[1]={Strike,0};s.deck[2]={Defend,0};s.deck[3]={Defend,1};
 require(event_card_enchant(s,0,EventEnchantment::Sharp,3)&&event_card_enchant(s,3,EventEnchantment::Nimble,2),"prepare distinct Claws enchantments");
 require(gain_world_relic(s,WorldRelicClaws)&&s.world_relics.choice==WorldRelicChoice::Transform,"Claws opens its optional batch");uint16_t claws[]={3,0};save_pending();
 require(world_relic_select(s,claws,2),"Claws accepts selection order independent of deck order");
 require(s.deck_count==4&&s.deck[0].kind==Strike&&s.deck[1].kind==Defend&&s.deck[2].kind==EventCardMaul&&s.deck[3].kind==EventCardMaul,"batch replacements append after untouched originals");
 require(!s.deck[2].upgraded&&s.event_cards.deck[2].enchantment==EventEnchantment::Sharp&&s.event_cards.deck[2].enchantment_amount==3,"batch orders by pile index at removal");
 require(s.deck[3].upgraded&&s.event_cards.deck[3].enchantment==EventEnchantment::None,"Claws keeps upgrade but rejects a block-only enchantment on Maul");save_pending();
 reset();add(WorldRelicNutritiousSoup);require(gain_world_relic(s,WorldRelicEmptyCage),"open removal after Eternal enchantments");uint16_t removable[MaxDeck];unsigned available=world_relic_eligible(s,removable,MaxDeck);
 for(unsigned i=0;i<available;i++)require(s.deck[removable[i]].kind!=Strike,"native relic choice excludes Eternal basic attacks");save_pending();
 reset();add(WorldRelicNutritiousSoup);unsigned eternal=0;for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind==Strike)++eternal;add(WorldRelicPandorasBox);
 unsigned kept=0;for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind==Strike){++kept;require(s.event_cards.deck[i].enchantment==EventEnchantment::TezcatarasEmber,"Pandora retains the Eternal card facts");}
 require(kept==eternal,"Pandora does not select Eternal basics");
 // Seventeen equal current-pile indexes cross the locked runtime's insertion
 // threshold. This fixed permutation distinguishes its unstable partition sort.
 reset();s.deck_count=18;for(unsigned i=0;i<17;i++)s.deck[i]={i&1?Defend:Strike,0};s.deck[17]={Bash,0};original=s;Card predicted[17];EventCardFacts predicted_facts;
 for(unsigned i=0;i<17;i++)require(world_transform_card(original,world_rng(original,WorldRng::Niche),original.deck[i],false,predicted[i],predicted_facts),"prepare independent transform draws");
 add(WorldRelicPandorasBox);const unsigned tie_order[]={0,14,13,12,11,10,9,15,8,6,5,4,3,2,1,7,16};require(s.deck[0].kind==Bash,"untouched starter precedes batch replacements");
 for(unsigned i=0;i<17;i++)require(card_token(s.deck[i+1])==card_token(predicted[tie_order[i]]),"equal-index batch order follows locked comparison sort");
 reset();s.deck_count=MaxDeck;for(unsigned i=0;i<MaxDeck-1;i++)s.deck[i]={i&1?Defend:Strike,0};s.deck[MaxDeck-1]={Bash,0};combat();
 require(gain_world_relic(s,WorldRelicPandorasBox),"obtain full batch in combat");world_relic_acquisition_resume(s);
 require(s.deck_count==1&&s.world_relics.transform_count==MaxDeck-1&&s.world_relics.transform_cursor==0,"all originals removed before first asynchronous append");save_pending();resolve_pending(s);
 require(s.deck_count==MaxDeck&&!world_relic_pending(s)&&!s.world_relics.transform_count,"saved full batch resumes every replacement");
 reset();add(WorldRelicSelfFormingClay);add(WorldRelicCentennialPuzzle);combat();s.hp=s.max_hp=80;
 require(gain_world_relic(s,WorldRelicLeafyPoultice),"obtain maximum-HP loss in combat");save_pending();resolve_pending(s);
 require(s.max_hp==68&&s.hp==68&&s.world_relics.self_forming_clay==3&&relic(WorldRelicCentennialPuzzle).values[0]==1,"LoseMaxHp sends real damage before committing new maximum");
 reset();add(WorldRelicRegalite);require(event_add_deck_card(s,{EventCardByrdonisEgg,0})>=0&&event_add_deck_card(s,{EventCardByrdonisEgg,0})>=0,"prepare two hatchable eggs");combat();int prior_block=s.block;unsigned prior_generated=s.regent.cards_created;
 require(gain_world_relic(s,WorldRelicByrdpip)&&!s.world_relics.byrdpip_present,"pet waits for permanent and combat transformations");save_pending();resolve_pending(s);
 for(unsigned i=0;i<s.deck_count;i++)require(s.deck[i].kind!=EventCardByrdonisEgg,"permanent eggs transformed");for(unsigned i=0;i<s.card_count;i++)require(s.cards[i].kind!=EventCardByrdonisEgg,"combat eggs transformed");
 require(s.world_relics.byrdpip_present&&s.regent.cards_created==prior_generated+2&&s.block==prior_block+4,"combat transforms emit generated-card callbacks before the pet appears");
 reset();require(original_event_begin(s,OriginalEventId::ColorfulPhilosophers,5,0)&&original_event_choose(s,0),"prepare existing subscribed card rewards");add(WorldRelicSilverCrucible);add(WorldRelicSilkenTress);
 require(relic(WorldRelicSilverCrucible).values[0]==0&&relic(WorldRelicSilkenTress).values[0]==0,"RelicObtained uses AfterModifyingRewards rather than spending card-option callbacks");
 for(unsigned i=0;i<s.original_events.reward_card_count;i++){Card card=card_from_token(s.original_events.reward_cards[i]);require(!card_upgradeable(card)&&s.original_events.reward_facts[i].enchantment==EventEnchantment::Glam,"all pre-existing rewards receive the newly obtained modifiers");}
 WorldCardOptions options;options.flags=WorldIsCardReward|WorldNoUpgradeRoll;options.odds=WorldCardOdds::Uniform;Card next;EventCardFacts next_facts;
 require(world_create_reward(s,world_rng(s,WorldRng::Rewards),1,options,&next,&next_facts),"generate a later card reward");
 require(relic(WorldRelicSilverCrucible).values[0]==1&&relic(WorldRelicSilkenTress).values[0]==1,"normal generation consumes card-option callbacks once per reward");save_pending();
 reset();add(WorldRelicPumpkinCandle);relic(WorldRelicPumpkinCandle).values[0]=0;require(world_relic_disabled(s,0)&&world_relic_display(s,0,amount)&&amount==0,"disabled candle still shows its original zero counter");
 reset();char title_key[96]{};require(world_relic_title_key(s,WorldRelicSeaGlass,title_key,sizeof title_key)&&!std::strcmp(title_key,"SEA_GLASS.title"),"canonical Sea Glass has its ordinary title key");
 require(world_relic_setup_offer(s,WorldRelicSeaGlass,int(Character::Defect))&&world_relic_title_key(s,WorldRelicSeaGlass,title_key,sizeof title_key)&&!std::strcmp(title_key,"SEA_GLASS.DEFECT.title"),"prepared Sea Glass title follows the offered character");
 require(gain_world_relic(s,WorldRelicSeaGlass)&&s.world_relics.choice==WorldRelicChoice::OfferCard&&world_relic_select(s,nullptr,0),"obtain and leave the prepared Sea Glass card grid");
 require(world_relic_setup_offer(s,WorldRelicSeaGlass,int(Character::Silent))&&world_relic_title_key(s,WorldRelicSeaGlass,title_key,sizeof title_key)&&!std::strcmp(title_key,"SEA_GLASS.SILENT.title"),"a new offer uses its newly prepared title");
 require(world_relic_title_key(s,WorldRelicSeaGlass,title_key,sizeof title_key,0)&&!std::strcmp(title_key,"SEA_GLASS.DEFECT.title"),"an owned Sea Glass keeps its own title while another offer exists");save_pending();
 reset();add(WorldRelicDriftwood);add(WorldRelicPaelsWing);auto&w=s.world_relics;w.group_count=w.reward_count=1;w.group_active[0]=w.group_skip[0]=1;w.rewards[0].group=1;w.rewards[0].kind=5;w.rewards[0].value=uint16_t(card_token({EventCardLanternKey,0}));
 w.choice=WorldRelicChoice::OfferReward;w.choice_group=1;w.return_phase=uint8_t(Phase::Map);w.choice_max=1;w.choice_cancelable=1;w.offer_count=1;w.offers[0]=0;s.phase=Phase::WorldRelicChoice;save_pending();old_deck=s.deck_count;uint16_t special=0;
 require(world_relic_select(s,&special,1)&&s.deck_count==old_deck+1&&s.deck[old_deck].kind==EventCardLanternKey,"special card reward adds its exact card directly");
 require(s.phase==Phase::Map&&!world_relic_pending(s)&&relic(WorldRelicPaelsWing).values[0]==0,"special card does not expose card-reward alternatives");
 // PotionReward.OnSelect rejects TooFull without opening a replacement grid.
 // The player can free any real belt slot while its reward remains available.
 reset();add(WorldRelicBeltBuckle);require(add_potion_slots(s,MaxPotionSlots-potion_slot_count(s)),"expand potion belt to its actual capacity");
 for(unsigned i=0;i<MaxPotionSlots;i++)potion_set(s,i,Potion::Blood);potion_set(s,15,Potion::FairyInABottle);
 single_reward(3,unsigned(Potion::Strength));save_pending();uint16_t bottle=0;original=s;
 require(!world_relic_select(s,&bottle,1)&&!std::memcmp(&s,&original,sizeof s),"full reward rejection preserves menu, queue, revision and every bottle");
 require(can_discard_potion(s,15)&&discard_potion(s,15)&&potion_at(s,15)==Potion::Empty,"automatic potion can be discarded from the sixteenth slot");
 require(w.choice==WorldRelicChoice::OfferReward&&!(w.rewards[0].flags&1),"discard keeps the same unclaimed reward menu");
 for(const auto&frame:w.hook_frames)require(!frame.active,"discard creates no inert suspended potion hook");save_pending();
 require(world_relic_select(s,&bottle,1)&&potion_at(s,15)==Potion::Strength,"claim fills the freed sixteenth slot directly");
 for(unsigned i=0;i<15;i++)require(potion_at(s,i)==Potion::Blood,"claim leaves every other bottle intact");
 require(s.phase==Phase::Map&&!world_relic_pending(s)&&!w.group_count&&!w.reward_count&&!w.choice_group&&!w.choice_reward&&!w.offer_count,"last potion clears the completed reward group's saved indexes");save_pending();
 reset();add(WorldRelicSozu);for(unsigned i=0;i<potion_slot_count(s);i++)potion_set(s,i,Potion::Blood);single_reward(3,unsigned(Potion::Strength));
 require(world_relic_select(s,&bottle,1)&&s.phase==Phase::Map&&!world_relic_pending(s),"Sozu's NotAllowed result consumes the reward even with a full belt");
 for(unsigned i=0;i<potion_slot_count(s);i++)require(potion_at(s,i)==Potion::Blood,"Sozu never replaces a held bottle");save_pending();
 reset();add(WorldRelicBeltBuckle);add(WorldRelicReptileTrinket);single_reward(4,1);
 for(unsigned i=0;i<MaxWorldRelicHookFrames+2;i++){
  s.hp=40;require(potion_set(s,0,Potion::Blood)&&can_use_potion(s,0,-1)&&use_potion(s,0,-1),"noncombat potion remains usable over a reward menu");
  require(s.hp==56&&w.choice==WorldRelicChoice::OfferReward&&s.phase==Phase::WorldRelicChoice&&!w.rewards[0].flags,"drinking heals without dismissing or consuming the reward");
  require(procure_potion(s,Potion::Blood)&&can_discard_potion(s,0)&&discard_potion(s,0),"a replenished bottle can be discarded while rewards stay open");
  for(const auto&frame:w.hook_frames)require(!frame.active,"reward potion actions do not accumulate no-op hook frames");
 }
 require(!s.effect_count&&!w.step_count&&!s.strength&&!s.dexterity,"reward use does not activate combat-only potion relic bonuses");save_pending();
 // A relic-acquisition reward can also suspend an active combat. Its hooks
 // must still resume after that menu; only genuinely noncombat hooks are empty.
 reset();add(WorldRelicRedSkull);combat();s.hp=35;hook(WorldRelicHook::AfterHpChanged,80,35,-1);require(s.strength==3,"prepare active Red Skull before a nested combat reward");
 single_reward(4,1);w.return_phase=uint8_t(Phase::Combat);potion_set(s,0,Potion::Blood);
 require(use_potion(s,0,-1)&&s.hp==51&&w.choice==WorldRelicChoice::OfferReward,"healing can preserve a reward suspended inside combat");save_pending();
 require(world_relic_select(s,&bottle,1)&&s.phase==Phase::Combat&&!world_relic_pending(s)&&!s.strength,"nested combat reward resumes the deferred HP listener");save_pending();
 // Acquiring a reward-producing relic nests a new set inside its parent.
 // Closing that child must resume, then close the already claimed parent.
 reset();w.group_count=w.reward_count=1;w.group_active[0]=w.group_skip[0]=1;w.rewards[0].group=1;w.rewards[0].kind=2;w.rewards[0].value=WorldRelicCauldron;
 w.choice=WorldRelicChoice::OfferReward;w.choice_group=1;w.return_phase=uint8_t(Phase::Map);w.choice_max=1;w.choice_cancelable=1;w.offer_count=1;w.offers[0]=0;s.phase=Phase::WorldRelicChoice;
 uint16_t cauldron=0;require(world_relic_select(s,&cauldron,1)&&w.choice==WorldRelicChoice::OfferReward&&w.choice_group==2&&w.offer_count==5,"Cauldron opens a separately saved nested reward group");save_pending();
 original=s;w.offers[1]=w.offers[0];require(!valid_world_relic_state(s),"duplicate reward-menu indexes are rejected");s=original;
 require(world_relic_select(s,nullptr,0)&&s.phase==Phase::Map&&!world_relic_pending(s)&&!w.group_count&&!w.reward_count,"closing nested rewards resumes and releases the parent group");save_pending();
 reset();single_reward(5,unsigned(card_token({DefectGeneticAlgorithm,1})));w.rewards[0].tokens[0]=123;save_pending();old_deck=s.deck_count;
 uint16_t recovered=0;require(world_relic_select(s,&recovered,1)&&s.deck_count==old_deck+1&&s.deck[old_deck].kind==DefectGeneticAlgorithm&&s.deck[old_deck].upgraded&&s.defect.genetic_deck[old_deck]==123,"stolen-card reward preserves its upgrade and permanent growth");save_pending();
 reset();s.deck_count=2;s.deck[0]={Strike,0};s.deck[1]={Defend,0};for(auto&facts:s.event_cards.deck)facts=EventCardFacts{};
 require(event_card_enchant(s,0,EventEnchantment::TezcatarasEmber,1),"prepare a permanent card that cannot be removed");single_reward(6);save_pending();
 uint16_t removal=0;require(world_relic_select(s,&removal,1)&&w.choice==WorldRelicChoice::Remove,"card-removal reward opens a saved deck selection");
 uint16_t removal_options[MaxDeck];require(world_relic_eligible(s,removal_options,MaxDeck)==1&&removal_options[0]==1,"reward removal respects Eternal eligibility");save_pending();
 original=s;require(!world_relic_select(s,&removal,1)&&!std::memcmp(&s,&original,sizeof s),"protected card rejection is atomic");
 require(world_relic_select(s,nullptr,0)&&w.choice==WorldRelicChoice::OfferReward&&!(w.rewards[0].flags&1),"cancel leaves card removal available in its parent reward set");save_pending();
 require(world_relic_select(s,&removal,1),"reopen card-removal reward");removal=1;
 require(world_relic_select(s,&removal,1)&&s.deck_count==1&&s.deck[0].kind==Strike&&s.phase==Phase::Map&&!world_relic_pending(s),"one successful removal completes and releases its reward");save_pending();
 single_reward(6);removal=0;require(world_relic_select(s,&removal,1)&&w.choice==WorldRelicChoice::OfferReward&&s.deck_count==1,"no removable cards returns to the still-skippable reward set");
 require(world_relic_select(s,nullptr,0),"skip removal when only protected cards remain");save_pending();
 // These two relics are called by Weak/Vulnerable directly, not ordinary
 // AbstractModel override hooks. Modifiers precede Debilitate and rounding.
 reset();add(WorldRelicPaperPhrog);prepared_hand({Strike,Defend});s.energy=3;s.enemies[0].vulnerable=1;
 require(damage(s,{Strike,0},0,0)==10,"Paper Phrog changes powered player vulnerability to 175 percent before flooring");
 s.expansion.osty_hp=s.expansion.osty_max_hp=1;require(damage(s,{NecroFlatten,0},0)==18,"a pet dealer does not inherit Paper Phrog from its owner");
 s.necro.debilitate[0]=1;require(damage(s,{Strike,0},0,0)==15&&damage(s,{NecroFlatten,0},0)==24,"Debilitate doubles the adjusted excess vulnerability for the correct dealer");s.necro.debilitate[0]=0;
 require(play(s,0,0)&&s.enemies[0].hp==1990,"Paper Phrog preview and resolved player attack agree");save_pending();
 reset();add(WorldRelicPaperKrane);prepared_hand({Defend});s.enemies[0].move=0;s.enemies[0].weak=1;
 require(enemy_damage(s,0)==7,"Paper Krane makes the legacy powered attack use 60 percent weakness");s.necro.debilitate[0]=1;
 require(enemy_damage(s,0)==2,"Krane is applied before Debilitate's stronger weakness");relic(WorldRelicPaperKrane).flags|=WorldRelicMelted;
 require(enemy_damage(s,0)==6,"a melted Krane no longer modifies weakness");
 reset();add(WorldRelicHistoryCourse);add(WorldRelicMusicBox);prepared_hand({Strike});s.energy=3;
 require(play(s,0,0)&&s.hand.count==1,"Music Box creates a hand clone after the first attack");unsigned music_clone=s.hand.ids[0];
 require(!s.world_relics.card_is_dupe[music_clone]&&play(s,0,0)&&relic(WorldRelicHistoryCourse).values[0]==int(music_clone+1),"an ordinary Music Box clone remains eligible for History Course");
 relic(WorldRelicMusicBox).flags|=WorldRelicMelted;s.turn=2;hook(WorldRelicHook::BeforeSideTurnStart);
 unsigned history_dupe=s.card_count;int history_first_hp=s.enemies[0].hp,history_second_hp=s.enemies[1].hp;
 require(history_first_hp==1988&&history_second_hp==2000,"both manual Music Box attacks resolve against their chosen target");hook(WorldRelicHook::AfterAutoPrePlay);
 require(s.card_count==history_dupe+1&&s.world_relics.card_is_dupe[history_dupe]&&s.zones[history_dupe]==Zone::Power,"History Course's replay dupe finishes in non-circulating storage");
 for(const Pile*p:{&s.draw,&s.hand,&s.discard,&s.exhaust})for(unsigned i=0;i<p->count;i++)require(p->ids[i]!=history_dupe,"a completed replay dupe never enters draw, hand, discard or exhaust");
 // History passes a null target to AutoPlay; the replay chooses a living
 // enemy independently of the original manual attack's target.
 int history_first_loss=history_first_hp-s.enemies[0].hp,history_second_loss=history_second_hp-s.enemies[1].hp;
 require((history_first_loss==6&&history_second_loss==0)||(history_first_loss==0&&history_second_loss==6),"History replay attacks exactly one automatically selected enemy once");
 require(relic(WorldRelicHistoryCourse).values[0]==0,"History replay does not replace the next turn's original-card history");save_pending();
 // Earring pays before automatic play. Orbit and resource history see that
 // payment; CardPlay still reports automatic zero-spent metadata.
 reset();add(WorldRelicWhisperingEarring);add(WorldRelicBrilliantScarf);prepared_hand({Bash,DefectHelixDrill,Defend});s.energy=2;orbit_fixture(2);
 hook(WorldRelicHook::AfterAutoPrePlayLate);
 require(s.hand.count==0&&s.energy==0&&s.regent.orbit_remainder[0]==1&&s.defect.energy_spent_turn==3,"Earring awaits Orbit refund before choosing the next affordable card");
 require(s.enemies[0].hp==1984&&s.block==5&&relic(WorldRelicBrilliantScarf).values[0]==0,"Helix Drill sees paid-auto energy history while Scarf sees only automatic plays");save_pending();
 reset();add(WorldRelicWhisperingEarring);add(WorldRelicGalacticDust);prepared_hand({RegentStardust});s.energy=0;s.expansion.stars=5;relic(WorldRelicGalacticDust).values[0]=5;
 s.regent.powers[unsigned(RPower::ChildOfTheStars)]=2;s.regent.power_order[s.regent.power_count++]=uint8_t(RPower::ChildOfTheStars);
 s.regent.powers[unsigned(RPower::BlackHole)]=10;s.regent.power_order[s.regent.power_count++]=uint8_t(RPower::BlackHole);
 s.expansion.after_play_order[0]=uint8_t(PostPlayActor::RegentBlackHole);
 require(validate(s),"paid-auto star fixture registers its Black Hole listener before play");
 hook(WorldRelicHook::AfterAutoPrePlayLate);
 require(s.expansion.stars==0&&s.block==20&&relic(WorldRelicGalacticDust).values[0]==0,"paid-auto star spending triggers both the power and relic listeners");
 require(s.enemies[0].hp+s.enemies[1].hp==3975,"Stardust keeps captured star X and automatic zero-spent metadata suppresses Black Hole");save_pending();
 reset();add(WorldRelicWhisperingEarring);add(WorldRelicChemicalX);add(WorldRelicIntimidatingHelmet);add(WorldRelicIvoryTile);prepared_hand({RegentHeavenlyDrill});s.energy=4;orbit_fixture();
 world_relic_hook(s,WorldRelicHook::AfterAutoPrePlayLate);relic_front();relic_front();
 require(s.energy==0&&relic(WorldRelicWhisperingEarring).values[1]==4,"paid energy and captured X survive before Orbit completes");save_pending();resolve_pending(s);
 require(s.enemies[0].hp==1904&&s.energy==1&&s.block==0,"captured energy X differs from the later automatic ResourceInfo energy value");
 require(s.defect.energy_spent_turn==4&&!world_relic_pending(s),"paid-auto history is recorded once and every continuation finishes");save_pending();
 reward_capacity();
 std::printf("world relic runtime checks: %u; conservative v15 save ceiling %zu / %u\n",checks,EncodedCapacityCeiling,SaveCapacity);return 0;
}
