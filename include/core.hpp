#pragma once
#include <stdint.h>
#include <stddef.h>
#include "relic_set.hpp"

namespace spire {
constexpr int IroncladCards=29, SilentCards=29, CardKinds=112, MaxCards=512, MaxDeck=128, MaxHand=10, MaxEnemies=2, MaxEffects=512;
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
    Alchemize, Anointed, Automation, BeatDown, Bolas, Calamity, Catastrophe, DarkShackles, Discovery, DramaticEntrance, Entropy, Equilibrium, EternalArmor, Fasten, Fisticuffs, GoldAxe, HiddenGem, JackOfAllTrades, Jackpot, Mayhem, MindBlast, Nostalgia, Omnislice, Panache, PrepTime, Production, Prolong, Prowess, Rend, Restlessness, RollingBoulder, Salvo, SecretTechnique, SecretWeapon, SeekerStrike, Splash, Stratagem, TheBomb, TheGambit, ThinkingAhead, ThrummingHatchet, UltimateDefend, UltimateStrike, Volley };
enum class Character : uint8_t { Ironclad, Silent };
enum class CardColor : uint8_t { Ironclad, Silent, Colorless };
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
    Map, Shop, Remove, Event, Transform, Treasure, Error, DiscardChoice, ExhaustManyChoice, ReturnChoice, BulkChoice };
enum class Room : uint8_t { Combat, Event, Shop, Camp, Treasure, Final };
struct MapNode { Room room; uint8_t links,visited,event; };
enum class EventId : uint8_t { Baths, Vegetation, Grove, AromaOfChaos, DoorsOfLightAndDark };
constexpr unsigned EventKinds=5;
enum class EventDeckAction : uint8_t { None, Upgrade, Remove, Transform };
enum class Zone : uint8_t { Draw, Hand, Discard, Exhaust, Power, Limbo };
enum class EffectKind : uint8_t { Damage, Block, Vulnerable, Draw, ChooseExhaust, RandomExhaust,
    CopyDiscard, Strength, Pain, Embrace, Finish, LoseHp, Energy, Weak, NoDraw, ExhaustCard, WhirlwindHits, Poison, ChooseDiscard, CreateShivs, Dexterity, Noxious, Accuracy, RandomPoison, DrawIfNoAttacks, FillHand, ChooseExhaustMany, NoBlock, GreedDamage, ChooseReturn, RandomDamage, Demon, Blades,
    DodgeBlock, DrawNextTurn, EscapeSkillBlock, Bulk };
struct Effect { EffectKind kind; int16_t value, target; int16_t aux=0; };
struct Enemy { int16_t hp, max_hp, block, strength, vulnerable; uint8_t move; int16_t weak=0, poison=0; };
struct Pile { uint16_t ids[MaxCards]; uint16_t count; };
enum class LogKind : uint8_t { Turn, Play, Damage, Block, Exhaust, Draw, Shuffle, Enemy,
    Strength, Pain, Embrace, Heal, Reward, Upgrade, Capacity, Gold, Buy, Remove, Event, Transform, Relic, Potion, LoseHp, Energy, NoDraw, Weak, Poison, Discard, Create, Dexterity, Noxious, Accuracy, PoisonDamage };
struct Log { LogKind kind; int32_t a,b; };
enum class Potion : uint8_t { Empty, Fire, Block, Blood, Energy, Strength, Dexterity,
    Swift, Explosive, Vulnerable, Weak, Fruit, Forge, Flex, Speed, Bronze, Fortifier,
    AttackPotion, BeetleJuice, BottledPotential, Clarity, ColorlessPotion, CureAll, DistilledChaos, DropletOfPrecognition, Duplicator, EntropicBrew, FairyInABottle, FyshOil, GamblersBrew, GigantificationPotion, HeartOfIron, LiquidMemories, LuckyTonic, MazalethsGift, OrobicAcid, PotionOfBinding, PowderedDemise, PowerPotion, RadiantTincture, RegenPotion, ShacklingPotion, ShipInABottle, SkillPotion, SneckoOil, StableSerum, TouchOfInsanity, SoldiersStew, Ashwater, PoisonPotion, GhostInAJar, CunningPotion, Ambergris, FoulPotion, GlowwaterPotion, PotionShapedRock };
