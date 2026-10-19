// v13: bounded card-instance facts for the pinned single-player Silent pool.
enum class SPower:uint8_t {
 Accelerant,Afterimage,Blur,Burst,CorrosiveWave,Envenom,FanOfKnives,FreeSkill,
 MasterPlanner,PhantomBlades,SerpentForm,ShadowStep,Shadowmeld,Speedster,
 ToolsOfTheTrade,Tracking,WellLaidPlans,WraithForm,DoubleDamage,EnergyNextTurn,
 InfiniteBlades,NoxiousFumes,Count
};
constexpr int SPowerCount=(int)SPower::Count, MaxNightmares=16;
struct SilentSnapshot {
 Card card{};
 uint8_t turn_cost=0,combat_cost=0,replay=0,sly=0,retain=0,inky=0,bullet_free=0;
 uint16_t discount=0,turn_origin=0,combat_origin=0,bullet_origin=0,return_turn=0;
 int16_t blade_damage=0,iron_damage=0;
 uint16_t stomp_discount=0,stomp_turn_origin=0,stomp_combat_origin=0;
 // v14 appends these snapshot facts in the codec; the v13 snapshot prefix
 // remains byte-for-byte stable. Permanent bindings and play history are not
 // clone modifiers and are deliberately absent.
 int16_t defect_value=0,defect_cost_delta=0,defect_turn_origin=0,defect_combat_origin=0;
 uint16_t regent_kick_discount=0,regent_kick_turn_origin=0,regent_kick_combat_origin=0;
 int16_t regent_punch_damage=0;
 uint8_t necro_ethereal=0,necro_flatten_zero=0;
 int16_t necro_cost_delta=0,necro_turn_origin=0,necro_combat_origin=0,necro_flatten_origin=0,necro_scythe_value=0;
 uint8_t star_turn_free=0,star_combat_free=0;
};
struct SilentState {
 int16_t powers[SPowerCount]{};
 // Power entries are unique; SPowerCount+i denotes an independent Nightmare.
 uint8_t power_order[SPowerCount+MaxNightmares]{},power_count=0;
 uint16_t attacks_finished=0,skills_finished=0,discarded_turn=0,drawn_combat=0;
 uint8_t shiv_finished=0;
 // Sly/Retain: bit 0 is a combat keyword, bit 1 expires after the hand flush.
 uint8_t sly[MaxCards]{},retain[MaxCards]{},inky[MaxCards]{},bullet_free[MaxCards]{};
 // Pinpoint uses AddThisTurn; UpMySleeve uses AddThisCombat. Later Set
 // modifiers hide earlier reductions, with separate expiry boundaries.
 uint16_t discount[MaxCards]{},turn_origin[MaxCards]{},combat_origin[MaxCards]{},bullet_origin[MaxCards]{};
 int16_t strangle[MaxEnemies]{},enemy_artifact[MaxEnemies]{};
 // BeforeCardPlayed captures the actual old amount, excluding new stacks
 // installed by the card itself. Nested automatic plays have independent slots.
 int16_t afterimage_before[MaxAutoFrames+1]{},serpent_before[MaxAutoFrames+1]{};
 int16_t strangle_before[MaxAutoFrames+1][MaxEnemies]{};
 // Expertise captures the actual next draw at its own play depth.
 uint8_t retain_waiting[MaxAutoFrames+1]{};
 int16_t retain_drawn[MaxAutoFrames+1]{};
 SilentSnapshot nightmares[MaxNightmares]{};
 uint8_t nightmare_active[MaxNightmares]{};
 uint16_t extra_rewards=0;
};
