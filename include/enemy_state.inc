// Original solo encounter state. LegacyNibbit preserves the adapted old saves.
// This module owns behavior facts; presentation never advances these counters.
enum class EnemyId:uint8_t { LegacyNibbit,
 Aeonglass,
 Architect,
 AssassinRubyRaider,
 AxeRubyRaider,
 Axebot,
 BattleFriendV1,
 BattleFriendV2,
 BattleFriendV3,
 BowlbugEgg,
 BowlbugNectar,
 BowlbugRock,
 BowlbugSilk,
 BruteRubyRaider,
 BygoneEffigy,
 Byrdonis,
 CalcifiedCultist,
 CeremonialBeast,
 Chomper,
 CorpseSlug,
 CrossbowRubyRaider,
 Crusher,
 CubexConstruct,
 DampCultist,
 DecimillipedeSegmentBack,
 DecimillipedeSegmentFront,
 DecimillipedeSegmentMiddle,
 DevotedSculptor,
 Entomancer,
 Exoskeleton,
 EyeWithTeeth,
 Fabricator,
 FakeMerchantMonster,
 FatGremlin,
 FlailKnight,
 Flyconid,
 Fogmog,
 FossilStalker,
 FrogKnight,
 FuzzyWurmCrawler,
 GasBomb,
 GlobeHead,
 GremlinMerc,
 Guardbot,
 HauntedShip,
 HunterKiller,
 InfestedPrism,
 Inklet,
 KinFollower,
 KinPriest,
 KnowledgeDemon,
 LagavulinMatriarch,
 LeafSlimeM,
 LeafSlimeS,
 LivingFog,
 LivingShield,
 LouseProgenitor,
 MagiKnight,
 Mawler,
 MechaKnight,
 MysteriousKnight,
 Myte,
 Nibbit,
 Noisebot,
 Ovicopter,
 OwlMagistrate,
 Parafright,
 PhantasmalGardener,
 PhrogParasite,
 PunchConstruct,
 Queen,
 Rocket,
 ScrollOfBiting,
 Seapunk,
 SewerClam,
 ShrinkerBeetle,
 SkulkingColony,
 SlimedBerserker,
 SlitheringStrangler,
 SludgeSpinner,
 SlumberingBeetle,
 SnappingJaxfruit,
 SneakyGremlin,
 SoulFysh,
 SoulNexus,
 SpectralKnight,
 SpinyToad,
 Stabbot,
 TerrorEel,
 TestSubject,
 TheForgotten,
 TheInsatiable,
 TheLost,
 TheObscura,
 ThievingHopper,
 Toadpole,
 TorchHeadAmalgam,
 ToughEgg,
 TrackerRubyRaider,
 Tunneler,
 TurretOperator,
 TwigSlimeM,
 TwigSlimeS,
 TwoTailedRat,
 Vantom,
 VineShambler,
 WaterfallGiant,
 Wriggler,
 Zapbot,Count };