constexpr int PotionKinds=55, RelicKinds=116;
static_assert(RelicKinds<=RelicSet::Words*32,"Relic registry exceeds bounded set storage");
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
    OrnamentalFan=1u<<27, LetterOpener=1u<<28, Nunchaku=1u<<29,
    AmethystAubergine=0x80000000u|30u,
    Bellows=0x80000000u|31u,
    BeltBuckle=0x80000000u|32u,
    BeatingRemnant=0x80000000u|33u,
    BookOfFiveRings=0x80000000u|34u,
    BowlerHat=0x80000000u|35u,
    Bread=0x80000000u|36u,
    BurningSticks=0x80000000u|37u,
    Candelabra=0x80000000u|38u,
    Chandelier=0x80000000u|39u,
    ChemicalX=0x80000000u|40u,
    CloakClasp=0x80000000u|41u,
    DingyRug=0x80000000u|42u,
    DragonFruit=0x80000000u|43u,
    EternalFeather=0x80000000u|44u,
    FestivePopper=0x80000000u|45u,
    FrozenEgg=0x80000000u|46u,
    GamblingChip=0x80000000u|47u,
    GamePiece=0x80000000u|48u,
    GhostSeed=0x80000000u|49u,
    Gorget=0x80000000u|50u,
    GremlinHorn=0x80000000u|51u,
    IceCream=0x80000000u|52u,
    IntimidatingHelmet=0x80000000u|53u,
    JossPaper=0x80000000u|54u,
    Kusarigama=0x80000000u|55u,
    LavaLamp=0x80000000u|56u,
    LeesWaffle=0x80000000u|57u,
    LoomingFruit=0x80000000u|58u,
    LuckyFysh=0x80000000u|59u,
    MeatOnTheBone=0x80000000u|60u,
    MembershipCard=0x80000000u|61u,
    MiniatureCannon=0x80000000u|62u,
    MoltenEgg=0x80000000u|63u,
    MummifiedHand=0x80000000u|64u,
    Pantograph=0x80000000u|65u,
    ParryingShield=0x80000000u|66u,
    Pendulum=0x80000000u|67u,
    Permafrost=0x80000000u|68u,
    PetrifiedToad=0x80000000u|69u,
    Planisphere=0x80000000u|70u,
    Pocketwatch=0x80000000u|71u,
    PotionBelt=0x80000000u|72u,
    RainbowRing=0x80000000u|73u,
    RazorTooth=0x80000000u|74u,
    RedMask=0x80000000u|75u,
    ReptileTrinket=0x80000000u|76u,
    RingingTriangle=0x80000000u|77u,
    RippleBasin=0x80000000u|78u,
    ScreamingFlagon=0x80000000u|79u,
    SparklingRouge=0x80000000u|80u,
    StoneCalendar=0x80000000u|81u,
    StoneCracker=0x80000000u|82u,
    StrikeDummy=0x80000000u|83u,
    SturdyClamp=0x80000000u|84u,
    TheAbacus=0x80000000u|85u,
    TheCourier=0x80000000u|86u,
    TinyMailbox=0x80000000u|87u,
    Toolbox=0x80000000u|88u,
    ToxicEgg=0x80000000u|89u,
    TungstenRod=0x80000000u|90u,
    TuningFork=0x80000000u|91u,
    UnceasingTop=0x80000000u|92u,
    UnsettlingLamp=0x80000000u|93u,
    Vambrace=0x80000000u|94u,
    VenerableTeaSet=0x80000000u|95u,
    VeryHotCocoa=0x80000000u|96u,
    VexingPuzzlebox=0x80000000u|97u,
    WarPaint=0x80000000u|98u,
    Whetstone=0x80000000u|99u,
    WhiteBeastStatue=0x80000000u|100u,
    LizardTail=0x80000000u|101u,
    Brimstone=0x80000000u|102u,
    CharonsAshes=0x80000000u|103u,
    DemonTongue=0x80000000u|104u,
    PaperPhrog=0x80000000u|105u,
    RedSkull=0x80000000u|106u,
    RuinedHelmet=0x80000000u|107u,
    SelfFormingClay=0x80000000u|108u,
    HelicalDart=0x80000000u|109u,
    NinjaScroll=0x80000000u|110u,
    PaperKrane=0x80000000u|111u,
    SneckoSkull=0x80000000u|112u,
    Tingsha=0x80000000u|113u,
    ToughBandages=0x80000000u|114u,
    TwistedFunnel=0x80000000u|115u };
