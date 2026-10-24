#pragma once
#include <stdint.h>
#include <stddef.h>
#include "world_rng.hpp"
#include "world_relics.hpp"
#include "run_options_state.hpp"
#include "run_history_event.hpp"

namespace spire {
using CardKind=uint16_t;
using RelicMask=uint64_t;
constexpr int IroncladCards=85, SilentCards=86, DefectCards=86, RegentCards=86, NecroCards=86, CardKinds=558, MaxCards=512, MaxDeck=192, MaxHand=10, MaxEnemies=8, MaxEffects=512;
constexpr int LegacyEnemies=2;
constexpr unsigned LegacyPotionSlots=2,MaxPotionSlots=16;
static_assert(CardKinds<16384,"Card model tokens must fit signed effect operands");
constexpr int MapRows=12, MapColumns=3, MapNodes=MapRows*MapColumns;
enum CardId : CardKind { Strike, Defend, Bash, Anger, PommelStrike, ShrugItOff, BurningPact,
    TrueGrit, TwinStrike, Inflame, IronWave, Whirlwind, FeelNoPain, DarkEmbrace,
    Bloodletting, Hemokinesis, Impervious, Offering, Bludgeon, Thunderclap,
    Uppercut, BodySlam, BattleTrance, FiendFire, Finesse, FlashOfSteel,
    StrikeSilent, DefendSilent, Neutralize, Survivor, Acrobatics, Backflip,
    DaggerThrow, DeadlyPoison, PoisonedStab, CloakAndDagger, BladeDance,
    DaggerSpray, Dash, LegSweep, Footwork, NoxiousFumes, BouncingFlask,
    Prepared, Slice, Deflect, SuckerPunch, Adrenaline, Accuracy, LeadingStrike, Shiv, Impatience, MasterOfStrategy, Shockwave, Scrawl, Purity, HandOfGreed, PanicButton,
    Headbutt, SwordBoomerang, PerfectedStrike, FlameBarrier, DemonForm,
    DodgeAndRoll, InfiniteBlades, Backstab, EscapePlan, Predator,
    Alchemize, Anointed, Automation, BeatDown, Bolas, Calamity, Catastrophe, DarkShackles, Discovery, DramaticEntrance, Entropy, Equilibrium, EternalArmor, Fasten, Fisticuffs, GoldAxe, HiddenGem, JackOfAllTrades, Jackpot, Mayhem, MindBlast, Nostalgia, Omnislice, Panache, PrepTime, Production, Prolong, Prowess, Rend, Restlessness, RollingBoulder, Salvo, SecretTechnique, SecretWeapon, SeekerStrike, Splash, Stratagem, TheBomb, TheGambit, ThinkingAhead, ThrummingHatchet, UltimateDefend, UltimateStrike, Volley, Soul, SovereignBlade,
    // Append-only NSP2/12 IDs; the v3-v12 save/model prefix remains unchanged.
    Aggression, Armaments, AshenStrike, Barricade, BloodWall, Brand, Break,
    Breakthrough, Bully, Cascade, Cinder, Colossus, Conflagration, Corruption,
    CrimsonMantle, Cruelty, Dismantle, Dominate, DrumOfBattle, EvilEye,
    ExpectAFight, Feed, FightMe, ForgottenRitual, Havoc, Hellraiser,
    HowlFromBeyond, InfernalBlade, Inferno, Juggernaut, Juggling, Mangle,
    MoltenFist, NotYet, OneTwoPunch, PactsEnd, Pillage, PrimalForce, Pyre, Rage,
    Rampage, Rupture, SecondWind, SetupStrike, Spite, Stampede, Stoke, Stomp,
    StoneArmor, Taunt, TearAsunder, Thrash, Tremble, Unmovable, Unrelenting, Vicious,
    Abrasive, Accelerant, Afterimage, Anticipate, Assassinate, BladeOfInk, Blur,
    BubbleBubble, BulletTime, Burst, CalculatedGamble, CorrosiveWave,
    EchoingSlash, Envenom, Expertise, Expose, FanOfKnives, Finisher,
    Flechettes, FlickFlack, Sidestep, GrandFinale, HandTrick, Haze,
    HiddenDaggers, KnifeTrap, Malaise, MasterPlanner, MementoMori, Mirage,
    Murder, Nightmare, Outbreak, PhantomBlades, PiercingWail, Pinpoint,
    Pounce, PreciseCut, Reflex, Ricochet, SerpentForm, ShadowStep, Shadowmeld,
    Skewer, Snakebite, Speedster, StormOfSteel, Strangle, Suppress, Tactician,
    TheHunt, ToolsOfTheTrade, Tracking, Untouchable, UpMySleeve,
    WellLaidPlans, WraithForm, GiantRock,
#include "defect_ids.inc"
#include "regent_ids.inc"
#include "necro_ids.inc"
    Dazed,Wound,Slimed,Burn,Void,Fuel,MinionStrike,MinionDiveBomb,MinionSacrifice,Debris,SweepingGaze,
#include "event_card_ids.inc"
};
static_assert(Aggression==114&&Abrasive==170&&GiantRock==227&&Dazed==486&&SweepingGaze==496&&EventCardBadLuck==497&&EventCardWrithe+1==CardKinds,"Keep appended character IDs stable");
enum class Character : uint8_t { Ironclad, Silent, Defect, Regent, Necrobinder };
enum class CardColor : uint8_t { Ironclad, Silent, Colorless, Token, Defect, Regent, Necrobinder };
constexpr int CharacterKinds=5;
struct CharacterDefinition { const char* key; const char* name; int starting_hp; RelicMask starting_relic; };
extern const CharacterDefinition character_definitions[CharacterKinds];
CardColor card_color(unsigned kind);
bool card_generated(unsigned kind);
bool card_in_pool(unsigned kind, Character);
unsigned character_pool(Character, CardKind* out, bool rewards_only=false);
enum class CardType : uint8_t { Attack, Skill, Power, Status, Curse, Quest };
struct Definition { const char* key; int16_t cost, damage, block, draw, power;
    int16_t damage_up, block_up, draw_up, power_up; CardType type; bool targeted;
    int16_t hp_loss=0, energy_gain=0, energy_up=0; bool exhausts=false; int16_t creates=0, creates_up=0; int16_t cost_up=0; uint8_t keywords=0,upgraded_keywords=0; };
extern const Definition definitions[CardKinds];
struct Card { CardKind kind; uint8_t upgraded; };
// Keep historical model tokens byte-for-byte: bit 8 is the upgrade flag;
// new model-ID bits occupy bit 9 upward instead of colliding with that flag.
constexpr int card_token(Card c){return int(c.kind&255u)|int((c.kind&0x7f00u)<<1)|(int(c.upgraded)<<8);}
constexpr Card card_from_token(int v){return {CardKind((unsigned(v)&255u)|((unsigned(v)>>1)&0x7f00u)),uint8_t((unsigned(v)>>8)&1u)};}
bool card_upgradeable(Card);
bool card_removable(Card);
bool card_transformable(Card);
enum class Phase : uint8_t { Combat, ExhaustChoice, Reward, Camp, Upgrade, Complete, Dead,
    Map, Shop, Remove, Event, Transform, Treasure, Error, DiscardChoice, ExhaustManyChoice, ReturnChoice, ExtendedChoice, WorldRelicChoice, OriginalEventChoice };
enum class Room : uint8_t { Combat, Event, Shop, Camp, Treasure, Final };
struct MapNode { Room room; uint8_t links,visited,event; };
enum class EventId : uint8_t { Baths, Vegetation, Grove, AromaOfChaos, DoorsOfLightAndDark };
constexpr unsigned EventKinds=5;
enum class EventDeckAction : uint8_t { None, Upgrade, Remove, Transform };
enum class Zone : uint8_t { Draw, Hand, Discard, Exhaust, Power, Limbo };
enum class EffectKind : uint8_t { Damage, Block, Vulnerable, Draw, ChooseExhaust, RandomExhaust,
    CopyDiscard, Strength, Pain, Embrace, Finish, LoseHp, Energy, Weak, NoDraw, ExhaustCard, WhirlwindHits, Poison, ChooseDiscard, CreateShivs, Dexterity, Noxious, Accuracy, RandomPoison, DrawIfNoAttacks, FillHand, ChooseExhaustMany, NoBlock, GreedDamage, ChooseReturn, RandomDamage, Demon, Blades,
    DodgeBlock, DrawNextTurn, EscapeSkillBlock, Extended };
struct Effect { EffectKind kind; int16_t value, target; int16_t aux=0; };
struct Enemy { int32_t hp, max_hp; int16_t block, strength, vulnerable; uint8_t move; int16_t weak=0, poison=0; };
struct Pile { uint16_t ids[MaxCards]; uint16_t count; };
enum class LogKind : uint8_t { Turn, Play, Damage, Block, Exhaust, Draw, Shuffle, Enemy,
    Strength, Pain, Embrace, Heal, Reward, Upgrade, Capacity, Gold, Buy, Remove, Event, Transform, Relic, Potion, LoseHp, Energy, NoDraw, Weak, Poison, Discard, Create, Dexterity, Noxious, Accuracy, PoisonDamage, RelicIndex };
struct Log { LogKind kind; int32_t a,b; };
enum class Potion : uint8_t { Empty, Fire, Block, Blood, Energy, Strength, Dexterity,
    Swift, Explosive, Vulnerable, Weak, Fruit, Forge, Flex, Speed, Bronze, Fortifier,
    AttackPotion, BeetleJuice, BottledPotential, Clarity, ColorlessPotion, CureAll, DistilledChaos, DropletOfPrecognition, Duplicator, EntropicBrew, FairyInABottle, FyshOil, GamblersBrew, GigantificationPotion, HeartOfIron, LiquidMemories, LuckyTonic, MazalethsGift, OrobicAcid, PotionOfBinding, PowderedDemise, PowerPotion, RadiantTincture, RegenPotion, ShacklingPotion, ShipInABottle, SkillPotion, SneckoOil, StableSerum, TouchOfInsanity, SoldiersStew, Ashwater, PoisonPotion, GhostInAJar, CunningPotion, Ambergris, FoulPotion, GlowwaterPotion, PotionShapedRock, BoneBrew, PotOfGhouls, PotionOfDoom, FocusPotion, EssenceOfDarkness, PotionOfCapacity, StarPotion, CosmicConcoction, KingsCourage };
constexpr int PotionKinds=64, RelicKinds=33;
static_assert(RelicKinds<64,"Relic masks use 64 bits");
constexpr int ShopCardCount=7, ShopRelicStart=7, ShopPotionStart=10, ShopRemove=13, ShopLeave=14;
struct PotionDefinition { const char* key; const char* name; const char* description;
    uint8_t rarity; bool combat_only, targeted; uint8_t pool=0; bool automatic=false; };
extern const PotionDefinition potion_definitions[PotionKinds];
const PotionDefinition* potion_definition(Potion);
bool card_colorless(unsigned kind);

enum Relic : RelicMask { Vajra=1, Anchor=2, BagOfPreparation=4, BurningBlood=8,
    Lantern=16, OddlySmoothStone=32, BloodVial=64, HappyFlower=128, Orichalcum=256, PenNib=512,
    Strawberry=1024, Pear=2048, Mango=4096, BagOfMarbles=8192, Akabeko=16384, BronzeScales=32768, RingOfTheSnake=65536,
    HornCleat=1u<<17, CaptainsWheel=1u<<18, MercuryHourglass=1u<<19, ArtOfWar=1u<<20,
    RegalPillow=1u<<21, MealTicket=1u<<22, OldCoin=1u<<23, CentennialPuzzle=1u<<24, Kunai=1u<<25, Shuriken=1u<<26,
    OrnamentalFan=1u<<27, LetterOpener=1u<<28, Nunchaku=1u<<29,CrackedCore=1ull<<30,DivineRight=1ull<<31,BoundPhylactery=1ull<<32 };
enum class RelicHook : uint8_t { CombatStart, TurnStart, TurnEnd, Victory, AfterHandDraw };
struct RelicDefinition { RelicMask id; const char* key; const char* name; const char* description; int asset; };
extern const RelicDefinition relic_definitions[RelicKinds];
const RelicDefinition* relic_definition(RelicMask);

#include "expansion_state.inc"
#include "event_card_state.inc"
#include "ironclad_state.inc"
#include "silent_state.inc"
#include "defect_state.inc"
#include "regent_state.inc"
#include "necro_state.inc"
#include "enemy_state.inc"
#include "world_state.inc"
#include "world_factory_state.inc"
#include "original_event_state.inc"
#include "world_relic_state.inc"
#include "power_order_state.inc"

struct State {
    Phase phase;
    uint8_t practice, floor, enemy_count;
    int16_t hp,max_hp,energy,block,strength,pain,embrace,turn;
    uint8_t power_order[2], power_count;
    uint32_t rng_cards,rng_world,seed,revision;
    Card deck[MaxDeck]; uint16_t deck_count;
    Card cards[MaxCards]; Zone zones[MaxCards]; uint16_t card_count;
    Pile draw,hand,discard,exhaust,powers;
    Enemy enemies[MaxEnemies];
    Effect effects[MaxEffects]; uint16_t effect_count;
    int16_t playing;
    CardKind reward[3];
    Log logs[24]; uint16_t log_count;
    // Entire run state is serialized. Presentation clocks never enter this state.
    MapNode map[MapNodes]; int16_t current_node;
    uint8_t next_row, encounter, removals;
    RelicMask relics, shop_relics[3];
    uint8_t relic_count, relic_order[RelicKinds], relic_counters[RelicKinds];
    int16_t dexterity;
    uint8_t double_attack;
    RelicMask relic_flash;
    uint32_t relic_revision;
    int16_t gold,last_gold;
    Potion potions[MaxPotionSlots];
    uint8_t potion_slots=LegacyPotionSlots;
    CardKind shop_cards[ShopCardCount]; uint8_t shop_potions[3]; uint16_t shop_sold;
    int16_t shop_prices[13];
    EventId event_id; uint8_t event_page,event_count,event_remaining;
    int16_t event_value,event_first;
    uint8_t no_draw; // Cleared at the end of the player side, before next-turn draw.
    int16_t temporary_strength, temporary_dexterity, vigor, attack_vigor, thorns;
    uint8_t reward_claimed; // bit 0: gold; bit 1: card; bit 2: potion.
    Potion reward_potion;
    uint8_t potion_chance; // Legacy expedition drop odds in percent, not a PC RNG stream.
    Character character;
    int16_t noxious_fumes, accuracy;
    uint8_t discard_required; // Mandatory choice suspended before the queued Finish.
    uint8_t exhaust_limit; // Optional multi-exhaust allows 0..limit actual hand IDs.
    int16_t no_block; // Decrements after the enemy side, not immediately on play.
    int16_t demon_form, infinite_blades, block_next_turn, draw_next_turn, flame_barrier;
    uint8_t treasure_state; // 0 closed, 1 revealed, 2 claimed; no implicit rerolls.
    RelicMask treasure_relic; // Zero means the exhausted prototype pool offers 50 gold.
    EventDeckAction event_deck_action; // v10: event choices must never use shop/camp completion.
    ExpansionState expansion;
    IroncladState ironclad;
    SilentState silent;
    DefectState defect;
    RegentState regent;
    NecroState necro;
    WorldState world;
    WorldFactoryState world_factory;
    EventCardState event_cards;
    OriginalEventState original_events;
    EventRunFacts event_run;
    EnemyWorldState enemy_world;
    WorldRelicState world_relics;
    uint8_t attacked_this_turn; // Retained through enemy actions until energy reset.
    RunOptions run_options{};
    PowerOrderState power_sequence{};
    uint16_t test_subject_kills=0;
};
constexpr unsigned LegacySaveVersion=15,CustomSaveVersion=16,SaveVersion=17,SaveCapacity=262144;
void power_order_set(State&,unsigned actor,unsigned id,bool present,unsigned instance=0);
void power_order_clear_actor(State&,unsigned actor);
void power_order_prune(State&);
void power_order_reconcile(State&);
void power_order_migrate(State&);
bool valid_power_order(const State&);
#include "event_card_api.inc"
CardType card_type(const State&,Card,int instance=-1,int deck_index=-1);
uint8_t card_keywords(const State&,Card,int instance=-1,int deck_index=-1);
bool card_removable(const State&,Card,int deck_index=-1);
bool card_transformable(const State&,Card,int deck_index=-1);
bool card_retains(const State&,Card,int instance=-1,int deck_index=-1);
bool card_innate(const State&,Card,int instance=-1,int deck_index=-1);
bool card_exhausts(const State&,Card,int instance=-1,int deck_index=-1);
// Zone::Power denotes the original removed-from-combat result. Active plays
// expose their captured result; all other instance queries are pure previews.
bool card_play_result(const State&,int instance,Zone&pile,bool&top);
struct SaveData { uint8_t bytes[SaveCapacity]; size_t size; };
unsigned potion_slot_count(const State&);
Potion potion_at(const State&,unsigned slot);
bool potion_set(State&,unsigned slot,Potion);
bool add_potion_slots(State&,unsigned count);
void new_run(State&, uint32_t seed, bool practice=false, Character character=Character::Ironclad);
void enter_combat(State&);
bool play(State&, int hand_index, int target);
bool can_play_card(const State&,int instance);
int max_energy(const State&);
bool card_targets_enemy(const State&,Card,int instance=-1,int deck_index=-1);
bool choose_exhaust(State&, int hand_index);
bool choose_discard(State&, const uint16_t* card_ids, unsigned count);
bool choose_exhaust_many(State&, const uint16_t* card_ids, unsigned count);
unsigned choice_min(const State&);
unsigned choice_max(const State&);
bool card_retains(Card);
bool card_innate(Card);
bool card_strike_tag(unsigned kind);
const Pile& choice_pile(const State&);
bool choose_return_to_draw(State&, int card_id);
inline bool combat_phase(Phase p) { return p==Phase::Combat || p==Phase::ExhaustChoice || p==Phase::DiscardChoice || p==Phase::ExhaustManyChoice || p==Phase::ReturnChoice || p==Phase::ExtendedChoice; }
inline bool choice_phase(Phase p) { return p==Phase::ExhaustChoice || p==Phase::DiscardChoice || p==Phase::ExhaustManyChoice || p==Phase::ReturnChoice || p==Phase::ExtendedChoice || p==Phase::WorldRelicChoice || p==Phase::OriginalEventChoice; }
unsigned random_reward_card(State&);
bool end_turn(State&);
bool choose_reward(State&, int index);
bool camp(State&, bool upgrade);
bool upgrade_card(State&, int deck_index);
int cost(Card);
int card_rarity(unsigned kind); // 2 common, 3 uncommon, 4 rare in the pinned registry.
int card_base_damage(const State&,Card,int target=-1,int instance=-1);
int damage(const State&, Card, int target, int instance=-1);
int enemy_damage(const Enemy&);
bool validate(const State&);
bool save(const State&,SaveData&);
bool load(State&,const uint8_t*,size_t);
const Pile& pile(const State&,Zone);
uint32_t random_below(uint32_t&, uint32_t bound);
// Presentation observers are process-local and never enter State or a save.
// Callbacks may inspect the exact recorded state, but must not invoke actions.
// Swapping returns the previous binding for a nested, scoped registration.
struct CoreDamageResult {
    // CreatureCmd.Damage keeps actual HP loss and overkill separate. The
    // receiver convention matches damage commands: -1 player, -2 Osty.
    enum Flag : uint8_t { FullyBlocked=1,BlockBroken=2,ShowZero=4,Hurt=8 };
    int32_t blocked=0,unblocked=0,overkill=0;
    int8_t receiver=-1;uint8_t flags=0,shake_strength=0;
    int amount()const{return unblocked+overkill;}
};
struct CoreObserver {
    void* context=nullptr;
    void(*on_log)(void*,const State&,Log)=nullptr;
    void(*on_enemy_animation)(void*,const State&,int,const char*)=nullptr;
    void(*on_effect)(void*,const State&,Effect)=nullptr;
    // Establish a display command boundary before synchronous damage/power
    // hooks run. Appending preserves existing four-field observer bindings.
    void(*before_effect)(void*,const State&,Effect)=nullptr;
    void(*on_damage)(void*,const State&,const CoreDamageResult&)=nullptr;
    void(*on_power_flash)(void*,const State&,int,const char*)=nullptr;
    // Observe the still-intact card/facts before a permanent-deck compaction.
    // This callback has no return value and cannot change canonical removal.
    void(*on_deck_removing)(void*,const State&,int)=nullptr;
    // One notification per newly created combat identity, including summons.
    // UI statistics must count instances without changing spawned_models.
    void(*on_enemy_spawned)(void*,const State&,unsigned,uint16_t)=nullptr;
    void(*on_run_fact)(void*,const State&,unsigned,unsigned)=nullptr;
    void(*on_history)(void*,const State&,const RunHistoryNotice&)=nullptr;
};
enum class CoreRunFact : uint8_t { ShopGoldSpent,RestChoice };
CoreObserver set_core_observer(CoreObserver);
void notify_enemy_animation(const State&,int slot,const char* trigger);
void notify_power_flash(const State&,int receiver,const char* key);
void notify_deck_removing(const State&,int index);
void notify_enemy_spawned(const State&,unsigned model,uint16_t identity);
void notify_run_fact(const State&,CoreRunFact,unsigned value);
void notify_history(const State&,RunHistoryEvent,unsigned model=0,unsigned detail=0,unsigned flags=0,const char*key=nullptr);
void record(State&,LogKind,int a=0,int b=0);
void create_map(State&);
void finish_room(State&);
bool reachable(const State&,int node);
bool choose_node(State&,int column);
bool buy(State&,int item);
bool remove_card(State&,int deck_index);
bool leave_shop(State&);
bool event_choice(State&,int option);
bool event_option_enabled(const State&,int option);
bool cancel_event_selection(State&);
void complete_event_selection(State&);
bool transform_card(State&,int deck_index);
bool open_treasure(State&);
bool take_treasure(State&);
bool leave_treasure(State&);
bool use_potion(State&,int slot,int target);
bool can_use_potion(const State&,int slot,int target);
bool can_discard_potion(const State&,int slot);
bool discard_potion(State&,int slot);
void draw_cards(State&,int count);
void damage_unpowered(State&,int target,int amount);
void finish_combat_if_won(State&);
bool claim_reward_gold(State&);
bool claim_reward_potion(State&);
bool leave_rewards(State&);
void increase_max_hp(State&,int amount);
void gain_gold(State&,int amount);
int rest_heal_amount(const State&);
void relic_after_hp_loss(State&,int actual_loss);
int remove_price(const State&);
bool has_upgrade(const State&);
void heal(State&,int amount);
void award_victory(State&);
bool gain_relic(State&,RelicMask id);
RelicMask random_relic(State&);
void relic_hook(State&,RelicHook);
void relic_card_played(State&,Card);
unsigned relic_counter_limit(RelicMask id);
bool relic_counter_is_turn_scoped(RelicMask id);
void relic_reset_turn_counters(State&);
void relic_attack_begin(State&);
void relic_attack_end(State&);
int relic_attack_multiplier(const State&,bool preview=false);
int card_block(const State&,Card,int instance=-1);
unsigned relic_counter(const State&,RelicMask id);

// Extended card/potion continuations share the ordinary combat dispatcher.
int power_value(const State&,XPower);
void gain_power(State&,XPower,int);
void queue_extension(State&,XOp,int a=0,int b=0,bool first=false);
int cost(const State&,int card_id);
int card_preview_cost(const State&,Card,int instance);
bool card_exhausts(Card);
bool card_can_generate(unsigned);
Card choice_card(const State&,int token);
bool choose_extended(State&,const uint16_t*,unsigned);
void resolve_pending(State&);
// card_source=-2 uses the active card for non-attack losses; -1 explicitly
// identifies a power, potion or enemy source for damage-history hooks.
// Returns this DamageResult's HP loss before death prevention or healing.
// dealer is an explicit enemy slot; nested non-enemy damage leaves it at -1.
int player_hp_loss(State&,int,bool attack,bool blockable,int card_source=-2,int dealer=-1);
void player_kill(State&,bool force=false);
void player_lose_max_hp(State&,int amount);
void before_potion_use(State&,Potion,int target);
void gain_unpowered_block(State&,int);
void apply_vulnerable(State&,int target,int amount);
void apply_weak(State&,int target,int amount);
void apply_poison(State&,int target,int amount);
bool character_description_value(const State&,Card,int target,int instance,const char* variable,int& out);
bool character_deck_description_value(const State&,Card,int deck_index,const char* variable,int& out);
void notify_deck_card_created(State&,int instance,int deck_index);
void notify_deck_removed(State&,int index);
void notify_deck_replaced(State&,int index);
void notify_deck_copied(State&,int to,int from);
int enemy_damage(const State&,int);
bool potion_in_pool(Potion,Character);
Potion random_potion(State&,bool in_combat=false);
bool procure_potion(State&,Potion);
bool procure_potion_at(State&,Potion,unsigned slot);
void execute_potion(State&,Potion,int);
bool lab_potion(State&,Potion);
bool lab_card(State&,Card);
int card_star_cost(const State&,Card,int instance=-1);
int card_free_preview_cost(const State&,Card,int instance=-1);
void regent_gain_stars(State&,int);
bool necro_grimoire_choice(State&,int deck_index);
} // namespace spire
