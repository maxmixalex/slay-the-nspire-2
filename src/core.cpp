#include "core.hpp"

namespace spire {
#include "card_definitions.inc"
#include "expansion_forward.inc"
#include "ironclad_forward.inc"
#include "silent_forward.inc"
#include "core_actions.inc"
#include "expansion_core.inc"
#include "ironclad_core.inc"
#include "silent_core.inc"
#include "core_turns.inc"
#include "expansion_effects.inc"
#include "expansion_validation.inc"
#include "ironclad_validation.inc"
#include "silent_validation.inc"
#include "extension_effect_validation.inc"
#include "state_validation.inc"
#include "state_codec.inc"
bool character_description_value(const State&s,Card c,int target,int instance,const char*variable,int&out){
    return ironclad_description_value(s,c,target,instance,variable,out)||
           silent_description_value(s,c,target,instance,variable,out);
}
} // namespace spire
