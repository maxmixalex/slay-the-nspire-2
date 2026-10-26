static uint32_t progress_add(uint32_t value,uint32_t amount){return amount>UINT32_MAX-value?UINT32_MAX:value+amount;}
static void progress_clear_run(UiRunHistory&r){auto*bytes=reinterpret_cast<uint8_t*>(&r);for(unsigned i=0;i<sizeof r;++i)bytes[i]=0;}
static bool progress_matches(const UiRunHistory&r,const State&s){
    return r.serial&&s.world.active&&!s.practice&&r.seed==s.seed&&r.run_seed==s.world.run_seed&&
        r.character==unsigned(s.character)&&r.ascension==s.world.ascension&&r.start_region==s.world.rooms[0].region&&
        r.mode==unsigned(s.run_options.mode)&&r.modifiers==s.run_options.modifiers&&r.daily_day==s.run_options.daily_day;
}
int App::bind_history_journal(){
 uint32_t live[UiHistoryCapacity+1]{};unsigned n=0;if(progress.current.serial)live[n++]=progress.current.serial;
 for(unsigned i=0;i<progress.count;++i)if(progress.history[i].serial)live[n++]=progress.history[i].serial;
 return progress.journal.bind(progress.current.serial,live,n);
}
bool App::run_timer_visible()const{
    // NRunTimer observes the map and capstone visibility even when the
    // preference is off. The preference requests the same label everywhere.
    return !title&&game.world.active&&!game.practice&&
        (show_run_timer||game.phase==Phase::Map||map_window.visible||panel==1||panel==3||paused||settings_open);
}
void App::run_timer_label(char*out,size_t capacity)const{
    TextBuffer text{out,capacity,0};const auto&r=progress.current;
    if(!progress_matches(r,game)||r.partial){text.text("—");return;}
    const unsigned hours=r.seconds/3600,minutes=r.seconds/60%60;
    if(hours){text.number(hours);text.ch(':');}
    if(minutes<10)text.ch('0');text.number(minutes);text.ch(':');
    if(r.seconds%60<10)text.ch('0');text.number(r.seconds%60);
}
static UiFloorHistory* progress_observe_floor(App&a,const State&s){
    auto&r=a.progress.current;if(!progress_matches(r,s)||r.outcome||!s.world.room_open)return nullptr;
    if(r.floor_count){auto&last=r.floors[r.floor_count-1];if(last.act==s.world.act_index&&last.point==s.world.current_point)return &last;}
    if(r.floor_count==UiHistoryFloors){r.partial=1;r.facts.complete=0;return nullptr;}
    const bool complete=r.floor_count||!r.partial;
    auto&f=r.floors[r.floor_count++];f={};f.floor=s.world.total_floor;f.act=s.world.act_index;
    f.region=s.world.rooms[s.world.act_index].region;f.point=s.world.current_point;f.room=s.world.event_combat_parent?s.world.parent_room_type:s.world.room_type;
    f.hp=uint16_t(mx(0,s.hp));f.max_hp=uint16_t(mx(0,s.max_hp));f.gold=uint16_t(mx(0,s.gold));f.entered_seconds=r.seconds;
    // The canonical retained room still carries the preceding combat's
    // enemies and turn. A floor owns an encounter only after its real spawn.
    f.encounter=0;f.event=s.original_events.active?unsigned(s.original_events.id):0;
    if(f.point<s.world.maps[f.act].count)f.map_type=uint8_t(s.world.maps[f.act].points[f.point].type);
    f.facts=complete?1:0;r.facts.last_room=f.room;
    if(const int owner=a.bind_history_journal();owner>=0)a.progress.journal.owners[owner].observe(r.floor_count-1,complete);
    a.progress_dirty=true;return &f;
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
    const bool loaded=callbacks.load_progress&&callbacks.load_progress(progress);
    if(!loaded)clear_ui_progress(progress);
    // A browser profile_info callback is asynchronously cached. Reuse the
    // ordinary current-run read below instead of scanning profiles at boot.
    // Existing metadata, including NSH1-7, always retains its decoded flags.
    ftue_profile_pending=!loaded&&callbacks.load_progress&&callbacks.save_progress&&
        callbacks.current_profile&&callbacks.profile_info&&callbacks.load;
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
bool App::seen_ftue(FtueId id)const{
    const uint16_t bit=ftue_bit(id);return !bit||!(progress.tutorial_flags&FtueEnabled)||bool(progress.tutorial_flags&bit);
}
bool App::seen_popup(FtueId id)const{
    const uint16_t bit=ftue_bit(id);
    // Old profiles did not observe FTUE completion. Preserve their previous
    // unobstructed UI without manufacturing seen bits. Fresh profiles use
    // the source popup rule, which ignores the ordinary tutorial preference.
    return !bit||!(progress.tutorial_flags&FtueInitialized)||bool(progress.tutorial_flags&bit);
}
void App::mark_ftue(FtueId id){
    const uint16_t bit=ftue_bit(id);if(!bit)return;restore_progress();
    // Source auto-completion may run without displaying a modal (for
    // example, proceeding after collecting late-floor rewards). It must
    // not turn an unknown legacy profile into a newly initialized one or
    // accidentally make its unseen Ascension popup eligible.
    if(!(progress.tutorial_flags&FtueInitialized))return;
    const uint16_t flags=progress.tutorial_flags|bit;
    if(flags==progress.tutorial_flags)return;
    progress.tutorial_flags=flags;ftue_profile_pending=false;progress_dirty=true;persist_progress();
}
void App::set_ftues_enabled(bool enabled){
    restore_progress();const uint16_t flags=uint16_t((progress.tutorial_flags|FtueInitialized)&~FtueEnabled)|(enabled?FtueEnabled:0);
    if(flags==progress.tutorial_flags)return;
    progress.tutorial_flags=flags;ftue_profile_pending=false;progress_dirty=true;persist_progress();
}
void App::reset_ftues(){
    restore_progress();progress.tutorial_flags=FtueFresh;ftue_profile_pending=false;progress_dirty=true;persist_progress();
}
void App::begin_progress(bool resumed){
    if(!game.world.active||game.practice)return;
    restore_progress();progress.characters_started|=uint8_t(1u<<unsigned(game.character));
    if(resumed&&progress_matches(progress.current,game)){
        // NSH1–5 did not observe these facts. Continue collecting forward,
        // while the old prefix stays explicitly unknown for score/badges.
        if(!progress.current.facts.version){progress.current.facts.version=1;progress.current.facts.complete=0;}
        progress_seen_phase=255;progress_seen_revision=0;sync_progress();return;
    }
    auto&r=progress.current;progress_clear_run(r);
    progress.current_saved_at=progress.current_saved_revision=0;
    if(!resumed)for(unsigned i=0;i<sizeof UiCurrentBuildId;++i)r.build_id[i]=UiCurrentBuildId[i];
    progress.current_discoveries={};r.facts.version=1;r.facts.complete=resumed?0:1;
    progress.map_drawing.count=0;map_drawing.cancel();
    r.serial=progress.next_serial++;if(!progress.next_serial)progress.next_serial=1;
    r.seed=game.seed;r.run_seed=game.world.run_seed;r.character=unsigned(game.character);r.ascension=game.world.ascension;
    r.mode=unsigned(game.run_options.mode);r.modifiers=game.run_options.modifiers;r.daily_day=game.run_options.daily_day;r.modifier_order=game.run_options.setup_flags&2;
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
    const uint8_t bit=uint8_t(1u<<(id%8));if(!(progress.discovered_potions[id/8]&bit)){progress.discovered_potions[id/8]|=bit;progress_dirty=true;
        if(progress_matches(progress.current,game)&&!progress.current.outcome)progress.current_discoveries.add(2,id);}
}
void App::discover_card(CardKind kind){
    const unsigned id=unsigned(kind);if(id>=CardKinds)return;const uint8_t bit=uint8_t(1u<<(id%8));
    if(!(progress.discovered_cards[id/8]&bit)){
        progress.discovered_cards[id/8]|=bit;progress_dirty=true;
        if(progress_matches(progress.current,game)&&!progress.current.outcome)progress.current_discoveries.add(0,id);
        // NHoverTipSet can discover a generated card while its parent's
        // grid holder is being pressed. NCardLibrary rebuilds on filter/search
        // actions, not on this observation: rebuilding here moves the grid
        // under the pointer and changes the captured model before release.
    }
}
void App::discover_relic(RelicId relic){
    const unsigned id=unsigned(relic);if(id>=WorldRelicKinds)return;const uint8_t bit=uint8_t(1u<<(id%8));
    if(!(progress.discovered_relics[id/8]&bit)){progress.discovered_relics[id/8]|=bit;progress_dirty=true;
        if(progress_matches(progress.current,game)&&!progress.current.outcome)progress.current_discoveries.add(1,id);}
}
void App::observe_progress_log(Log log){
    if(!progress_matches(progress.current,game)||progress.current.outcome)return;
    if(log.kind==LogKind::Heal&&log.a>0){observe_history(game,{RunHistoryEvent::Healed,uint16_t(log.a),uint16_t(unsigned(log.a)>>16),0,nullptr});return;}
    if(log.kind!=LogKind::Gold||log.a<=0)return;
    auto&r=progress.current;r.gold_gained=progress_add(r.gold_gained,unsigned(log.a));
    if(auto*f=progress_observe_floor(*this,game))f->gold_gained=progress_add(f->gold_gained,unsigned(log.a));
    progress_dirty=true;
}
void App::observe_progress_effect(const State&s){
    auto&r=progress.current;if(s.regent.cards_finished_turn>=20&&progress_matches(r,s)&&!r.outcome&&!r.facts.combo){r.facts.combo=1;progress_dirty=true;}
}
void App::observe_progress_damage(const State&s,const CoreDamageResult&damage){
    if(damage.receiver!=-1||damage.unblocked<=0)return;
    if(auto*f=progress_observe_floor(*this,s)){
        if((f->facts&3)==1){const unsigned previous=f->facts>>2;auto&perfect=progress.current.facts.perfect_bosses;
            perfect=uint16_t(perfect>=previous?perfect-previous:0);}
        f->facts|=2;progress_dirty=true;
        observe_history(s,{RunHistoryEvent::DamageTaken,uint16_t(damage.unblocked),uint16_t(unsigned(damage.unblocked)>>16),0,nullptr});
    }
}
void App::observe_run_fact(const State&s,unsigned kind,unsigned value){
    if(kind==unsigned(CoreRunFact::ArchitectAttack)){if(&s==&game)ui_architect_screen_effect(*this,value!=0);return;}
    if(kind==unsigned(CoreRunFact::DoomActivated)){
        // Only the original command knows whether Doom actually executes;
        // a large counter or later HP delta cannot stand in for this cue.
        if(&s==&game&&value<=unsigned(s.enemy_count))observe_doom(value);
        return;
    }
    auto&r=progress.current;if(!progress_matches(r,s)||r.outcome)return;
    if(kind==unsigned(CoreRunFact::ShopGoldSpent)){r.facts.shop_spent=progress_add(r.facts.shop_spent,value);observe_history(s,{RunHistoryEvent::GoldSpent,uint16_t(value),uint16_t(value>>16),0,nullptr});}
    else if(kind==unsigned(CoreRunFact::RestChoice)&&value<8){if(auto*f=progress_observe_floor(*this,s))f->rest_choices|=uint8_t(1u<<value);observe_history(s,{RunHistoryEvent::RestChoice,uint16_t(value),0,0,nullptr});}
    else return;progress_dirty=true;
}
void App::observe_history(const State&s,const RunHistoryNotice&notice){
 if(&s!=&game||!progress_matches(progress.current,s)||progress.current.outcome||notice.kind==RunHistoryEvent::None||notice.kind>=RunHistoryEvent::Count)return;
 auto*f=progress_observe_floor(*this,s);if(!f)return;auto&journal=progress.journal;const int owner=bind_history_journal();if(owner<0)return;
 const unsigned floor=unsigned(f-progress.current.floors);auto&observed=journal.owners[owner];
 // A legacy NSH1–6 floor may already exist before this observer is installed.
 // Its earlier actions remain unknown even as later actions are collected.
 if(!observed.observed(floor))observed.observe(floor,false);
 unsigned model=notice.model,flags=notice.flags;
 if(notice.kind==RunHistoryEvent::EventChoice){
  const int text=notice.key?content.world_string_id(flags&128?"relics":"events",notice.key):-1;flags&=127;
  if(text<0){observed.lose(floor);progress_dirty=true;return;}model=unsigned(text);
 }
 if(notice.kind==RunHistoryEvent::CardTransformed&&notice.flags==2){
  // The canonical batch queue predates this journal and retains only the
  // original kind. Keep the original upgraded token in its observed pending
  // row, including across NSH7 saves, until the actual replacement is added.
  int match=-1;
  for(unsigned i=0;i<journal.count;++i){const auto&e=journal.at(i);
   if(e.owner!=owner||e.floor!=floor||e.kind!=RunHistoryEvent::CardTransformed||e.flags!=1||card_from_token(e.model).kind!=notice.model||card_from_token(e.detail).kind!=card_from_token(notice.detail).kind)continue;
   if(match<0)match=int(i);if(e.detail==notice.detail){match=int(i);break;}
  }
  if(match>=0){auto&e=journal.at(unsigned(match));e.detail=notice.detail;e.flags=0;}
  else{observed.lose(floor);journal.append(unsigned(owner),floor,notice.kind,unsigned(card_token({CardKind(notice.model),0})),notice.detail,4);}
  progress_dirty=true;return;
 }
 if(notice.kind==RunHistoryEvent::FurCoat)for(unsigned i=0;i<journal.count;++i){const auto&e=journal.at(i);if(e.owner==owner&&e.floor==floor&&e.kind==notice.kind)return;}
 if(notice.kind>=RunHistoryEvent::GoldSpent&&notice.kind<=RunHistoryEvent::MaxHpLost)
  journal.amount(unsigned(owner),floor,notice.kind,uint32_t(model)|(uint32_t(notice.detail)<<16));
 else journal.append(unsigned(owner),floor,notice.kind,model,notice.detail,flags);
 progress_dirty=true;
}
static void progress_begin_battle(App&a,const State&s){
    auto&r=a.progress.current;if(!world_room_is_combat(s)||r.last_battle==s.world.combat_count)return;
    r.last_battle=s.world.combat_count;r.last_enemy_identity=0;
    auto*floor=progress_observe_floor(a,s);r.facts.last_room=r.facts.last_battle_room=s.world.room_type;
    if(floor){const int owner=a.progress.journal.owner(r.serial);
        if(!floor->encounter&&owner>=0&&!a.progress.journal.owners[owner].incomplete(unsigned(floor-r.floors)))floor->encounter=uint16_t(s.enemy_world.encounter);
        a.observe_history(s,{RunHistoryEvent::CombatStarted,uint16_t(s.enemy_world.encounter),s.world.combat_count,s.world.room_type,nullptr});}
    if(s.world.room_type==unsigned(WorldRoom::Elite)&&r.facts.elite_rooms<UINT16_MAX)++r.facts.elite_rooms;
    if(s.world.room_type==unsigned(WorldRoom::Boss)&&r.facts.boss_rooms<UINT16_MAX)++r.facts.boss_rooms;
    for(auto&v:r.battle_seen)v=0;
    const unsigned encounter=unsigned(s.enemy_world.encounter),dimensions[2]={0,unsigned(s.character)+1};
    if(encounter<EncounterKinds)for(unsigned index:dimensions){
        auto&stat=a.progress.encounters[encounter][index];stat.seen=progress_add(stat.seen,1);
    }
    a.progress_dirty=true;
}
void App::observe_enemy_spawned(const State&s,unsigned model,uint16_t identity){
    auto&r=progress.current;
    if(&s!=&game||!progress_matches(r,s)||r.outcome||!s.enemy_world.active||!s.world.combat_count||!world_room_is_combat(s)||model>=EnemyKinds)return;
    progress_begin_battle(*this,s);
    if(!identity||identity<=r.last_enemy_identity)return;
    r.last_enemy_identity=identity;
    if(!progress.enemies[model][0].seen)progress.current_discoveries.add(3,model);
    if(r.battle_seen[model]<UINT16_MAX)++r.battle_seen[model];
    const unsigned dimensions[2]={0,unsigned(s.character)+1};
    for(unsigned index:dimensions){auto&stat=progress.enemies[model][index];stat.seen=progress_add(stat.seen,1);}
    progress_dirty=true;
}
static bool progress_end_battle(App&a,const State&s,bool won){
    auto&r=a.progress.current;
    if(!r.last_battle||r.finished_battle==r.last_battle)return false;
    if(won&&r.facts.last_battle_room==unsigned(WorldRoom::Boss)&&r.floor_count){auto&point=r.floors[r.floor_count-1];
        if((point.facts&3)==1&&r.facts.perfect_bosses<UINT16_MAX)++r.facts.perfect_bosses;
        if((point.facts>>2)<63)point.facts=uint8_t(point.facts+4);
    }
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
    bool changed_room=progress_seen_floor!=game.world.total_floor;
    progress_seen_revision=game.revision;progress_seen_phase=unsigned(game.phase);
    progress_seen_floor=game.world.total_floor;progress_seen_battle=game.world.combat_count;
    progress_seen_ending=game.world.combat_ending;
    auto&r=progress.current;
    progress_observe_floor(*this,game);
    const unsigned region=game.world.rooms[game.world.act_index].region;
    if(region<4)progress.discovered_acts|=uint8_t(1u<<region);
    for(unsigned i=0;i<game.deck_count;++i)discover_card(game.deck[i].kind);
    for(unsigned i=0;i<potion_slot_count(game);++i)discover_potion(potion_at(game,i));
    if(game.phase==Phase::Shop){for(auto potion:game.shop_potions)discover_potion(Potion(potion));for(auto relic:game.world.shop_relics)discover_relic(relic);}
    if(game.phase==Phase::Reward&&!game.world.combat_ending){discover_potion(game.reward_potion);discover_relic(game.world.reward_relic);}
    if(game.original_events.active&&unsigned(game.original_events.id)&&unsigned(game.original_events.id)<=OriginalEventKinds){const unsigned id=unsigned(game.original_events.id)-1;progress.discovered_events[id/8]|=uint8_t(1u<<(id%8));}
    for(unsigned i=0;i<game.world_relics.count;++i)discover_relic(game.world_relics.entries[i].id);
    if(game.world.combat_count&&game.enemy_world.active)progress_begin_battle(*this,game);
    if(r.floor_count){auto&f=r.floors[r.floor_count-1];
        if(f.act==game.world.act_index&&f.point==game.world.current_point){
            f.hp=uint16_t(mx(0,game.hp));f.max_hp=uint16_t(mx(0,game.max_hp));f.gold=uint16_t(mx(0,game.gold));
            f.seconds=r.seconds>=f.entered_seconds?r.seconds-f.entered_seconds:0;
            if(world_room_is_combat(game)&&game.enemy_world.active){
                const int owner=progress.journal.owner(r.serial);uint16_t first_battle=0;
                for(unsigned i=progress.journal.count;i;--i){const auto&e=progress.journal.at(i-1);if(e.owner!=owner||e.floor!=r.floor_count-1)break;if(e.kind==RunHistoryEvent::CombatStarted)first_battle=e.detail;}
                if(owner>=0&&!progress.journal.owners[owner].incomplete(r.floor_count-1)&&first_battle==game.world.combat_count)f.turns=uint16_t(mx(0,game.turn));
            }
            if(game.original_events.active)f.event=unsigned(game.original_events.id);
        }
    }
    if(game.world.combat_count&&game.enemy_world.active&&world_room_is_combat(game)){
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
    // This precedes the original Standard/Custom/Daily statistics filter.
    // The outcome guard above makes persistence retries and repeated finish
    // notifications idempotent, including an abandoned mid-respawn run.
    progress.test_subject_kills=progress_add(progress.test_subject_kills,s.test_subject_kills);
    if(r.outcome==1&&ui_run_facts_complete(r))progress.architect_damage=progress_add(progress.architect_damage,ui_run_score(r).total);
    auto&stats=progress.characters[r.character];
    if(r.mode==unsigned(RunMode::Standard)&&r.outcome==1){stats.wins=progress_add(stats.wins,1);stats.current_streak=progress_add(stats.current_streak,1);
        // ProgressSaveManager counts distinct ModelIds in the final deck,
        // once per victory, independently from copy count or upgrade level.
        uint8_t unique[(CardKinds+7)/8]{};
        for(unsigned i=0;i<r.deck_count;++i){const unsigned id=r.deck[i].card.kind;if(id<CardKinds&&!(unique[id/8]&(1u<<(id%8)))){
            unique[id/8]|=uint8_t(1u<<(id%8));progress.card_wins[id]=progress_add(progress.card_wins[id],1);}}
        if(stats.current_streak>stats.best_streak)stats.best_streak=stats.current_streak;
        if(r.ascension==stats.highest_ascension&&stats.highest_ascension<10)++stats.highest_ascension;
        progress.current_streak=progress_add(progress.current_streak,1);if(progress.current_streak>progress.best_streak)progress.best_streak=progress.current_streak;
        if(!r.partial&&(!stats.fastest_win_seconds||r.seconds<stats.fastest_win_seconds))stats.fastest_win_seconds=r.seconds;
    }else if(r.mode==unsigned(RunMode::Standard)){stats.losses=progress_add(stats.losses,1);stats.current_streak=0;progress.current_streak=0;}
    stats.play_seconds=progress_add(stats.play_seconds,r.seconds);stats.gold_gained=progress_add(stats.gold_gained,r.gold_gained);
    stats.floors_climbed=progress_add(stats.floors_climbed,r.floor);
    progress.history[progress.next_history]=r;progress.next_history=uint8_t((progress.next_history+1)%UiHistoryCapacity);
    if(progress.count<UiHistoryCapacity)++progress.count;
    uint32_t live[UiHistoryCapacity+1]{};live[0]=r.serial;
    for(unsigned i=0;i<progress.count;++i)live[i+1]=progress.history[i].serial;
    progress.journal.prune(live,progress.count+1);
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
