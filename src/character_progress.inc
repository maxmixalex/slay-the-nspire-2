void App::prepare_profile_run(State&s){
    restore_progress();s.world.unlocks=TimelineUnlocks{};
    s.world.first_run=uint8_t(s.run_options.mode==RunMode::Standard&&first_standard_run());
}
void App::finish_character_progress(const State&){
    const auto&r=progress.current;
    if(r.mode!=unsigned(RunMode::Standard)||!r.outcome)return;
    unsigned mask=profile_character_unlocks(progress)|1u|(1u<<unsigned(Character::Silent));
    // Pinned characters.*.unlockText: play one run with the predecessor.
    // Save/return/Continue never enters this settlement boundary.
    if(r.character==unsigned(Character::Silent))mask|=1u<<unsigned(Character::Regent);
    if(r.character==unsigned(Character::Regent))mask|=1u<<unsigned(Character::Necrobinder);
    if(r.character==unsigned(Character::Necrobinder))mask|=1u<<unsigned(Character::Defect);
    progress.character_unlocks=uint8_t(mask);
}
