// Every boundary is an ordinary saved effect, so a selection inside a hook
// cannot run the next side, draw or reward before the selected effect returns.
enum class WorldStage:uint8_t {
 EnterRoom,CombatStart,CombatStartLate,PlayerStart,PlayerEnemyStart,
 PlayerClearBlock,PlayerBlockRelics,PlayerBlockCards,PlayerEnergy,
 PlayerEnergyRelics,PlayerEnergyLate,PlayerBeforeDraw,PlayerBeforeDrawRelics,
 PlayerBeforeDrawCards,PlayerDraw,PlayerAfterDraw,PlayerAfterDrawRelics,
 PlayerAfterDrawLate,PlayerAfterSide,PlayerAuto,PlayerAutoRelics,
 PlayerAutoCards,PlayerAutoLate,PlayerReady,
 EndVeryEarly,EndEarly,EndPowers,EndRelics,EndOrbs,EndHand,
 EndFlush,EndAfterFlush,EndSidePowers,EndSideEnemies,EndSideRelics,SwitchEnemy,
 EnemyBeforeStart,EnemyAfterStart,EnemyActBegin,EnemyBeforeEndVeryEarly,
 EnemyBeforeEndEarly,EnemyBeforeEnd,EnemyEndPowers,EnemyEndRelics,EnemyAfterEnd,
 NextPlayer,VictoryEarly,VictoryNormal,VictoryEnd,VictoryCleanup,
 BeforeCardRelics,BeforeCardCards,DrawRelics,DrawCards,DrawEnemies,
 StarsSpent,HandEmptied,OrbPassive,Count
};
static void world_stage(State&,WorldStage,int value=0,bool first=true);
static void world_shared_effect(State&,WorldStage,int);
static bool world_shared_active(const State&);
static void victory_cleanup(State&);
static void before_current_cards(State&);
static bool world_victory_pending(const State&);
static void world_card_entered(State&,int);
