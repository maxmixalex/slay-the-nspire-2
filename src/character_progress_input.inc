void App::choose_character(Character value){
    if(unsigned(value)>=CharacterKinds)return;
    // Locked portraits remain inspectable, with their original prerequisite.
    // The embark command is the authority that enforces the playable mask.
    selected_character=value;character_random=false;
    if(character_select&&selected_ascension>character_max_ascension())selected_ascension=uint8_t(character_max_ascension());
}
bool App::character_available(unsigned character)const{
    if(practice_entry)return character<=CharacterKinds;
    const unsigned mask=profile_character_unlocks(progress);
    return character<CharacterKinds?bool(mask&(1u<<character)):character==CharacterKinds&&mask==31;
}
unsigned App::character_max_ascension()const{
    if(practice_entry||(progress.timeline.flags&TimelineLegacyAccess)||(!progress.character_unlocks&&!(progress.timeline.flags&TimelineInitialized)))return 10;
    if(character_random){unsigned n=0;for(const auto&c:progress.characters)if(c.highest_ascension>n)n=c.highest_ascension;return n;}
    return character_available(unsigned(selected_character))?progress.characters[unsigned(selected_character)].highest_ascension:0;
}
bool App::run_mode_available(RunMode mode)const{return unsigned(mode)<=unsigned(RunMode::Daily);}
bool App::first_standard_run()const{
    if((progress.timeline.flags&TimelineLegacyAccess)||(!progress.character_unlocks&&!(progress.timeline.flags&TimelineInitialized)))return false;
    for(const auto&c:progress.characters)if(c.wins||c.losses)return false;
    return true;
}
