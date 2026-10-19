// Included after Card/Pile definitions inside namespace spire. All dimensions
// are bounded; no coroutine stack or transient pointer is part of a save.
constexpr int MaxAutoFrames=12, MaxPowerInstances=16, MaxBombs=16, MaxOrbs=10,MaxPostPlayPowers=52;
enum class XPower:uint8_t {
 Retain,Plating,Buffer,Intangible,Ritual,Regen,Clarity,Radiance,Gigantic,
 Duplicator,Automation,Calamity,Mayhem,Nostalgia,Panache,PrepTime,Fasten,
 RollingBoulder,Stratagem,Entropy,Gambit,Focus,Count
};
constexpr int XPowerCount=(int)XPower::Count;
enum class PostPlayActor:uint8_t {
 Calamity=1,Rage=2,Rupture=3,Afterimage=4,SerpentForm=5,MasterPlanner=6,
 PanacheStart=7,Relics=23,Cards=24,Enemies=25,
 DefectStorm=26,DefectSubroutine=27,RegentBlackHole=28,RegentPaleBlueDot=29,RegentVoidForm=30,
 RegentMonologueStart=31,NecroDevourLife=47,NecroHaunt=48,DefectSmokestack=49,DefectTrashToTreasure=50,RegentArsenal=51,RegentPillarOfCreation=52,
 SilentEnvenom=53,RegentMonarchsGaze=54,NecroReaperForm=55,Count=56
};
enum class XChoice:uint16_t {None,Offer,Draw,Discard,Top,DiscardRedraw,Exhaust,Free,Transform,
#include "ironclad_choices.inc"
#include "silent_choices.inc"
#include "defect_choices.inc"
#include "regent_choices.inc"
#include "necro_choices.inc"
 Count};
enum class XOp:uint16_t {
 Power,AllDamage,StrengthLoss,AllRare,Offer,Generate,ChooseDraw,ChooseDiscard,
 ChooseTop,DiscardRedraw,ExhaustAny,Free,Transform,AutoDraw,AutoDiscard,AutoTop,
 AutoInstance,Draw,DrawOne,EscapeBlock,EnchantReplay,Create,CopyDamage,DamageBlock,
 ReturnNext,Prolong,Restless,Bomb,ShuffleAll,RandomCosts,StrikeReplay,PotionDone,
 Demise,ExhaustHand,Strength,Dexterity,AfterPlay,TurnEnd,EndDiscard,EnemyStart,
 EnemyAct,EnemyDone,TurnStart,TurnDraw,TurnReady,RandomHits,Potion,PlayerDamage,
 Noop,Summon,Forge,ChannelDark,OrbsEnd,BoulderTick,TurnRelics,
#include "ironclad_ops.inc"
#include "silent_ops.inc"
 PostPlayListener,
#include "defect_ops.inc"
#include "regent_ops.inc"
#include "necro_ops.inc"
 AllCharactersLate,AllCharactersBefore,AllCharactersBody,AllCharactersFlush,AllCharactersSideEnd,AllCharactersDrawBefore,AllCharactersBeetle,
 Count
};
static_assert((unsigned)XOp::Count<32768,"Operation IDs must fit signed effect operands");
struct XFrame {
 int16_t playing=-1,target=-1,x=0,last_damage=0,vigor=0;
 uint16_t remaining=0;
 uint8_t double_attack=0,triple=0,returning=0;
};
struct ExpansionState {
 // Relevant player powers share their actual acquisition order across hooks.
 // Zero terminates the prefix; 7..22 identify independent Panache instances.
 uint8_t after_play_order[MaxPostPlayPowers]{};
 int16_t powers[XPowerCount]{};
 int16_t automation[MaxPowerInstances]{},panache[MaxPowerInstances]{},boulder[MaxPowerInstances]{};
 uint8_t automation_count[MaxPowerInstances]{},panache_count[MaxPowerInstances]{},panache_skip[MaxPowerInstances]{};
 // Number of Boulder instances already applied when Entropy first appeared.
 // Entropy stacks in place; later independent Boulders remain after it.
 uint8_t entropy_boulder_prefix=0;
 // 0 = no cost override, n+1 = explicit cost n. turn_cost expires after the
 // physical card play (including its replays) or the side's turn end.
 uint8_t turn_cost[MaxCards]{},combat_cost[MaxCards]{},replay[MaxCards]{};
 uint16_t return_turn[MaxCards]{};
 int16_t blade_damage[MaxCards]{};
 int16_t restore_strength[MaxEnemies]{},demise[MaxEnemies]{},beetle[MaxEnemies]{},doom[MaxEnemies]{};
 Pile choices{};Card offers[3]{};XChoice choice=XChoice::None;
 uint8_t choice_min=0,choice_max=0,choice_free=0;
 XFrame frames[MaxAutoFrames]{};uint8_t depth=0;
 int16_t target=-1,x=0,last_damage=0,last_drawn=-1;
 uint16_t remaining=0,cards_finished=0,nonpower_started=0;
 uint8_t triple=0,returning=0,enemy_side=0,potion_active=0,skip_enemy=0;
 int16_t bomb_damage[MaxBombs]{};uint8_t bomb_turns[MaxBombs]{};
 int16_t stars=0,osty_hp=0,osty_max_hp=0;
 uint8_t orb_slots=0,orb_count=0;int16_t dark[MaxOrbs]{};
 // The original free-card helpers set energy and stars independently.
 uint8_t star_turn_free[MaxCards]{},star_combat_free[MaxCards]{};
};
