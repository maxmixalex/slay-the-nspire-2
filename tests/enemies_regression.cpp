#include "core.hpp"
#include "world_rng.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

using namespace spire;
static unsigned checks=0,encounters=0;
static const char*scenario="initial";
static SaveData saved,again;
static State restored,untouched;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"enemy regression:%d [%s] %s\n",__LINE__,scenario,#x);std::exit(1);}}while(0)

static void persist(State&s){
 CHECK(validate(s));CHECK(save(s,saved));CHECK(load(restored,saved.bytes,saved.size));CHECK(save(restored,again));
 CHECK(saved.size==again.size&&!std::memcmp(saved.bytes,again.bytes,saved.size));s=restored;
}
static void same(const State&a,const State&b){
 CHECK(save(a,saved));CHECK(save(b,again));CHECK(saved.size==again.size&&!std::memcmp(saved.bytes,again.bytes,saved.size));
}
static void effect(State&s,XOp op,int a=0,int b=0){queue_extension(s,op,a,b);resolve_pending(s);CHECK(s.phase!=Phase::Error);}
static void power(State&s,int target,EnemyPower p,int amount,int source=-1){effect(s,XOp::EnemyApplyPower,amount,(int)p+80*(target+1)+720*(source+1));}
static int alive(const State&s,EnemyId kind){
 for(int i=0;i<s.enemy_count;++i)if(s.enemy_world.models[i].kind==kind&&enemy_allow_target(s,i))return i;return -1;
}
static unsigned count(const State&s,EnemyId kind){
 unsigned n=0;for(int i=0;i<s.enemy_count;++i)if(s.enemy_world.models[i].kind==kind&&enemy_allow_target(s,i))++n;return n;
}
static int add_card(State&s,CardKind kind,int pile=1){int id=s.card_count;effect(s,XOp::EnemyGenerateCard,kind,1+1024*pile);CHECK(s.card_count==id+1);return id;}
static State fight(EncounterId encounter,unsigned ascension=0,bool special_deck=false){
 State s{};new_run(s,0x41cef1ea,false);s.phase=Phase::Combat;s.current_node=1;s.floor=s.turn=1;
 s.relics=s.relic_count=s.relic_flash=0;s.hp=s.max_hp=999;s.energy=99;s.playing=-1;
 if(special_deck){
  s.deck[0]={DefectGeneticAlgorithm,1};s.defect.genetic_deck[0]=37;
  // Permanent cost offsets belong to the original enchantment that created
  // them; an untagged -1 offset is deliberately rejected by the save codec.
  CHECK(event_card_enchant(s,0,EventEnchantment::TezcatarasEmber,1));
 }
 CHECK(encounter_setup(s,encounter,ascension));
 // Construct real instances after initial enemy powers, just as enter_combat
 // does. This fixture isolates combat from event/map room prerequisites.
 for(unsigned i=0;i<s.deck_count;++i){int id=add_card(s,s.deck[i].kind,0);s.cards[id].upgraded=s.deck[i].upgraded;notify_deck_card_created(s,id,i);}
 effect(s,XOp::EnemySideHook,1,0);draw_cards(s,5);CHECK(s.phase==Phase::Combat);persist(s);return s;
}
static void choose_pending(State&s){
 for(unsigned steps=0;steps<32&&choice_phase(s.phase);++steps){
  persist(s);
  if(s.phase==Phase::ExtendedChoice){
   const auto&p=choice_pile(s);uint16_t ids[MaxHand]{};unsigned n=choice_min(s);CHECK(n<=MaxHand&&n<=p.count);
   for(unsigned i=0;i<n;++i)ids[i]=p.ids[i];
   if(s.expansion.choice==XChoice::EnemyKnowledge)ids[0]=MaxCards+1;
   CHECK(choose_extended(s,ids,n));
  }else if(s.phase==Phase::DiscardChoice){uint16_t ids[MaxHand];for(unsigned i=0;i<s.discard_required;++i)ids[i]=s.hand.ids[i];CHECK(choose_discard(s,ids,s.discard_required));}
  else if(s.phase==Phase::ExhaustChoice)CHECK(choose_exhaust(s,0));
  else if(s.phase==Phase::ExhaustManyChoice)CHECK(choose_exhaust_many(s,nullptr,0));
  else if(s.phase==Phase::ReturnChoice)CHECK(choose_return_to_draw(s,s.discard.ids[0]));
  else CHECK(false);
 }
 CHECK(!choice_phase(s.phase)&&s.phase!=Phase::Error);
}
static void quiet_turn(State&s){
 CHECK(s.phase==Phase::Combat);s.hp=s.max_hp;s.block=9999;CHECK(end_turn(s));choose_pending(s);persist(s);
}
static void move(State&s,int actor,int node){
 CHECK(actor>=0&&actor<s.enemy_count);s.enemy_world.models[actor].node=(uint8_t)node;
 queue_extension(s,XOp::EnemyMove,actor,node);queue_extension(s,XOp::EnemyMoveFinish,actor,node);resolve_pending(s);CHECK(s.phase!=Phase::Error);
}
static void kill(State&s,int actor){CHECK(actor>=0);s.enemies[actor].block=0;damage_unpowered(s,actor,9999);resolve_pending(s);CHECK(s.phase!=Phase::Error);}
static void cast(State&s,CardKind kind,int target=0){
 while(s.hand.count==MaxHand){int id=s.hand.ids[--s.hand.count];s.discard.ids[s.discard.count++]=(uint16_t)id;s.zones[id]=Zone::Discard;}
 int id=add_card(s,kind);s.energy=99;int at=-1;for(unsigned i=0;i<s.hand.count;++i)if(s.hand.ids[i]==id)at=(int)i;
 CHECK(at>=0&&play(s,at,target));choose_pending(s);
}
static void zero_weight_branch_and_gold_bits(){
 scenario="Flyconid zero-weight branch preserves original fallback and RNG";
 auto s=fight(EncounterId::FlyconidNormal);int t=alive(s,EnemyId::Flyconid);CHECK(t>=0);
 auto&m=s.enemy_world.models[t];m.node=m.previous=2;m.age=4;m.flags|=EnemyActed;m.history_count=4;
 const uint8_t history[]={2,0,1,2};std::memcpy(m.history,history,sizeof history);
 for(unsigned i=0;i<3;++i)m.seen_moves[i]=1;
 auto expected=world_rng(s,WorldRng::MonsterAi);expected.next();
 effect(s,XOp::EnemySideHook,1,0);
 CHECK(m.node==0&&m.history_count==5&&m.history[4]==0);
 CHECK(!std::memcmp(&world_rng(s,WorldRng::MonsterAi),&expected,sizeof expected));persist(s);

 scenario="native binary32 escape gold proportions preserve every rounding boundary";
 const uint32_t cards_rng=s.rng_cards,legacy_world_rng=s.rng_world;
 reference::Random streams[WorldRngCount];std::memcpy(streams,s.world.rng,sizeof streams);
 for(unsigned spawned=1;spawned<=EnemyKinds;++spawned)for(unsigned escaped=0;escaped<=spawned;++escaped){
  s.enemy_world.spawned_model_count=(uint16_t)spawned;s.enemy_world.escaped_count=(uint16_t)escaped;
  // Volatile stores require the division and subtraction to round separately
  // to binary32, matching the original C# expression on all host compilers.
  volatile float quotient=float(escaped)/float(spawned);
  volatile float proportion=1.0f-quotient;float value=proportion;uint32_t bits=0;
  static_assert(sizeof value==sizeof bits,"binary32 host float required");std::memcpy(&bits,&value,sizeof bits);
  CHECK(encounter_gold_proportion_bits(s)==bits);
  if(escaped==spawned)CHECK(bits==0);
 }
 s.enemy_world.spawned_model_count=0;s.enemy_world.escaped_count=0;CHECK(encounter_gold_proportion_bits(s)==0x3f800000U);
 s.enemy_world.encounter=EncounterId::GremlinMercNormal;auto&fat=s.enemy_world.models[0];fat.kind=EnemyId::FatGremlin;
 fat.flags=EnemyPresent;fat.stolen_gold=37;CHECK(encounter_gold_proportion_bits(s)==0x3f800000U);
 fat.flags|=EnemyEscaped;fat.stolen_gold=0;CHECK(encounter_gold_proportion_bits(s)==0x3f000000U);
 fat.stolen_gold=37;CHECK(encounter_gold_proportion_bits(s)==0);
 CHECK(s.rng_cards==cards_rng&&s.rng_world==legacy_world_rng&&!std::memcmp(streams,s.world.rng,sizeof streams));
}
static void metadata_and_all_encounters(){
 scenario="model and encounter lookup";CHECK(EnemyKinds==109&&EncounterKinds==89);
 for(unsigned i=1;i<EnemyKinds;++i){const auto*d=enemy_definition(EnemyId(i));CHECK(d&&d->key&&d->model);CHECK(enemy_id(d->key)==EnemyId(i)&&enemy_id(d->model)==EnemyId(i));}
 CHECK(!enemy_definition(EnemyId(255)));CHECK(encounter_id("missing")==EncounterId::None);
 CHECK(encounter_min_gold(EncounterId::NibbitsNormal)==10&&encounter_max_gold(EncounterId::NibbitsNormal)==20);
 CHECK(encounter_min_gold(EncounterId::EntomancerElite,3)==26&&encounter_max_gold(EncounterId::EntomancerElite,3)==33);
 CHECK(encounter_min_gold(EncounterId::AeonglassBoss,3)==75&&encounter_max_gold(EncounterId::AeonglassBoss)==100);
 CHECK(encounter_min_gold(EncounterId::FakeMerchantEventEncounter,10)==300);
 CHECK(!encounter_rewards_allowed(EncounterId::BattlewornDummyEventV1Encounter));
 // The pinned normal pair deliberately inherits the empty Tags collection;
 // only the solitary weak encounter declares EncounterTag.Nibbit.
 CHECK(!encounter_shares_tags(EncounterId::NibbitsNormal,EncounterId::NibbitsWeak));
 CHECK(encounter_shares_tags(EncounterId::BowlbugsNormal,EncounterId::BowlbugsWeak));
 for(unsigned region=0;region<4;++region)for(auto type:{EncounterClass::Weak,EncounterClass::Normal,EncounterClass::Elite,EncounterClass::Boss}){
  EncounterId pool[EncounterKinds];unsigned n=encounter_pool(region,type,pool,EncounterKinds);CHECK(n>0&&n<EncounterKinds);
  for(unsigned i=0;i<n;++i){CHECK(encounter_definition(pool[i])->type==type);for(unsigned j=0;j<i;++j)CHECK(pool[i]!=pool[j]);}
 }
 // Every non-display encounter executes through the public full-turn driver at
 // ordinary and deadly ascension. Saves cover the mutable move/RNG state, not
 // merely the existence of registry entries.
 for(unsigned id=1;id<EncounterKinds;++id){auto encounter=EncounterId(id);const auto*d=encounter_definition(encounter);CHECK(d&&encounter_id(d->key)==encounter);
  if(d->type==EncounterClass::Display)continue;scenario=d->key;
  for(unsigned ascension:{0u,9u}){
   auto s=fight(encounter,ascension);++encounters;CHECK(s.enemy_count>0&&s.enemy_count<=MaxEnemies);
   for(int i=0;i<s.enemy_count;++i){CHECK(s.enemy_world.models[i].history_count==1);CHECK(enemy_intent(s,i).move_key[0]);}
   for(unsigned turn=0;turn<10&&s.phase==Phase::Combat;++turn){
    State continued=s;quiet_turn(s);quiet_turn(continued);same(s,continued);
   }
   CHECK(s.phase==Phase::Combat||s.phase==Phase::Reward||s.phase==Phase::Dead);
  }
 }
 CHECK(encounters==174);
}
static void caps_and_waking(){
 scenario="extra player turn keeps the planned intent and move RNG";
 auto extra=fight(EncounterId::NibbitsNormal);quiet_turn(extra);
 EnemyModelState planned[MaxEnemies];for(int t=0;t<extra.enemy_count;++t)planned[t]=extra.enemy_world.models[t];
 effect(extra,XOp::EnemySideHook,2,0);
 for(int t=0;t<extra.enemy_count;++t){const auto&a=planned[t];const auto&b=extra.enemy_world.models[t];
  CHECK(a.node==b.node&&a.previous==b.previous&&a.history_count==b.history_count);
  CHECK(!std::memcmp(&a.rng,&b.rng,sizeof a.rng));
 }persist(extra);
 scenario="hard caps and side-scoped loss";
 auto s=fight(EncounterId::ExoskeletonsWeak);int t=alive(s,EnemyId::Exoskeleton),before=s.enemies[t].hp;
 damage_unpowered(s,t,100);resolve_pending(s);CHECK(s.enemies[t].hp==before-9);persist(s);
 s=fight(EncounterId::SkulkingColonyElite);t=alive(s,EnemyId::SkulkingColony);before=s.enemies[t].hp;
 damage_unpowered(s,t,50);damage_unpowered(s,t,50);resolve_pending(s);CHECK(s.enemies[t].hp==before-20);
 effect(s,XOp::EnemySideHook,0,0);damage_unpowered(s,t,50);resolve_pending(s);CHECK(s.enemies[t].hp==before-40);persist(s);
 s=fight(EncounterId::InkletsNormal);t=alive(s,EnemyId::Inklet);before=s.enemies[t].hp;
 damage_unpowered(s,t,8);resolve_pending(s);CHECK(s.enemies[t].hp==before-1&&!enemy_power_value(s,t,EnemyPower::Slippery));
 scenario="Lagavulin wakes from actual HP damage";s=fight(EncounterId::LagavulinMatriarchBoss);t=0;
 CHECK(s.enemies[t].block==12&&enemy_power_value(s,t,EnemyPower::Asleep)==3);
 damage_unpowered(s,t,12);resolve_pending(s);CHECK(enemy_power_value(s,t,EnemyPower::Asleep)==3);
 damage_unpowered(s,t,1);resolve_pending(s);CHECK(!enemy_power_value(s,t,EnemyPower::Asleep)&&!enemy_power_value(s,t,EnemyPower::Plating));
 CHECK(enemy_intent(s,t).flags==EnemyIntentStun);persist(s);quiet_turn(s);CHECK(s.phase==Phase::Combat&&s.enemy_world.models[t].node==1);
}
static void death_replacement_and_revival(){
 scenario="Axebot Stock creates three distinct combat identities";auto s=fight(EncounterId::AxebotsNormal);
 unsigned identity=s.enemy_world.models[0].combat_id;
 for(int remaining=1;remaining>=0;--remaining){kill(s,alive(s,EnemyId::Axebot));int t=alive(s,EnemyId::Axebot);CHECK(t>=0&&s.phase==Phase::Combat);
  CHECK(s.enemy_world.models[t].combat_id>identity);identity=s.enemy_world.models[t].combat_id;
  CHECK(enemy_power_value(s,t,EnemyPower::Stock)==remaining);CHECK(s.enemies[t].max_hp>=90-10*remaining&&s.enemies[t].max_hp<=98-10*remaining);persist(s);
 }kill(s,alive(s,EnemyId::Axebot));CHECK(s.phase==Phase::Reward);

 scenario="Phrog death summons four stunned Wrigglers without extra turns";s=fight(EncounterId::PhrogParasiteElite);
 s.thorns=9999;quiet_turn(s);quiet_turn(s);CHECK(s.phase==Phase::Combat&&count(s,EnemyId::Wriggler)==4);
 for(int t=0;t<s.enemy_count;++t)if(enemy_allow_target(s,t)){CHECK(s.enemy_world.models[t].kind==EnemyId::Wriggler);CHECK(s.enemy_world.models[t].age==0);}
 persist(s);

 scenario="Test Subject retains combat across both transformations";s=fight(EncounterId::TestSubjectBoss);
 for(int form=1;form<=2;++form){kill(s,0);CHECK(s.phase==Phase::Combat&&!enemy_allow_target(s,0)&&enemy_keeps_combat_alive(s));persist(s);
  move(s,0,0);CHECK(s.enemies[0].hp==100*(form+1)&&s.enemy_world.models[0].data[0]==form);
  CHECK(enemy_power_value(s,0,form==1?EnemyPower::PainfulStabs:EnemyPower::Nemesis)==1);persist(s);
 }CHECK(!enemy_power_value(s,0,EnemyPower::Adaptable));
 effect(s,XOp::EnemySideHook,0,1);CHECK(enemy_power_value(s,0,EnemyPower::Intangible)==1);
 effect(s,XOp::EnemySideHook,0,1);CHECK(!enemy_power_value(s,0,EnemyPower::Intangible));kill(s,0);CHECK(s.phase==Phase::Reward);

 scenario="Decimillipede segments reattach only while another segment lives";s=fight(EncounterId::DecimillipedeElite);
 CHECK(s.enemy_count==3);for(int t=0;t<3;++t)CHECK(!(s.enemies[t].max_hp&1));CHECK(!enemy_death_is_fatal(s,0));
 kill(s,0);CHECK(s.phase==Phase::Combat&&!enemy_allow_target(s,0));persist(s);move(s,0,4);CHECK(s.enemies[0].hp==25);
 kill(s,0);kill(s,1);CHECK(s.phase==Phase::Combat&&enemy_death_is_fatal(s,2));kill(s,2);CHECK(s.phase==Phase::Reward);

 scenario="Illusion revive and minion-only victory";s=fight(EncounterId::FogmogNormal);move(s,alive(s,EnemyId::Fogmog),0);
 int eye=alive(s,EnemyId::EyeWithTeeth);CHECK(eye>=0);kill(s,eye);CHECK(s.phase==Phase::Combat&&(s.enemy_world.models[eye].flags&EnemyDown));persist(s);
 move(s,eye,30);CHECK(s.enemies[eye].hp==6&&enemy_allow_target(s,eye));kill(s,alive(s,EnemyId::Fogmog));CHECK(s.phase==Phase::Reward);

 scenario="Waterfall Giant has true large HP before saved explosion";s=fight(EncounterId::WaterfallGiantBoss);power(s,0,EnemyPower::SteamEruption,12,0);kill(s,0);
 CHECK(s.phase==Phase::Combat&&enemy_infinite_hp(s,0)&&s.enemies[0].hp==999999999&&s.enemies[0].max_hp==999999999);persist(s);
 move(s,0,6);CHECK(!enemy_power_value(s,0,EnemyPower::SteamEruption)&&s.enemy_world.models[0].data[1]==12);persist(s);
 move(s,0,7);CHECK(s.phase==Phase::Reward);
}
static void death_power_cleanup(){
 // The original Illusion policy removes non-temporary debuffs regardless of
 // which subsystem stores them. Native buffs and temporary Strength wrappers
 // survive; Adaptable/Reattach/SteamEruption preserve only their own powers.
 const NDebuff cleared[]={NDebuff::Weak,NDebuff::Vulnerable,NDebuff::Poison,
  NDebuff::Beetle,NDebuff::Demise,NDebuff::Doom,NDebuff::Strangle,
  NDebuff::Debilitate,NDebuff::Hang,NDebuff::Oblivion,NDebuff::SicEm,NDebuff::Conqueror};
 const auto mark=[&](State&s,int t){
  for(auto p:cleared)effect(s,XOp::NApplyCopiedDebuff,int(p)*MaxEnemies+t,1);
  power(s,t,EnemyPower::Strength,9);
  effect(s,XOp::NTempStrength,int(NTempStrength::DarkShackles)*MaxEnemies+t,3);
  effect(s,XOp::NTempStrength,int(NTempStrength::PiercingWail)*MaxEnemies+t,2);
  power(s,t,EnemyPower::TemporaryStrength,2);
  power(s,t,EnemyPower::Artifact,2);power(s,t,EnemyPower::Thorns,2);
  CHECK(s.enemies[t].strength==6&&s.expansion.restore_strength[t]==5);
  persist(s);
 };
 const auto cleaned=[&](const State&s,int t){
  CHECK(!s.enemies[t].weak&&!s.enemies[t].vulnerable&&!s.enemies[t].poison);
  CHECK(!s.expansion.beetle[t]&&!s.expansion.demise[t]&&!s.expansion.doom[t]);
  CHECK(!s.silent.strangle[t]&&!s.regent.conqueror[t]);
  CHECK(!s.necro.debilitate[t]&&!s.necro.hang[t]&&!s.necro.oblivion[t]&&!s.necro.sic_em[t]);
  for(unsigned f=0;f<=MaxAutoFrames;++f){CHECK(!s.silent.strangle_before[f][t]);CHECK(!s.necro.oblivion_before[f][t]);}
  for(unsigned i=0;i<s.necro.debuff_count[t];++i){const auto p=NDebuff(s.necro.debuff_order[t][i]-1);
   CHECK(p==NDebuff::Strength||p>=NDebuff::DarkShackles);
  }
 };
 for(auto encounter:{EncounterId::FogmogNormal,EncounterId::TheObscuraNormal}){
  scenario=encounter==EncounterId::FogmogNormal?"EyeWithTeeth death clears all shared debuffs":"Parafright death clears all shared debuffs";
  auto s=fight(encounter);move(s,0,0);
  const auto kind=encounter==EncounterId::FogmogNormal?EnemyId::EyeWithTeeth:EnemyId::Parafright;
  const int t=alive(s,kind);CHECK(t>=0);mark(s,t);
  const uint16_t identity=s.enemy_world.models[t].combat_id;
  reference::Random streams[WorldRngCount];std::memcpy(streams,s.world.rng,sizeof streams);
  const uint32_t cards_rng=s.rng_cards,legacy_world_rng=s.rng_world;
  s.enemies[t].block=0;damage_unpowered(s,t,9999);
  CHECK(s.enemy_world.models[t].flags&EnemyDying);
  // A save made between lethal damage and its queued death resumes the same
  // cleanup, without consuming gameplay randomness or replacing the actor.
  persist(s);State continued=s;resolve_pending(s);resolve_pending(continued);same(s,continued);
  CHECK(s.phase==Phase::Combat&&(s.enemy_world.models[t].flags&EnemyDown));cleaned(s,t);
  CHECK(s.enemy_world.models[t].combat_id==identity&&s.enemies[t].strength==6);
  CHECK(s.expansion.restore_strength[t]==5&&s.necro.temp_strength[t][int(NTempStrength::DarkShackles)]==3&&s.necro.temp_strength[t][int(NTempStrength::PiercingWail)]==2);
  CHECK(enemy_power_value(s,t,EnemyPower::TemporaryStrength)==2);
  CHECK(enemy_power_value(s,t,EnemyPower::Artifact)==2&&enemy_power_value(s,t,EnemyPower::Thorns)==2);
  CHECK(enemy_power_value(s,t,EnemyPower::Illusion)==1&&enemy_power_value(s,t,EnemyPower::Minion)==1);
  CHECK(s.rng_cards==cards_rng&&s.rng_world==legacy_world_rng&&!std::memcmp(streams,s.world.rng,sizeof streams));persist(s);
  move(s,t,30);CHECK(enemy_allow_target(s,t)&&s.enemies[t].hp==s.enemies[t].max_hp);cleaned(s,t);
  CHECK(s.enemy_world.models[t].combat_id==identity&&s.enemies[t].strength==6);persist(s);
 }
 for(auto encounter:{EncounterId::TestSubjectBoss,EncounterId::DecimillipedeElite,EncounterId::WaterfallGiantBoss}){
  scenario=encounter==EncounterId::TestSubjectBoss?"Adaptable clears temporary Strength on death":
   encounter==EncounterId::DecimillipedeElite?"Reattach clears temporary Strength on death":"SteamEruption clears temporary Strength on death";
  auto s=fight(encounter);const int t=0;
  if(encounter==EncounterId::WaterfallGiantBoss)power(s,t,EnemyPower::SteamEruption,12,t);
  mark(s,t);kill(s,t);CHECK(s.phase==Phase::Combat);cleaned(s,t);
  CHECK(!s.enemies[t].strength&&!s.expansion.restore_strength[t]);
  for(auto amount:s.necro.temp_strength[t])CHECK(!amount);
  CHECK(!enemy_power_value(s,t,EnemyPower::TemporaryStrength));
  CHECK(!enemy_power_value(s,t,EnemyPower::Artifact)&&!enemy_power_value(s,t,EnemyPower::Thorns));persist(s);
  quiet_turn(s);CHECK(!s.enemies[t].strength&&!s.expansion.restore_strength[t]);cleaned(s,t);persist(s);
 }
}
static void last_primary_death_cascade(){
 for(auto encounter:{EncounterId::FogmogNormal,EncounterId::TheObscuraNormal,EncounterId::FabricatorNormal,EncounterId::OvicopterNormal}){
  scenario="last primary death resolves every minion death before rewards";
  auto s=fight(encounter);move(s,0,0);const int melancholy=add_card(s,NecroMelancholy);
  CHECK(gain_world_relic(s,WorldRelicGremlinHorn));resolve_pending(s);s.energy=0;
  unsigned minions=0;for(int t=0;t<s.enemy_count;++t)if(s.enemies[t].hp>0&&enemy_power_value(s,t,EnemyPower::Minion))++minions;
  CHECK(minions==(encounter==EncounterId::FabricatorNormal?2u:encounter==EncounterId::OvicopterNormal?3u:1u));
  const unsigned deaths=s.necro.creature_deaths;
  s.enemies[0].block=0;damage_unpowered(s,0,9999);CHECK(s.enemy_world.models[0].flags&EnemyDying);
  persist(s);State continued=s;resolve_pending(s);resolve_pending(continued);same(s,continued);
  CHECK(s.phase==Phase::Reward&&s.necro.creature_deaths==deaths+1+minions);
  CHECK(s.energy==int(1+minions)&&s.necro.cost_delta[melancholy]==-int(1+minions));
  for(int t=0;t<s.enemy_count;++t){CHECK(s.enemies[t].hp==0);CHECK(!(s.enemy_world.models[t].flags&EnemyDying));
   if(t)CHECK(enemy_power_value(s,t,EnemyPower::Minion)==1);
  }persist(s);
 }
 scenario="a second living primary prevents the minion death cascade";
 auto s=fight(EncounterId::FogmogNormal);move(s,0,0);const int eye=alive(s,EnemyId::EyeWithTeeth);CHECK(eye>=0);
 s.enemy_world.slot_limit=MaxEnemies;effect(s,XOp::EnemySpawn,int(EnemyId::Nibbit),2);
 const int other=alive(s,EnemyId::Nibbit);CHECK(other>=0);const int eye_hp=s.enemies[eye].hp;
 kill(s,0);CHECK(s.phase==Phase::Combat&&s.enemies[eye].hp==eye_hp&&enemy_allow_target(s,eye));persist(s);
 kill(s,other);CHECK(s.phase==Phase::Reward&&s.enemies[eye].hp==0);persist(s);
}
static void death_cascade_suspended_draw(){
 scenario="a minion's death draw suspends the cascade before the remaining minions die";
 auto s=fight(EncounterId::OvicopterNormal);move(s,0,0);const int melancholy=add_card(s,NecroMelancholy);
 CHECK(gain_world_relic(s,WorldRelicGremlinHorn));resolve_pending(s);gain_power(s,XPower::Stratagem,1);s.energy=0;
 // The leader's Horn draw uses the final draw-pile card. The first minion's
 // Horn must then shuffle and suspend for Stratagem with two deaths queued.
 while(s.draw.count>1){int id=s.draw.ids[--s.draw.count];s.discard.ids[s.discard.count++]=(uint16_t)id;s.zones[id]=Zone::Discard;}
 const unsigned deaths=s.necro.creature_deaths;
 damage_unpowered(s,0,9999);resolve_pending(s);
 CHECK(s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Draw);
 CHECK(s.necro.creature_deaths==deaths+2&&s.energy==2&&s.necro.cost_delta[melancholy]==-2);
 unsigned pending_alive=0,pending_dead=0;
 for(int t=0;t<s.enemy_count;++t)if(s.enemy_world.models[t].flags&EnemyDying){if(s.enemies[t].hp>0)++pending_alive;else ++pending_dead;}
 CHECK(pending_alive==2&&pending_dead==1&&enemy_keeps_combat_alive(s));persist(s);
 State continued=s;untouched=s;uint16_t invalid=MaxCards+2;
 CHECK(!choose_extended(s,&invalid,1));CHECK(!std::memcmp(&s,&untouched,sizeof s));
 choose_pending(s);choose_pending(continued);same(s,continued);
 CHECK(s.phase==Phase::Reward&&s.necro.creature_deaths==deaths+4&&s.energy==4&&s.necro.cost_delta[melancholy]==-4);
 for(int t=0;t<s.enemy_count;++t)CHECK(!s.enemies[t].hp&&!(s.enemy_world.models[t].flags&EnemyDying));persist(s);
}
static void stolen_assets_and_timeout(){
 State s{};
 for(unsigned ascension:{0u,8u}){
  scenario=ascension?"Gremlin Surprise preserves A8 creation, HP and add order":"Gremlin Surprise preserves creation, HP and add order";
  s=fight(EncounterId::GremlinMercNormal,ascension);s.gold=99;move(s,0,0);CHECK(s.gold==79);
  // With Niche seed 2 both original rolls select the same HP. Fat has a
  // combat ID already, but is not in the enemy list used for Sneaky's roll.
  world_rng(s,WorldRng::Niche).reset(2);auto expected_rng=world_rng(s,WorldRng::Niche);expected_rng.next();expected_rng.next();
  const unsigned next_id=s.enemy_world.next_creature_id;const int expected_hp=ascension?14:13;
  kill(s,0);int fat=alive(s,EnemyId::FatGremlin),sneaky=alive(s,EnemyId::SneakyGremlin);CHECK(fat>=0&&sneaky>=0);
  CHECK(s.enemies[fat].hp==expected_hp&&s.enemies[fat].max_hp==expected_hp&&s.enemies[sneaky].hp==expected_hp&&s.enemies[sneaky].max_hp==expected_hp);
  const auto&actual_rng=world_rng(s,WorldRng::Niche);CHECK(actual_rng.calls==expected_rng.calls&&actual_rng.good==expected_rng.good);
  for(unsigned i=0;i<4;++i)CHECK(actual_rng.words[i]==expected_rng.words[i]);
  CHECK(s.enemy_world.models[fat].combat_id==next_id&&s.enemy_world.models[sneaky].combat_id==next_id+1&&s.enemy_world.next_creature_id==next_id+2);
  CHECK(s.enemy_world.models[fat].stolen_gold==20);persist(s);
  effect(s,XOp::EnemySideHook,0,0);CHECK(s.enemy_world.turn_count==2&&s.enemy_world.turn_order[0]==sneaky&&s.enemy_world.turn_order[1]==fat);
  CHECK(s.enemy_world.turn_ids[0]==s.enemy_world.models[sneaky].combat_id&&s.enemy_world.turn_ids[1]==s.enemy_world.models[fat].combat_id);persist(s);
  kill(s,fat);CHECK(s.gold==79&&encounter_returned_gold_count(s)==1&&encounter_returned_gold(s,0)==20);persist(s);
 }

 scenario="Hopper returns an optional exact-card reward before deck insertion";s=fight(EncounterId::ThievingHopperWeak,0,true);
 const EventCardFacts enchanted=s.event_cards.deck[0];CHECK(enchanted.energy_offset==-1&&!card_removable(s,s.deck[0],0));
 s.event_cards.deck[0].enchantment=EventEnchantment::None;s.event_cards.deck[0].enchantment_amount=0;CHECK(!validate(s));s.event_cards.deck[0]=enchanted;
 while(s.hand.count){int id=s.hand.ids[--s.hand.count];s.discard.ids[s.discard.count++]=(uint16_t)id;s.zones[id]=Zone::Discard;}
 unsigned deck=s.deck_count;move(s,0,0);CHECK(s.deck_count==deck-1&&s.enemy_world.models[0].captured_card);
 // ThievingHopper filters DeckVersion, rarity and Imbued. Its forced
 // CardPileCmd.RemoveFromDeck path does not exclude Eternal cards.
 CHECK(s.enemy_world.models[0].captured_deck.card.kind==DefectGeneticAlgorithm&&s.enemy_world.models[0].captured_deck.genetic==37);persist(s);
 CHECK(s.enemy_world.models[0].captured_deck.facts.enchantment==EventEnchantment::TezcatarasEmber&&s.enemy_world.models[0].captured_deck.facts.enchantment_amount==1&&s.enemy_world.models[0].captured_deck.facts.energy_offset==-1);
 kill(s,0);CHECK(s.deck_count==deck-1&&encounter_combat_reward_count(s)==1&&!s.enemy_world.models[0].captured_card);
 EventDeckSnapshot returned{};unsigned reward_kind=9;int reward_value=-1;
 CHECK(encounter_combat_reward(s,0,reward_kind,reward_value)&&reward_kind==2);
 CHECK(encounter_combat_reward_card(s,0,returned)&&returned.card.kind==DefectGeneticAlgorithm&&returned.card.upgraded);
 CHECK(returned.facts.enchantment==EventEnchantment::TezcatarasEmber&&returned.facts.enchantment_amount==1);
 CHECK(returned.genetic==37&&returned.facts.energy_offset==-1&&reward_value==card_token(returned.card));
 CHECK(encounter_returned_gold_count(s)==0);persist(s);
 encounter_add_card_reward(s);CHECK(encounter_combat_reward_count(s)==2&&encounter_combat_reward(s,1,reward_kind,reward_value)&&reward_kind==1&&reward_value==3);persist(s);
 s=fight(EncounterId::ThievingHopperWeak);deck=s.deck_count;move(s,0,0);CHECK(s.deck_count==deck-1);persist(s);
 effect(s,XOp::EnemyEscape,0);CHECK(s.deck_count==deck-1&&!s.enemy_world.models[0].captured_card&&encounter_escaped_enemies(s)==1);persist(s);

 scenario="Battleworn Dummy ends after three enemy turns";s=fight(EncounterId::BattlewornDummyEventV3Encounter);
 quiet_turn(s);CHECK(!enemy_encounter_timed_out(s));quiet_turn(s);CHECK(!enemy_encounter_timed_out(s));quiet_turn(s);
 CHECK(s.phase==Phase::Reward&&enemy_encounter_timed_out(s)&&encounter_escaped_enemies(s)==1);persist(s);
}
static void escape_and_real_death_edges(){
 for(unsigned ascension:{0u,9u}){
  scenario="Hopper's complete five-move escape is not an AfterDeath";
  auto s=fight(EncounterId::ThievingHopperWeak,ascension);const int melancholy=add_card(s,NecroMelancholy);
  const unsigned deaths=s.necro.creature_deaths,deck=s.deck_count;
  for(unsigned turn=0;turn<4;++turn)quiet_turn(s);
  CHECK(s.phase==Phase::Combat&&enemy_intent(s,0).flags==EnemyIntentEscape);
  CHECK(s.deck_count==deck-1&&s.enemy_world.models[0].captured_card);
  State continued=s;quiet_turn(s);quiet_turn(continued);same(s,continued);
  CHECK(s.phase==Phase::Reward&&encounter_escaped_enemies(s)==1);
  CHECK(s.necro.creature_deaths==deaths&&s.necro.cost_delta[melancholy]==0);
  CHECK(s.deck_count==deck-1&&!s.enemy_world.models[0].captured_card&&encounter_combat_reward_count(s)==0);
  CHECK(encounter_gold_proportion_bits(s)==0);persist(s);

  scenario="Dummy's real three-turn timeout is not an AfterDeath";
  s=fight(EncounterId::BattlewornDummyEventV3Encounter,ascension);const int dummy_melancholy=add_card(s,NecroMelancholy);
  const unsigned dummy_deaths=s.necro.creature_deaths;
  quiet_turn(s);quiet_turn(s);continued=s;quiet_turn(s);quiet_turn(continued);same(s,continued);
  CHECK(s.phase==Phase::Reward&&enemy_encounter_timed_out(s)&&encounter_escaped_enemies(s)==1);
  CHECK(s.necro.creature_deaths==dummy_deaths&&s.necro.cost_delta[dummy_melancholy]==0);persist(s);

  scenario="Fat Gremlin's saved flee preserves the living teammate and death listeners";
  s=fight(EncounterId::GremlinMercNormal,ascension);const int fat_melancholy=add_card(s,NecroMelancholy);
  CHECK(gain_world_relic(s,WorldRelicGremlinHorn));resolve_pending(s);s.gold=99;
  move(s,0,0);kill(s,0);quiet_turn(s);
  const int fat=alive(s,EnemyId::FatGremlin),sneaky=alive(s,EnemyId::SneakyGremlin);CHECK(fat>=0&&sneaky>=0);
  CHECK(enemy_intent(s,fat).flags==EnemyIntentEscape);
  const unsigned fat_deaths=s.necro.creature_deaths;const int old_delta=s.necro.cost_delta[fat_melancholy],old_energy=s.energy;
  const unsigned old_identity=s.enemy_world.models[fat].combat_id,slot=s.enemy_world.models[fat].slot;
  queue_extension(s,XOp::EnemyMove,fat,1);queue_extension(s,XOp::EnemyMoveFinish,fat,1);
  persist(s);continued=s;resolve_pending(s);resolve_pending(continued);same(s,continued);
  CHECK(s.phase==Phase::Combat&&enemy_allow_target(s,sneaky)&&!enemy_allow_target(s,fat));
  CHECK(encounter_escaped_enemies(s)==1&&s.necro.creature_deaths==fat_deaths);
  CHECK(s.necro.cost_delta[fat_melancholy]==old_delta&&s.energy==old_energy);
  CHECK(s.gold==79&&encounter_returned_gold_count(s)==0&&encounter_gold_proportion_bits(s)==0);persist(s);
  continued=s;effect(s,XOp::EnemyEscape,fat);same(s,continued);

  // Exercise the generic bounded-slot reuse contract after the real flee.
  // This extra spawn is a controlled fixture, not Gremlin Merc's original AI.
  scenario="reusing an escaped slot does not suppress the new actor's real death";
  effect(s,XOp::EnemySpawn,int(EnemyId::Nibbit),int(slot));
  CHECK(alive(s,EnemyId::Nibbit)==fat&&s.enemy_world.models[fat].combat_id>old_identity);
  CHECK(s.necro.creature_deaths==fat_deaths&&s.necro.cost_delta[fat_melancholy]==old_delta);persist(s);
  damage_unpowered(s,fat,9999);persist(s);continued=s;resolve_pending(s);resolve_pending(continued);same(s,continued);
  CHECK(s.phase==Phase::Combat&&s.necro.creature_deaths==fat_deaths+1&&s.energy==old_energy+1);
  CHECK(s.necro.cost_delta[fat_melancholy]==old_delta-1&&encounter_escaped_enemies(s)==1);persist(s);
  continued=s;effect(s,XOp::EnemyDie,fat);same(s,continued);
 }

 scenario="Escape cannot cancel an already pending real death or its stolen-gold reward";
 auto s=fight(EncounterId::GremlinMercNormal);const int melancholy=add_card(s,NecroMelancholy);
 CHECK(gain_world_relic(s,WorldRelicGremlinHorn));resolve_pending(s);s.gold=99;move(s,0,0);kill(s,0);
 const int fat=alive(s,EnemyId::FatGremlin);CHECK(fat>=0);const unsigned deaths=s.necro.creature_deaths;const int delta=s.necro.cost_delta[melancholy],energy=s.energy;
 damage_unpowered(s,fat,9999);CHECK(s.enemies[fat].hp==0&&(s.enemy_world.models[fat].flags&EnemyDying));
 queue_extension(s,XOp::EnemyEscape,fat,0,true);persist(s);State continued=s;resolve_pending(s);resolve_pending(continued);same(s,continued);
 CHECK(s.phase==Phase::Combat&&!encounter_escaped_enemies(s));
 CHECK(s.necro.creature_deaths==deaths+1&&s.necro.cost_delta[melancholy]==delta-1&&s.energy==energy+1);
 CHECK(encounter_returned_gold_count(s)==1&&encounter_returned_gold(s,0)==20);persist(s);

 scenario="Escape cannot remove a dead Test Subject awaiting revival";
 s=fight(EncounterId::TestSubjectBoss);kill(s,0);CHECK(s.phase==Phase::Combat&&!enemy_allow_target(s,0));
 continued=s;effect(s,XOp::EnemyEscape,0);same(s,continued);
 CHECK(!encounter_escaped_enemies(s)&&enemy_keeps_combat_alive(s));move(s,0,0);CHECK(s.enemies[0].hp==200);persist(s);
}
static void choices_statuses_and_eight_slots(){
 scenario="Knowledge choices suspend, reject invalid input atomically, and resume";auto s=fight(EncounterId::KnowledgeDemonBoss);
 for(int stage=0;stage<3;++stage){effect(s,XOp::EnemyKnowledgeChoice,0);CHECK(s.phase==Phase::ExtendedChoice&&choice_min(s)==1&&choice_max(s)==1);persist(s);
  CHECK(choice_card(s,MaxCards).kind==EventCardDisintegration);untouched=s;uint16_t invalid=MaxCards+2;CHECK(!choose_extended(s,&invalid,1));CHECK(!std::memcmp(&s,&untouched,sizeof s));
  s.expansion.offers[0].kind=Strike;CHECK(!validate(s));s=untouched;
  s.expansion.offers[1].upgraded=1;CHECK(!validate(s));s=untouched;
  s.expansion.choices.ids[1]=MaxCards+2;CHECK(!validate(s));s=untouched;
  s.enemy_world.choice_stage=3;CHECK(!validate(s));s=untouched;
  s.enemy_world.models[0].data[0]=(int16_t)(stage+1);CHECK(!validate(s));s=untouched;
  s.enemy_world.models[0].kind=EnemyId::Nibbit;CHECK(!validate(s));s=untouched;
  uint16_t token=MaxCards+1;CHECK(choose_extended(s,&token,1));CHECK(s.enemy_world.models[0].data[0]==stage+1);persist(s);
 }CHECK(enemy_power_value(s,-1,EnemyPower::MindRot)==1&&enemy_power_value(s,-1,EnemyPower::Sloth)==3&&enemy_power_value(s,-1,EnemyPower::WasteAway)==1);

 scenario="Wither upgrades remain attached to each instance";s=fight(EncounterId::AeonglassBoss);int wither=add_card(s,EventCardWither);
 effect(s,XOp::EnemyActionSpecial,0,0);CHECK(s.enemy_world.card_value[wither]==1);int later=add_card(s,EventCardWither,2);CHECK(s.enemy_world.card_value[later]==1);persist(s);
 scenario="Frantic Escape adds Sandpit time and increases only its own cost";s=fight(EncounterId::TheInsatiableBoss);power(s,0,EnemyPower::Sandpit,4,0);
 int frantic=add_card(s,EventCardFranticEscape);int original_cost=cost(s,frantic);effect(s,XOp::EnemyFranticEscape,frantic);
 CHECK(enemy_power_value(s,0,EnemyPower::Sandpit)==5&&cost(s,frantic)==original_cost+1);persist(s);

 scenario="eight physical enemies receive all-target effects and serialize";s=fight(EncounterId::CubexConstructNormal);s.enemy_world.slot_limit=MaxEnemies;
 for(int slot=1;slot<MaxEnemies;++slot)effect(s,XOp::EnemySpawn,(int)EnemyId::Nibbit,slot);
 CHECK(s.enemy_count==8);persist(s);int hp[MaxEnemies];for(int t=0;t<MaxEnemies;++t)hp[t]=s.enemies[t].hp;
 cast(s,Thunderclap);for(int t=1;t<MaxEnemies;++t)CHECK(s.enemies[t].hp==hp[t]-4&&s.enemies[t].vulnerable==1);persist(s);
 untouched=s;s.enemy_world.models[7].slot=s.enemy_world.models[0].slot;CHECK(!validate(s));s=untouched;
 s.enemy_world.models[7].combat_id=s.enemy_world.models[0].combat_id;CHECK(!validate(s));s=untouched;
 s.enemy_world.returned_gold_count=MaxEnemyExtraRewards+1;CHECK(!validate(s));s=untouched;
 s.enemy_world.models[7].captured_card=1;CHECK(!validate(s));s=untouched;persist(s);
}
static int visual_fact(const State&s,int target,const char*key){int value=-9;CHECK(enemy_visual_fact(s,target,key,value));return value;}
static void visual_predicates_and_unpowered_death_block(){
 scenario="original intent kinds and ascension-specific status card counts";
 auto intent_state=fight(EncounterId::ThievingHopperWeak);
 intent_state.enemy_world.models[0].node=0;
 CHECK(enemy_intent(intent_state,0).flags==(EnemyIntentAttack|EnemyIntentCardDebuff));
 intent_state=fight(EncounterId::ShrinkerBeetleWeak);
 intent_state.enemy_world.models[0].node=0;
 CHECK(enemy_intent(intent_state,0).flags==EnemyIntentDebuffStrong);
 for(unsigned ascension:{0u,9u}){
  intent_state=fight(EncounterId::AeonglassBoss,ascension);intent_state.enemy_world.models[0].node=2;
  CHECK(enemy_intent(intent_state,0).status_cards==(ascension?2:1));
  intent_state=fight(EncounterId::TestSubjectBoss,ascension);intent_state.enemy_world.models[0].node=6;
  CHECK(!strcmp(enemy_intent(intent_state,0).move_key,"BURNING_GROWL_MOVE"));
  CHECK(enemy_intent(intent_state,0).status_cards==(ascension?5:3));
 }
 scenario="Paper Krane weak intent and attack precede Debilitate amplification";
 auto s=fight(EncounterId::NibbitsNormal);CHECK(gain_world_relic(s,WorldRelicPaperKrane));resolve_pending(s);
 power(s,0,EnemyPower::Weak,2);s.enemy_world.models[0].node=0;s.block=0;
 CHECK(enemy_intent(s,0).damage==7);int before=s.hp;move(s,0,0);CHECK(before-s.hp==7);
 power(s,0,EnemyPower::Debilitate,2);CHECK(enemy_intent(s,0).damage==2);before=s.hp;move(s,0,0);CHECK(before-s.hp==2);persist(s);

 scenario="Crab Rage adds unpowered block despite Dexterity and Frail";
 s=fight(EncounterId::KaiserCrabBoss);int rocket=alive(s,EnemyId::Rocket),crusher=alive(s,EnemyId::Crusher);
 CHECK(rocket>=0&&crusher>=0);power(s,rocket,EnemyPower::Dexterity,7);power(s,rocket,EnemyPower::Frail,2);
 s.enemies[rocket].block=0;kill(s,crusher);CHECK(s.enemies[rocket].block==99);
 CHECK(visual_fact(s,rocket,"kaiser_left_dead")==1&&visual_fact(s,rocket,"kaiser_right_dead")==0);persist(s);

 scenario="Cubex does not leave its burrow merely because charge is intended";
 s=fight(EncounterId::CubexConstructNormal);CHECK(visual_fact(s,0,"IsBurrowed")==1&&visual_fact(s,0,"IsCharging")==0);
 CHECK(s.enemy_world.models[0].node==0);move(s,0,0);CHECK(visual_fact(s,0,"IsBurrowed")==0&&visual_fact(s,0,"IsCharging")==1);persist(s);
 int value=77;CHECK(!enemy_visual_fact(s,0,"unknown_predicate",value)&&value==77);

 scenario="Crossbow reload state changes after the move and survives a save";
 s=fight(EncounterId::RubyRaidersNormal);int crossbow=alive(s,EnemyId::CrossbowRubyRaider);CHECK(crossbow>=0);
 CHECK(visual_fact(s,crossbow,"IsCrossbowReloaded")==0);move(s,crossbow,1);CHECK(visual_fact(s,crossbow,"IsCrossbowReloaded")==1);persist(s);
 move(s,crossbow,0);CHECK(visual_fact(s,crossbow,"IsCrossbowReloaded")==0);persist(s);

 scenario="Death keeps the puffed branch after powers are removed";
 s=fight(EncounterId::FuzzyWurmCrawlerWeak);move(s,0,2);CHECK(visual_fact(s,0,"IsPuffed")==1);kill(s,0);
 CHECK(visual_fact(s,0,"IsPuffed")==1);persist(s);

 scenario="Thorned death is a Boolean fact even when the power amount is five";
 s=fight(EncounterId::ToadpolesWeak);power(s,0,EnemyPower::Thorns,5);CHECK(visual_fact(s,0,"Thorns")==1);kill(s,0);
 CHECK(enemy_power_value(s,0,EnemyPower::Thorns)==0&&visual_fact(s,0,"Thorns")==1);persist(s);

 scenario="Underground death retains the Burrowed animation predicate";
 s=fight(EncounterId::TunnelerWeak);power(s,0,EnemyPower::Burrowed,1);kill(s,0);
 CHECK(enemy_power_value(s,0,EnemyPower::Burrowed)==0&&visual_fact(s,0,"Burrowed")==1);persist(s);
 untouched=s;s.enemy_world.models[0].data[8]=4;CHECK(!validate(s));s=untouched;
 s.enemy_world.models[0].data[9]=5;CHECK(!validate(s));s=untouched;persist(s);

 scenario="Lagavulin shell eyes remain awake after healing above half HP";
 s=fight(EncounterId::LagavulinMatriarchBoss);CHECK(visual_fact(s,0,"IsShellAwake")==0);
 s.enemies[0].block=0;damage_unpowered(s,0,s.enemies[0].hp-s.enemies[0].max_hp/2);resolve_pending(s);
 CHECK(visual_fact(s,0,"IsAwake")==1&&visual_fact(s,0,"IsShellAwake")==1);
 effect(s,XOp::EnemyHeal,999,0);CHECK(visual_fact(s,0,"IsShellAwake")==1);persist(s);

 scenario="Vantom tail charge persists between moves independently of its intent";
 s=fight(EncounterId::VantomBoss);CHECK(visual_fact(s,0,"tail_charge")==1);
 const int charge[]={2,3,0,1};
 for(int node=0;node<4;++node){s.block=9999;move(s,0,node);CHECK(visual_fact(s,0,"tail_charge")==charge[node]);persist(s);}

 scenario="Sewer Clam growth follows completed casts and has a finite cap";
 s=fight(EncounterId::SewerClamNormal);CHECK(visual_fact(s,0,"coral_growth_step")==0);
 for(int cast=1;cast<=9;++cast){move(s,0,0);CHECK(visual_fact(s,0,"coral_growth_step")== (cast<7?cast:7));persist(s);}
}

int main(){
 zero_weight_branch_and_gold_bits();
 death_power_cleanup();
 last_primary_death_cascade();
 death_cascade_suspended_draw();
 escape_and_real_death_edges();
 visual_predicates_and_unpowered_death_block();
 metadata_and_all_encounters();caps_and_waking();death_replacement_and_revival();stolen_assets_and_timeout();choices_statuses_and_eight_slots();
 std::printf("original enemy checks: %u; executed encounter/ascension variants: %u\n",checks,encounters);return 0;
}
