void App::open_run_menu(RunMenu screen){
    if(!title||has_save)return;
    run_mode.screen=screen;run_mode.started=map_clock_ms;run_mode.focus=-1;
    character_select=false;practice_entry=false;progress_page_scroll.reset();pointer_cancel();
    if(screen==RunMenu::Singleplayer){compendium_selection=0;for(auto&motion:compendium_motion)motion={};return;}
    run_mode.pending={};selected_character=Character::Ironclad;selected_ascension=0;
    selected_region=reference::ActKind::Overgrowth;selected_seed[0]=0;
    if(screen==RunMenu::Custom)run_mode.pending.mode=RunMode::Custom;
    else if(screen==RunMenu::Daily)refresh_daily_run();
}
void App::refresh_daily_run(){
    if(run_mode.screen!=RunMenu::Daily)return;
    const uint32_t now=callbacks.wall_clock?callbacks.wall_clock():0;
    if(!now)return;
    const uint32_t day=now/86400;
    if(run_mode.pending.mode==RunMode::Daily&&run_mode.pending.daily_day==day)return;
    prepare_daily_run(day,selected_character,run_mode.pending,selected_character,selected_ascension,selected_seed);
}
void App::run_modifier_select(unsigned index){
    if(run_mode.screen!=RunMenu::Custom||index>=RunModifierKinds)return;
    toggle_run_modifier(run_mode.pending,RunModifier(index));run_mode.focus=int16_t(index+9);
}
bool App::start_selected_run(const RunOptions&options){
    if(!valid_run_options(options))return false;
    uint32_t seed=0;char canonical[65]{};
    if(!practice_entry&&*selected_seed){
        if(!reference::canonicalize_ascii(selected_seed,canonical,sizeof canonical)){message("种子无效");return false;}++run_number;
    }else{
        bool randomized=false;seed=next_run_seed(&randomized);
        if(randomized&&!practice_entry){static const char digits[]="0123456789ABCDEF";for(unsigned i=0;i<8;++i)canonical[i]=digits[(seed>>((7-i)*4))&15];}
    }
    if(*canonical){unsigned length=0;while(canonical[length])++length;const uint64_t hash=reference::hash64(canonical,length);seed=uint32_t(hash)^uint32_t(hash>>32);if(!seed)seed=1;}
    forget_run_checkpoint();new_run(game,seed,practice_entry,selected_character);
    if(!practice_entry){
        game.run_options=options;
        if(*canonical&&!world_set_seed(game,canonical)){message("种子无效");return false;}
        if(!world_begin_run(game,selected_region,selected_ascension)){message("无法开始远征");return false;}
    }
    character_select=run_settings=false;run_mode={};title=false;reset_presentation();begin_progress();
    if(game.world.active){map_visuals.bind(game,map_clock_ms);if(const auto*map=world_current_map(game))map_visuals.initial_marker(map->root,map_clock_ms);}
    return true;
}
bool App::run_modes_action(UiAction action){
    if(action==UiAction::OpenStandardRun){
        if(title&&run_mode.screen==RunMenu::Singleplayer){run_mode.screen=RunMenu::Closed;run_mode.pending={};practice_entry=false;character_select=true;selected_seed[0]=0;selected_region=reference::ActKind::Overgrowth;}return true;
    }
    if(action==UiAction::OpenCustomRun||action==UiAction::OpenDailyRun){
        if(title&&run_mode.screen==RunMenu::Singleplayer)open_run_menu(action==UiAction::OpenCustomRun?RunMenu::Custom:RunMenu::Daily);return true;
    }
    if(action==UiAction::StartConfiguredRun){
        if(run_mode.screen==RunMenu::Daily){refresh_daily_run();if(run_mode.pending.mode!=RunMode::Daily)return true;}
        if(run_mode.screen==RunMenu::Custom||run_mode.screen==RunMenu::Daily)start_selected_run(run_mode.pending);return true;
    }
    if(action==UiAction::RandomizeRun){
        if(run_mode.screen==RunMenu::Custom){reference::Random rng;rng.reset(next_run_seed());selected_character=RunModeCharacters[rng.below(CharacterKinds)];pick_run_modifiers(rng,selected_character,run_mode.pending);}
        return true;
    }
    if(run_mode.screen!=RunMenu::Custom)return false;
    if(action==UiAction::RunAscensionDown){if(selected_ascension)--selected_ascension;return true;}
    if(action==UiAction::RunAscensionUp){if(selected_ascension<10)++selected_ascension;return true;}
    if(action==UiAction::RunSeed){
        map_preview.open=true;run_seed_editing=true;map_preview.begin_edit();unsigned n=0;do{map_preview.draft[n]=selected_seed[n];}while(selected_seed[n++]);return true;
    }
    if(action==UiAction::RunSeedClear){selected_seed[0]=0;return true;}
    switch(action){
    case UiAction::SelectIronclad:selected_character=Character::Ironclad;return true;
    case UiAction::SelectSilent:selected_character=Character::Silent;return true;
    case UiAction::SelectRegent:selected_character=Character::Regent;return true;
    case UiAction::SelectNecrobinder:selected_character=Character::Necrobinder;return true;
    case UiAction::SelectDefect:selected_character=Character::Defect;return true;
    default:return false;
    }
}
bool App::run_modes_input(Key key){
    if(run_mode.screen==RunMenu::Closed||map_preview.open)return false;
    if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;return true;}
    if(key==Key::Back){if(run_mode.screen==RunMenu::Singleplayer)run_mode={};else open_run_menu(RunMenu::Singleplayer);pointer_cancel();return true;}
    if(run_mode.screen==RunMenu::Singleplayer){
        if(key==Key::Left||key==Key::Up)compendium_selection=uint8_t((compendium_selection+2)%3);
        else if(key==Key::Right||key==Key::Down)compendium_selection=uint8_t((compendium_selection+1)%3);
        else if(key==Key::Confirm){const UiAction actions[]={UiAction::OpenStandardRun,UiAction::OpenDailyRun,UiAction::OpenCustomRun};ui_action(actions[compendium_selection%3]);}
        return true;
    }
    if(key==Key::EndTurn||(run_mode.screen==RunMenu::Daily&&key==Key::Confirm)){ui_action(UiAction::StartConfiguredRun);return true;}
    if(run_mode.screen==RunMenu::Daily)return true;
    auto&focus=run_mode.focus;if(focus<0)focus=0;
    if(key==Key::Map)focus=9;
    else if(key==Key::Right){if(focus<4)++focus;else if(focus<9)focus=9;}
    else if(key==Key::Left){if(focus>=9)focus=8;else if(focus>0)--focus;}
    else if(key==Key::Up){if(focus>=10)--focus;else if(focus==9)focus=8;else if(focus>=5)focus-=5;}
    else if(key==Key::Down){if(focus>=9){if(focus<int(9+RunModifierKinds)-1)++focus;}else if(focus<5)focus=7;else focus=9;}
    else if(key==Key::Confirm){
        const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectDefect,
            UiAction::RunAscensionDown,UiAction::RunAscensionUp,UiAction::RunSeed,UiAction::RandomizeRun};
        if(focus<9)ui_action(actions[focus]);else run_modifier_select(unsigned(focus-9));
    }
    return true;
}
