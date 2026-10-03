#pragma once
#include <stdint.h>
#include <stddef.h>

namespace spire {
constexpr int CardKinds=14, MaxCards=192, MaxDeck=64, MaxHand=10, MaxEnemies=2, MaxEffects=96;
constexpr int MapRows=12, MapColumns=3, MapNodes=MapRows*MapColumns;
enum CardId : uint8_t { Strike, Defend, Bash, Anger, PommelStrike, ShrugItOff, BurningPact,
    TrueGrit, TwinStrike, Inflame, IronWave, Whirlwind, FeelNoPain, DarkEmbrace };
enum class CardType : uint8_t { Attack, Skill, Power };
struct Definition { const char* key; int8_t cost, damage, block, draw, power;
    int8_t damage_up, block_up, draw_up, power_up; CardType type; bool targeted; };
extern const Definition definitions[CardKinds];
struct Card { uint8_t kind, upgraded; };
enum class Phase : uint8_t { Combat, ExhaustChoice, Reward, Camp, Upgrade, Complete, Dead,
    Map, Shop, Remove, Event, Transform, Treasure, Error };
enum class Room : uint8_t { Combat, Event, Shop, Camp, Treasure, Final };
struct MapNode { Room room; uint8_t links,visited,event; };
enum class EventId : uint8_t { Baths, Vegetation, Grove };
enum class Zone : uint8_t { Draw, Hand, Discard, Exhaust, Power, Limbo };
enum class EffectKind : uint8_t { Damage, Block, Vulnerable, Draw, ChooseExhaust, RandomExhaust,
    CopyDiscard, Strength, Pain, Embrace, Finish };
struct Effect { EffectKind kind; int16_t value, target; };
struct Enemy { int16_t hp, max_hp, block, strength, vulnerable; uint8_t move; };
struct Pile { uint16_t ids[MaxCards]; uint16_t count; };
enum class LogKind : uint8_t { Turn, Play, Damage, Block, Exhaust, Draw, Shuffle, Enemy,
    Strength, Pain, Embrace, Heal, Reward, Upgrade, Capacity, Gold, Buy, Remove, Event, Transform, Relic, Potion };
struct Log { LogKind kind; int16_t a,b; };
enum class Potion : uint8_t { Empty, Fire, Block, Blood };
constexpr int RelicKinds=10;
enum Relic : uint16_t { Vajra=1, Anchor=2, BagOfPreparation=4, BurningBlood=8,
    Lantern=16, OddlySmoothStone=32, BloodVial=64, HappyFlower=128, Orichalcum=256, PenNib=512 };
enum class RelicHook : uint8_t { CombatStart, TurnStart, TurnEnd, Victory };
struct RelicDefinition { uint16_t id; const char* key; const char* name; const char* description; int asset; };
extern const RelicDefinition relic_definitions[RelicKinds];
const RelicDefinition* relic_definition(unsigned);

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
    uint16_t relics, shop_relic;
    uint8_t relic_count, relic_order[RelicKinds], relic_counters[RelicKinds];
    int16_t dexterity;
    uint8_t double_attack;
    uint16_t relic_flash;
    uint32_t relic_revision;
    int16_t gold,last_gold;
    Potion potions[2];
    uint8_t shop_cards[4],shop_potion,shop_sold;
    int16_t shop_prices[6];
    EventId event_id; uint8_t event_page,event_count,event_remaining;
    int16_t event_value,event_first;
};
struct SaveData { uint8_t bytes[16384]; size_t size; };
void new_run(State&, uint32_t seed, bool practice=false);
void enter_combat(State&);
bool play(State&, int hand_index, int target);
bool choose_exhaust(State&, int hand_index);
bool end_turn(State&);
bool choose_reward(State&, int index);
bool camp(State&, bool upgrade);
bool upgrade_card(State&, int deck_index);
int cost(Card);
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
bool transform_card(State&,int deck_index);
bool take_treasure(State&);
bool use_potion(State&,int slot,int target);
int remove_price(const State&);
bool has_upgrade(const State&);
void heal(State&,int amount);
void award_victory(State&);
bool gain_relic(State&,unsigned id);
unsigned random_relic(State&);
void relic_hook(State&,RelicHook);
void relic_attack_begin(State&);
void relic_attack_end(State&);
int relic_attack_multiplier(const State&,bool preview=false);
int card_block(const State&,Card);
unsigned relic_counter(const State&,unsigned id);

} // namespace spire
