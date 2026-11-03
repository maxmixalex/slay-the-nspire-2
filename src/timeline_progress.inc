// Original ProgressSaveManager boundaries; collection discovery is independent.
void App::timeline_prepare_run(State&s){
    restore_progress();s.world.unlocks=s.run_options.mode==RunMode::Daily?TimelineUnlocks{}:timeline_unlocks(progress.timeline);
    uint32_t runs=0;for(const auto&c:progress.characters)runs=progress_add(runs,progress_add(c.wins,c.losses));
    s.world.first_run=uint8_t(s.run_options.mode==RunMode::Standard&&!runs&&(progress.timeline.flags&TimelineInitialized)&&!(progress.timeline.flags&TimelineLegacyAccess));
}
void App::timeline_win_battle(const State&s){
    if(s.run_options.mode!=RunMode::Standard)return;
    const auto&r=progress.current;const unsigned character=unsigned(s.character);
    const uint32_t now=callbacks.wall_clock?callbacks.wall_clock():0;
    if(r.facts.last_battle_room==unsigned(WorldRoom::Boss))
        timeline_obtain(progress.timeline,timeline_character_epoch(character,s.world.act_index+2),now);
    if(r.facts.last_battle_room!=unsigned(WorldRoom::Boss)&&r.facts.last_battle_room!=unsigned(WorldRoom::Elite))return;
    unsigned elites=0,bosses=0;
    for(unsigned id=0;id<EncounterKinds;++id){const auto*d=encounter_definition(EncounterId(id));if(!d||d->act>3)continue;
        const auto wins=progress.encounters[id][character+1].wins;
        if(d->type==EncounterClass::Elite)elites=progress_add(elites,wins);
        else if(d->type==EncounterClass::Boss)bosses=progress_add(bosses,wins);
    }
    if(elites>=15)timeline_obtain(progress.timeline,timeline_character_epoch(character,5),now);
    if(bosses>=15)timeline_obtain(progress.timeline,timeline_character_epoch(character,6),now);
}
void App::timeline_finish_run(const State&s){
    auto&t=progress.timeline;const auto&r=progress.current;
    t.characters_known|=progress.characters_started|uint8_t(1u<<r.character);
    if(r.mode!=unsigned(RunMode::Standard))return;
    static constexpr OriginalEventId ancients[]={OriginalEventId::Neow,OriginalEventId::Darv,OriginalEventId::Nonupeipe,OriginalEventId::Tanx,OriginalEventId::Orobas,OriginalEventId::Pael,OriginalEventId::Tezcatara,OriginalEventId::Vakuu};
    for(unsigned a=0;a<WorldActCount;++a){const auto&map=s.world.maps[a];
        if(s.world.map_ready[a]&&map.root<map.count&&s.world.visited[a][map.root])
            for(unsigned i=0;i<8;++i)if(s.world.rooms[a].ancient==unsigned(ancients[i]))t.ancients_seen|=uint8_t(1u<<i);
    }
    const uint32_t now=callbacks.wall_clock?callbacks.wall_clock():0;
    timeline_obtain(t,Epoch::Neow,now);
    const Epoch next=r.character==unsigned(Character::Silent)?Epoch::Regent1:r.character==unsigned(Character::Regent)?Epoch::Necrobinder1:r.character==unsigned(Character::Necrobinder)?Epoch::Defect1:Epoch::Count;
    timeline_obtain(t,next,now);
    if(r.outcome==1){
        if(r.ascension==1)timeline_obtain(t,timeline_character_epoch(r.character,7),now);
        uint32_t wins=0;for(const auto&c:progress.characters)wins=progress_add(wins,c.wins);
        if(wins>=3)timeline_obtain(t,Epoch::CustomAndSeeds,now);
        if(t.characters_known==31)timeline_obtain(t,Epoch::DailyRun,now);
    }
    if(t.characters_known==31)timeline_obtain(t,Epoch::Orobas,now);
    if((t.ancients_seen&0xfdu)==0xfdu)timeline_obtain(t,Epoch::Darv,now);
    // A partial legacy run has no reliable historical score. Future complete
    // runs still earn progress normally; no old score or timestamp is invented.
    if(ui_run_facts_complete(r))timeline_add_score(t,ui_run_score(r).total,now);
}
