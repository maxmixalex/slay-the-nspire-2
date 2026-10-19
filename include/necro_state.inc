// v14: Necrobinder's single-player model state. Pet HP and enemy Doom remain
// in ExpansionState, shared with the original Summon/Doom/Soul potions.
enum class NPower : uint8_t {
 Calcify,CallOfTheVoid,Countdown,DanseMacabre,Demesne,DevourLife,Friendship,
 Haunt,Lethality,NecroMastery,Neurosurge,Pagestorm,ReaperForm,SentryMode,
 Shroud,SleightOfFlesh,SpiritOfAsh,Veilpiercer,BorrowedTime,EnergyNextTurn,
 SummonNextTurn,ForbiddenGrimoire,Count
};
constexpr int NPowerCount=(int)NPower::Count;
enum class NDebuff:uint8_t {
 Weak,Vulnerable,Poison,Strength,Beetle,Demise,Doom,Strangle,Debilitate,Hang,Oblivion,SicEm,Conqueror,
 DarkShackles,Mangle,EnfeeblingTouch,PiercingWail,CrushUnder,DyingStar,MonarchsGaze,Count
};
constexpr int NDebuffCount=(int)NDebuff::Count;
enum class NTempStrength:uint8_t {DarkShackles,Mangle,EnfeeblingTouch,PiercingWail,CrushUnder,DyingStar,MonarchsGaze,Count};
constexpr int NTempStrengthCount=(int)NTempStrength::Count;
struct NecroState {
 int16_t powers[NPowerCount]{};
 uint8_t power_order[NPowerCount]{},power_count=0;
 uint16_t drawn_turn=0,ethereal_played=0,osty_attacks_turn=0,attacks_started_turn=0;
 uint16_t creature_deaths=0;
 int16_t player_doom=0;
 uint8_t doom_applied_turn=0,enemy_dead_mask=0,osty_was_alive=0;
 // Per-instance additions preserve CardEnergyCost's later Set boundaries.
 int16_t cost_delta[MaxCards]{},turn_origin[MaxCards]{},combat_origin[MaxCards]{};
 uint8_t ethereal[MaxCards]{},fetch_played[MaxCards]{},flatten_zero[MaxCards]{};
 int16_t flatten_origin[MaxCards]{};
 int16_t debilitate[MaxEnemies]{},hang[MaxEnemies]{},oblivion[MaxEnemies]{},sic_em[MaxEnemies]{};
 // Temporary wrappers retain separate acquisition order and Artifact attempts.
 // ExpansionState::restore_strength keeps the unchanged historical total.
 int16_t temp_strength[MaxEnemies][NTempStrengthCount]{};
 // Every CardPlay (including replays) has its own captured Before values.
 int16_t oblivion_before[MaxAutoFrames+1][MaxEnemies]{};
 int16_t attack_total[MaxAutoFrames+1]{};
 uint16_t play_index[MaxAutoFrames+1]{},resource_energy[MaxAutoFrames+1]{};
 uint8_t first_attack[MaxAutoFrames+1]{};
 uint8_t debuff_order[MaxEnemies][NDebuffCount]{},debuff_count[MaxEnemies]{};
 int16_t scythe_value[MaxCards]{};
 uint16_t scythe_deck[MaxDeck]{},deck_origin[MaxCards]{};
 uint16_t grimoire_pending=0;
};
