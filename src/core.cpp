#include "core.hpp"
#include "inplace_reset.hpp"
#include "resource_transfer.hpp"
#include "world_card_factory.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "debug_commands.hpp"
#include "run_options.hpp"
#include "power_values.hpp"

namespace spire {
ResourceTransferWorkspace resource_transfer_workspace{};
uint8_t resource_transfer_uses=0;
#include "card_definitions.inc"
#include "expansion_forward.inc"
#include "ironclad_forward.inc"
#include "silent_forward.inc"
#include "defect_forward.inc"
#include "regent_forward.inc"
#include "necro_forward.inc"
#include "all_characters_forward.inc"
#include "event_card_forward.inc"
#include "enemy_forward.inc"
#include "world_relic_forward.inc"
#include "world_shared_forward.inc"
#include "power_order_core.inc"
#include "core_actions.inc"
#include "expansion_core.inc"
#include "ironclad_core.inc"
#include "silent_core.inc"
#include "defect_core.inc"
#include "regent_core.inc"
#include "necro_core.inc"
#include "all_characters_core.inc"
#include "event_card_core.inc"
#include "card_result_core.inc"
#include "enemy_core.inc"
#include "enemy_token_core.inc"
#include "world_relic_core.inc"
#include "post_play_core.inc"
#include "core_turns.inc"
#include "world_shared_core.inc"
#include "expansion_effects.inc"
#include "expansion_validation.inc"
#include "ironclad_validation.inc"
#include "silent_validation.inc"
#include "defect_validation.inc"
#include "regent_validation.inc"
#include "necro_validation.inc"
#include "event_card_validation.inc"
#include "enemy_validation.inc"
#include "extension_effect_validation.inc"
#include "state_validation.inc"
#include "state_codec.inc"
int max_energy(const State&s){return wr_max_energy(s,3+ironclad_max_energy(s)+necro_max_energy(s));}
void before_potion_use(State&s,Potion potion,int target){
    const auto* definition=potion_definition(potion);
    // Callers may retain a selected enemy for self/all-enemy bottles. Only a
    // genuinely targeted potion turns the Surrounded player's facing.
    enemy_before_potion(s,definition&&definition->targeted?target:-1);
}
bool character_description_value(const State&s,Card c,int target,int instance,const char*variable,int&out){
    return ironclad_description_value(s,c,target,instance,variable,out)||
           silent_description_value(s,c,target,instance,variable,out)||
           defect_description_value(s,c,target,instance,variable,out)||
           regent_description_value(s,c,target,instance,variable,out)||
           necro_description_value(s,c,target,instance,variable,out)||
           event_card_description_value(s,c,target,instance,variable,out)||
           enemy_description_value(s,c,target,instance,variable,out)||
           enemy_token_description_value(s,c,target,instance,variable,out);
}
#if SPIRE_DEBUG_COMMANDS
#include "debug_core.inc"
#endif
} // namespace spire
