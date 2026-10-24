// Debug warps use real room constructors while retaining the run's map graph.
// A room replacement is not a claimed map visit or a grant of its old reward.
bool debug_enter_room(State&s,WorldRoom room,EncounterId encounter,OriginalEventId event){
    const bool combat=room==WorldRoom::Monster||room==WorldRoom::Elite||room==WorldRoom::Boss;
    if(!s.world.active||room==WorldRoom::None||unsigned(room)>unsigned(WorldRoom::Map)||
       unsigned(encounter)>=EncounterKinds||unsigned(event)>=unsigned(OriginalEventId::Count)||
       (!combat&&encounter!=EncounterId::None)||(room!=WorldRoom::Event&&event!=OriginalEventId::None)||
       (event!=OriginalEventId::None&&!original_event_implemented(event)))return false;
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
        ok=world_enter_event(s,event);break;
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
