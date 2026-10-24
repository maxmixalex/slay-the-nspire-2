void App::restore_preferences(){
    UiPreferences value{};
    if(callbacks.load_preferences){UiPreferences loaded{};
        if(callbacks.load_preferences(loaded))value=loaded;
    }
    fast_mode=value.fast_mode;
}
void App::set_fast_mode(bool value){
    if(fast_mode==value)return;
    fast_mode=value;
    if(callbacks.save_preferences&&!callbacks.save_preferences({fast_mode}))
        message(content.english()?"Could not save settings.":"设置保存失败，请重试");
}
