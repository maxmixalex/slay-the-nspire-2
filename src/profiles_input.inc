unsigned App::current_profile()const{
    const unsigned value=callbacks.current_profile?callbacks.current_profile():1;
    return value>=1&&value<=UiProfileCount?value:1;
}
void App::refresh_profiles(){
    for(unsigned i=0;i<UiProfileCount;++i){profile_summaries[i]={};
        if(callbacks.profile_info)callbacks.profile_info(i+1,profile_summaries[i]);
        if(i+1==current_profile()&&progress_loaded&&(progress.next_serial>1||progress.characters_started||progress.last_saved_at)){
            auto&summary=profile_summaries[i];summary.exists=true;summary.last_saved_at=progress.last_saved_at;summary.play_seconds=0;
            for(const auto&character:progress.characters)summary.play_seconds=progress_add(summary.play_seconds,character.play_seconds);
        }
    }
}
void App::open_profiles(){
    if(!title||!callbacks.request_profile||!callbacks.profile_info||profile_pending)return;
    if(!persist_progress()){message(content.english()?"Could not save history.":"历史记录保存失败，请重试");return;}
    profiles_open=true;profile_selection=uint8_t(current_profile()-1);profiles_started=map_clock_ms;
    compendium_open=statistics_open=history_open=library=character_select=settings_open=false;bestiary.open=false;
    detail_card=detail_item=-1;pointer_cancel();refresh_profiles();
    for(auto&motion:profile_motion)motion={};
    for(auto&label:profile_labels)label={};
}
bool App::request_profile_change(unsigned target,bool erase){
    if(!title||!profiles_open||profile_pending||target<1||target>UiProfileCount)return false;
    if(!erase&&target==current_profile()){profiles_open=false;pointer_cancel();return true;}
    if(!persist_progress())return false;
    if(!callbacks.request_profile||!callbacks.request_profile(target,erase)){
        message(content.english()?"Could not change save profile.":"无法更换存档档案，请重试");return false;
    }
    profile_target=uint8_t(target);profile_delete=erase;profile_pending=true;pointer_cancel();return true;
}
void App::finish_profile_change(bool success){
    if(!profile_pending)return;
    const bool erased=profile_delete,active=profile_target==current_profile();profile_pending=false;
    if(!success){message(content.english()?"Could not update save profile.":"无法更新存档档案，请重试");refresh_profiles();return;}
    if(!erased||active){
        // All old live-model references belong to the previous profile.
        // A profile switch returns to the menu; Continue is a separate action.
        progress_loaded=false;restore_progress();restore_preferences();forget_run_checkpoint();
        reset_in_place(game);reset_presentation();refresh_save();menu=0;
        compendium_open=statistics_open=history_open=library=character_select=settings_open=false;bestiary.open=false;
        paused=help=false;detail_card=detail_item=-1;map_preview.open=false;
    }
    profiles_open=erased;profile_selection=uint8_t(current_profile()-1);refresh_profiles();pointer_cancel();
}
bool App::profiles_input(Key key){
    if(!profiles_open)return false;if(profile_pending)return true;
    if(key==Key::Back){profiles_open=false;pointer_cancel();return true;}
    const unsigned column=profile_selection%UiProfileCount,row=profile_selection/UiProfileCount;
    if(key==Key::Left||key==Key::Right){
        profile_selection=uint8_t(row*UiProfileCount+(column+UiProfileCount+(key==Key::Left?-1:1))%UiProfileCount);
    }else if(key==Key::Up)profile_selection=uint8_t(column);
    else if(key==Key::Down&&profile_summaries[column].exists)profile_selection=uint8_t(column+UiProfileCount);
    else if(key==Key::Confirm){
        if(!row)request_profile_change(column+1);
        else if(profile_summaries[column].exists){profile_target=uint8_t(column+1);frontend_confirm=5;frontend_choice=0;pointer_cancel();}
    }else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
    return true;
}
