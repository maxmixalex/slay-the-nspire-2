// Silent's explicit integration surface; definitions live in silent_core.inc.
static bool queue_silent(State&,Card,int target,int energy);
static bool silent_effect(State&,Effect);
static void silent_combat_start(State&);
static void silent_card_created(State&,int id);
static void silent_card_reset(State&,int id);
static void silent_card_copied(State&,int to,int from);
static void silent_temporary_cost_set(State&,int id);
static void silent_combat_cost_set(State&,int id);
static int silent_cost(const State&,int id,int current);
static bool silent_can_play(const State&,Card);
static int silent_extra_replays(State&,Card);
static void silent_before_play(State&,int id);
static void silent_after_play(State&,int id);
static void silent_card_drawn(State&,int id);
static void silent_card_discarded(State&,int id);
static void silent_attack_damage(State&,int target,int unblocked);
// Call after damage/victory attribution, but before the card's Finish.
static void silent_attack_killed(State&,int target);
static int silent_base_damage(const State&,Card,int target,int instance,int current);
// Combines Vulnerable/Cruelty, Tracking and DoubleDamage with one final rounding.
static int silent_attack_factor(const State&,int target,int amount);
// Apply to any raw Block gain once, after additive Dexterity for card gains.
static int silent_modify_block(const State&,int amount);
static bool silent_retains(const State&,int id);
// Checks Blur at block-clear time; its layer expires at AfterSideTurnStart.
static bool silent_keeps_block(State&);
// Called after energy resets and before hand draw; resets side-scoped history.
static void silent_turn_start(State&);
static int silent_hand_draw_count(const State&);
static void silent_before_hand_draw(State&);
// After the regular hand draw: Noxious/Wraith/ShadowStep/Tools hooks.
static void silent_turn_ready(State&);
// After player hand flush, before enemy choices/draws: remove temporary effects.
static void silent_player_side_end(State&);
// Replaces the legacy once-per-enemy poison loop; also used by Outbreak.
static void silent_enemy_poison(State&);
static bool silent_choose(State&,const uint16_t*,unsigned);
static bool valid_silent_state(const State&);
static bool valid_silent_effect(const State&,const Effect&);
static bool silent_description_value(const State&,Card,int target,int instance,const char* variable,int& out);
static bool silent_blocks_debuff(State&,int target);
static bool silent_next_reward(State&);
static void silent_combat_end(State&);
static bool silent_card_targets_enemy(const State&,Card);
