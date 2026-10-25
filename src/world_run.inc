// Included by run.cpp. The historical expedition is a separate loadable mode.
namespace {
#include "world_run_pools.inc"
using reference::PointType;
using reference::NoPoint;
using reference::math::Real;
static bool world_fail(State&s){s.phase=Phase::Error;return false;}
static bool world_make_shop(State&);
static bool world_make_rewards(State&);
static bool world_open_treasure(State&);
static bool world_take_treasure(State&);
static bool world_prepare_treasure(State&);
static bool world_buy(State&,int);
static bool world_make_rest(State&);
static void world_continue_shop(State&);
static WorldRoom world_roll_unknown(State&,unsigned);
static unsigned world_map_variant(const State&s){
    // Deck listeners precede relic listeners: Golden Compass wins over Spoils.
    return world_relic_golden_path(s,s.world.act_index)?1:event_card_spoils_act(s)==int(s.world.act_index)?2:0;
}
static bool world_fill_encounters(State&s,WorldActRooms&out,EncounterClass kind,
                                  EncounterId*queue,unsigned count,EncounterId previous=EncounterId::None){
    EncounterId pool[EncounterKinds];unsigned n=encounter_pool(out.region,kind,pool,EncounterKinds);
    if(!n||n>EncounterKinds)return world_fail(s);
    auto&rng=world_rng(s,WorldRng::UpFront);
    EncounterId bag[EncounterKinds];unsigned remaining=0;
    for(unsigned i=0;i<count;++i){
        if(!remaining){for(unsigned j=0;j<n;++j)bag[j]=pool[j];remaining=n;}
        // GrabBag.GrabAndRemove checks that a permitted entry exists, then
        // rejection-samples the whole weighted bag. Filtering first consumes
        // different draws. All ActModel entries have weight one.
        bool filtered=false;
        for(unsigned j=0;j<remaining;++j)if(bag[j]!=previous&&!encounter_shares_tags(bag[j],previous)){filtered=true;break;}
        unsigned selected;
        do{selected=rng.below(remaining);}while(filtered&&(bag[selected]==previous||encounter_shares_tags(bag[selected],previous)));
        queue[i]=bag[selected];for(unsigned j=selected+1;j<remaining;++j)bag[j-1]=bag[j];--remaining;previous=queue[i];
    }
    return true;
}
static bool world_make_room_sets(State&s,unsigned first_region){
    auto&rng=world_rng(s,WorldRng::UpFront);
    bool darv[WorldActCount]{};unsigned shared=1;
    // RunManager assigns a prefix of the remaining shared ancient list to
    // each later act before generating any act's rooms. Darv is its sole item.
    for(unsigned a=1;a<WorldActCount;++a){unsigned count=rng.below(shared+1);darv[a]=count!=0;shared-=count;}
    for(unsigned a=0;a<WorldActCount;++a){
        auto&q=s.world.rooms[a];q={};q.region=uint8_t(a==0?first_region:a+1);
        const auto&pool=WorldRegionPools[q.region];
        for(unsigned j=0;j<pool.event_count;++j)q.events[q.event_count++]=uint8_t(pool.events[j]);
        for(auto id:WorldSharedEvents){if(q.event_count==MaxWorldEventQueue)return world_fail(s);q.events[q.event_count++]=uint8_t(id);}
        reference::shuffle(q.events,q.event_count,rng);
        q.encounter_count=pool.rooms;q.elite_count=15;
        if(!world_fill_encounters(s,q,EncounterClass::Weak,q.encounters,pool.weak))return false;
        if(!world_fill_encounters(s,q,EncounterClass::Normal,q.encounters+pool.weak,pool.rooms-pool.weak,q.encounters[pool.weak-1]))return false;
        if(!world_fill_encounters(s,q,EncounterClass::Elite,q.elites,q.elite_count))return false;
        EncounterId bosses[EncounterKinds];unsigned n=encounter_pool(q.region,EncounterClass::Boss,bosses,EncounterKinds);
        if(!n||n>EncounterKinds)return world_fail(s);
        unsigned b=rng.below(n);q.bosses[0]=bosses[b];q.boss_count=1;
        unsigned ancient=rng.below(pool.ancient_count+unsigned(darv[a]));
        q.ancient=uint8_t(ancient==pool.ancient_count?OriginalEventId::Darv:pool.ancients[ancient]);
        if(a==2&&s.world.ascension>=10){
            if(n<2)return world_fail(s);for(unsigned j=b+1;j<n;++j)bosses[j-1]=bosses[j];
            q.bosses[1]=bosses[rng.below(n-1)];q.boss_count=2;
        }
        q.ready=1;
    }
    return true;
}
static bool world_enter_event(State&s,OriginalEventId id,bool mark_visited=true){
    if(id<=OriginalEventId::None||id>=OriginalEventId::Count)return world_fail(s);
    if(!original_event_begin(s,id,world_run_seed(s),s.world.act_index))return world_fail(s);
    if(mark_visited)s.world.visited_events[unsigned(id)]=1;
    return true;
}
static void world_reset_unknown_odds(State&s){
    auto&w=s.world;w.unknown_monster_bits=0x3dcccccdU;w.unknown_elite_bits=has_run_modifier(s,RunModifier::DeadlyEvents)?0x3dcccccdU:0xbf800000U;
    w.unknown_shop_bits=0x3cf5c28fU;w.unknown_treasure_bits=0x3ca3d70aU;
}
static bool world_enter_act(State&s,unsigned act){
    if(act>=WorldActCount||!s.world.rooms[act].ready)return world_fail(s);
    auto&w=s.world;w.act_index=uint8_t(act);w.current_point=NoPoint;w.room_open=0;w.point_had_shop=0;w.finish_pending=0;
    w.map_column=3;w.map_row=0;world_reset_unknown_odds(s);
    reference::ActSettings settings;settings.act=reference::ActKind(w.rooms[act].region);
    settings.increased_elites=w.ascension>=1;settings.second_boss=w.rooms[act].boss_count==2;
    settings.variant=uint8_t(world_map_variant(s));
    settings.big_game_hunter=has_run_modifier(s,RunModifier::BigGameHunter);
    reference::QuotaTrace trace;
    if(!reference::generate_act_map(w.maps[act],w.run_seed,settings,trace))return world_fail(s);
    w.map_ready[act]=1;world_relic_map_generated(s);
    s.current_node=-1;s.next_row=0;s.phase=Phase::Map;w.room_type=uint8_t(WorldRoom::Map);
    if(has_run_modifier(s,RunModifier::CursedRun)){
        WorldCardOptions options;options.pools[0]=WorldCardPool::Curse;CardKind cards[CardKinds];
        unsigned n=world_card_candidates(s,options,cards,CardKinds),count=0;
        for(unsigned i=0;i<n;++i)if(world_card_modifier_allowed(cards[i]))cards[count++]=cards[i];
        if(!count||event_add_deck_card(s,{cards[world_rng(s,WorldRng::Niche).below(count)],0})<0)return world_fail(s);
    }
    ++s.revision;
    // Only the first Neow is automatically entered. Later ancients remain a
    // selectable starting point, exactly like RunManager.EnterAct.
    return act?true:world_choose_point(s,w.maps[act].root);
}
static EncounterId world_pull_encounter(State&s,WorldRoom type){
    auto&q=s.world.rooms[s.world.act_index];
    if(type==WorldRoom::Boss){unsigned at=q.boss_cursor++?1:0;return q.bosses[at<q.boss_count?at:0];}
    if(type==WorldRoom::Elite)return q.elite_count?q.elites[q.elite_cursor++%q.elite_count]:EncounterId::None;
    return q.encounter_count?q.encounters[q.encounter_cursor++%q.encounter_count]:EncounterId::None;
}
static bool world_start_combat(State&s,EncounterId id){
    if(id==EncounterId::None||unsigned(id)>=EncounterKinds)return world_fail(s);
    s.enemy_world.encounter=id;++s.world.combat_count;enter_combat(s);return s.phase!=Phase::Error;
}
static OriginalEventId world_pull_event(State&s,bool forced){
    auto&q=s.world.rooms[s.world.act_index];if(!q.event_count)return OriginalEventId::None;
    for(unsigned attempts=0;attempts<q.event_count;++attempts){
        auto id=OriginalEventId(q.events[q.event_cursor%q.event_count]);
        if(!s.world.visited_events[unsigned(id)]&&original_event_allowed(s,id,s.world.act_index,s.world.total_floor))break;
        ++q.event_cursor;
    }
    // Original RoomSet permits repetition after one complete unsuccessful
    // scan. Lantern Key replaces the selected event after this scan.
    return forced?OriginalEventId::WarHistorianRepy:OriginalEventId(q.events[q.event_cursor%q.event_count]);
}
static bool world_free_travel(const State&s){return has_run_modifier(s,RunModifier::Flight)||(has_world_relic(s,WorldRelicWingedBoots)&&world_relic_counter(s,WorldRelicWingedBoots)>0);}
static void world_use_free_travel(State&s){
    if(has_run_modifier(s,RunModifier::Flight))return;
    for(unsigned i=0;i<s.world_relics.count;++i){auto&r=s.world_relics.entries[i];
        if(r.id==WorldRelicWingedBoots&&!(r.flags&WorldRelicMelted)&&r.values[0]<3)++r.values[0];
    }
}
} // namespace

