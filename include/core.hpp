#pragma once
#include <stdint.h>
#include <stddef.h>

namespace spire {
constexpr int IroncladCards=29, SilentCards=29, CardKinds=114, MaxCards=512, MaxDeck=128, MaxHand=10, MaxEnemies=2, MaxEffects=512;
constexpr int MapRows=12, MapColumns=3, MapNodes=MapRows*MapColumns;
enum CardId : uint8_t { Strike, Defend, Bash, Anger, PommelStrike, ShrugItOff, BurningPact,
    TrueGrit, TwinStrike, Inflame, IronWave, Whirlwind, FeelNoPain, DarkEmbrace,
    Bloodletting, Hemokinesis, Impervious, Offering, Bludgeon, Thunderclap,
    Uppercut, BodySlam, BattleTrance, FiendFire, Finesse, FlashOfSteel,
    StrikeSilent, DefendSilent, Neutralize, Survivor, Acrobatics, Backflip,
    DaggerThrow, DeadlyPoison, PoisonedStab, CloakAndDagger, BladeDance,
    DaggerSpray, Dash, LegSweep, Footwork, NoxiousFumes, BouncingFlask,
    Prepared, Slice, Deflect, SuckerPunch, Adrenaline, Accuracy, LeadingStrike, Shiv, Impatience, MasterOfStrategy, Shockwave, Scrawl, Purity, HandOfGreed, PanicButton,
    Headbutt, SwordBoomerang, PerfectedStrike, FlameBarrier, DemonForm,
    DodgeAndRoll, InfiniteBlades, Backstab, EscapePlan, Predator,
    Alchemize, Anointed, Automation, BeatDown, Bolas, Calamity, Catastrophe, DarkShackles, Discovery, DramaticEntrance, Entropy, Equilibrium, EternalArmor, Fasten, Fisticuffs, GoldAxe, HiddenGem, JackOfAllTrades, Jackpot, Mayhem, MindBlast, Nostalgia, Omnislice, Panache, PrepTime, Production, Prolong, Prowess, Rend, Restlessness, RollingBoulder, Salvo, SecretTechnique, SecretWeapon, SeekerStrike, Splash, Stratagem, TheBomb, TheGambit, ThinkingAhead, ThrummingHatchet, UltimateDefend, UltimateStrike, Volley, Soul, SovereignBlade };
enum class Character : uint8_t { Ironclad, Silent };
enum class CardColor : uint8_t { Ironclad, Silent, Colorless, Token };
constexpr int CharacterKinds=2;
struct CharacterDefinition { const char* key; const char* name; int starting_hp; uint32_t starting_relic; };
extern const CharacterDefinition character_definitions[CharacterKinds];
CardColor card_color(unsigned kind);
bool card_generated(unsigned kind);
bool card_in_pool(unsigned kind, Character);
unsigned character_pool(Character, uint8_t* out, bool rewards_only=false);
enum class CardType : uint8_t { Attack, Skill, Power };
struct Definition { const char* key; int8_t cost, damage, block, draw, power;
    int8_t damage_up, block_up, draw_up, power_up; CardType type; bool targeted;
    int8_t hp_loss=0, energy_gain=0, energy_up=0; bool exhausts=false; int8_t creates=0, creates_up=0; int8_t cost_up=0; uint8_t keywords=0,upgraded_keywords=0; };
extern const Definition definitions[CardKinds];
struct Card { uint8_t kind, upgraded; };
enum class Phase : uint8_t { Combat, ExhaustChoice, Reward, Camp, Upgrade, Complete, Dead,
    Map, Shop, Remove, Event, Transform, Treasure, Error, DiscardChoice, ExhaustManyChoice, ReturnChoice, ExtendedChoice };
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
struct Enemy { int16_t hp, max_hp, block, strength, vulnerable; uint8_t move; int16_t weak=0, poison=0; };
struct Pile { uint16_t ids[MaxCards]; uint16_t count; };
enum class LogKind : uint8_t { Turn, Play, Damage, Block, Exhaust, Draw, Shuffle, Enemy,
    Strength, Pain, Embrace, Heal, Reward, Upgrade, Capacity, Gold, Buy, Remove, Event, Transform, Relic, Potion, LoseHp, Energy, NoDraw, Weak, Poison, Discard, Create, Dexterity, Noxious, Accuracy, PoisonDamage };
struct Log { LogKind kind; int32_t a,b; };
enum class Potion : uint8_t { Empty, Fire, Block, Blood, Energy, Strength, Dexterity,
    Swift, Explosive, Vulnerable, Weak, Fruit, Forge, Flex, Speed, Bronze, Fortifier,
    AttackPotion, BeetleJuice, BottledPotential, Clarity, ColorlessPotion, CureAll, DistilledChaos, DropletOfPrecognition, Duplicator, EntropicBrew, FairyInABottle, FyshOil, GamblersBrew, GigantificationPotion, HeartOfIron, LiquidMemories, LuckyTonic, MazalethsGift, OrobicAcid, PotionOfBinding, PowderedDemise, PowerPotion, RadiantTincture, RegenPotion, ShacklingPotion, ShipInABottle, SkillPotion, SneckoOil, StableSerum, TouchOfInsanity, SoldiersStew, Ashwater, PoisonPotion, GhostInAJar, CunningPotion, Ambergris, FoulPotion, GlowwaterPotion, PotionShapedRock, BoneBrew, PotOfGhouls, PotionOfDoom, FocusPotion, EssenceOfDarkness, PotionOfCapacity, StarPotion, CosmicConcoction, KingsCourage };
constexpr int PotionKinds=64, RelicKinds=30;
static_assert(RelicKinds<32,"Expand relic masks and versioned save widths before adding more relics");
constexpr int ShopCardCount=7, ShopRelicStart=7, ShopPotionStart=10, ShopRemove=13, ShopLeave=14;
struct PotionDefinition { const char* key; const char* name; const char* description;
    uint8_t rarity; bool combat_only, targeted; uint8_t pool=0; bool automatic=false; };
extern const PotionDefinition potion_definitions[PotionKinds];
const PotionDefinition* potion_definition(Potion);
bool card_colorless(unsigned kind);

enum Relic : uint32_t { Vajra=1, Anchor=2, BagOfPreparation=4, BurningBlood=8,
    Lantern=16, OddlySmoothStone=32, BloodVial=64, HappyFlower=128, Orichalcum=256, PenNib=512,
    Strawberry=1024, Pear=2048, Mango=4096, BagOfMarbles=8192, Akabeko=16384, BronzeScales=32768, RingOfTheSnake=65536,
    HornCleat=1u<<17, CaptainsWheel=1u<<18, MercuryHourglass=1u<<19, ArtOfWar=1u<<20,
    RegalPillow=1u<<21, MealTicket=1u<<22, OldCoin=1u<<23, CentennialPuzzle=1u<<24, Kunai=1u<<25, Shuriken=1u<<26,
    OrnamentalFan=1u<<27, LetterOpener=1u<<28, Nunchaku=1u<<29 };
enum class RelicHook : uint8_t { CombatStart, TurnStart, TurnEnd, Victory };
struct RelicDefinition { uint32_t id; const char* key; const char* name; const char* description; int asset; };
extern const RelicDefinition relic_definitions[RelicKinds];
const RelicDefinition* relic_definition(unsigned);

#include "expansion_state.inc"

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
    uint8_t reward[3];
    Log logs[24]; uint16_t log_count;
    // Entire run state is serialized. Presentation clocks never enter this state.
    MapNode map[MapNodes]; int16_t current_node;
    uint8_t next_row, encounter, removals;
    uint32_t relics, shop_relics[3];
    uint8_t relic_count, relic_order[RelicKinds], relic_counters[RelicKinds];
    int16_t dexterity;
    uint8_t double_attack;
    uint32_t relic_flash;
    uint32_t relic_revision;
    int16_t gold,last_gold;
    Potion potions[2];
    uint8_t shop_cards[ShopCardCount],shop_potions[3]; uint16_t shop_sold;
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
    uint32_t treasure_relic; // Zero means the exhausted prototype pool offers 50 gold.
    EventDeckAction event_deck_action; // v10: event choices must never use shop/camp completion.
    ExpansionState expansion;
    uint8_t attacked_this_turn; // Retained through enemy actions until energy reset.

};
struct SaveData { uint8_t bytes[65536]; size_t size; };
void new_run(State&, uint32_t seed, bool practice=false, Character character=Character::Ironclad);
void enter_combat(State&);
bool play(State&, int hand_index, int target);
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
inline bool choice_phase(Phase p) { return p==Phase::ExhaustChoice || p==Phase::DiscardChoice || p==Phase::ExhaustManyChoice || p==Phase::ReturnChoice || p==Phase::ExtendedChoice; }
unsigned random_reward_card(State&);
bool end_turn(State&);
bool choose_reward(State&, int index);
bool camp(State&, bool upgrade);
bool upgrade_card(State&, int deck_index);
int cost(Card);
int card_rarity(unsigned kind); // 2 common, 3 uncommon, 4 rare in the pinned registry.
int card_base_damage(const State&,Card);
int damage(const State&, Card, int target, int instance=-1);
int enemy_damage(const Enemy&);
bool validate(const State&);
bool save(const State&,SaveData&);
bool load(State&,const uint8_t*,size_t);
const Pile& pile(const State&,Zone);
uint32_t random_below(uint32_t&, uint32_t bound);
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
bool gain_relic(State&,unsigned id);
unsigned random_relic(State&);
void relic_hook(State&,RelicHook);
void relic_card_played(State&,Card);
unsigned relic_counter_limit(unsigned id);
bool relic_counter_is_turn_scoped(unsigned id);
void relic_reset_turn_counters(State&);
void relic_attack_begin(State&);
void relic_attack_end(State&);
int relic_attack_multiplier(const State&,bool preview=false);
int card_block(const State&,Card);
unsigned relic_counter(const State&,unsigned id);

// Extended card/potion continuations share the ordinary combat dispatcher.
int power_value(const State&,XPower);
void gain_power(State&,XPower,int);
void queue_extension(State&,XOp,int a=0,int b=0,bool first=false);
int cost(const State&,int card_id);
bool card_exhausts(Card);
bool card_can_generate(unsigned);
Card choice_card(const State&,int token);
bool choose_extended(State&,const uint16_t*,unsigned);
void resolve_pending(State&);
void player_hp_loss(State&,int,bool attack,bool blockable);
int enemy_damage(const State&,int);
bool potion_in_pool(Potion,Character);
Potion random_potion(State&);
bool procure_potion(State&,Potion);
void execute_potion(State&,Potion,int);
bool lab_potion(State&,Potion);
bool lab_card(State&,Card);
} // namespace spire
