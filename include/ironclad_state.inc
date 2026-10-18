// v13: bounded combat-local facts for the complete single-player Ironclad pool.
enum class IPower:uint8_t {
 Aggression,Barricade,Colossus,Corruption,CrimsonMantle,Cruelty,Hellraiser,
 Inferno,Juggernaut,Juggling,OneTwoPunch,Pyre,Rage,Rupture,Stampede,Unmovable,Vicious,FreeAttack,Count
};
constexpr int IPowerCount=(int)IPower::Count;
struct IroncladState {
 int16_t powers[IPowerCount]{};
 uint8_t power_order[IPowerCount]{},power_count=0;
 uint16_t attacks_started=0,attacks_finished=0,hp_losses_turn=0,hp_losses_combat=0,exhausted_turn=0;
 uint16_t block_events_turn=0,block_events_play[MaxAutoFrames+1]{};
 int16_t rupture_pending[MaxAutoFrames+1]{};
 uint8_t rupture_registered[MaxAutoFrames+1]{};
 int16_t damage_bonus[MaxCards]{};
 // AddThisTurn modifiers survive card play; later Set modifiers hide earlier
 // reductions until that Set expires. Both boundaries are saved explicitly.
 uint16_t stomp_discount[MaxCards]{},stomp_turn_origin[MaxCards]{},stomp_combat_origin[MaxCards]{};
 uint8_t forced_exhaust[MaxCards]{};
 int16_t setup_strength=0,crimson_self_damage=0,inferno_self_damage=0;
 // Draw returns the original drawn instance even if Hellraiser's nested play
 // draws again. One capture per bounded play frame preserves that distinction.
 int16_t pillage_drawn[MaxAutoFrames+1]{};
 uint8_t pillage_waiting[MaxAutoFrames+1]{};
};