bool world_begin_run(State&s,reference::ActKind first_region,unsigned ascension){
    if(s.practice||s.world.active||unsigned(first_region)>1||ascension>10||s.hp<=0||!s.deck_count)return false;
    s.world.active=1;s.world.ascension=uint8_t(ascension);s.world.act_index=0;
    world_rng(s,WorldRng::UpFront);init_world_relics(s);original_event_new_run(s);
    s.potion_slots=uint8_t(ascension>=4?2:3);
    if(ascension>=5&&event_add_deck_card(s,{EventCardAscendersBane,0},nullptr,true)<0)return world_fail(s);
    if(s.phase==Phase::Error||!world_make_room_sets(s,unsigned(first_region)))return false;
    if(has_run_modifier(s,RunModifier::DeadlyEvents))world_remove_relic_from_bags(s,WorldRelicJuzuBracelet);
    if(s.run_options.modifiers&RunExclusiveDeckModifiers){
        s.deck_count=0;for(auto&c:s.deck)c={};for(auto&f:s.event_cards.deck)f={};
        for(auto&v:s.defect.genetic_deck)v=0;for(auto&v:s.necro.scythe_deck)v=0;
    }
    return world_enter_act(s,0);
}
const char* world_region_key(const State&s){return s.world.active&&s.world.act_index<WorldActCount&&s.world.rooms[s.world.act_index].region<4?WorldRegionPools[s.world.rooms[s.world.act_index].region].key:"";}
bool world_room_is_combat(const State&s){return s.world.active&&s.world.room_type>=uint8_t(WorldRoom::Monster)&&s.world.room_type<=uint8_t(WorldRoom::Boss);}
bool world_retained_reward_group(const State&s){
    const auto&w=s.world;const auto&r=s.world_relics;const auto*map=world_current_map(s);
    if(!world_room_is_combat(s)||!map||!w.room_open||w.final_event||w.event_combat_parent||s.original_events.active||
       s.phase!=Phase::WorldRelicChoice||r.choice!=WorldRelicChoice::OfferReward||r.return_phase!=uint8_t(Phase::Reward)||
       !r.choice_group||r.choice_group>r.group_count||r.choice_min)return false;
    const unsigned end=map->second_boss!=NoPoint?map->second_boss:map->boss;
    const unsigned group=r.choice_group-1;
    return w.current_point!=end&&r.group_active[group]&&r.group_skip[group]&&!r.group_owner[group];
}
bool world_can_travel(const State&s){
#if SPIRE_DEBUG_COMMANDS
    if(debug_travel(s))return s.world.active&&s.hp>0&&s.phase!=Phase::Error;
#endif
    const auto&w=s.world;
    if(!w.active||w.finished||w.final_event||s.hp<=0||w.finish_pending||w.rest_pending||
       w.shop_free_remove||w.shop_auto_active||w.shop_pending_item!=255||
       (world_relic_pending(s)&&!world_retained_reward_group(s)))return false;
    if(s.phase==Phase::Map)return !w.room_open;
    if(!w.room_open||w.room_open>2)return false;
    // NMapScreen's travel permission is enabled by the room, not by the
    // Proceed button. Top-bar map travel therefore works before that button
    // is pressed in an already eligible merchant/treasure/reward room.
    switch(s.phase){
    case Phase::Shop:return w.room_type==uint8_t(WorldRoom::Shop)&&!s.original_events.active;
    case Phase::Treasure:return w.room_type==uint8_t(WorldRoom::Treasure)&&!s.original_events.active&&
        (w.room_open==2||s.treasure_state==2||(s.treasure_state==1&&w.treasure_relic==NoWorldRelic));
    case Phase::Camp:return w.room_type==uint8_t(WorldRoom::Rest)&&!s.original_events.active&&
        (w.room_open==2||world_rest_can_proceed(s));
    case Phase::Reward:return !s.original_events.active&&!w.event_combat_parent;
    case Phase::WorldRelicChoice:return world_retained_reward_group(s);
    case Phase::Event:{
        const auto&e=s.original_events;
        return w.room_type==uint8_t(WorldRoom::Event)&&!w.event_combat_parent&&
            (!e.active||(e.finished&&!e.combat_pending&&e.selection==OriginalEventSelection::None&&
                e.step_cursor==e.step_count&&!e.reward_active));
    }
    default:return false;
    }
}
bool world_reachable(const State&s,unsigned point){
    const auto*map=world_current_map(s);const auto&w=s.world;
#if SPIRE_DEBUG_COMMANDS
    if(debug_travel(s))return map&&s.hp>0&&point<map->count&&map->points[point].active;
#endif
    if(!map||!world_can_travel(s)||point>=map->count||!map->points[point].active||w.visited[w.act_index][point])return false;
    if(w.current_point>=map->count)return point==map->root;
    if(map->points[w.current_point].children.contains(uint8_t(point)))return true;
    return world_free_travel(s)&&map->points[point].row==map->points[w.current_point].row+1;
}
unsigned world_available_points(const State&s,uint8_t*out,unsigned capacity){
    const auto*map=world_current_map(s);if(!map)return 0;unsigned n=0;
    for(unsigned col=0;col<reference::Columns;++col)for(unsigned i=0;i<map->count;++i)if(map->points[i].col==col&&world_reachable(s,i)){if(out&&n<capacity)out[n]=uint8_t(i);++n;}return n;
}
bool world_choose_column(State&s,int col){
    if(col<0||col>=reference::Columns)return false;const auto*map=world_current_map(s);if(!map)return false;
    for(unsigned i=0;i<map->count;++i)if(map->points[i].col==col&&world_reachable(s,i))return world_choose_point(s,i);return false;
}
static void world_history_leave(const State&);
bool world_choose_point(State&s,unsigned point){
#if SPIRE_DEBUG_COMMANDS
    if(debug_travel(s)&&!debug_detail::transaction_active())return debug_travel_to(s,point);
#endif
    if(!world_reachable(s,point))return false;
    auto&w=s.world;
    // Opening/closing the UI never exits a room. A direct top-bar selection
    // performs the same one-time completion as Proceed, at this boundary.
    if(w.room_open==1&&!world_finish_room(s))return false;
    // NRewardsScreen.Proceed opens the map without skipping its terminal
    // set. Commit its remaining choices only at this real destination exit.
    if(world_retained_reward_group(s)){
        if(!world_relic_select(s,nullptr,0)||world_relic_pending(s))return false;
    }
    world_history_leave(s);
    const auto&p=w.maps[w.act_index].points[point];
    WorldRoom room=WorldRoom::None;
    switch(p.type){
    case PointType::Monster:room=WorldRoom::Monster;break;case PointType::Elite:room=WorldRoom::Elite;break;
    case PointType::Boss:room=WorldRoom::Boss;break;case PointType::Shop:room=WorldRoom::Shop;break;
    case PointType::Rest:room=WorldRoom::Rest;break;case PointType::Treasure:room=WorldRoom::Treasure;break;
    case PointType::Start:case PointType::Unknown:room=WorldRoom::Event;break;default:return false;
    }
    if(p.type==PointType::Unknown)room=world_roll_unknown(s,point);
    if(room==WorldRoom::None)return world_fail(s);
    if(w.current_point<w.maps[w.act_index].count&&!w.maps[w.act_index].points[w.current_point].children.contains(uint8_t(point)))
#if SPIRE_DEBUG_COMMANDS
        if(!debug_travel(s))
#endif
        world_use_free_travel(s);
    // Retain all old room offers until now, including unopened chest relics,
    // optional rest actions and finished event text. No UI-only action can
    // clear these or replay their acquisition hooks.
    s.original_events.active=0;w.finish_pending=0;s.necro.grimoire_pending=0;
    w.current_point=uint8_t(point);w.map_column=p.col;w.map_row=p.row;w.room_type=uint8_t(room);w.room_open=1;w.point_had_shop=room==WorldRoom::Shop;
    w.visited[w.act_index][point]=1;w.visited_rooms[w.act_index][point]=uint8_t(room);
    ++w.total_floor;s.floor=uint8_t(w.total_floor);s.treasure_state=0;s.treasure_relic=0;
    w.treasure_relic=NoWorldRelic;w.treasure_gold=0;w.treasure_gold_claimed=0;w.reward_count=0;w.reward_relic=NoWorldRelic;
    w.rest_count=w.rest_pending=0;w.rest_used=0;w.rest_deck_before=0;
    s.event_deck_action=EventDeckAction::None;s.event_first=-1;s.event_remaining=0;
    event_card_before_room(s,p.type==PointType::Unknown,true);
    if(s.phase==Phase::Dead||s.phase==Phase::Error)return s.phase!=Phase::Error;
    switch(room){
    case WorldRoom::Monster:case WorldRoom::Elite:case WorldRoom::Boss:
        if(!world_start_combat(s,world_pull_encounter(s,room)))return false;break;
    case WorldRoom::Shop:if(!world_make_shop(s))return false;break;
    case WorldRoom::Rest:if(!world_make_rest(s))return false;break;
    case WorldRoom::Treasure:s.phase=Phase::Treasure;break;
    case WorldRoom::Event:{
        OriginalEventId id=p.type==PointType::Start?OriginalEventId(w.rooms[w.act_index].ancient):world_pull_event(s,event_card_forces_historian(s,w.act_index));
        if(!world_enter_event(s,id,p.type!=PointType::Start))return false;
        ++w.rooms[w.act_index].event_cursor;break;
    }
    default:return world_fail(s);
    }
    // Combat setup invokes this after constructing/shuffling the deck and
    // before the initial draw. Noncombat rooms have no combat continuation.
    if(!world_room_is_combat(s)){
        world_relic_hook(s,WorldRelicHook::AfterRoomEntered);
        if(has_run_modifier(s,RunModifier::Terminal)&&s.max_hp>1)player_lose_max_hp(s,1);
    }
    if(room==WorldRoom::Treasure&&!world_prepare_treasure(s))return world_fail(s);
    if(room==WorldRoom::Shop)world_resume_room(s);
    ++s.revision;return s.phase!=Phase::Error;
}
bool world_refresh_map_modifiers(State&s){
    auto&w=s.world;if(!w.active||!w.map_ready[w.act_index])return false;
    unsigned variant=world_map_variant(s);if(w.maps[w.act_index].config.variant==variant)return true;
    // These modifiers are obtained at an ancient. Retain its visited coordinate
    // when replacing the graph; no arbitrary route is silently reconnected.
    if(w.current_point!=w.maps[w.act_index].root)return world_fail(s);
    reference::ActSettings settings;settings.act=reference::ActKind(w.rooms[w.act_index].region);
    settings.increased_elites=w.ascension>=1;settings.second_boss=w.rooms[w.act_index].boss_count==2;settings.variant=uint8_t(variant);
    settings.big_game_hunter=has_run_modifier(s,RunModifier::BigGameHunter);
    reference::QuotaTrace trace;if(!reference::generate_act_map(w.maps[w.act_index],w.run_seed,settings,trace))return world_fail(s);
    for(auto&v:w.visited[w.act_index])v=0;for(auto&v:w.visited_rooms[w.act_index])v=0;
    w.current_point=w.maps[w.act_index].root;w.visited[w.act_index][w.current_point]=1;
    w.visited_rooms[w.act_index][w.current_point]=uint8_t(WorldRoom::Event);
    world_relic_map_generated(s);return true;
}
bool world_enter_event_combat(State&s,const char*key,bool resume_parent){
    if(!s.world.active||!s.world.room_open||!s.original_events.active||s.world.event_combat_parent)return false;
    EncounterId id=encounter_id(key);if(id==EncounterId::None)return false;
    s.world.event_combat_parent=1;s.world.parent_room_type=s.world.room_type;s.world.room_type=uint8_t(WorldRoom::Monster);
    s.original_events.combat_pending=1;s.original_events.resume_after_combat=resume_parent;s.original_events.encounter=uint16_t(id);
    event_card_before_room(s,false,false);return world_start_combat(s,id);
}
static void world_history_leave(const State&s){
    const auto&w=s.world;if(!w.room_open)return;
    if(s.phase==Phase::Reward){
        if(!(s.reward_claimed&2))for(unsigned i=0;i<w.reward_count;++i)notify_history(s,RunHistoryEvent::CardSkipped,unsigned(card_token(w.reward_cards[i])));
        if(!(s.reward_claimed&4)&&s.reward_potion!=Potion::Empty)notify_history(s,RunHistoryEvent::PotionSkipped,unsigned(s.reward_potion));
        if(!w.reward_relic_claimed&&w.reward_relic!=NoWorldRelic)notify_history(s,RunHistoryEvent::RelicSkipped,w.reward_relic);
    }else if(s.phase==Phase::Shop){
        // MerchantRoom.Exit records the remaining inventory, including any
        // actual Courier replacement stocked after a completed purchase.
        for(unsigned i=0;i<ShopRemove;++i)if(!(s.shop_sold&(1u<<i))){
            if(i<ShopCardCount)notify_history(s,RunHistoryEvent::CardSkipped,unsigned(card_token(w.shop_cards[i])));
            else if(i<ShopPotionStart){const auto id=w.shop_relics[i-ShopRelicStart];if(id!=NoWorldRelic)notify_history(s,RunHistoryEvent::RelicSkipped,id);}
            else if(s.shop_potions[i-ShopPotionStart])notify_history(s,RunHistoryEvent::PotionSkipped,s.shop_potions[i-ShopPotionStart]);
        }
    }
}
bool world_finish_room(State&s){
    auto&w=s.world;if(!w.active)return false;
    if(s.hp<=0){w.finished=1;s.phase=Phase::Dead;original_event_finish_run(s);return true;}
    if(w.shop_free_remove||w.shop_auto_active||w.shop_pending_item!=255)return false;
    if((world_relic_pending(s)&&!world_retained_reward_group(s))||s.phase==Phase::OriginalEventChoice){w.finish_pending=1;return false;}
    w.finish_pending=0;
    if(w.rest_pending){
        unsigned index=w.rest_pending-1;if(index>=w.rest_count)return world_fail(s);
        const auto option=WorldRestOption(w.rest_options[index]);
        if(option==WorldRestOption::Cook){
            if(s.deck_count==w.rest_deck_before){w.rest_pending=0;w.rest_deck_before=0;s.phase=Phase::Camp;++s.revision;return true;}
            if(s.deck_count+2!=w.rest_deck_before)return world_fail(s);
            increase_max_hp(s,5);
        }
        notify_run_fact(s,CoreRunFact::RestChoice,unsigned(option));
        w.rest_used|=uint32_t(1)<<index;w.rest_pending=0;w.rest_deck_before=0;s.phase=Phase::Camp;
        if(has_world_relic(s,WorldRelicMiniatureTent))world_relic_flash(s,WorldRelicMiniatureTent);
        else w.rest_used=w.rest_count==32?~uint32_t(0):(uint32_t(1)<<w.rest_count)-1;
        // Selection finishes the option, not the room. The original rest site
        // always reveals Proceed after its callbacks and remaining-options
        // update, including when all options were cleared without a Tent.
        ++s.revision;return true;
    }
    if(w.event_combat_parent&&s.original_events.combat_pending){
        world_history_leave(s);
        w.event_combat_parent=0;w.room_type=w.parent_room_type;w.parent_room_type=0;
        original_event_after_combat(s,true);
        if(s.original_events.active||s.phase==Phase::Dead||s.phase==Phase::Error)return s.phase!=Phase::Error;
    }
    if(s.original_events.active&&(!s.original_events.finished||s.original_events.combat_pending||
       s.original_events.selection!=OriginalEventSelection::None||s.original_events.step_cursor<s.original_events.step_count))return false;
    const auto*map=world_current_map(s);
    if(!map||!w.room_open||w.current_point>=map->count)return false;
    if(w.current_point==map->root&&!world_refresh_map_modifiers(s))return false;
    if(w.room_open==2)return true;
    if(w.final_event){w.room_open=0;s.original_events.active=0;w.finished=1;s.phase=Phase::Complete;original_event_finish_run(s);++s.revision;return true;}
    ++w.rooms_completed;
    const unsigned end=map->second_boss!=NoPoint?map->second_boss:map->boss;
    if(w.current_point==end){
        world_history_leave(s);
        w.room_open=0;s.original_events.active=0;
        s.treasure_state=0;s.treasure_relic=0;w.treasure_relic=NoWorldRelic;w.treasure_gold=0;w.treasure_gold_claimed=0;
        s.event_deck_action=EventDeckAction::None;s.event_first=-1;s.event_remaining=0;s.necro.grimoire_pending=0;
        if(w.act_index+1<WorldActCount)return world_enter_act(s,w.act_index+1);
        w.final_event=1;w.room_open=1;w.room_type=uint8_t(WorldRoom::Event);s.phase=Phase::Event;
        // Architect is another room at the final map coordinate, not a floor.
        if(!world_enter_event(s,OriginalEventId::TheArchitect))return false;
    }else{
        // Complete the room once while retaining its real phase, contents and
        // callbacks. Proceed only asks the frontend to open its map window.
        w.room_open=2;
    }
    ++s.revision;return true;
}
void world_resume_room(State&s){
    if(!s.world.active||world_relic_pending(s)||s.phase==Phase::OriginalEventChoice||s.phase==Phase::Error||s.phase==Phase::Dead)return;
    if(s.world.room_type==unsigned(WorldRoom::Shop))world_continue_shop(s);
    if(s.world.finish_pending&&!world_relic_pending(s)&&s.phase!=Phase::OriginalEventChoice)world_finish_room(s);
}

#include "world_run_rooms.inc"
#include "world_run_rest.inc"
#include "world_run_validation.inc"
