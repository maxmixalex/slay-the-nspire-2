// Included after Card/Pile definitions inside namespace spire. All dimensions
// are bounded; no coroutine stack or transient pointer is part of a save.
constexpr int MaxAutoFrames=12, MaxPowerInstances=16, MaxBombs=16, MaxOrbs=10;
enum class XPower:uint8_t {
 Retain,Plating,Buffer,Intangible,Ritual,Regen,Clarity,Radiance,Gigantic,
 Duplicator,Automation,Calamity,Mayhem,Nostalgia,Panache,PrepTime,Fasten,
 RollingBoulder,Stratagem,Entropy,Gambit,Focus,Count
};
constexpr int XPowerCount=(int)XPower::Count;
enum class XChoice:uint8_t {None,Offer,Draw,Discard,Top,DiscardRedraw,Exhaust,Free,Transform,Count};
enum class XOp:uint8_t {
 Power,AllDamage,StrengthLoss,AllRare,Offer,Generate,ChooseDraw,ChooseDiscard,
 ChooseTop,DiscardRedraw,ExhaustAny,Free,Transform,AutoDraw,AutoDiscard,AutoTop,
 AutoInstance,Draw,DrawOne,EscapeBlock,EnchantReplay,Create,CopyDamage,DamageBlock,
 ReturnNext,Prolong,Restless,Bomb,ShuffleAll,RandomCosts,StrikeReplay,PotionDone,
 Demise,ExhaustHand,Strength,Dexterity,AfterPlay,TurnEnd,EndDiscard,EnemyStart,
 EnemyAct,EnemyDone,TurnStart,TurnDraw,TurnReady,RandomHits,Potion,PlayerDamage,
 Noop,Summon,Forge,ChannelDark,OrbsEnd,Count
};
struct XFrame {
 int16_t playing=-1,target=-1,x=0,last_damage=0,vigor=0;
 uint16_t remaining=0;
 uint8_t double_attack=0,triple=0,returning=0;
};
struct ExpansionState {
 int16_t powers[XPowerCount]{};
 int16_t automation[MaxPowerInstances]{},panache[MaxPowerInstances]{},boulder[MaxPowerInstances]{};
 uint8_t automation_count[MaxPowerInstances]{},panache_count[MaxPowerInstances]{},panache_skip[MaxPowerInstances]{};
 // 0 = no cost override, n+1 = explicit cost n. turn_cost expires after the
 // physical card play (including its replays) or at the next player turn.
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
};
