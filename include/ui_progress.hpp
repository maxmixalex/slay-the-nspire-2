#pragma once
#include "core.hpp"

namespace spire {
// Separate optional UI history. Never inserted into a simulation save or RNG
// stream. Only observed runs contribute: old saves cannot invent earlier time,
// fights or map-point statistics that their format never recorded.
constexpr unsigned UiHistoryCapacity=32,UiHistoryFloors=96;
constexpr unsigned UiProgressCapacity=512*1024;
struct UiEnemyStatistics {uint32_t seen=0,wins=0,losses=0;};
struct UiCharacterStatistics {
    uint32_t wins=0,losses=0,current_streak=0,best_streak=0;
    uint32_t play_seconds=0,fastest_win_seconds=0,gold_gained=0,floors_climbed=0;
    uint8_t highest_ascension=0;
};
struct UiFloorHistory {
    uint32_t entered_seconds=0,seconds=0,gold_gained=0;
    uint16_t floor=0,encounter=0,hp=0,max_hp=0,gold=0,turns=0;
    uint8_t act=0,region=0,point=255,room=0,event=0;
};
struct UiRunHistory {
    uint64_t run_seed=0;
    uint32_t serial=0,seed=0,started_at=0,seconds=0,gold_gained=0;
    uint16_t floor=0,hp=0,max_hp=0,gold=0,deck_count=0,relic_count=0;
    uint16_t last_battle=0,finished_battle=0,encounter=0,last_enemy_identity=0;
    uint8_t character=0,ascension=0,outcome=0,start_region=0,regions[3]{};
    uint8_t floor_count=0,partial=0,event=0,abandoned=0,potion_count=0;
    uint8_t potions[MaxPotionSlots]{};
    char seed_text[65]{};
    uint16_t battle_seen[EnemyKinds]{};
    EventDeckSnapshot deck[MaxDeck]{};
    uint16_t relics[WorldRelicCapacity]{};
    UiFloorHistory floors[UiHistoryFloors]{};
};
struct UiProgress {
    uint32_t next_serial=1;
    uint8_t count=0,next_history=0,discovered_acts=0,characters_started=0;
    uint32_t current_streak=0,best_streak=0,last_saved_at=0;
    uint8_t discovered_cards[(CardKinds+7)/8]{},discovered_relics[(WorldRelicKinds+7)/8]{};
    uint8_t discovered_potions[(PotionKinds+7)/8]{},discovered_events[(OriginalEventKinds+7)/8]{};
    uint32_t card_wins[CardKinds]{};
    UiCharacterStatistics characters[CharacterKinds]{};
    // Slot 0 is all characters, 1..5 are the actual character dimensions.
    UiEnemyStatistics enemies[EnemyKinds][CharacterKinds+1]{};
    UiEnemyStatistics encounters[EncounterKinds][CharacterKinds+1]{};
    UiRunHistory current{},history[UiHistoryCapacity]{};
};
size_t encode_ui_progress(const UiProgress&,uint8_t*,size_t);
void clear_ui_progress(UiProgress&);
// Decoding is used only on initial load into cleared UI storage. Failure does
// not mutate a State; callers discard this optional metadata on failure.
bool decode_ui_progress(const uint8_t*,size_t,UiProgress&);
struct UiProgressSummary {uint32_t play_seconds=0,last_saved_at=0;bool exists=false;};
bool decode_ui_progress_summary(const uint8_t*,size_t,UiProgressSummary&);
} // namespace spire
