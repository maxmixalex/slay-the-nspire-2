// Original relic combat bridge. Model values refer to the pinned 41cef1ea assembly.
// Hook args: Side hooks a=0 player/1 enemy. Before/AfterCardPlayed:
// a=card instance, b=EnergyValue (including auto-play/X), c=stars spent | (autoPlay ? 0x10000 : 0).
// AfterCardChangedPiles: a=instance,b=from Zone,c=to Zone. Card notifications a=id.
// AfterHpLost: a=unblocked HP damage,b=(attack?1:0)|(blockable?2:0)|(combat?4:0),c=source card/-1.
// AfterAttack: a=attacker (-1 player,-2 Osty,>=0 enemy),b=target,c=source card/-1.
// AfterBlock: a=modified positive block,b=target (-1 player),c=source card/-1.
// AfterBlockBroken a=target. AfterEnemyDeath a=target,b=isMinion. AfterStarsSpent a=spent.
// BeforeHandDraw precedes wr_hand_draw(...,true); AfterHandDraw is
// AfterPlayerTurnStart, AfterHandDrawLate is AfterPlayerTurnStartLate.
static int wr_max_energy(const State&,int current);
static int wr_hand_draw(State&,int current,bool commit);
static int wr_energy_cost(const State&,Card,int instance,int current,bool late);
static int wr_star_cost(const State&,Card,int instance,int current);
static int wr_damage_add(const State&,Card,int instance,int current,bool powered=true);
static int wr_damage_percent(const State&,Card,int instance,int target,bool powered=true);
static int wr_incoming_damage_percent(const State&,int dealer,bool powered);
// WeakPower/VulnerablePower call these directly on the affected player's
// first matching relic. Apply before Debilitate, without rounding damage.
static int wr_enemy_weak_percent(const State&,int current_percent);
static int wr_player_vulnerable_percent(const State&,int current_percent,bool pet);
static int wr_block_percent(const State&,Card,int instance,bool powered=true);
static int wr_player_hp_loss(State&,int current);
static int wr_enemy_hp_loss(State&,int current,bool powered);
static int wr_x_value(State&,int current);
static int wr_play_count(State&,int instance,int current,bool commit);
static int wr_poison_amount(State&,int target,int current);
// A visible enemy debuff: type is EnemyPower, source=-1 means no card.
// Call after Artifact rejection, before application; is_internal suppresses
// temporary power's mirrored Strength application (the outer power was doubled).
static int wr_debuff_amount(State&,int target,EnemyPower,int current,int source,bool is_internal=false);
static bool wr_can_play(const State&,bool auto_play);
static bool wr_can_draw(State&,bool from_hand_draw);
static bool wr_reset_energy(const State&);
static bool wr_flush(const State&);
static bool wr_retain_block(State&);
static bool wr_revive(State&);
static bool wr_extra_turn(State&,bool commit);
static int wr_orb_value(const State&,DOrb,int current);
static int wr_orb_passive_count(State&,int slot,int current,bool commit);
void world_relic_resume(State&);
void world_relic_gain_strength(State&,int);
void world_relic_gain_dexterity(State&,int);

static void wr_power_after_block_clear(State&);
static void wr_power_side_end(State&,int side);
bool world_relic_replace(State&,unsigned serial,RelicId);

static bool wr_automatic_choice(State&);
