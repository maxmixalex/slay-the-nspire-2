static uint32_t progress_add(uint32_t value,uint32_t amount){return amount>UINT32_MAX-value?UINT32_MAX:value+amount;}
static void progress_clear_run(UiRunHistory&r){auto*bytes=reinterpret_cast<uint8_t*>(&r);for(unsigned i=0;i<sizeof r;++i)bytes[i]=0;}
static bool progress_matches(const UiRunHistory&r,const State&s){
    return r.serial&&s.world.active&&!s.practice&&r.seed==s.seed&&r.run_seed==s.world.run_seed&&
        r.character==unsigned(s.character)&&r.ascension==s.world.ascension&&r.start_region==s.world.rooms[0].region;
}
static void progress_snapshot(UiRunHistory&r,const State&s){
    r.floor=s.world.total_floor;r.hp=uint16_t(mx(0,s.hp));r.max_hp=uint16_t(mx(0,s.max_hp));r.gold=uint16_t(mx(0,s.gold));
    r.potion_count=uint8_t(potion_slot_count(s));for(unsigned i=0;i<MaxPotionSlots;++i)r.potions[i]=unsigned(potion_at(s,i));
    r.deck_count=s.deck_count;
    for(unsigned i=0;i<s.deck_count;++i)r.deck[i]={s.deck[i],s.event_cards.deck[i],s.defect.genetic_deck[i],s.necro.scythe_deck[i]};
    r.relic_count=0;
    if(s.world_relics.active){for(unsigned i=0;i<s.world_relics.count&&r.relic_count<WorldRelicCapacity;++i)
        if(!(s.world_relics.entries[i].flags&WorldRelicMelted))r.relics[r.relic_count++]=s.world_relics.entries[i].id;}
    else for(unsigned i=0;i<s.relic_count&&r.relic_count<WorldRelicCapacity;++i)r.relics[r.relic_count++]=s.relic_order[i];
    for(unsigned i=0;i<3;++i)r.regions[i]=s.world.rooms[i].region;
    r.encounter=unsigned(s.enemy_world.encounter);r.event=unsigned(s.original_events.id);
}
void App::restore_progress(){
    if(progress_loaded)return;
    clear_ui_progress(progress);
    if(callbacks.load_progress&&!callbacks.load_progress(progress))clear_ui_progress(progress);
    progress_loaded=true;progress_dirty=false;progress_seen_phase=255;progress_remainder_ms=0;
}
bool App::persist_progress(){
    if(!progress_loaded||!progress_dirty)return true;
    if(progress_matches(progress.current,game)&&!progress.current.outcome)progress_snapshot(progress.current,game);
    if(callbacks.save_progress){const uint32_t previous_saved=progress.last_saved_at;
        if(callbacks.wall_clock)progress.last_saved_at=callbacks.wall_clock();
        if(!callbacks.save_progress(progress)){progress.last_saved_at=previous_saved;return false;}}
    progress_dirty=false;return true;
}
void App::begin_progress(bool resumed){
    if(!game.world.active||game.practice)return;
    restore_progress();progress.characters_started|=uint8_t(1u<<unsigned(game.character));
    if(resumed&&progress_matches(progress.current,game)){
        progress_seen_phase=255;progress_seen_revision=0;sync_progress();return;
    }
    auto&r=progress.current;progress_clear_run(r);
    r.serial=progress.next_serial++;if(!progress.next_serial)progress.next_serial=1;
    r.seed=game.seed;r.run_seed=game.world.run_seed;r.character=unsigned(game.character);r.ascension=game.world.ascension;
    r.start_region=game.world.rooms[0].region;r.partial=resumed;r.started_at=callbacks.wall_clock?callbacks.wall_clock():0;
    for(unsigned i=0;i<65;++i)r.seed_text[i]=game.world.seed_text[i];
    // Earlier fights in an old save have no observed per-enemy outcome.
    // Start counting from the resumed battle, never backfill invented wins.
    if(resumed&&!combat_phase(game.phase)&&!game.world.combat_ending)r.finished_battle=game.world.combat_count;
    progress_snapshot(r,game);progress_dirty=true;progress_seen_phase=255;progress_seen_revision=0;progress_remainder_ms=0;
    sync_progress();persist_progress();
}
void App::progress_time(uint32_t milliseconds){
    auto&r=progress.current;
    if(title||paused||settings_open||!progress_matches(r,game)||r.outcome||game.phase==Phase::Dead||game.phase==Phase::Complete||game.phase==Phase::Error)return;
    progress_remainder_ms+=milliseconds;
    if(progress_remainder_ms>=1000){const unsigned seconds=progress_remainder_ms/1000;progress_remainder_ms%=1000;
        r.seconds=progress_add(r.seconds,seconds);
        if(r.floor_count){auto&floor=r.floors[r.floor_count-1];floor.seconds=r.seconds>=floor.entered_seconds?r.seconds-floor.entered_seconds:0;}
        progress_dirty=true;}
}
void App::discover_potion(Potion potion){
    const unsigned value=unsigned(potion);if(!value||value>PotionKinds)return;const unsigned id=value-1;
    const uint8_t bit=uint8_t(1u<<(id%8));if(!(progress.discovered_potions[id/8]&bit)){progress.discovered_potions[id/8]|=bit;progress_dirty=true;}
}
void App::discover_card(CardKind kind){
    const unsigned id=unsigned(kind);if(id>=CardKinds)return;const uint8_t bit=uint8_t(1u<<(id%8));
    if(!(progress.discovered_cards[id/8]&bit)){
        progress.discovered_cards[id/8]|=bit;progress_dirty=true;
        // NHoverTipSet can discover a generated card while its parent's
        // grid holder is being pressed. NCardLibrary rebuilds on filter/search
        // actions, not on this observation: rebuilding here moves the grid
        // under the pointer and changes the captured model before release.
    }
}
void App::discover_relic(RelicId relic){
    const unsigned id=unsigned(relic);if(id>=WorldRelicKinds)return;const uint8_t bit=uint8_t(1u<<(id%8));
    if(!(progress.discovered_relics[id/8]&bit)){progress.discovered_relics[id/8]|=bit;progress_dirty=true;}
}
void App::observe_progress_log(Log log){
    if(log.kind!=LogKind::Gold||log.a<=0||!progress_matches(progress.current,game)||progress.current.outcome)return;
    auto&r=progress.current;r.gold_gained=progress_add(r.gold_gained,unsigned(log.a));
    if(r.floor_count)r.floors[r.floor_count-1].gold_gained=progress_add(r.floors[r.floor_count-1].gold_gained,unsigned(log.a));
    progress_dirty=true;
}
static void progress_begin_battle(App&a,const State&s){
    auto&r=a.progress.current;if(r.last_battle==s.world.combat_count)return;
    r.last_battle=s.world.combat_count;r.last_enemy_identity=0;
    for(auto&v:r.battle_seen)v=0;
    const unsigned encounter=unsigned(s.enemy_world.encounter),dimensions[2]={0,unsigned(s.character)+1};
    if(encounter<EncounterKinds)for(unsigned index:dimensions){
        auto&stat=a.progress.encounters[encounter][index];stat.seen=progress_add(stat.seen,1);
    }
    a.progress_dirty=true;
}
void App::observe_enemy_spawned(const State&s,unsigned model,uint16_t identity){
    auto&r=progress.current;
    if(&s!=&game||!progress_matches(r,s)||r.outcome||!s.enemy_world.active||!s.world.combat_count||model>=EnemyKinds)return;
    progress_begin_battle(*this,s);
    if(!identity||identity<=r.last_enemy_identity)return;
    r.last_enemy_identity=identity;
    if(r.battle_seen[model]<UINT16_MAX)++r.battle_seen[model];
    const unsigned dimensions[2]={0,unsigned(s.character)+1};
    for(unsigned index:dimensions){auto&stat=progress.enemies[model][index];stat.seen=progress_add(stat.seen,1);}
    progress_dirty=true;
}
static bool progress_end_battle(App&a,const State&s,bool won){
    auto&r=a.progress.current;
    if(!r.last_battle||r.finished_battle==r.last_battle)return false;
    const unsigned dimensions[2]={0,unsigned(s.character)+1};
    // ProgressSaveManager iterates SpawnedEnemies, including each summon and
    // duplicate model. A resurrection keeps the same combat identity.
    for(unsigned model=0;model<EnemyKinds;++model)if(r.battle_seen[model])for(unsigned index:dimensions){
        auto&stat=a.progress.enemies[model][index];
        if(won)stat.wins=progress_add(stat.wins,r.battle_seen[model]);
        else stat.losses=progress_add(stat.losses,r.battle_seen[model]);
    }
    const unsigned encounter=unsigned(s.enemy_world.encounter);
    if(encounter<EncounterKinds)for(unsigned index:dimensions){
        auto&stat=a.progress.encounters[encounter][index];
        if(won)stat.wins=progress_add(stat.wins,1);else stat.losses=progress_add(stat.losses,1);
    }
    r.finished_battle=r.last_battle;a.progress_dirty=true;return true;
}
void App::sync_progress(){
    if(title||!game.world.active||game.practice||!progress_matches(progress.current,game)||progress.current.outcome)return;
    if(progress_seen_revision==game.revision&&progress_seen_phase==unsigned(game.phase)&&
       progress_seen_floor==game.world.total_floor&&progress_seen_battle==game.world.combat_count&&
       progress_seen_ending==bool(game.world.combat_ending))return;
    progress_seen_revision=game.revision;progress_seen_phase=unsigned(game.phase);
    progress_seen_floor=game.world.total_floor;progress_seen_battle=game.world.combat_count;
    progress_seen_ending=game.world.combat_ending;
    auto&r=progress.current;
    const unsigned region=game.world.rooms[game.world.act_index].region;
    if(region<4)progress.discovered_acts|=uint8_t(1u<<region);
    for(unsigned i=0;i<game.deck_count;++i)discover_card(game.deck[i].kind);
    for(unsigned i=0;i<potion_slot_count(game);++i)discover_potion(potion_at(game,i));
    if(game.phase==Phase::Shop){for(auto potion:game.shop_potions)discover_potion(Potion(potion));for(auto relic:game.world.shop_relics)discover_relic(relic);}
    if(game.phase==Phase::Reward&&!game.world.combat_ending){discover_potion(game.reward_potion);discover_relic(game.world.reward_relic);}
    if(game.original_events.active&&unsigned(game.original_events.id)&&unsigned(game.original_events.id)<=OriginalEventKinds){const unsigned id=unsigned(game.original_events.id)-1;progress.discovered_events[id/8]|=uint8_t(1u<<(id%8));}
    for(unsigned i=0;i<game.world_relics.count;++i)discover_relic(game.world_relics.entries[i].id);
    if(r.floor_count){auto&f=r.floors[r.floor_count-1];
        if(f.act==game.world.act_index&&f.point==game.world.current_point){
            f.hp=uint16_t(mx(0,game.hp));f.max_hp=uint16_t(mx(0,game.max_hp));f.gold=uint16_t(mx(0,game.gold));
            f.turns=uint16_t(mx(0,game.turn));f.seconds=r.seconds>=f.entered_seconds?r.seconds-f.entered_seconds:0;
            if(game.enemy_world.active)f.encounter=unsigned(game.enemy_world.encounter);
            if(game.original_events.active)f.event=unsigned(game.original_events.id);
        }
    }
    bool changed_room=false;
    if(game.world.room_open&&(!r.floor_count||r.floors[r.floor_count-1].act!=game.world.act_index||r.floors[r.floor_count-1].point!=game.world.current_point)){
        if(r.floor_count<UiHistoryFloors){auto&f=r.floors[r.floor_count++];f={};
            f.floor=game.world.total_floor;f.act=game.world.act_index;f.region=uint8_t(region);f.point=game.world.current_point;
            f.room=game.world.room_type;f.hp=uint16_t(mx(0,game.hp));f.max_hp=uint16_t(mx(0,game.max_hp));f.gold=uint16_t(mx(0,game.gold));
            f.entered_seconds=r.seconds;f.encounter=game.enemy_world.active?unsigned(game.enemy_world.encounter):0;
            f.event=game.original_events.active?unsigned(game.original_events.id):0;changed_room=true;
        }else r.partial=1;
    }
    if(game.world.combat_count&&game.enemy_world.active){
        progress_begin_battle(*this,game);
        // A resumed old save only exposes actors still present. Ordered
        // identities prevent re-counting slots or revived creatures.
        unsigned slots[MaxEnemies],count=0;
        for(unsigned i=0;i<game.enemy_count;++i)if(game.enemy_world.models[i].combat_id>r.last_enemy_identity)slots[count++]=i;
        for(unsigned i=1;i<count;++i){unsigned slot=slots[i],j=i;
            while(j&&game.enemy_world.models[slots[j-1]].combat_id>game.enemy_world.models[slot].combat_id){slots[j]=slots[j-1];--j;}slots[j]=slot;}
        for(unsigned i=0;i<count;++i){const auto&m=game.enemy_world.models[slots[i]];observe_enemy_spawned(game,unsigned(m.kind),m.combat_id);}
        const bool won=game.world.combat_ending||game.phase==Phase::Reward;
        if(won||game.phase==Phase::Dead)changed_room=progress_end_battle(*this,game,won)||changed_room;
    }
    progress_dirty=true;
    if(game.phase==Phase::Dead||game.phase==Phase::Complete)finish_progress();
    else if(changed_room)persist_progress();
}
void App::finish_progress(bool abandoned,const State*source){
    const State&s=source?*source:game;
    if(!progress_matches(progress.current,s)||progress.current.outcome)return;
    auto&r=progress.current;if(!abandoned&&s.phase!=Phase::Dead&&s.phase!=Phase::Complete)return;
    if(s.enemy_world.active&&r.last_battle==s.world.combat_count)
        progress_end_battle(*this,s,s.world.combat_ending||s.phase==Phase::Complete);
    progress_snapshot(r,s);r.outcome=abandoned?3:s.phase==Phase::Complete?1:2;r.abandoned=abandoned;
    auto&stats=progress.characters[r.character];
    if(r.outcome==1){stats.wins=progress_add(stats.wins,1);stats.current_streak=progress_add(stats.current_streak,1);
        // ProgressSaveManager counts distinct ModelIds in the final deck,
        // once per victory, independently from copy count or upgrade level.
        uint8_t unique[(CardKinds+7)/8]{};
        for(unsigned i=0;i<r.deck_count;++i){const unsigned id=r.deck[i].card.kind;if(id<CardKinds&&!(unique[id/8]&(1u<<(id%8)))){
            unique[id/8]|=uint8_t(1u<<(id%8));progress.card_wins[id]=progress_add(progress.card_wins[id],1);}}
        if(stats.current_streak>stats.best_streak)stats.best_streak=stats.current_streak;
        if(r.ascension==stats.highest_ascension&&stats.highest_ascension<10)++stats.highest_ascension;
        progress.current_streak=progress_add(progress.current_streak,1);if(progress.current_streak>progress.best_streak)progress.best_streak=progress.current_streak;
        if(!r.partial&&(!stats.fastest_win_seconds||r.seconds<stats.fastest_win_seconds))stats.fastest_win_seconds=r.seconds;
    }else{stats.losses=progress_add(stats.losses,1);stats.current_streak=0;progress.current_streak=0;}
    stats.play_seconds=progress_add(stats.play_seconds,r.seconds);stats.gold_gained=progress_add(stats.gold_gained,r.gold_gained);
    stats.floors_climbed=progress_add(stats.floors_climbed,r.floor);
    progress.history[progress.next_history]=r;progress.next_history=uint8_t((progress.next_history+1)%UiHistoryCapacity);
    if(progress.count<UiHistoryCapacity)++progress.count;
    progress_dirty=true;persist_progress();
}
bool App::bestiary_available()const{
    if(!progress.discovered_acts)return false;
    for(const auto&model:progress.enemies)if(model[0].wins)return true;return false;
}
bool App::act_discovered(unsigned region)const{return region<4&&(progress.discovered_acts&(1u<<region));}
const UiEnemyStatistics& App::enemy_statistics(unsigned model,int character)const{
    static const UiEnemyStatistics none{};
    return model<EnemyKinds&&character>=-1&&character<CharacterKinds?progress.enemies[model][character+1]:none;
}
const UiEnemyStatistics& App::encounter_statistics(unsigned encounter,int character)const{
    static const UiEnemyStatistics none{};
    return encounter<EncounterKinds&&character>=-1&&character<CharacterKinds?progress.encounters[encounter][character+1]:none;
}
const UiRunHistory* App::history_at(unsigned newest_first)const{
    return newest_first<progress.count?&progress.history[(progress.next_history+UiHistoryCapacity-1-newest_first)%UiHistoryCapacity]:nullptr;
}
const UiFloorHistory* App::current_floor_history(unsigned act,unsigned point)const{
    const auto&r=progress.current;if(!progress_matches(r,game))return nullptr;
    for(unsigned i=0;i<r.floor_count;++i)if(r.floors[i].act==act&&r.floors[i].point==point)return &r.floors[i];return nullptr;
}
