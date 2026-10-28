#include "debug_ancient_options.inc"
RelicId debug_ancient_offer(OriginalEventId event,const char*partial){
    if(!partial||!*partial)return NoWorldRelic;
    for(unsigned i=0;i<WorldRelicKinds;++i)if(debug_ancient_offer_allowed(event,RelicId(i))){
        for(const char*start=world_relic_key(RelicId(i));*start;++start){const char*a=start,*b=partial;
            while(*a&&*b){char x=*a,y=*b;if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y)break;++a;++b;}
            if(!*b)return RelicId(i);
        }
    }return NoWorldRelic;
}
// Debug warps use real room constructors while retaining the run's map graph.
// A room replacement is not a claimed map visit or a grant of its old reward.
bool debug_enter_room(State&s,WorldRoom room,EncounterId encounter,OriginalEventId event,RelicId forced_offer){
    const bool combat=room==WorldRoom::Monster||room==WorldRoom::Elite||room==WorldRoom::Boss;
    if(!s.world.active||room==WorldRoom::None||unsigned(room)>unsigned(WorldRoom::Map)||
       unsigned(encounter)>=EncounterKinds||unsigned(event)>=unsigned(OriginalEventId::Count)||
       (!combat&&encounter!=EncounterId::None)||(room!=WorldRoom::Event&&event!=OriginalEventId::None)||
       (event!=OriginalEventId::None&&!original_event_implemented(event))||
       (forced_offer!=NoWorldRelic&&(room!=WorldRoom::Event||!debug_ancient_offer_allowed(event,forced_offer))))return false;
    if(!debug_detail::begin(s))return false;
    debug_detail::abandon_room(s);auto&w=s.world;
    if(room==WorldRoom::Map){w.room_type=uint8_t(room);++s.revision;return debug_detail::finish(s,true);}
    // An act-map with no current room gains its real Start coordinate once.
    // Subsequent debug replacements retain that coordinate's recorded icon.
    if(w.current_point==reference::NoPoint){
        const auto&m=w.maps[w.act_index];w.current_point=m.root;
        w.map_column=m.points[m.root].col;w.map_row=m.points[m.root].row;
        if(!w.visited[w.act_index][m.root]){
            w.visited[w.act_index][m.root]=1;w.visited_rooms[w.act_index][m.root]=uint8_t(WorldRoom::Event);
            ++w.total_floor;s.floor=uint8_t(w.total_floor);
        }
    }
    if(w.total_floor>64)return debug_detail::finish(s,false);
    // Reopening the same coordinate cannot count its completion twice.
    if(w.rooms_completed>=w.total_floor)w.rooms_completed=w.total_floor?w.total_floor-1:0;
    w.room_type=uint8_t(room);w.room_open=1;w.point_had_shop=room==WorldRoom::Shop;
    bool ok=true;
    switch(room){
    case WorldRoom::Monster:case WorldRoom::Elite:case WorldRoom::Boss:
        if(w.combat_count>=255){ok=false;break;}
        if(encounter==EncounterId::None)encounter=world_pull_encounter(s,room);
        ok=world_start_combat(s,encounter);break;
    case WorldRoom::Shop:ok=world_make_shop(s);break;
    case WorldRoom::Rest:ok=world_make_rest(s);break;
    case WorldRoom::Treasure:s.phase=Phase::Treasure;break;
    case WorldRoom::Event:
        if(event==OriginalEventId::None){event=world_pull_event(s,false);++w.rooms[w.act_index].event_cursor;}
        ok=world_enter_event(s,event);
        if(ok&&forced_offer!=NoWorldRelic){
            // AncientEventModel replaces the first generated option while
            // retaining the other two and their prepared random facts.
            auto&e=s.original_events;ok=e.offer_count&&e.option_count&&world_relic_setup_offer(s,forced_offer);
            if(ok){e.offers[0]=forced_offer;e.options[0]=0;const char*prefix="INITIAL.options.";const char*key=world_relic_key(forced_offer);unsigned n=0;
                for(;*prefix&&n+1<sizeof(e.option_keys[0]);++prefix)e.option_keys[0][n++]=*prefix;
                for(;*key&&n+1<sizeof(e.option_keys[0]);++key)e.option_keys[0][n++]=*key;e.option_keys[0][n]=0;}
        }break;
    default:ok=false;break;
    }
    if(ok&&!combat)world_relic_hook(s,WorldRelicHook::AfterRoomEntered);
    if(ok&&room==WorldRoom::Treasure)ok=world_prepare_treasure(s);
    if(ok&&room==WorldRoom::Shop)world_resume_room(s);
    ++s.revision;return debug_detail::finish(s,ok);
}

