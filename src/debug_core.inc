// Both cold operations need a complete State, but a console transaction is
// synchronous and invalidates the title's read-only preview before writing.
// Keep one aggregate-initialized record: no global constructor or large
// stack temporary is supported by the freestanding calculator host.
namespace state_workspace {
enum class Use:uint8_t {Empty,SavedRead,SavedReady,DebugRollback};
static State buffer{};
static Use use=Use::Empty;
static const void* saved_owner=nullptr;
State* begin_saved_read(const void* owner){
    if(!owner||use==Use::SavedRead||use==Use::DebugRollback)return nullptr;
    use=Use::SavedRead;saved_owner=owner;return &buffer;
}
bool finish_saved_read(const void* owner,bool valid){
    if(use!=Use::SavedRead||saved_owner!=owner)return false;
    use=valid?Use::SavedReady:Use::Empty;if(!valid)saved_owner=nullptr;return valid;
}
const State* saved_probe(const void* owner){
    return use==Use::SavedReady&&saved_owner==owner?&buffer:nullptr;
}
void invalidate_saved_probe(const void* owner){
    if(use==Use::SavedReady&&saved_owner==owner){use=Use::Empty;saved_owner=nullptr;}
}
}
namespace debug_detail {
static bool active=false;
static CoreObserver previous_observer{};
bool transaction_active(){return active;}

bool begin(State&s){
    if(active||resolving||state_workspace::use==state_workspace::Use::SavedRead||
       &s==&state_workspace::buffer||!validate(s))return false;
    state_workspace::use=state_workspace::Use::DebugRollback;
    state_workspace::saved_owner=nullptr;state_workspace::buffer=s;active=true;
    // No presentation callback may retain a failed transaction's intermediate
    // identities. The App resets its presentation after a successful command.
    previous_observer=set_core_observer({});return true;
}
bool finish(State&s,bool success){
    if(!active)return false;
    success=success&&s.phase!=Phase::Error&&validate(s);
    if(!success)s=state_workspace::buffer;
    set_core_observer(previous_observer);active=false;
    state_workspace::use=state_workspace::Use::Empty;return success;
}
static void abandon_relic_actions(State&s){
    auto&w=s.world_relics;
    w.step_count=w.step_cursor=w.running=0;
    w.choice=WorldRelicChoice::None;w.return_phase=w.choice_filter=w.choice_cancelable=0;
    w.choice_owner=w.choice_min=w.choice_max=0;w.choice_enchantment=EventEnchantment::None;w.choice_amount=0;
    w.offer_count=w.selected_count=0;w.choice_group=w.choice_reward=0;
    w.reward_count=w.group_count=0;
    for(auto&v:w.group_active)v=0;for(auto&v:w.group_skip)v=0;for(auto&v:w.group_owner)v=0;
    for(auto&v:w.rewards)reset_in_place(v);for(auto&f:w.hook_frames)reset_in_place(f);
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
    for(unsigned i=0;i<MaxDeck;++i)s.event_cards.deck[i]=state_workspace::buffer.event_cards.deck[i];
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
    resolve_pending(s);
    // Gremlin Horn can draw across a shuffle while the dying actors still
    // own their death continuations. Stratagem then opens a real selection.
    // A force-kill answers those new selections with the first eligible
    // cards, preserving the existing draw/death hooks and avoiding a second
    // command just to finish the requested kill. Ordinary kills still await
    // their player's choice; this path exists only in the debug build.
    if(target<0){
        for(unsigned attempts=0;s.phase==Phase::ExtendedChoice&&attempts<MaxEffects;++attempts){
            uint16_t ids[MaxHand];const unsigned count=s.expansion.choice_min;
            if(count>MaxHand||count>s.expansion.choices.count)return debug_detail::finish(s,false);
            for(unsigned i=0;i<count;++i)ids[i]=s.expansion.choices.ids[i];
            if(!choose_extended(s,ids,count))return debug_detail::finish(s,false);
        }
        if(combat_phase(s.phase)&&s.phase!=Phase::Combat)return debug_detail::finish(s,false);
    }
    finish_combat_if_won(s);resolve_pending(s);++s.revision;
    return debug_detail::finish(s,true);
}

namespace debug_detail {
static State* session=nullptr;
static bool god=false,travel=false;
static bool idle(const State&s){return s.phase==Phase::Combat&&s.hp>0&&!s.effect_count&&s.playing<0&&!s.expansion.depth&&!world_relic_pending(s);}
static bool equal(const char*a,const char*b){
    if(!a||!b)return false;for(;;++a,++b){char x=*a,y=*b;if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y)return false;if(!x)return true;}
}
static int16_t add(int value,int amount,int ceiling=9999,int floor=0){return int16_t(max(floor,min(ceiling,value+amount)));}
static void god_powers(State&s,bool enabled){
    // Original GodMode applies Strength, Regen and Buffer, then reapplies on
    // CombatSetUp. Use the engine's real bounded model amounts, not a hidden
    // invulnerability shortcut that bypasses damage/death listeners.
    world_relic_gain_strength(s,enabled?9999-s.strength:-s.strength);
    gain_power(s,XPower::Regen,enabled?9999-power_value(s,XPower::Regen):-power_value(s,XPower::Regen));
    gain_power(s,XPower::Buffer,enabled?9999-power_value(s,XPower::Buffer):-power_value(s,XPower::Buffer));
}
}
void debug_reset_session(State&s){if(debug_detail::session==&s){debug_detail::session=nullptr;debug_detail::god=debug_detail::travel=false;}}
bool debug_godmode(const State&s){return debug_detail::session==&s&&debug_detail::god;}
bool debug_travel(const State&s){return debug_detail::session==&s&&debug_detail::travel;}
void debug_set_travel(State&s,bool enabled){if(debug_detail::session!=&s){debug_detail::session=&s;debug_detail::god=false;}debug_detail::travel=enabled;}
bool debug_set_godmode(State&s,bool enabled){
    if(s.hp<=0||!debug_detail::begin(s))return false;
    if(combat_phase(s.phase))debug_detail::god_powers(s,enabled);
    ++s.revision;if(!debug_detail::finish(s,true))return false;
    if(debug_detail::session!=&s){debug_detail::session=&s;debug_detail::travel=false;}
    debug_detail::god=enabled;return true;
}
void debug_combat_setup(State&s){if(debug_godmode(s))debug_detail::god_powers(s,true);}

