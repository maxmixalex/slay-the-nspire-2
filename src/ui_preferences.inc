void App::restore_preferences(){
    UiPreferences value{};
    if(callbacks.load_preferences){UiPreferences loaded{};
        if(callbacks.load_preferences(loaded))value=loaded;
    }
    fast_mode=value.fast_mode;long_press=value.long_press;skip_intro_logo=value.skip_intro_logo;end_turn_hold={};
    show_card_indices=value.show_card_indices;text_effects=value.text_effects;show_run_timer=value.show_run_timer;
    keyboard_only=value.keyboard_only;phobia_mode=value.phobia_mode;screen_shake=value.screen_shake;bestiary_actions_preferred=value.bestiary_actions_preferred;
    input_bindings=value.bindings_saved?value.bindings:default_input_bindings(calculator_keyboard);
    // Upgrade only the untouched old calculator defaults. User rebinding is preserved.
    if(calculator_keyboard&&value.bindings_saved){
        auto old=default_input_bindings(false);const uint16_t cards[]={'1','3','7','9','0','Z','X','C','V','B'};
        for(unsigned bank=0;bank<2;++bank){auto&keys=old.keys[bank];keys[2]='M';keys[4]='Q';keys[6]='D';keys[7]='F';keys[10]=' ';
            keys[11]='8';keys[12]='2';keys[13]='4';keys[14]='6';for(unsigned i=0;i<10;++i)keys[15+i]=cards[i];}
        old.keys[1][0]='5';bool same=true;for(unsigned b=0;b<2;++b)for(unsigned i=0;i<InputCommandCount;++i)if(input_bindings.keys[b][i]!=old.keys[b][i])same=false;
        if(same)input_bindings=default_input_bindings(true);
    }
    binding_listening=-1;physical_keys_cancel();
}
void App::persist_preferences(){
    UiPreferences value{fast_mode,show_card_indices,text_effects,keyboard_only,phobia_mode,screen_shake,bestiary_actions_preferred};
    value.bindings=input_bindings;value.bindings_saved=true;value.show_run_timer=show_run_timer;value.long_press=long_press;value.skip_intro_logo=skip_intro_logo;
    if(callbacks.save_preferences&&!callbacks.save_preferences(value))
        message(content.language()!=2?"Could not save settings.":"设置保存失败，请重试");
}
void App::set_fast_mode(bool value){
    if(fast_mode==value)return;
    fast_mode=value;
    persist_preferences();
}
unsigned App::available_languages()const{
    return callbacks.available_languages?callbacks.available_languages()&((1u<<LocaleLanguageCount)-1u):locale_language_mask(content.language());
}
bool App::request_language(unsigned language){
    // NSettingsScreen disables language changes during a run; the original
    // dropdown relocalizes/reopens the main-menu settings without a popup.
    if(!title||!settings_open||!locale_language_valid(language)||
       !(available_languages()&locale_language_mask(language)))return false;
    // NLanguageDropdown ignores the selected locale without dismissing the
    // open menu. Only a real locale change closes/reopens Settings.
    if(language==content.language())return true;
    settings_language_open=false;
    if(callbacks.request_language&&callbacks.request_language(language))return true;
    message(content.language()!=2?"Could not change language.":"无法切换语言。");return false;
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
        if(unsigned(action)>=unsigned(UiAction::SettingsLanguageFirst)&&unsigned(action)<=unsigned(UiAction::SettingsLanguageLast))request_language(unsigned(action)-unsigned(UiAction::SettingsLanguageFirst)+1);
        else if(action==UiAction::SettingsLanguagePrevious){if(settings_language_item)settings_language_item--;settings_language_top=uint8_t(settings_layout::language_reveal(locale_language_count(available_languages()),settings_language_item,settings_language_top));}
        else if(action==UiAction::SettingsLanguageNext){if(settings_language_item+1<locale_language_count(available_languages()))settings_language_item++;settings_language_top=uint8_t(settings_layout::language_reveal(locale_language_count(available_languages()),settings_language_item,settings_language_top));}
        else if(action==UiAction::SettingsScrollUp){if(settings_language_top)--settings_language_top;}
        else if(action==UiAction::SettingsScrollDown){if(settings_language_top<settings_layout::language_maximum(locale_language_count(available_languages())))++settings_language_top;}
        else if(action==UiAction::SettingsLanguageEnglish)request_language(1);
        else if(action==UiAction::SettingsLanguageChinese)request_language(2);
        else if(action==UiAction::SettingsLanguageClose||action==UiAction::Back||action==UiAction::SettingsBack)settings_language_open=false;
        return;
    }
    switch(action){
    case UiAction::SettingsBack:case UiAction::Back:
        settings_focus::blur_rows(button_motions,MaxButtonMotions,map_clock_ms);settings_open=false;binding_listening=-1;break;
    case UiAction::SettingsGeneralTab:case UiAction::SettingsInputTab:
        if(settings_tab&&action==UiAction::SettingsGeneralTab)settings_focus::blur_rows(button_motions,MaxButtonMotions,map_clock_ms);
        settings_tab=action==UiAction::SettingsInputTab;settings_item=settings_top=0;break;
    case UiAction::SettingsLanguage:
        if(title&&locale_language_count(available_languages())>1){settings_language_open=true;settings_language_item=0;
            for(unsigned i=0;i<locale_language_count(available_languages());++i)if(locale_language_at(available_languages(),i)==content.language()){settings_language_item=uint8_t(i);break;}
            settings_language_top=uint8_t(settings_layout::language_reveal(locale_language_count(available_languages()),settings_language_item,0));}break;
    case UiAction::SettingsFastMode:fast_mode=!fast_mode;persist_preferences();break;
    case UiAction::SettingsCardIndices:show_card_indices=!show_card_indices;persist_preferences();break;
    case UiAction::SettingsRunTimer:show_run_timer=!show_run_timer;persist_preferences();break;
    case UiAction::SettingsLongPress:long_press=!long_press;end_turn_hold={};persist_preferences();break;
    case UiAction::SettingsTextEffects:text_effects=!text_effects;invalidate_hand_tiles();persist_preferences();break;
    case UiAction::SettingsSkipIntroLogo:{
        skip_intro_logo=!skip_intro_logo;
        const char*toast=content.world_string("settings_ui",skip_intro_logo?"TOAST_SKIP_INTRO_LOGO_ON":"TOAST_SKIP_INTRO_LOGO_OFF");
        message(*toast?toast:(content.language()!=2?(skip_intro_logo?"Intro logo will be skipped.":"Intro logo will be shown."):(skip_intro_logo?"下次启动时跳过开场Logo动画。":"下次启动时显示开场Logo动画。")));
        persist_preferences();break;
    }
    case UiAction::SettingsKeyboardOnly:keyboard_only=!keyboard_only;if(!keyboard_only)pointer.visible=true;persist_preferences();break;
    case UiAction::SettingsPhobia:phobia_mode=!phobia_mode;persist_preferences();break;
    case UiAction::SettingsShakeDown:if(screen_shake){--screen_shake;screen_effect(ScreenShakePresentation::Trauma,ScreenShakePresentation::Medium);persist_preferences();}break;
    case UiAction::SettingsShakeUp:if(screen_shake<4){++screen_shake;screen_effect(ScreenShakePresentation::Trauma,ScreenShakePresentation::Medium);persist_preferences();}break;
    case UiAction::SettingsReset:if(settings_tab)reset_input_bindings();else{frontend_confirm=4;frontend_choice=0;}break;
    case UiAction::SettingsControls:help=true;break;
    case UiAction::SettingsFeedback:break;
    case UiAction::SettingsCredits:open_auxiliary(AuxiliaryScreen::Credits);break;
    case UiAction::SettingsScrollUp:if(settings_top)settings_top--;break;
    case UiAction::SettingsScrollDown:if(settings_top<(settings_tab?settings_layout::InputMaximum:settings_layout::GeneralMaximum))settings_top++;break;
    default:break;
    }
}
bool App::settings_input(Key key){
    if(!settings_open)return false;
    if(binding_listening>=0){if(key==Key::Back&&pointer.dispatching)binding_listening=-1;return true;}
    if(settings_language_open){
        if(key==Key::Back)settings_language_open=false;
        else if(key==Key::Up){if(settings_language_item)settings_language_item--;settings_language_top=uint8_t(settings_layout::language_reveal(locale_language_count(available_languages()),settings_language_item,settings_language_top));}
        else if(key==Key::Down){if(settings_language_item+1<locale_language_count(available_languages()))settings_language_item++;settings_language_top=uint8_t(settings_layout::language_reveal(locale_language_count(available_languages()),settings_language_item,settings_language_top));}
        else if(key==Key::Confirm)request_language(locale_language_at(available_languages(),settings_language_item));
        return true;
    }
    const unsigned count=settings_tab?InputCommandCount+3u:settings_layout::GeneralRows+1u; // Mode/rows, Reset, Back.
    if(key==Key::Back)settings_action(UiAction::SettingsBack);
    else if(key==Key::Inspect){if(settings_tab)settings_focus::blur_rows(button_motions,MaxButtonMotions,map_clock_ms);settings_tab^=1;settings_item=settings_top=0;}
    else if(key==Key::Up||key==Key::Down){
        if(key==Key::Up){if(settings_item)--settings_item;}
        else if(settings_item+1<count)++settings_item;
        if(!settings_tab&&settings_item<settings_layout::GeneralRows){
            if(settings_item<settings_top)settings_top=settings_item;
            else if(settings_item>=settings_top+settings_layout::GeneralVisible)settings_top=settings_item+1-settings_layout::GeneralVisible;
        }
        if(settings_tab&&settings_item>=1&&settings_item<=InputCommandCount){const unsigned row=settings_item-1;
            if(row<settings_top)settings_top=row;else if(row>=settings_top+settings_layout::InputVisible)settings_top=row+1-settings_layout::InputVisible;}
    }else if(key==Key::Left||key==Key::Right){
        if(!settings_tab&&settings_item==1)settings_action(key==Key::Left?UiAction::SettingsShakeDown:UiAction::SettingsShakeUp);
        // NSettingsPanel assigns left/right to the current control. Inspect
        // remains the host's separate tab shortcut; arrows do not change tabs.
    }else if(key==Key::Confirm){
        static const UiAction general[]={UiAction::SettingsLanguage,UiAction::SettingsShakeUp,UiAction::SettingsFastMode,
            UiAction::SettingsPhobia,UiAction::SettingsRunTimer,UiAction::SettingsCardIndices,UiAction::SettingsLongPress,UiAction::SettingsTextEffects,
            UiAction::SettingsSkipIntroLogo,UiAction::SettingsCredits,UiAction::SettingsReset,UiAction::SettingsBack};
        if(!settings_tab)settings_action(general[settings_item%(settings_layout::GeneralRows+1)]);
        else if(!settings_item)settings_action(UiAction::SettingsKeyboardOnly);
        else if(settings_item<=InputCommandCount)begin_key_rebind(settings_item-1);
        else settings_action(settings_item==InputCommandCount+1?UiAction::SettingsReset:UiAction::SettingsBack);
    }
    return true;
}
