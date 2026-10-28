// v14 Regent state. Stars and forged damage remain in ExpansionState so the
// older colorless/potion paths and saves share exactly one resource system.
enum class RPower:uint8_t {
 Arsenal,BlackHole,ChildOfTheStars,Furnace,Genesis,MonarchsGaze,PaleBlueDot,
 Parry,PillarOfCreation,Reflect,Royalties,SeekingEdge,SpectrumShift,SwordSage,
 TheSealedThrone,Tyranny,VoidForm,ForegoneConclusion,EnergyNextTurn,StarsNextTurn,
 Monologue,Orbit,Count
};
constexpr int RPowerCount=(int)RPower::Count;
struct RegentState {
 int16_t powers[RPowerCount]{};
 uint8_t power_order[RPowerCount]{},power_count=0;
 uint16_t cards_created=0,stars_gained_turn=0,cards_finished_turn=0,skills_finished_turn=0;
 uint16_t hits_turn[MaxEnemies]{},void_played_turn=0;
 uint16_t kick_discount[MaxCards]{},kick_turn_origin[MaxCards]{},kick_combat_origin[MaxCards]{};
 int16_t punch_damage[MaxCards]{};
 // Orbit and Monologue are truly instanced powers; their counters and
 // acquisition positions cannot be replaced by one merged amount.
 int16_t orbit[MaxPowerInstances]{},monologue[MaxPowerInstances]{},monologue_applied[MaxPowerInstances]{};
 uint8_t orbit_remainder[MaxPowerInstances]{};
 int16_t monologue_capture[MaxPowerInstances][MaxAutoFrames+1]{};
 int16_t stars_spent[MaxAutoFrames+1]{},star_x[MaxAutoFrames+1]{};
 uint8_t automatic[MaxAutoFrames+1]{};
 int16_t conqueror[MaxEnemies]{};
 uint8_t pale_activated=0,end_turn_requested=0;
};