constexpr int EnemyKinds=(int)EnemyId::Count;
enum class EncounterId:uint8_t { None,
 AeonglassBoss,
 AxebotsNormal,
 BattlewornDummyEventV1Encounter,
 BattlewornDummyEventV2Encounter,
 BattlewornDummyEventV3Encounter,
 BowlbugsNormal,
 BowlbugsWeak,
 BygoneEffigyElite,
 ByrdonisElite,
 CeremonialBeastBoss,
 ChompersNormal,
 ConstructMenagerieNormal,
 CorpseSlugsNormal,
 CorpseSlugsWeak,
 CubexConstructNormal,
 CultistsNormal,
 DecimillipedeElite,
 DenseVegetationEventEncounter,
 DevotedSculptorWeak,
 EntomancerElite,
 ExoskeletonsNormal,
 ExoskeletonsWeak,
 FabricatorNormal,
 FakeMerchantEventEncounter,
 FlyconidNormal,
 FogmogNormal,
 FossilStalkerNormal,
 FrogKnightNormal,
 FuzzyWurmCrawlerWeak,
 GlobeHeadNormal,
 GremlinMercNormal,
 HauntedShipNormal,
 HunterKillerNormal,
 InfestedPrismsElite,
 InkletsNormal,
 KaiserCrabBoss,
 KnightsElite,
 KnowledgeDemonBoss,
 LagavulinMatriarchBoss,
 LivingFogNormal,
 LouseProgenitorNormal,
 MawlerNormal,
 MechaKnightElite,
 MysteriousKnightEventEncounter,
 MytesNormal,
 NibbitsNormal,
 NibbitsWeak,
 OvergrowthCrawlers,
 OvicopterNormal,
 OwlMagistrateNormal,
 PhantasmalGardenersElite,
 PhrogParasiteElite,
 PunchConstructNormal,
 PunchOffEventEncounter,
 QueenBoss,
 RubyRaidersNormal,
 ScrollsOfBitingNormal,
 ScrollsOfBitingWeak,
 SeapunkNormal,
 SeapunkWeak,
 SewerClamNormal,
 ShrinkerBeetleWeak,
 SkulkingColonyElite,
 SlimedBerserkerNormal,
 SlimesNormal,
 SlimesWeak,
 SlitheringStranglerNormal,
 SludgeSpinnerWeak,
 SlumberingBeetleNormal,
 SnappingJaxfruitNormal,
 SoulFyshBoss,
 SoulNexusElite,
 SpinyToadNormal,
 TerrorEelElite,
 TestSubjectBoss,
 TheArchitectEventEncounter,
 TheInsatiableBoss,
 TheKinBoss,
 TheLostAndForgottenNormal,
 TheObscuraNormal,
 ThievingHopperWeak,
 ToadpolesWeak,
 TunnelerWeak,
 TurretOperatorWeak,
 TwoTailedRatsNormal,
 VantomBoss,
 VineShamblerNormal,
 WaterfallGiantBoss,Count };
constexpr int EncounterKinds=(int)EncounterId::Count;
enum class EnemyPower:uint8_t {
 Adaptable,
 Artifact,
 Asleep,
 BackAttackLeft,
 BackAttackRight,
 BattlewornDummyTimeLimit,
 Burrowed,
 ChainsOfBinding,
 Constrict,
 CrabRage,
 Cruelty,
 CurlUp,
 Dampen,
 Debilitate,
 Dexterity,
 Disintegration,
 Enrage,
 EscapeArtist,
 Flutter,
 Frail,
 Galvanic,
 HardToKill,
 HardenedShell,
 Hatch,
 Heist,
 Hex,
 HighVoltage,
 Illusion,
 Imbalanced,
 Infested,
 Intangible,
 MindRot,
 Minion,
 Nemesis,
 PainfulStabs,
 PaperCuts,
 PersonalHive,
 Plating,
 Plow,
 PossessSpeed,
 PossessStrength,
 Rampart,
 Ravenous,
 Reattach,
 Ringing,
 Ritual,
 Sandpit,
 Shriek,
 Shrink,
 Skittish,
 Slippery,
 Sloth,
 Slow,
 Slumber,
 Smoggy,
 Soar,
 SteamEruption,
 Stock,
 Strength,
 Suck,
 Surprise,
 Surrounded,
 Swipe,
 Tainted,
 Tangled,
 TemporaryStrength,
 Tender,
 Territorial,
 Thievery,
 Thorns,
 Vigor,
 VitalSpark,
 Vulnerable,
 WasteAway,
 Weak,
 WitheringPresence,Count };
constexpr int EnemyPowerCount=(int)EnemyPower::Count;
enum class EnemyAffliction:uint8_t { None,Bound,Entangled,Galvanized,Hexed,Tainted,Ringing,Smog,Count };
constexpr int MaxEnemyMoves=32,MaxEnemyMoveHistory=8;
enum EnemyIntentFlag:uint16_t { EnemyIntentAttack=1,EnemyIntentBlock=2,EnemyIntentBuff=4,
 EnemyIntentDebuff=8,EnemyIntentStatus=16,EnemyIntentHeal=32,EnemyIntentSummon=64,
 EnemyIntentEscape=128,EnemyIntentStun=256,EnemyIntentSleep=512,EnemyIntentDeath=1024,
 EnemyIntentUnknown=2048 };
