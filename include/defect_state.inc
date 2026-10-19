// v14: explicit bounded facts for the complete pinned single-player Defect pool.
// Index zero is the oldest / rightmost orb; Dark=0 preserves old dark-only saves.
enum class DOrb:uint8_t { Dark,Lightning,Frost,Plasma,Glass,Count };
enum class DPower:uint8_t {
 BiasedCognition,Coolant,CreativeAi,EchoForm,Feral,Hailstorm,Iteration,
 LightningRod,Loop,MachineLearning,SignalBoost,Smokestack,Spinner,Storm,
 Subroutine,Thunder,TrashToTreasure,ConsumingShadow,FreePower,EnergyNextTurn,
 FocusedStrikeFocus,HotfixFocus,HyperbeamFocus,SynchronizeFocus,Count
};
constexpr int DPowerCount=(int)DPower::Count;
struct DefectState {
 int16_t powers[DPowerCount]{};
 uint8_t power_order[DPowerCount]{},power_count=0,orb_kind[MaxOrbs]{};
 uint16_t lightning_channeled=0,physical_started=0,cards_finished=0;
 uint16_t energy_spent_turn=0,zero_attacks_started=0,feral_used=0;
 uint8_t status_drawn=0;
 int16_t temporary_focus=0;
 // Claw damage and Genetic Algorithm block use separate card instances.
 int16_t value[MaxCards]{},cost_delta[MaxCards]{},turn_origin[MaxCards]{},combat_origin[MaxCards]{};
 uint16_t genetic_deck[MaxDeck]{},deck_origin[MaxCards]{};
 int16_t storm_before[MaxAutoFrames+1]{},subroutine_before[MaxAutoFrames+1]{};
 uint16_t spent[MaxAutoFrames+1]{};
 uint8_t scrape_waiting[MaxAutoFrames+1]{},scrape_count[MaxAutoFrames+1]{};
 uint16_t scrape_drawn[MaxAutoFrames+1][MaxHand]{};
};
