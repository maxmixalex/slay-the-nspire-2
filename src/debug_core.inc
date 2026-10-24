// Compiled only with SPIRE_DEBUG_COMMANDS. Keep the large rollback state in
// static storage: the calculator stack cannot hold a second State.
namespace debug_detail {
static State rollback{};
static bool active=false;
static CoreObserver previous_observer{};

bool begin(State&s){
    if(active||resolving||!validate(s))return false;
    rollback=s;active=true;
    // No presentation callback may retain a failed transaction's intermediate
    // identities. The App resets its presentation after a successful command.
    previous_observer=set_core_observer({});return true;
}
bool finish(State&s,bool success){
    if(!active)return false;
    success=success&&s.phase!=Phase::Error&&validate(s);
    if(!success)s=rollback;
    set_core_observer(previous_observer);active=false;return success;
}
static void abandon_relic_actions(State&s){
    auto&w=s.world_relics;
    w.step_count=w.step_cursor=w.running=0;
    w.choice=WorldRelicChoice::None;w.return_phase=w.choice_filter=w.choice_cancelable=0;
    w.choice_owner=w.choice_min=w.choice_max=0;w.choice_enchantment=EventEnchantment::None;w.choice_amount=0;
    w.offer_count=w.selected_count=0;w.choice_group=w.choice_reward=0;
    w.reward_count=w.group_count=0;
    for(auto&v:w.group_active)v=0;for(auto&v:w.group_skip)v=0;for(auto&v:w.group_owner)v=0;
    for(auto&v:w.rewards)v={};for(auto&f:w.hook_frames)f={};
    w.transform_count=w.transform_cursor=w.byrdpip_count=w.byrdpip_cursor=0;
    world_relic_clear_offers(s);
}
void abandon_room(State&s){
    // Abandoning a room does not award a victory or run any old continuation.
    // Preserve permanent card growth/enchantments and the actual relic bag,
    // instances and Pael's Tooth snapshots; discard only combat/room data.
    world_clear_player_powers(s);
    reset_in_place(s.expansion);reset_in_place(s.ironclad);silent_combat_start(s);
    defect_combat_start(s);reset_in_place(s.regent);necro_combat_start(s);
    reset_in_place(s.event_cards);
    for(unsigned i=0;i<MaxDeck;++i)s.event_cards.deck[i]=rollback.event_cards.deck[i];
    enemy_reset_combat(s);s.enemy_count=0;
    s.phase=Phase::Map;s.turn=0;s.energy=0;s.playing=-1;s.effect_count=s.card_count=0;
    s.draw.count=s.hand.count=s.discard.count=s.exhaust.count=s.powers.count=0;
    s.discard_required=s.exhaust_limit=s.attacked_this_turn=0;s.relic_flash=0;
    abandon_relic_actions(s);
    auto&r=s.world_relics;
    for(auto&v:r.card_is_dupe)v=0;for(auto&v:r.play_serial)v=0;
    r.manual_plays_turn=0;r.self_forming_clay=r.reptile_trinket=r.helical_dart=0;
    r.byrdpip_present=r.legion_present=0;r.last_flash=NoWorldRelic;
    reset_in_place(s.original_events);
    s.event_deck_action=EventDeckAction::None;s.event_first=-1;s.event_remaining=s.event_page=0;
    s.treasure_state=0;s.treasure_relic=0;s.reward_claimed=0;s.reward_potion=Potion::Empty;s.last_gold=0;
    s.current_node=-1;s.next_row=0;
    auto&w=s.world;
    w.room_open=w.event_combat_parent=w.parent_room_type=w.final_event=w.finished=w.finish_pending=0;
    w.point_had_shop=w.combat_ending=0;w.combat_bonus_gold=0;
    w.reward_count=w.reward_relic_claimed=w.reward_rerolled=w.reward_can_reroll=0;
    w.reward_relic=w.treasure_relic=NoWorldRelic;w.treasure_gold=0;w.treasure_gold_claimed=0;
    w.shop_pending_item=255;w.shop_pending_price=0;w.shop_auto_active=w.shop_auto_cursor=w.shop_free_remove=0;
    w.rest_count=w.rest_pending=0;w.rest_used=0;w.rest_deck_before=0;
    if(s.hp<=0)s.hp=s.max_hp;
    s.event_run.completion_recorded=0;
}
} // namespace debug_detail

bool debug_kill_enemies(State&s,int target){
    if(target< -1||target>=s.enemy_count||!saved_combat_phase(s)||s.hp<=0)return false;
    if(target>=0&&(s.phase!=Phase::Combat||s.effect_count||s.playing>=0||s.expansion.depth||world_relic_pending(s)))return false;
    bool found=false;
    for(int i=0;i<s.enemy_count;++i)if((target<0||i==target)&&
        (s.enemy_world.active?enemy_slot_exists(s,i):s.enemies[i].hp>0))found=true;
    if(!found||!debug_detail::begin(s))return false;
    if(target<0){
        s.effect_count=0;s.phase=Phase::Combat;s.discard_required=s.exhaust_limit=0;
        clear_extended_choice(s);s.event_cards.choice_copies=0;
        debug_detail::abandon_relic_actions(s);
        s.world.combat_ending=0;
    }
    // A force-kill ends all phases of this creature. The ordinary EnemyDie
    // pipeline still returns stolen cards/gold, removes source debuffs, fires
    // AfterEnemyDeath, handles minions, and constructs the actual reward.
    constexpr EnemyPower continuation[]={EnemyPower::Illusion,EnemyPower::Adaptable,
        EnemyPower::Reattach,EnemyPower::SteamEruption,EnemyPower::Stock,
        EnemyPower::Infested,EnemyPower::Surprise};
    for(int i=0;i<s.enemy_count;++i){
        if(target>=0&&i!=target)continue;
        if(s.enemy_world.active){
            if(!enemy_slot_exists(s,i))continue;
            for(auto p:continuation)enemy_clear_power(s,i,p);
            auto&m=s.enemy_world.models[i];m.flags&=uint8_t(~EnemyDown);m.flags|=EnemyDying;
            s.enemies[i].hp=0;queue_extension(s,XOp::EnemyDie,i,0,false);
        }else s.enemies[i].hp=0;
    }
    resolve_pending(s);finish_combat_if_won(s);resolve_pending(s);++s.revision;
    return debug_detail::finish(s,true);
}