enum class EncounterClass:uint8_t { Weak,Normal,Elite,Boss,Event,Display };
struct EnemyDefinition { const char*key; const char*model; int16_t min_hp,max_hp,min_hp_8,max_hp_8; };
struct EncounterDefinition { const char*key; EncounterClass type; uint8_t act; };
struct EnemyIntent { const char*move_key; uint16_t flags; int16_t damage,hits; };
enum EnemySlotFlag:uint8_t { EnemyPresent=1,EnemyActed=2,EnemyDying=4,EnemyRemoved=8,
 EnemyDown=16,EnemyStunned=32,EnemyImmediate=64,EnemyEscaped=128 };
struct EnemyModelState {
 EnemyId kind=EnemyId::LegacyNibbit;
 reference::Random rng{};
 uint16_t combat_id=0;
 uint8_t flags=0,node=0,previous=0,consecutive=0,owner=255,slot=0,resume_node=0,spawned_this_side=0;
 uint16_t age=0,seen_moves[MaxEnemyMoves]{};
 uint8_t history[MaxEnemyMoveHistory]{},history_count=0;
 int16_t data[12]{},power[EnemyPowerCount]{},power_aux[EnemyPowerCount]{};
 uint8_t power_order[EnemyPowerCount]{},power_count=0,duration_skip[EnemyPowerCount]{},source[EnemyPowerCount]{};
 uint16_t captured_card=0;
 EventDeckSnapshot captured_deck{};
 int16_t stolen_gold=0,attack_unblocked_hits=0,attack_total_damage=0,attack_vigor=0;
};
struct EnemyWorldState {
 uint8_t active=0,ascension=0,side=0,actor=255,timed_out=0;
 uint16_t next_creature_id=1;
 uint8_t slot_limit=0,turn_order[MaxEnemies]{},turn_count=0;
 uint16_t turn_ids[MaxEnemies]{};
 reference::Random composition_rng{};
 EncounterId encounter=EncounterId::None;
 EnemyModelState models[MaxEnemies]{};
 int16_t player_power[EnemyPowerCount]{},player_aux[EnemyPowerCount]{};
 uint8_t player_power_order[EnemyPowerCount]{},player_power_count=0,player_duration_skip[EnemyPowerCount]{},player_source[EnemyPowerCount]{};
 // Snapshot identity is an instance ID, never a card model or hand position.
 uint8_t affliction[MaxCards]{},dampen_restore[MaxCards]{};
 int16_t affliction_amount[MaxCards]{},card_value[MaxCards]{};
 uint8_t spawned_models[EnemyKinds]{},dampen_casters=0,bound_played=0,used_types=0;
 uint16_t escaped_count=0,spawned_model_count=0,cards_played_turn=0,bound_draws_turn=0;
 uint8_t facing=0,choice_enemy=255,choice_stage=0;
 int16_t turn_damage[MaxEnemies]{},turn_attacks[MaxEnemies]{};
};
struct State;
const EnemyDefinition* enemy_definition(EnemyId);
const EncounterDefinition* encounter_definition(EncounterId);
EncounterId encounter_id(const char* model_or_key);
EnemyId enemy_id(const char* model_or_key);
// Spawn history counts distinct canonical monster models, not live slots.
unsigned encounter_spawned_models(const State&);
unsigned encounter_escaped_enemies(const State&);
bool encounter_shares_tags(EncounterId,EncounterId);
int encounter_min_gold(EncounterId,unsigned ascension=0);
int encounter_max_gold(EncounterId,unsigned ascension=0);
bool encounter_rewards_allowed(EncounterId);
uint32_t encounter_gold_proportion_bits(const State&);
EnemyIntent enemy_intent(const State&,int);
unsigned encounter_pool(unsigned act,EncounterClass,EncounterId*out,unsigned capacity);
bool encounter_setup(State&,EncounterId,unsigned ascension=0);
bool enemy_allow_target(const State&,int);
bool enemy_keeps_combat_alive(const State&);
int enemy_power_value(const State&,int target,EnemyPower); // -1 is the player.
void enemy_note_ally_created(State&);
bool enemy_encounter_timed_out(const State&);
void enemy_stun(State&,int target);
bool enemy_infinite_hp(const State&,int);
bool enemy_death_is_fatal(const State&,int);
