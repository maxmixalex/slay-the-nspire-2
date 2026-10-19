// Explicit integration surface for all 86 Necrobinder cards and SweepingGaze.
static bool queue_necro(State&,Card,int target,int energy);
static bool necro_effect(State&,Effect);
static bool necro_choose(State&,const uint16_t*,unsigned);
static void necro_combat_start(State&);
static void necro_combat_end(State&);
static void necro_card_created(State&,int id);
static void necro_card_reset(State&,int id);
static void necro_card_copied(State&,int to,int from);
static void necro_temporary_cost_set(State&,int id);
static void necro_combat_cost_set(State&,int id);
static int necro_cost(const State&,int id,int current);
static bool necro_can_play(const State&,Card);
static bool necro_ethereal(const State&,int id);
static bool necro_osty_attack(Card);
static int necro_osty_bonus(const State&);
static int necro_base_damage(const State&,Card,int target,int instance,int current);
static int necro_base_block(const State&,Card,int instance,int current);
// Integrate these factors with the other damage factors before final rounding.
static int necro_damage_multiplier(const State&,Card,int target,int instance);
static int necro_vulnerable_multiplier(const State&,int target,int current_percent);
static int necro_weak_multiplier(const State&,int target,int current_percent);
// Called once after assigning the new leaf's depth, before queue_current.
static void necro_start_instance(State&,int id,int resource_energy);
// Called BEFORE Silent consumes FreeSkill/Veilpiercer and before card effects.
static void necro_before_play(State&,int id);
static void necro_after_play(State&,int id);
static void necro_after_play_listener(State&,unsigned actor);
// After all AfterCardPlayed listeners, before either replay or final pile move.
static void necro_after_play_late(State&,int id);
static void necro_after_play_group(State&,int id);
static void necro_card_drawn(State&,int id);
// total_damage is blocked + actual unblocked HP damage, excluding overkill.
static void necro_attack_damage(State&,int target,int total_damage,bool pet);
// Call after applying an owned non-temporary debuff (not an Artifact block).
static void necro_debuff_applied(State&,int target,int actual_change,bool doom=false);
static void necro_apply_doom(State&,int target,int amount);
// Shared pet HP mutation. Summon adds max HP and heals equally if already alive.
static void necro_summon(State&,int amount);
static void necro_osty_hp_lost(State&,int old_hp);
// Observe after every HP mutation; each distinct death triggers Melancholy once.
static void necro_observe_deaths(State&);
static void necro_turn_start(State&);
static int necro_max_energy(const State&);
static int necro_hand_draw_count(const State&);
static void necro_before_hand_draw(State&);
static void necro_turn_ready(State&);
static void necro_player_side_end(State&);
// Enemy Doom resolves for the whole side before its AfterSideTurnEnd effects.
static void necro_enemy_side_end(State&);
static bool necro_description_value(const State&,Card,int target,int instance,const char*,int&);
static bool valid_necro_state(const State&);
static bool valid_necro_effect(const State&,const Effect&);
// Shared death-prevention definitions follow the character modules.
static bool revive(State&);
static void death_cleanup(State&);
static void necro_note_debuff(State&,int target,NDebuff);
static void necro_prune_debuffs(State&);
static void necro_deck_card_created(State&,int card_id,int deck_index);
static void necro_deck_removed(State&,int deck_index);
static void necro_deck_replaced(State&,int deck_index);
static void necro_deck_copied(State&,int to,int from);
