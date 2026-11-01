void App::timeline_choose_character(Character value){
    if(!timeline_character_available(unsigned(value)))return;
    selected_character=value;character_random=false;
    if(!practice_entry&&!(progress.timeline.characters_known&(1u<<unsigned(value)))){
        progress.timeline.characters_known|=uint8_t(1u<<unsigned(value));progress_dirty=true;persist_progress();
    }
    if(selected_ascension>timeline_max_ascension())selected_ascension=uint8_t(timeline_max_ascension());
}
bool App::timeline_character_available(unsigned character)const{
    if(practice_entry)return true;
    const auto unlocks=timeline_unlocks(progress.timeline);
    if(character<CharacterKinds)return timeline_character_allowed(unlocks,character);
    for(unsigned i=0;i<CharacterKinds;++i)if(!timeline_character_allowed(unlocks,i))return false;
    return true;
}
unsigned App::timeline_max_ascension()const{
    if(practice_entry||(progress.timeline.flags&TimelineLegacyAccess))return 10;
    if(character_random){unsigned n=0;for(const auto&c:progress.characters)if(c.highest_ascension>n)n=c.highest_ascension;return n;}
    return timeline_unlocks(progress.timeline).has(timeline_character_epoch(unsigned(selected_character),4))?progress.characters[unsigned(selected_character)].highest_ascension:0;
}
bool App::timeline_mode_available(RunMode mode)const{
    return mode==RunMode::Standard||timeline_unlocks(progress.timeline).has(mode==RunMode::Daily?Epoch::DailyRun:Epoch::CustomAndSeeds);
}
void App::open_timeline(){
    if(!title)return;
    restore_progress();timeline_open=true;timeline_detail=false;timeline_selected=unsigned(Epoch::Neow);
    character_select=settings_open=compendium_open=statistics_open=history_open=library=false;run_mode={};
    progress_page_started=map_clock_ms;progress_page_scroll.reset();pointer_cancel();
    for(unsigned i=0;i<EpochCount;++i)if(timeline_revealable(progress.timeline,Epoch(i))){timeline_selected=uint8_t(i);break;}
}
unsigned App::timeline_items(uint8_t*out)const{
    unsigned n=0;
    for(unsigned i=0;i<EpochCount;++i)if(i==unsigned(Epoch::Neow)||progress.timeline.states[i]!=EpochState::None||(progress.timeline.flags&TimelineLegacyAccess)){
        unsigned at=n;while(at){const auto&a=timeline_epochs[out[at-1]],&b=timeline_epochs[i];
            if(a.era<b.era||(a.era==b.era&&a.position<b.position))break;out[at]=out[at-1];--at;}
        out[at]=uint8_t(i);++n;
    }return n;
}
void App::timeline_activate(unsigned id){
    if(!timeline_open||id>=EpochCount)return;
    if(!timeline_detail||timeline_selected!=id){timeline_selected=uint8_t(id);timeline_detail=true;progress_page_scroll.reset();pointer_cancel();return;}
    if(timeline_reveal(progress.timeline,Epoch(id),callbacks.wall_clock?callbacks.wall_clock():0)){
        progress_dirty=true;persist_progress();progress_page_scroll.reset();pointer_cancel();
    }
}
bool App::timeline_input(Key key){
    if(!timeline_open)return false;
    if(key==Key::Back||key==Key::Collection){if(timeline_detail){timeline_detail=false;progress_page_scroll.reset();}else timeline_open=false;pointer_cancel();return true;}
    if(key==Key::Confirm){timeline_activate(timeline_selected);return true;}
    if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;return true;}
    if(timeline_detail&&(key==Key::Up||key==Key::Down)){progress_page_scroll.wheel(key==Key::Up?-3:3);return true;}
    if(key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right){
        uint8_t items[EpochCount];const unsigned n=timeline_items(items);unsigned at=0;
        while(at<n&&items[at]!=timeline_selected)++at;
        const bool back=key==Key::Up||key==Key::Left;
        if(n){at=(at+n+(back?-1:1))%n;timeline_selected=items[at];progress_page_scroll.reset();}
        pointer_cancel();
    }return true;
}