bool debug_card(State&s,CardKind kind,DebugPile where,bool remove_card){
    if(kind>=CardKinds||s.hp<=0||unsigned(where)>unsigned(DebugPile::Deck))return false;
    if(where!=DebugPile::Deck&&!debug_detail::idle(s))return false;
    if(remove_card&&where!=DebugPile::Hand&&where!=DebugPile::Deck)return false;
    if(!remove_card&&where==DebugPile::Hand&&s.hand.count>=MaxHand)return false;
    if(!debug_detail::begin(s))return false;bool ok=true;
    if(where==DebugPile::Deck){
        if(remove_card){int at=0;while(at<s.deck_count&&s.deck[at].kind!=kind)++at;
            ok=at==s.deck_count||ec_remove_deck(s,at,true);
        }else ok=event_add_deck_card(s,{kind,0})>=0;
    }else if(remove_card){
        int at=0;while(at<s.hand.count&&s.cards[s.hand.ids[at]].kind!=kind)++at;
        if(at<s.hand.count){const int id=remove(s.hand,at);ok=put(s,s.powers,id,Zone::Power);if(!s.hand.count)world_relic_hook(s,WorldRelicHook::AfterHandEmptied);}
    }else{
        const int id=make_card(s,{kind,0});ok=id>=0;
        if(ok){if(kind==SovereignBlade)s.expansion.blade_damage[id]=10;
            Zone zone=where==DebugPile::Hand?Zone::Hand:where==DebugPile::Draw?Zone::Draw:where==DebugPile::Discard?Zone::Discard:Zone::Exhaust;
            Pile&dest=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:s.exhaust;
            ok=put(s,dest,id,zone);record(s,LogKind::Create,1,kind);notify_card_generated(s,id);
        }
    }
    resolve_pending(s);++s.revision;return debug_detail::finish(s,ok);
}
bool debug_upgrade(State&s,unsigned index){
    if(!debug_detail::idle(s)||index>=s.hand.count||!card_upgradeable(s.cards[s.hand.ids[index]])||!debug_detail::begin(s))return false;
    auto&card=s.cards[s.hand.ids[index]];card.upgraded=1;record(s,LogKind::Upgrade,card.kind);++s.revision;return debug_detail::finish(s,true);
}
bool debug_value(State&s,DebugValue kind,int amount,int target){
    if(unsigned(kind)>unsigned(DebugValue::Die)||amount< -9999||amount>9999||s.hp<=0)return false;
    if(kind!=DebugValue::Gold&&amount<0)return false;
    const bool combat=kind!=DebugValue::Heal&&kind!=DebugValue::Gold&&kind!=DebugValue::Die;
    if(combat&&!debug_detail::idle(s))return false;
    if((kind==DebugValue::Damage&&(target< -1||target>s.enemy_count))||
       (kind==DebugValue::Block&&(target<0||target>s.enemy_count))||
       (kind==DebugValue::Heal&&(target<0||target>1||(target==1&&s.expansion.osty_max_hp<=0))))return false;
    if(!debug_detail::begin(s))return false;
    switch(kind){
    case DebugValue::Gold:gain_gold(s,amount);break;
    case DebugValue::Heal:if(target==0)heal(s,amount);else s.expansion.osty_hp=debug_detail::add(s.expansion.osty_hp,amount,s.expansion.osty_max_hp);break;
    case DebugValue::Energy:s.energy=debug_detail::add(s.energy,amount,999);record(s,LogKind::Energy,amount);break;
    case DebugValue::Stars:regent_gain_stars(s,amount);break;
    case DebugValue::Draw:draw_cards(s,amount);break;
    case DebugValue::Block:if(!target)gain_unpowered_block(s,amount);else{auto&e=s.enemies[target-1];e.block=debug_detail::add(e.block,amount);record(s,LogKind::Block,amount,target-1);}break;
    case DebugValue::Damage:
        if(target==0)player_hp_loss(s,amount,false,true);
        else for(int i=0;i<s.enemy_count;++i)if(target<0||i==target-1)damage_unpowered(s,i,amount);
        break;
    case DebugValue::Die:player_kill(s,true);break;
    }
    resolve_pending(s);if(kind==DebugValue::Damage)finish_combat_if_won(s);resolve_pending(s);++s.revision;
    return debug_detail::finish(s,true);
}
const char* debug_power_key(unsigned id){
    static const char*const legacy[]={
#include "locale_status_index.inc"
    };
    static const char*const enemies[]={
#include "world_power_keys.inc"
    };
    static const char*const events[]={"HELLO_WORLD_POWER","CURIOUS_POWER","IMPROVEMENT_POWER","FEEDING_FRENZY_POWER","REBOUND_POWER"};
    return id<LegacyStatusKinds?legacy[id]:id<EventCardStatusStart?enemies[id-EnemyStatusStart]:id<StatusKinds?events[id-EventCardStatusStart]:nullptr;
}
int debug_power_id(const char*key,int target){
    // Creature powers share IDs with player powers. Resolve duplicate model
    // names against the target's family before applying their real hooks.
    if(target>0)for(unsigned i=EnemyStatusStart;i<EventCardStatusStart;++i)if(debug_detail::equal(key,debug_power_key(i)))return int(i);
    for(unsigned i=0;i<StatusKinds;++i)if(debug_detail::equal(key,debug_power_key(i)))return int(i);
    return -1;
}
static bool debug_apply_player_power(State&s,unsigned id,int amount){
    if(id>=EventCardStatusStart){ec_gain_power(s,EventCardPower(id-EventCardStatusStart),amount);return true;}
    if(id>=EnemyStatusStart){const auto p=EnemyPower(id-EnemyStatusStart);enemy_apply_power(s,-1,p,amount,-1,enemy_power_value(s,-1,p)!=0);return true;}
    if(id>=NecroStatusStart&&id<FiveCharacterExtraStatusStart){necro_gain_power(s,NPower(id-NecroStatusStart),amount);return true;}
    if(id>=RegentStatusStart&&id<NecroStatusStart){regent_gain_power(s,RPower(id-RegentStatusStart),amount);return true;}
    if(id>=DefectStatusStart&&id<RegentStatusStart){const auto p=DPower(id-DefectStatusStart);defect_gain_power(s,p,amount);if(defect_temporary_focus_power(p))defect_update_temporary_focus(s);return true;}
    if(id>=SilentStatusStart&&id<CharacterExtraStatusStart){silent_gain_power(s,SPower(id-SilentStatusStart),amount);return true;}
    if(id>=IroncladStatusStart&&id<SilentStatusStart){ironclad_gain_power(s,IPower(id-IroncladStatusStart),amount);return true;}
    if(id>=18&&id<40){
        const auto power=XPower(id-18);
        if(power==XPower::Automation||power==XPower::Panache||power==XPower::RollingBoulder){
            if(amount<=0)return false;int16_t*slots=power==XPower::Automation?s.expansion.automation:power==XPower::Panache?s.expansion.panache:s.expansion.boulder;
            unsigned at=0;while(at<MaxPowerInstances&&slots[at])++at;if(at==MaxPowerInstances)return false;slots[at]=int16_t(amount);
            if(power==XPower::Panache){s.expansion.panache_skip[at]=1;after_play_register(s,unsigned(PostPlayActor::PanacheStart)+at);}
            power_order_set(s,0,id,true,at);
        }
        if(power==XPower::Gambit){s.expansion.powers[unsigned(power)]=amount>0?1:0;power_order_set(s,0,id,amount>0);}else gain_power(s,power,amount);return true;
    }
    switch(id){
    case 0:world_relic_gain_strength(s,amount);break;
    case 1:world_relic_gain_dexterity(s,amount);break;
    case 2:s.vigor=debug_detail::add(s.vigor,amount);break;
    case 3:s.thorns=debug_detail::add(s.thorns,amount);break;
    case 4:case 5:enemy_apply_power(s,-1,id==4?EnemyPower::Weak:EnemyPower::Vulnerable,amount,-1);break;
    case 7:case 8:{int16_t&v=id==7?s.pain:s.embrace;v=debug_detail::add(v,amount,id==7?9999:99);const uint8_t value=id==7?0:1;unsigned at=0;while(at<s.power_count&&s.power_order[at]!=value)++at;
        if(v&&at==s.power_count&&s.power_count<2)s.power_order[s.power_count++]=value;
        if(!v&&at<s.power_count){for(unsigned i=at+1;i<s.power_count;++i)s.power_order[i-1]=s.power_order[i];s.power_order[--s.power_count]=0;}
        break;}
    case 9:s.no_draw=uint8_t(debug_detail::add(s.no_draw,amount,1));break;
    case 10:s.no_block=debug_detail::add(s.no_block,amount);break;
    case 11:s.noxious_fumes=debug_detail::add(s.noxious_fumes,amount);break;
    case 12:s.accuracy=debug_detail::add(s.accuracy,amount);break;
    case 13:s.demon_form=debug_detail::add(s.demon_form,amount);break;
    case 14:s.infinite_blades=debug_detail::add(s.infinite_blades,amount,MaxCards);break;
    case 15:s.flame_barrier=debug_detail::add(s.flame_barrier,amount);break;
    case 16:s.block_next_turn=debug_detail::add(s.block_next_turn,amount);break;
    case 17:s.draw_next_turn=debug_detail::add(s.draw_next_turn,amount,MaxCards);break;
    case 42:s.necro.player_doom=debug_detail::add(s.necro.player_doom,amount);break;
    case 43:{if(amount<=0)return false;unsigned at=0;while(at<MaxBombs&&s.expansion.bomb_turns[at])++at;if(at==MaxBombs)return false;
        s.expansion.bomb_damage[at]=int16_t(amount);s.expansion.bomb_turns[at]=3;power_order_set(s,0,id,true,at);break;}
    default:return false;
    }
    if(!power_instanced(id))power_order_set(s,0,id,power_amount(s,0,int(id))!=0);return true;
}
bool debug_power(State&s,unsigned id,int amount,int target){
    if(id>=StatusKinds||target<0||target>s.enemy_count||amount< -9999||amount>9999||!debug_detail::idle(s)||!debug_detail::begin(s))return false;
    bool ok=true;
    if(!target)ok=debug_apply_player_power(s,id,amount);
    else if(id>=EnemyStatusStart&&id<EventCardStatusStart){const auto p=EnemyPower(id-EnemyStatusStart);enemy_apply_power(s,target-1,p,amount,-1,enemy_power_value(s,target-1,p)!=0);}
    else{
        const int t=target-1;NDebuff debuff=NDebuff::Count;
        switch(id){case 0:debuff=NDebuff::Strength;break;case 4:debuff=NDebuff::Weak;break;case 5:debuff=NDebuff::Vulnerable;break;case 6:debuff=NDebuff::Poison;break;
        case 40:debuff=NDebuff::Demise;break;case 41:debuff=NDebuff::Beetle;break;case 42:debuff=NDebuff::Doom;break;
        case CharacterExtraStatusStart+1:debuff=NDebuff::Strangle;break;
        case CharacterExtraStatusStart+2:enemy_apply_power(s,t,EnemyPower::Artifact,amount,-1);break;
        case FiveCharacterExtraStatusStart:debuff=NDebuff::Conqueror;break;case FiveCharacterExtraStatusStart+1:debuff=NDebuff::Debilitate;break;
        case FiveCharacterExtraStatusStart+2:debuff=NDebuff::Hang;break;case FiveCharacterExtraStatusStart+3:debuff=NDebuff::Oblivion;break;case FiveCharacterExtraStatusStart+4:debuff=NDebuff::SicEm;break;
        default:ok=false;break;
        }
        if(debuff!=NDebuff::Count){
            if(amount<0&&debuff!=NDebuff::Strength){int16_t*v=debuff==NDebuff::Weak?&s.enemies[t].weak:debuff==NDebuff::Vulnerable?&s.enemies[t].vulnerable:debuff==NDebuff::Poison?&s.enemies[t].poison:debuff==NDebuff::Doom?&s.expansion.doom[t]:nullptr;
                if(v)*v=debug_detail::add(*v,amount);else ok=false;necro_prune_debuffs(s);
            }else necro_apply_copied_debuff(s,t,debuff,amount);
        }
    }
    resolve_pending(s);power_order_reconcile(s);++s.revision;return debug_detail::finish(s,ok);
}
bool debug_enchant(State&s,EventEnchantment enchantment,int amount,unsigned index){
    if(!debug_detail::idle(s)||index>=s.hand.count||enchantment==EventEnchantment::None||enchantment>=EventEnchantment::Count||amount<1||amount>9999)return false;
    const unsigned id=s.hand.ids[index];if(!event_card_can_enchant(s,s.cards[id],s.event_cards.cards[id],enchantment)||!debug_detail::begin(s))return false;
    const bool ok=event_card_apply_enchantment(s.cards[id],s.event_cards.cards[id],enchantment,amount);++s.revision;return debug_detail::finish(s,ok);
}
bool debug_afflict(State&s,EnemyAffliction affliction,int amount,unsigned index){
    if(!debug_detail::idle(s)||index>=s.hand.count||affliction==EnemyAffliction::None||affliction>=EnemyAffliction::Count||amount<1||amount>9999||!debug_detail::begin(s))return false;
    const bool ok=enemy_afflict_card(s,s.hand.ids[index],affliction,amount);++s.revision;return debug_detail::finish(s,ok);
}
bool debug_relic(State&s,RelicId id,bool remove){
    if(id>=WorldRelicKinds||s.hp<=0||!debug_detail::begin(s))return false;
    const bool ok=remove?lose_world_relic(s,id):gain_world_relic(s,id);resolve_pending(s);++s.revision;return debug_detail::finish(s,ok);
}
bool debug_potion(State&s,Potion potion){
    if(potion==Potion::Empty||unsigned(potion)>PotionKinds||s.hp<=0||!debug_detail::begin(s))return false;
    const bool ok=procure_potion(s,potion);resolve_pending(s);++s.revision;return debug_detail::finish(s,ok);
}
