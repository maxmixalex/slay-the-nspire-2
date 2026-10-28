// Encounter construction preserves original composition and Niche RNG draw order.
#include "enemy_encounter_definitions.inc"
static bool enemy_has_named_slots(const State&s){return unsigned(s.enemy_world.encounter)<EncounterKinds&&encounter_metadata[unsigned(s.enemy_world.encounter)].slots!=0;}
const EncounterDefinition* encounter_definition(EncounterId id){return unsigned(id)<EncounterKinds?&encounter_definitions[unsigned(id)]:nullptr;}
static bool enemy_text_equal(const char*a,const char*b){if(!a||!b)return false;while(*a&&*b){if(*a++!=*b++)return false;}return *a==*b;}
EncounterId encounter_id(const char*name){for(unsigned i=1;i<EncounterKinds;++i)if(enemy_text_equal(name,encounter_metadata[i].model)||enemy_text_equal(name,encounter_definitions[i].key))return EncounterId(i);return EncounterId::None;}
EnemyId enemy_id(const char*name){for(unsigned i=1;i<EnemyKinds;++i)if(enemy_text_equal(name,enemy_definitions[i].model)||enemy_text_equal(name,enemy_definitions[i].key))return EnemyId(i);return EnemyId::LegacyNibbit;}
unsigned encounter_spawned_models(const State&s){return s.enemy_world.spawned_model_count;}
unsigned encounter_returned_gold_count(const State&s){unsigned n=0;for(unsigned i=0;i<s.enemy_world.returned_gold_count;++i)n+=s.enemy_world.returned_reward_kind[i]==0;return n;}
int encounter_returned_gold(const State&s,unsigned index){for(unsigned i=0;i<s.enemy_world.returned_gold_count;++i)if(!s.enemy_world.returned_reward_kind[i]){if(!index)return s.enemy_world.returned_gold[i];--index;}return 0;}
void encounter_add_card_reward(State&s){auto&w=s.enemy_world;if(w.returned_gold_count>=MaxEnemyExtraRewards){fail(s);return;}unsigned i=w.returned_gold_count++;w.returned_reward_kind[i]=1;w.returned_gold[i]=3;}
unsigned encounter_combat_reward_count(const State&s){return s.enemy_world.returned_gold_count;}
bool encounter_combat_reward(const State&s,unsigned i,unsigned&kind,int&value){const auto&w=s.enemy_world;if(i>=w.returned_gold_count)return false;kind=w.returned_reward_kind[i];value=w.returned_gold[i];return true;}
bool encounter_combat_reward_card(const State&s,unsigned i,EventDeckSnapshot&snap){const auto&w=s.enemy_world;if(i>=w.returned_gold_count||w.returned_reward_kind[i]!=2)return false;snap=w.returned_cards[i];return true;}
unsigned encounter_escaped_enemies(const State&s){return s.enemy_world.escaped_count;}
uint32_t encounter_gold_proportion_bits(const State&s){
 if(s.enemy_world.encounter==EncounterId::GremlinMercNormal){uint32_t bits=0x3f800000U;for(const auto&m:s.enemy_world.models)if(m.kind==EnemyId::FatGremlin&&(m.flags&EnemyEscaped))bits=m.stolen_gold?0U:0x3f000000U;return bits;}
 if(!s.enemy_world.spawned_model_count)return 0x3f800000U;
 // Preserve both original binary32 rounding boundaries without requiring
 // compiler-rt floating point helpers on the freestanding ARMv5 target.
 using reference::math::Real;
 Real escaped=world_float_round(Real::integer(s.enemy_world.escaped_count)/Real::integer(s.enemy_world.spawned_model_count));
 return world_float_bits(world_float_round(Real::integer(1)-escaped));
}
bool encounter_shares_tags(EncounterId a,EncounterId b){return unsigned(a)<EncounterKinds&&unsigned(b)<EncounterKinds&&(encounter_metadata[unsigned(a)].tags&encounter_metadata[unsigned(b)].tags);}
bool encounter_rewards_allowed(EncounterId id){return unsigned(id)<EncounterKinds&&encounter_metadata[unsigned(id)].rewards;}
static int encounter_gold(EncounterId id,unsigned ascension,bool high){
 if(!encounter_rewards_allowed(id))return 0;if(id==EncounterId::FakeMerchantEventEncounter)return 300;
 const auto type=encounter_definitions[unsigned(id)].type;int amount=type==EncounterClass::Boss?100:type==EncounterClass::Elite?(high?45:35):(high?20:10);
 return ascension>=3?amount*3/4:amount;
}
int encounter_min_gold(EncounterId id,unsigned ascension){return encounter_gold(id,ascension,false);}
int encounter_max_gold(EncounterId id,unsigned ascension){return encounter_gold(id,ascension,true);}
unsigned encounter_pool(unsigned region,EncounterClass type,EncounterId*out,unsigned capacity){unsigned n=0;for(const auto&e:encounter_pool_entries)if(e.region==region&&encounter_definitions[unsigned(e.id)].type==type){if(out&&n<capacity)out[n]=e.id;++n;}return n;}
static void enemy_reset_combat(State&s){reset_in_place(s.enemy_world);}
static int enemy_unique_hp(State&s,EnemyId kind,int except=-1,int bonus=0,int pending_add=-1){
 const auto&d=enemy_definitions[unsigned(kind)];int low=(s.enemy_world.ascension>=8?d.min_hp_8:d.min_hp)+bonus,high=(s.enemy_world.ascension>=8?d.max_hp_8:d.max_hp)+bonus;
 // A created actor can already own an identity without belonging to the
 // original enemy list. Only actors added to that list constrain unique HP.
 int choices[128],n=0;for(int hp=low;hp<=high&&n<128;++hp){bool used=false;for(int i=0;i<s.enemy_count;++i)if(i!=except&&i!=pending_add&&enemy_slot_exists(s,i)&&s.enemies[i].max_hp==hp){used=true;break;}if(!used)choices[n++]=hp;}
 auto&r=world_rng(s,WorldRng::Niche);return n?choices[r.below((unsigned)n)]:r.range(low,high+1);
}
static int enemy_create(State&s,EnemyId kind,int slot,int owner=-1,int bonus=0,int pending_add=-1){
 if(kind==EnemyId::LegacyNibbit||unsigned(kind)>=EnemyKinds||slot<0||slot>=s.enemy_world.slot_limit)return -1;
 int t=-1;for(int i=0;i<s.enemy_count;++i)if(!enemy_slot_exists(s,i)&&i!=s.enemy_world.actor){t=i;break;}
 if(t<0){if(s.enemy_count>=MaxEnemies){fail(s);return -1;}t=s.enemy_count++;}
 power_order_clear_actor(s,unsigned(t+1));enemy_reset_shared_slot(s,t);auto&m=s.enemy_world.models[t];m=EnemyModelState{};m.kind=kind;m.slot=(uint8_t)slot;m.owner=owner<0?255:(uint8_t)owner;
 // HP is sampled before attaching this new combat identity.
 int hp=enemy_unique_hp(s,kind,t,bonus,pending_add);s.enemies[t].hp=s.enemies[t].max_hp=(int16_t)hp;
 m.flags=EnemyPresent;m.combat_id=s.enemy_world.next_creature_id++;m.spawned_this_side=s.enemy_world.side;
 const int col=world_map_column(s),row=world_map_row(s);m.rng.reset(world_run_seed(s)+(col<0?0:col)+(row<0?0:row)+world_act_index(s)+m.combat_id);
 if(!s.enemy_world.spawned_models[unsigned(kind)]){s.enemy_world.spawned_models[unsigned(kind)]=1;++s.enemy_world.spawned_model_count;}
 if(kind==EnemyId::TwoTailedRat)m.data[3]=-1;
 notify_enemy_spawned(s,unsigned(kind),m.combat_id);
 return t;
}
static void enemy_initial_powers(State&s,int t){
 auto&m=s.enemy_world.models[t];auto power=[&](EnemyPower p,int n){enemy_apply_power(s,t,p,n,t);};auto val=[&](EnemyValue v){return enemy_number(s,t,v);};
 switch(m.kind){
 case EnemyId::Aeonglass:power(EnemyPower::WitheringPresence,6);power(EnemyPower::Artifact,3);break;
 case EnemyId::Axebot:power(EnemyPower::Stock,2-m.data[0]);break;
 case EnemyId::BattleFriendV1:case EnemyId::BattleFriendV2:case EnemyId::BattleFriendV3:power(EnemyPower::BattlewornDummyTimeLimit,3);break;
 case EnemyId::BowlbugRock:power(EnemyPower::Imbalanced,1);break;
 case EnemyId::BygoneEffigy:power(EnemyPower::Slow,1);break;
 case EnemyId::Byrdonis:power(EnemyPower::Territorial,1);break;
 case EnemyId::Chomper:power(EnemyPower::Artifact,2);break;
 case EnemyId::CorpseSlug:power(EnemyPower::Ravenous,val(EnemyValue::RavenousStr));break;
 case EnemyId::Crusher:power(EnemyPower::BackAttackLeft,1);power(EnemyPower::CrabRage,1);break;
 case EnemyId::CubexConstruct:s.enemies[t].block=13;power(EnemyPower::Artifact,1);m.data[0]=m.data[8]=1;break;
 case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:{
  const auto&d=enemy_definitions[unsigned(m.kind)];int low=s.enemy_world.ascension>=8?d.min_hp_8:d.min_hp,high=s.enemy_world.ascension>=8?d.max_hp_8:d.max_hp;int hp=s.enemies[t].max_hp+(s.enemies[t].max_hp&1);
  for(int tries=0;tries<8;++tries){bool used=false;for(int i=0;i<s.enemy_count;++i)if(i!=t&&enemy_slot_exists(s,i)&&s.enemies[i].max_hp==hp)used=true;if(!used)break;hp+=2;if(hp>high)hp=low;}
  s.enemies[t].hp=s.enemies[t].max_hp=(int16_t)hp;power(EnemyPower::Reattach,25);break;}
 case EnemyId::Entomancer:power(EnemyPower::PersonalHive,1);break;
 case EnemyId::Exoskeleton:power(EnemyPower::HardToKill,9);break;
 case EnemyId::EyeWithTeeth:case EnemyId::Parafright:power(EnemyPower::Illusion,1);break;
 case EnemyId::FossilStalker:power(EnemyPower::Suck,3);break;
 case EnemyId::FrogKnight:case EnemyId::SlumberingBeetle:power(EnemyPower::Plating,val(EnemyValue::PlatingAmount));if(m.kind==EnemyId::SlumberingBeetle)power(EnemyPower::Slumber,3);break;
 case EnemyId::GasBomb:case EnemyId::KinFollower:case EnemyId::TorchHeadAmalgam:power(EnemyPower::Minion,1);break;
 case EnemyId::GlobeHead:power(EnemyPower::Galvanic,val(EnemyValue::GalvanicPowerAmount));break;
 case EnemyId::GremlinMerc:power(EnemyPower::Surprise,1);power(EnemyPower::Thievery,20);break;
 case EnemyId::InfestedPrism:power(EnemyPower::VitalSpark,val(EnemyValue::VitalSparkAmount));break;
 case EnemyId::Inklet:power(EnemyPower::Slippery,1);break;
 case EnemyId::LagavulinMatriarch:power(EnemyPower::Plating,12);power(EnemyPower::Asleep,3);break;
 case EnemyId::LivingShield:power(EnemyPower::Rampart,25);break;
 case EnemyId::LouseProgenitor:power(EnemyPower::CurlUp,val(EnemyValue::CurlBlock));break;
 case EnemyId::MechaKnight:power(EnemyPower::Artifact,3);break;
 case EnemyId::MysteriousKnight:power(EnemyPower::Strength,6);power(EnemyPower::Plating,6);break;
 case EnemyId::PhantasmalGardener:power(EnemyPower::Skittish,val(EnemyValue::SkittishAmount));break;
 case EnemyId::PhrogParasite:power(EnemyPower::Infested,4);break;
 case EnemyId::PunchConstruct:power(EnemyPower::Artifact,1);s.enemies[t].hp=(int16_t)max(1,s.enemies[t].hp-m.data[1]);break;
 case EnemyId::Rocket:enemy_apply_power(s,-1,EnemyPower::Surrounded,1,t);power(EnemyPower::BackAttackRight,1);power(EnemyPower::CrabRage,1);break;
 case EnemyId::ScrollOfBiting:power(EnemyPower::PaperCuts,2);break;
 case EnemyId::SewerClam:power(EnemyPower::Plating,s.enemy_world.ascension>=8?9:8);break;
 case EnemyId::SkulkingColony:power(EnemyPower::HardenedShell,20);break;
 case EnemyId::TerrorEel:power(EnemyPower::Shriek,val(EnemyValue::ShriekAmount));break;
 case EnemyId::TestSubject:power(EnemyPower::Adaptable,1);power(EnemyPower::Enrage,val(EnemyValue::EnrageAmount));break;
 case EnemyId::TheForgotten:power(EnemyPower::PossessSpeed,1);break;
 case EnemyId::TheLost:power(EnemyPower::PossessStrength,1);break;
 case EnemyId::ThievingHopper:power(EnemyPower::EscapeArtist,5);break;
 case EnemyId::ToughEgg:if(!m.data[0])power(EnemyPower::Hatch,s.enemy_world.side?2:1);break;
 case EnemyId::TwoTailedRat:m.data[0]=2;break;
 case EnemyId::Vantom:power(EnemyPower::Slippery,val(EnemyValue::SlipperyAmt));m.data[8]=1;break;
 case EnemyId::WaterfallGiant:m.data[0]=(int16_t)val(EnemyValue::BasePressureGunDamage);break;
 case EnemyId::Zapbot:power(EnemyPower::HighVoltage,2);break;
 default:break;
 }
}
static int enemy_spawn(State&s,EnemyId kind,int slot,int owner=-1,int pending_add=-1){
 EnemyObservedCommand command(s,CoreEnemyCommand::Spawn,-1,unsigned(kind),0,owner);
 int t=enemy_create(s,kind,slot,owner,0,pending_add);if(t<0)return t;
 if(kind==EnemyId::Wriggler)s.enemy_world.models[t].data[0]=1;
 enemy_initial_powers(s,t);if(s.enemy_world.side)s.enemy_world.models[t].data[11]=1;else{enemy_set_move(s,t,enemy_initial_node(s,t));s.enemy_world.models[t].previous=s.enemy_world.models[t].node;}
 world_relic_hook(s,WorldRelicHook::AfterEnemyAdded,t);
 if(has_run_modifier(s,RunModifier::Murderous))enemy_apply_power(s,t,EnemyPower::Strength,3);
 return t;
}
struct EnemyComposition{
 EnemyId kind[MaxEnemies]{};uint8_t slot[MaxEnemies]{},count=0;int16_t data[MaxEnemies][12]{};
 void add(EnemyId k,int where=-1){if(count>=MaxEnemies)return;kind[count]=k;slot[count]=where<0?count:(uint8_t)where;++count;}
 void repeat(EnemyId k,unsigned n){for(unsigned i=0;i<n;++i)add(k);}
};
static bool enemy_composition(State&s,EncounterId id,EnemyComposition&c){
 auto&r=s.enemy_world.composition_rng;
 const auto stagger=[&](unsigned count){int n=(int)r.below(3);for(unsigned i=0;i<count;++i)c.data[i][3]=(int16_t)((n+i)%3);};
 switch(id){
 case EncounterId::AeonglassBoss:{c.add(EnemyId::Aeonglass);break;}
 case EncounterId::AxebotsNormal:{c.add(EnemyId::Axebot);break;}
 case EncounterId::BattlewornDummyEventV1Encounter:{c.add(EnemyId::BattleFriendV1);break;}
 case EncounterId::BattlewornDummyEventV2Encounter:{c.add(EnemyId::BattleFriendV2);break;}
 case EncounterId::BattlewornDummyEventV3Encounter:{c.add(EnemyId::BattleFriendV3);break;}
 case EncounterId::BowlbugsNormal:{c.add(EnemyId::BowlbugRock);EnemyId pool[]={EnemyId::BowlbugEgg,EnemyId::BowlbugSilk,EnemyId::BowlbugNectar};unsigned n=3;for(int i=0;i<2;++i){unsigned k=r.below(n);c.add(pool[k]);for(unsigned j=k+1;j<n;++j)pool[j-1]=pool[j];--n;}break;}
 case EncounterId::BowlbugsWeak:{c.add(EnemyId::BowlbugRock);c.add(r.below(2)?EnemyId::BowlbugNectar:EnemyId::BowlbugEgg);break;}
 case EncounterId::BygoneEffigyElite:{c.add(EnemyId::BygoneEffigy);break;}
 case EncounterId::ByrdonisElite:{c.add(EnemyId::Byrdonis);break;}
 case EncounterId::CeremonialBeastBoss:{c.add(EnemyId::CeremonialBeast);break;}
 case EncounterId::ChompersNormal:{c.repeat(EnemyId::Chomper,2);c.data[1][0]=1;break;}
 case EncounterId::ConstructMenagerieNormal:{c.add(EnemyId::PunchConstruct);c.add(EnemyId::CubexConstruct);c.add(EnemyId::CubexConstruct);break;}
 case EncounterId::CorpseSlugsNormal:{c.repeat(EnemyId::CorpseSlug,3);stagger(3);break;}
 case EncounterId::CorpseSlugsWeak:{c.repeat(EnemyId::CorpseSlug,2);stagger(2);break;}
 case EncounterId::CubexConstructNormal:{c.add(EnemyId::CubexConstruct);break;}
 case EncounterId::CultistsNormal:{c.add(EnemyId::CalcifiedCultist);c.add(EnemyId::DampCultist);break;}
 case EncounterId::DecimillipedeElite:{c.add(EnemyId::DecimillipedeSegmentFront);c.add(EnemyId::DecimillipedeSegmentMiddle);c.add(EnemyId::DecimillipedeSegmentBack);stagger(3);break;}
 case EncounterId::DenseVegetationEventEncounter:{c.repeat(EnemyId::Wriggler,4);break;}
 case EncounterId::DevotedSculptorWeak:{c.add(EnemyId::DevotedSculptor);break;}
 case EncounterId::EntomancerElite:{c.add(EnemyId::Entomancer);break;}
 case EncounterId::ExoskeletonsNormal:{c.repeat(EnemyId::Exoskeleton,4);break;}
 case EncounterId::ExoskeletonsWeak:{c.repeat(EnemyId::Exoskeleton,3);break;}
 case EncounterId::FabricatorNormal:{c.add(EnemyId::Fabricator,2);break;}
 case EncounterId::FakeMerchantEventEncounter:{c.add(EnemyId::FakeMerchantMonster);break;}
 case EncounterId::FlyconidNormal:{c.add(r.below(2)?EnemyId::TwigSlimeM:EnemyId::LeafSlimeM);c.add(EnemyId::Flyconid);break;}
 case EncounterId::FogmogNormal:{c.add(EnemyId::Fogmog,1);break;}
 case EncounterId::FossilStalkerNormal:{c.add(EnemyId::FossilStalker);break;}
 case EncounterId::FrogKnightNormal:{c.add(EnemyId::FrogKnight);break;}
 case EncounterId::FuzzyWurmCrawlerWeak:{c.add(EnemyId::FuzzyWurmCrawler);break;}
 case EncounterId::GlobeHeadNormal:{c.add(EnemyId::GlobeHead);break;}
 case EncounterId::GremlinMercNormal:{c.add(EnemyId::GremlinMerc);break;}
 case EncounterId::HauntedShipNormal:{c.add(EnemyId::HauntedShip);break;}
 case EncounterId::HunterKillerNormal:{c.add(EnemyId::HunterKiller);break;}
 case EncounterId::InfestedPrismsElite:{c.add(EnemyId::InfestedPrism);break;}
 case EncounterId::InkletsNormal:{c.repeat(EnemyId::Inklet,3);c.data[1][0]=1;break;}
 case EncounterId::KaiserCrabBoss:{c.add(EnemyId::Crusher);c.add(EnemyId::Rocket);break;}
 case EncounterId::KnightsElite:{c.add(EnemyId::FlailKnight);c.add(EnemyId::SpectralKnight);c.add(EnemyId::MagiKnight);break;}
 case EncounterId::KnowledgeDemonBoss:{c.add(EnemyId::KnowledgeDemon);break;}
 case EncounterId::LagavulinMatriarchBoss:{c.add(EnemyId::LagavulinMatriarch);break;}
 case EncounterId::LivingFogNormal:{c.add(EnemyId::LivingFog,5);break;}
 case EncounterId::LouseProgenitorNormal:{c.add(EnemyId::LouseProgenitor);break;}
 case EncounterId::MawlerNormal:{c.add(EnemyId::Mawler);break;}
 case EncounterId::MechaKnightElite:{c.add(EnemyId::MechaKnight);break;}
 case EncounterId::MysteriousKnightEventEncounter:{c.add(EnemyId::MysteriousKnight);break;}
 case EncounterId::MytesNormal:{c.add(EnemyId::Myte);c.add(EnemyId::Myte);break;}
 case EncounterId::NibbitsNormal:{c.repeat(EnemyId::Nibbit,2);c.data[0][1]=1;break;}
 case EncounterId::NibbitsWeak:{c.add(EnemyId::Nibbit);c.data[0][0]=1;break;}
 case EncounterId::OvergrowthCrawlers:{c.add(EnemyId::ShrinkerBeetle);c.add(EnemyId::FuzzyWurmCrawler);break;}
 case EncounterId::OvicopterNormal:{c.add(EnemyId::Ovicopter,5);break;}
 case EncounterId::OwlMagistrateNormal:{c.add(EnemyId::OwlMagistrate);break;}
 case EncounterId::PhantasmalGardenersElite:{c.repeat(EnemyId::PhantasmalGardener,4);break;}
 case EncounterId::PhrogParasiteElite:{c.add(EnemyId::PhrogParasite);break;}
 case EncounterId::PunchConstructNormal:{c.add(EnemyId::PunchConstruct);break;}
 case EncounterId::PunchOffEventEncounter:{c.repeat(EnemyId::PunchConstruct,2);c.data[0][0]=1;c.data[0][1]=(int16_t)r.range(2,10);c.data[1][1]=(int16_t)r.range(2,10);break;}
 case EncounterId::QueenBoss:{c.add(EnemyId::TorchHeadAmalgam);c.add(EnemyId::Queen);break;}
 case EncounterId::RubyRaidersNormal:{EnemyId pool[]={EnemyId::AxeRubyRaider,EnemyId::AssassinRubyRaider,EnemyId::BruteRubyRaider,EnemyId::CrossbowRubyRaider,EnemyId::TrackerRubyRaider};unsigned n=5;for(int i=0;i<3;++i){unsigned k=r.below(n);c.add(pool[k]);for(unsigned j=k+1;j<n;++j)pool[j-1]=pool[j];--n;}break;}
 case EncounterId::ScrollsOfBitingNormal:{c.repeat(EnemyId::ScrollOfBiting,4);stagger(3);c.data[3][3]=2;break;}
 case EncounterId::ScrollsOfBitingWeak:{c.repeat(EnemyId::ScrollOfBiting,3);stagger(3);break;}
 case EncounterId::SeapunkNormal:{c.add(EnemyId::CalcifiedCultist);c.add(EnemyId::Seapunk);break;}
 case EncounterId::SeapunkWeak:{c.add(EnemyId::Seapunk);break;}
 case EncounterId::SewerClamNormal:{c.add(EnemyId::SewerClam);break;}
 case EncounterId::ShrinkerBeetleWeak:{c.add(EnemyId::ShrinkerBeetle);break;}
 case EncounterId::SkulkingColonyElite:{c.add(EnemyId::SkulkingColony);break;}
 case EncounterId::SlimedBerserkerNormal:{c.add(EnemyId::SlimedBerserker);break;}
 case EncounterId::SlimesNormal:{bool flip=r.below(2)==0;c.add(EnemyId::TwigSlimeM);c.add(EnemyId::LeafSlimeM);c.add(flip?EnemyId::LeafSlimeS:EnemyId::TwigSlimeS);c.add(flip?EnemyId::TwigSlimeS:EnemyId::LeafSlimeS);break;}
 case EncounterId::SlimesWeak:{bool twig=r.below(2)!=0;r.below(1);c.add(twig?EnemyId::TwigSlimeS:EnemyId::LeafSlimeS);c.add(r.below(2)?EnemyId::TwigSlimeM:EnemyId::LeafSlimeM);c.add(twig?EnemyId::LeafSlimeS:EnemyId::TwigSlimeS);break;}
 case EncounterId::SlitheringStranglerNormal:{unsigned mode=r.below(3);if(mode==0)c.add(EnemyId::SnappingJaxfruit);else if(mode==1)c.add(r.below(2)?EnemyId::TwigSlimeM:EnemyId::LeafSlimeM);else{c.add(r.below(2)?EnemyId::TwigSlimeS:EnemyId::LeafSlimeS);c.add(r.below(2)?EnemyId::TwigSlimeS:EnemyId::LeafSlimeS);}c.add(EnemyId::SlitheringStrangler);break;}
 case EncounterId::SludgeSpinnerWeak:{c.add(EnemyId::SludgeSpinner);break;}
 case EncounterId::SlumberingBeetleNormal:{c.add(EnemyId::BowlbugRock);c.add(EnemyId::BowlbugSilk);c.add(EnemyId::SlumberingBeetle);break;}
 case EncounterId::SnappingJaxfruitNormal:{c.add(EnemyId::SnappingJaxfruit);c.add(EnemyId::Flyconid);break;}
 case EncounterId::SoulFyshBoss:{c.add(EnemyId::SoulFysh);break;}
 case EncounterId::SoulNexusElite:{c.add(EnemyId::SoulNexus);break;}
 case EncounterId::SpinyToadNormal:{c.add(EnemyId::SpinyToad);break;}
 case EncounterId::TerrorEelElite:{c.add(EnemyId::TerrorEel);break;}
 case EncounterId::TestSubjectBoss:{c.add(EnemyId::TestSubject);break;}
 case EncounterId::TheArchitectEventEncounter:{c.add(EnemyId::Architect);break;}
 case EncounterId::TheInsatiableBoss:{c.add(EnemyId::TheInsatiable);break;}
 case EncounterId::TheKinBoss:{c.repeat(EnemyId::KinFollower,2);c.add(EnemyId::KinPriest);c.data[0][0]=1;break;}
 case EncounterId::TheLostAndForgottenNormal:{c.add(EnemyId::TheLost);c.add(EnemyId::TheForgotten);break;}
 case EncounterId::TheObscuraNormal:{c.add(EnemyId::TheObscura,1);break;}
 case EncounterId::ThievingHopperWeak:{c.add(EnemyId::ThievingHopper);break;}
 case EncounterId::ToadpolesWeak:{c.repeat(EnemyId::Toadpole,2);c.data[0][0]=1;break;}
 case EncounterId::TunnelerWeak:{c.add(EnemyId::Tunneler);break;}
 case EncounterId::TurretOperatorWeak:{c.add(EnemyId::LivingShield);c.add(EnemyId::TurretOperator);break;}
 case EncounterId::TwoTailedRatsNormal:{c.add(EnemyId::TwoTailedRat,2);c.add(EnemyId::TwoTailedRat,3);c.add(EnemyId::TwoTailedRat,4);stagger(3);break;}
 case EncounterId::VantomBoss:{c.add(EnemyId::Vantom);break;}
 case EncounterId::VineShamblerNormal:{c.add(EnemyId::VineShambler);break;}
 case EncounterId::WaterfallGiantBoss:{c.add(EnemyId::WaterfallGiant);break;}
 default:return false;
 }return c.count!=0;
}
bool encounter_setup(State&s,EncounterId id,unsigned ascension){
 if(id==EncounterId::None||unsigned(id)>=EncounterKinds||ascension>10)return false;
 enemy_reset_combat(s);auto&w=s.enemy_world;w.active=1;w.ascension=(uint8_t)ascension;w.encounter=id;
 w.slot_limit=encounter_metadata[unsigned(id)].slots;if(!w.slot_limit)w.slot_limit=MaxEnemies;
 const char*key=encounter_definitions[unsigned(id)].key;unsigned len=0;while(key[len])++len;
 w.composition_rng.reset(world_run_seed(s)+world_total_floor(s)+reference::hash64(key,len));
 EnemyComposition c;if(!enemy_composition(s,id,c)){fail(s);return false;}
 s.enemy_count=0;for(unsigned i=0;i<c.count;++i){int t=enemy_create(s,c.kind[i],c.slot[i]);if(t<0)return false;for(unsigned j=0;j<12;++j)w.models[t].data[j]=c.data[i][j];}
 // All starting actors exist before AfterAddedToRoom, including HP uniqueness checks.
 for(int t=0;t<s.enemy_count;++t)enemy_initial_powers(s,t);
 for(int t=0;t<s.enemy_count;++t){enemy_set_move(s,t,enemy_initial_node(s,t));w.models[t].previous=w.models[t].node;}
 // Initial creatures receive AfterRoomEntered relic effects. The distinct
 // AfterCreatureAddedToCombat hook belongs only to mid-combat summons.
 return s.phase!=Phase::Error;
}
