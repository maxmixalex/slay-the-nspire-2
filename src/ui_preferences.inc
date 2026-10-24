void App::restore_preferences(){
    UiPreferences value{};
    if(callbacks.load_preferences){UiPreferences loaded{};
        if(callbacks.load_preferences(loaded))value=loaded;
    }
    fast_mode=value.fast_mode;
    show_card_indices=value.show_card_indices;text_effects=value.text_effects;
    keyboard_only=value.keyboard_only;phobia_mode=value.phobia_mode;screen_shake=value.screen_shake;bestiary_actions_preferred=value.bestiary_actions_preferred;
    input_bindings=value.bindings_saved?value.bindings:default_input_bindings(calculator_keyboard);
    binding_listening=-1;physical_keys_cancel();
}
void App::persist_preferences(){
    UiPreferences value{fast_mode,show_card_indices,text_effects,keyboard_only,phobia_mode,screen_shake,bestiary_actions_preferred};
    value.bindings=input_bindings;value.bindings_saved=true;
    if(callbacks.save_preferences&&!callbacks.save_preferences(value))
        message(content.english()?"Could not save settings.":"设置保存失败，请重试");
}
void App::set_fast_mode(bool value){
    if(fast_mode==value)return;
    fast_mode=value;
    persist_preferences();
}
unsigned App::available_languages()const{
    return callbacks.available_languages?callbacks.available_languages()&3u:(content.english()?1u:2u);
}
bool App::request_language(unsigned language){
    // NSettingsScreen disables language changes during a run; the original
    // dropdown relocalizes/reopens the main-menu settings without a popup.
    if(!title||!settings_open||(language!=1&&language!=2)||
       !(available_languages()&(1u<<(language-1))))return false;
    settings_language_open=false;
    if(language==(content.english()?1u:2u))return true;
    if(callbacks.request_language&&callbacks.request_language(language))return true;
    message(content.english()?"Could not change language.":"无法切换语言。");return false;
}
bool App::reload_language(ResourceSource source){
    if(!title)return false;
    if(!init(source,callbacks))return false;
    reset_presentation();title=true;paused=library=character_select=help=false;
    settings_open=true;settings_language_open=false;settings_tab=settings_item=settings_top=0;
    frontend_confirm=frontend_choice=0;notice=nullptr;notice_ticks=0;
    map_preview.open=run_settings=run_seed_editing=false;return true;
}
void App::settings_action(UiAction action){
    if(!settings_open)return;
    if(binding_listening>=0){if(action==UiAction::SettingsBack||action==UiAction::Back)binding_listening=-1;return;}
    if(settings_language_open){
        if(action==UiAction::SettingsLanguageEnglish)request_language(1);
        else if(action==UiAction::SettingsLanguageChinese)request_language(2);
        else if(action==UiAction::SettingsLanguageClose||action==UiAction::Back||action==UiAction::SettingsBack)settings_language_open=false;
        return;
    }
    switch(action){
    case UiAction::SettingsBack:case UiAction::Back:settings_open=false;binding_listening=-1;break;
    case UiAction::SettingsGeneralTab:case UiAction::SettingsInputTab:
        settings_tab=action==UiAction::SettingsInputTab;settings_item=settings_top=0;break;
    case UiAction::SettingsLanguage:
        if(title&&available_languages()==3){settings_language_open=true;settings_language_item=content.english()?0:1;}break;
    case UiAction::SettingsFastMode:fast_mode=!fast_mode;persist_preferences();break;
    case UiAction::SettingsCardIndices:show_card_indices=!show_card_indices;persist_preferences();break;
    case UiAction::SettingsTextEffects:text_effects=!text_effects;invalidate_hand_tiles();persist_preferences();break;
    case UiAction::SettingsKeyboardOnly:keyboard_only=!keyboard_only;if(!keyboard_only)pointer.visible=true;persist_preferences();break;
    case UiAction::SettingsPhobia:phobia_mode=!phobia_mode;persist_preferences();break;
    case UiAction::SettingsShakeDown:if(screen_shake){--screen_shake;persist_preferences();}break;
    case UiAction::SettingsShakeUp:if(screen_shake<4){++screen_shake;persist_preferences();}break;
    case UiAction::SettingsReset:if(settings_tab)reset_input_bindings();else{frontend_confirm=4;frontend_choice=0;}break;
    case UiAction::SettingsControls:help=true;break;
    case UiAction::SettingsFeedback:open_auxiliary(AuxiliaryScreen::Feedback);break;
    case UiAction::SettingsCredits:open_auxiliary(AuxiliaryScreen::Credits);break;
    case UiAction::SettingsScrollUp:if(settings_top)settings_top--;break;
    case UiAction::SettingsScrollDown:if(settings_top<(settings_tab?InputCommandCount-5:3))settings_top++;break;
    default:break;
    }
}
bool App::settings_input(Key key){
    if(!settings_open)return false;
    if(binding_listening>=0){if(key==Key::Back&&pointer.dispatching)binding_listening=-1;return true;}
    if(settings_language_open){
        if(key==Key::Back)settings_language_open=false;
        else if(key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right)settings_language_item^=1;
        else if(key==Key::Confirm)request_language(settings_language_item+1);
        return true;
    }
    const unsigned count=settings_tab?InputCommandCount+3u:10u; // Mode/rows, Reset, Back.
    if(key==Key::Back)settings_action(UiAction::SettingsBack);
    else if(key==Key::Inspect){settings_tab^=1;settings_item=settings_top=0;}
    else if(key==Key::Up||key==Key::Down){
        settings_item=uint8_t((settings_item+count+(key==Key::Up?-1:1))%count);
        if(!settings_tab&&settings_item<8){if(settings_item<settings_top)settings_top=settings_item;else if(settings_item>=settings_top+5)settings_top=settings_item-4;}
        if(settings_tab&&settings_item>=1&&settings_item<=InputCommandCount){const unsigned row=settings_item-1;if(row<settings_top)settings_top=row;else if(row>=settings_top+5)settings_top=row-4;}
    }else if(key==Key::Left||key==Key::Right){
        if(!settings_tab&&settings_item==1)settings_action(key==Key::Left?UiAction::SettingsShakeDown:UiAction::SettingsShakeUp);
        else{settings_tab^=1;settings_item=settings_top=0;}
    }else if(key==Key::Confirm){
        static const UiAction general[]={UiAction::SettingsLanguage,UiAction::SettingsShakeUp,UiAction::SettingsFastMode,
            UiAction::SettingsPhobia,UiAction::SettingsCardIndices,UiAction::SettingsTextEffects,
            UiAction::SettingsFeedback,UiAction::SettingsCredits,UiAction::SettingsReset,UiAction::SettingsBack};
        if(!settings_tab)settings_action(general[settings_item%10]);
        else if(!settings_item)settings_action(UiAction::SettingsKeyboardOnly);
        else if(settings_item<=InputCommandCount)begin_key_rebind(settings_item-1);
        else settings_action(settings_item==InputCommandCount+1?UiAction::SettingsReset:UiAction::SettingsBack);
    }
    return true;
}