enum class RelicHook : uint8_t { CombatStart, TurnStart, TurnEnd, Victory };
struct RelicDefinition { uint32_t id; const char* key; const char* name; const char* description; int asset; };
extern const RelicDefinition relic_definitions[RelicKinds];
const RelicDefinition* relic_definition(unsigned);

#include "bulk_state.inc"

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
    RelicSet relics; uint32_t shop_relics[3];
    uint16_t relic_count, relic_order[RelicKinds], relic_counters[RelicKinds];
    int16_t dexterity;
    uint8_t double_attack;
    RelicSet relic_flash;
    uint32_t relic_revision;
    int16_t gold,last_gold;
    Potion potions[MaxPotionSlots]; uint8_t potion_slots;
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
    BulkState bulk;
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
inline bool combat_phase(Phase p) { return p==Phase::Combat || p==Phase::ExhaustChoice || p==Phase::DiscardChoice || p==Phase::ExhaustManyChoice || p==Phase::ReturnChoice || p==Phase::BulkChoice; }
inline bool choice_phase(Phase p) { return p==Phase::ExhaustChoice || p==Phase::DiscardChoice || p==Phase::ExhaustManyChoice || p==Phase::ReturnChoice || p==Phase::BulkChoice; }
unsigned random_reward_card(State&);
bool end_turn(State&);
bool choose_reward(State&, int index);
bool camp(State&, bool upgrade);
bool upgrade_card(State&, int deck_index);
int cost(Card);
int card_rarity(unsigned kind); // 2 common, 3 uncommon, 4 rare in the pinned registry.
int card_base_damage(const State&,Card);
int damage(const State&, Card, int target);
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


int cost(const State&,int card_id);
bool card_exhausts(Card);
bool card_can_generate(unsigned kind);
Card choice_card(const State&,int token);
bool choose_bulk(State&,const uint16_t*,unsigned);
void bulk_resolve(State&);
void bulk_enqueue(State&,BulkOp,int a=0,int b=0,bool first=false);
int bulk_power(const State&,BulkPower);
void bulk_power_add(State&,BulkPower,int);
int enemy_damage(const State&,int);
bool relic_in_pool(unsigned,Character);
void bulk_merchant_restock(State&,int);
bool card_defend_tag(unsigned);
void bulk_upgrade_random(State&,int count,CardType);
void bulk_use_potion(State&,Potion,int);
void bulk_relic_hook(State&,RelicHook,unsigned);
void bulk_relic_obtained(State&,unsigned);
void bulk_relic_played(State&,Card,unsigned);
void bulk_after_shuffle(State&);
void bulk_after_draw(State&,int);
void bulk_after_exhaust(State&,int);
void bulk_after_discard(State&,int);
void bulk_enemy_died(State&,int);
void bulk_add_strength(State&,int);
void bulk_add_dexterity(State&,int,bool temporary=false);
int bulk_effective_dexterity(const State&);
int bulk_attack_bonus(const State&,Card);
int bulk_card_block(State&,Card,int);
void bulk_player_damage(State&,int,bool attack);
bool bulk_revival(State&);
void bulk_deck_added(State&,Card&);
void bulk_room_entered(State&,Room);
void bulk_potion_changed(State&);
bool potion_in_pool(Potion,Character);
Potion random_potion(State&);
bool procure_potion(State&,Potion);
void bulk_grant_random(State&,int filter,bool upgraded=false,bool free=false);
int shop_price(const State&,int amount);

} // namespace spire
