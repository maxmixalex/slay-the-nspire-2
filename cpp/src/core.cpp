#include "core.hpp"
#include "detail/state_migration.hpp"
#include "inplace_reset.hpp"
#include "resource_transfer.hpp"
#include "world_card_factory.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "debug_commands.hpp"
#include "run_options.hpp"
#include "power_values.hpp"

namespace spire {
ForegroundWorkspace foreground_workspace{};
uint8_t resource_transfer_uses = 0;
unsigned foreground_scratch_top = 0;
#include "detail/gameplay/card_definitions.hpp"
#include "detail/gameplay/expansion_forward.hpp"
#include "detail/gameplay/ironclad_forward.hpp"
#include "detail/gameplay/silent_forward.hpp"
#include "detail/gameplay/defect_forward.hpp"
#include "detail/gameplay/regent_forward.hpp"
#include "detail/gameplay/necro_forward.hpp"
#include "detail/gameplay/all_characters_forward.hpp"
#include "detail/gameplay/event_card_forward.hpp"
#include "detail/gameplay/enemy_forward.hpp"
#include "detail/gameplay/world_relic_forward.hpp"
#include "detail/gameplay/world_shared_forward.hpp"
#include "detail/gameplay/power_order_core.hpp"
#include "detail/gameplay/core_actions.hpp"
#include "detail/gameplay/expansion_core.hpp"
#include "detail/gameplay/ironclad_core.hpp"
#include "detail/gameplay/silent_core.hpp"
#include "detail/gameplay/defect_core.hpp"
#include "detail/gameplay/regent_core.hpp"
#include "detail/gameplay/necro_core.hpp"
#include "detail/gameplay/all_characters_core.hpp"
#include "detail/gameplay/event_card_core.hpp"
#include "detail/gameplay/card_result_core.hpp"
#include "detail/gameplay/enemy_core.hpp"
#include "detail/gameplay/enemy_token_core.hpp"
#include "detail/gameplay/world_relic_core.hpp"
#include "detail/gameplay/post_play_core.hpp"
#include "detail/gameplay/core_turns.hpp"
#include "detail/gameplay/combat_selection_core.hpp"
#include "detail/gameplay/world_shared_core.hpp"
#include "detail/gameplay/expansion_effects.hpp"
#include "detail/gameplay/expansion_validation.hpp"
#include "detail/gameplay/ironclad_validation.hpp"
#include "detail/gameplay/silent_validation.hpp"
#include "detail/gameplay/defect_validation.hpp"
#include "detail/gameplay/regent_validation.hpp"
#include "detail/gameplay/necro_validation.hpp"
#include "detail/gameplay/event_card_validation.hpp"
#include "detail/gameplay/enemy_validation.hpp"
#include "detail/gameplay/extension_effect_validation.hpp"
#include "detail/gameplay/state_validation.hpp"
int max_energy(const State& s) {
    return wr_max_energy(s, 3 + ironclad_max_energy(s) + necro_max_energy(s));
}
void before_potion_use(State& s, Potion potion, int target) {
    const auto* definition = potion_definition(potion);
    // Callers may retain a selected enemy for self/all-enemy bottles. Only a
    // genuinely targeted potion turns the Surrounded player's facing.
    enemy_before_potion(s, definition && definition->targeted ? target : -1);
}
bool character_description_value(const State& s, Card c, int target, int instance,
                                 const char* variable, int& out) {
    return ironclad_description_value(s, c, target, instance, variable, out) ||
           silent_description_value(s, c, target, instance, variable, out) ||
           defect_description_value(s, c, target, instance, variable, out) ||
           regent_description_value(s, c, target, instance, variable, out) ||
           necro_description_value(s, c, target, instance, variable, out) ||
           event_card_description_value(s, c, target, instance, variable, out) ||
           enemy_description_value(s, c, target, instance, variable, out) ||
           enemy_token_description_value(s, c, target, instance, variable, out);
}
#include "detail/gameplay/debug_core.hpp"
} // namespace spire
namespace spire::state_migration {
void register_post_play(State& s, unsigned actor) {
    after_play_register(s, actor);
}
void set_temporary_cost(State& s, int id) {
    temporary_cost_set(s, id);
}
bool valid_silent(const State& s) {
    return valid_silent_state(s);
}
void restore_card_result_order(State& s) {
    card_result_migrate_order(s);
}
} // namespace spire::state_migration