bool debug_enter_act(State&s,unsigned act){
    if(!s.world.active||act>=WorldActCount||!debug_detail::begin(s))return false;
    debug_detail::abandon_room(s);auto&w=s.world;
    for(unsigned a=act;a<WorldActCount;++a){
        w.map_ready[a]=0;reset_in_place(w.maps[a]);
        for(auto&v:w.visited[a])v=0;for(auto&v:w.visited_rooms[a])v=0;
        auto&q=w.rooms[a];q.encounter_cursor=q.elite_cursor=q.event_cursor=q.boss_cursor=0;
    }
    unsigned visited=0;for(const auto&row:w.visited)for(auto v:row)visited+=v;
    w.total_floor=uint16_t(visited);s.floor=uint8_t(visited);
    if(w.rooms_completed>w.total_floor)w.rooms_completed=w.total_floor;
    // Validation requires all preceding acts to possess their actual graph.
    // Generate skipped intermediate maps through the same seeded API.
    for(unsigned a=0;a<act;++a)if(!w.map_ready[a]&&!world_enter_act(s,a))return debug_detail::finish(s,false);
    const bool ok=world_enter_act(s,act);++s.revision;
    return debug_detail::finish(s,ok);
}

bool debug_travel_to(State&s,unsigned point){
    if(!debug_travel(s)||!world_reachable(s,point)||!debug_detail::begin(s))return false;
    debug_detail::abandon_room(s);auto&w=s.world;
    // A second visit replaces that point's debug visit; it must not invent a
    // duplicate route node or consume Winged Boots / Flight charges.
    if(w.visited[w.act_index][point]){w.visited[w.act_index][point]=0;w.visited_rooms[w.act_index][point]=0;if(w.total_floor)--w.total_floor;}
    s.floor=uint8_t(w.total_floor);if(w.rooms_completed>w.total_floor)w.rooms_completed=w.total_floor;
    const bool ok=world_choose_point(s,point);
    return debug_detail::finish(s,ok);
}
bool debug_replace_act(State&s,reference::ActKind region){
    if(!s.world.active||unsigned(region)>=4||!debug_detail::begin(s))return false;
    debug_detail::abandon_room(s);auto&w=s.world;const unsigned act=w.act_index;
    auto&q=w.rooms[act];q={};q.region=uint8_t(region);const auto&pool=WorldRegionPools[q.region];auto&rng=world_rng(s,WorldRng::UpFront);
    for(unsigned i=0;i<pool.event_count;++i)q.events[q.event_count++]=uint8_t(pool.events[i]);
    for(auto id:WorldSharedEvents)q.events[q.event_count++]=uint8_t(id);
    reference::shuffle(q.events,q.event_count,rng);q.encounter_count=pool.rooms;q.elite_count=15;
    bool ok=world_fill_encounters(s,q,EncounterClass::Weak,q.encounters,pool.weak)&&
        world_fill_encounters(s,q,EncounterClass::Normal,q.encounters+pool.weak,pool.rooms-pool.weak,q.encounters[pool.weak-1])&&
        world_fill_encounters(s,q,EncounterClass::Elite,q.elites,q.elite_count);
    EncounterId bosses[EncounterKinds];const unsigned n=encounter_pool(q.region,EncounterClass::Boss,bosses,EncounterKinds);
    if(!ok||!n||n>EncounterKinds)return debug_detail::finish(s,false);
    unsigned chosen=rng.below(n);q.bosses[0]=bosses[chosen];q.boss_count=1;
    q.ancient=uint8_t(pool.ancients[rng.below(pool.ancient_count)]);
    if(act==2&&w.ascension>=10&&n>1){for(unsigned i=chosen+1;i<n;++i)bosses[i-1]=bosses[i];q.bosses[1]=bosses[rng.below(n-1)];q.boss_count=2;}
    q.ready=1;
    for(unsigned a=act;a<WorldActCount;++a){w.map_ready[a]=0;reset_in_place(w.maps[a]);for(auto&v:w.visited[a])v=0;for(auto&v:w.visited_rooms[a])v=0;}
    unsigned visited=0;for(const auto&row:w.visited)for(auto v:row)visited+=v;w.total_floor=uint16_t(visited);s.floor=uint8_t(visited);w.rooms_completed=uint16_t(min(w.rooms_completed,visited));
    ok=world_enter_act(s,act);++s.revision;return debug_detail::finish(s,ok);
}
