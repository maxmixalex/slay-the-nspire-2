void App::open_run_menu(RunMenu screen){
    if(!title||has_save)return;
    run_mode.screen=screen;run_mode.started=map_clock_ms;run_mode.focus=-1;
    character_select=false;character_random=false;practice_entry=false;progress_page_scroll.reset();pointer_cancel();character_select_presentation.reset();
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
bool App::start_selected_run(const RunOptions&options,bool animated){
    if(character_departure.active()||!valid_run_options(options))return false;
    if(!practice_entry&&!seen_ftue(FtueId::AcceptTutorials)&&open_ftue(FtueId::AcceptTutorials)){ftue.embark=options;character_departure.animated_ftue=animated;return false;}
    uint32_t seed=0;char canonical[65]{};
    if(!practice_entry&&*selected_seed){
        if(!reference::canonicalize_ascii(selected_seed,canonical,sizeof canonical)){message("种子无效");return false;}++run_number;
    }else{
        bool randomized=false;seed=next_run_seed(&randomized);
        if(randomized&&!practice_entry){static const char digits[]="0123456789ABCDEF";for(unsigned i=0;i<8;++i)canonical[i]=digits[(seed>>((7-i)*4))&15];}
    }
    uint64_t run_hash=seed;
    if(*canonical){unsigned length=0;while(canonical[length])++length;run_hash=reference::hash64(canonical,length);seed=uint32_t(run_hash)^uint32_t(run_hash>>32);if(!seed)seed=1;}
    const bool random=character_random;
    if(random){
        // StartRunLobby resolves Random after the three ActModel choices,
        // from the independent act_selection stream and ModelDb order.
        // This port exposes both first acts and all five characters.
        reference::Random rng;rng.named(run_hash,"act_selection");
        (void)rng.below(2);(void)rng.below(1);(void)rng.below(1);
        selected_character=RunModeCharacters[rng.below(CharacterKinds)];character_random=false;
    }
    forget_run_checkpoint();new_run(game,seed,practice_entry,selected_character);
    if(!practice_entry){
        game.run_options=options;
        if(*canonical&&!world_set_seed(game,canonical)){message("种子无效");return false;}
        if(!world_begin_run(game,selected_region,selected_ascension)){message("无法开始游戏");return false;}
    }
    if(animated){
        character_departure.phase=1;character_departure.character=uint8_t(selected_character);
        character_departure.started=map_clock_ms;character_departure.reveal_ms=random?CharacterDeparturePresentation::RandomRevealMs:0;
        character_departure.continuing=false;
        const int mask=content.ui_image(I12_CharacterTransitionFirst+(selected_character==Character::Silent?1:selected_character==Character::Necrobinder?2:0));
        content.prepare_images(&mask,1);
        pointer_cancel();hit_count=0;return true;
    }
    finish_selected_run();return true;
}
void App::finish_selected_run(){
    const auto entry_heal=reward_presentation.heal;
    character_select=run_settings=false;run_mode={};title=false;reset_presentation();turn_banner_suppress_intro=false;begin_progress();
    // The initial room was built behind the character transition. Its
    // actual Neow heal retains NAncientEventLayout's200ms delay after this
    // new room becomes visible behind its fade-in.
    if(entry_heal.active&&game.world.active&&game.original_events.id==OriginalEventId::Neow){
        reward_presentation.heal=entry_heal;reward_presentation.heal.start=map_clock_ms+200u;
    }
    if(game.world.active){map_visuals.bind(game,map_clock_ms);if(const auto*map=world_current_map(game))map_visuals.initial_marker(map->root,map_clock_ms);}
}
void App::advance_character_departure(){
    auto&m=character_departure;if(!m.active())return;
    if(m.phase==1&&map_clock_ms-m.started>=m.reveal_ms+CharacterDeparturePresentation::FadeMs){
        const auto character=m.character;
        if(m.continuing){title=character_select=run_settings=false;run_mode={};m.continuing=false;}
        else finish_selected_run();
        m.phase=2;m.character=character;m.started=map_clock_ms;
    }else if(m.phase==2&&map_clock_ms-m.started>=CharacterDeparturePresentation::FadeMs)m={};
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
        if(run_mode.screen==RunMenu::Custom||run_mode.screen==RunMenu::Daily)start_selected_run(run_mode.pending,true);return true;
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
        if(key==Key::Left){if(compendium_selection)--compendium_selection;}
        else if(key==Key::Right){if(compendium_selection<2)++compendium_selection;}
        else if(key==Key::Confirm){const UiAction actions[]={UiAction::OpenStandardRun,UiAction::OpenDailyRun,UiAction::OpenCustomRun};ui_action(actions[compendium_selection%3]);}
        return true;
    }
    if(key==Key::EndTurn||(run_mode.screen==RunMenu::Daily&&key==Key::Confirm)){ui_action(UiAction::StartConfiguredRun);return true;}
    if(run_mode.screen==RunMenu::Daily){if(key==Key::Up||key==Key::Down)progress_page_scroll.wheel(key==Key::Up?-4:4);return true;}
    auto&focus=run_mode.focus;if(focus<0)focus=0;const int previous=focus;
    if(key==Key::Map)focus=9;
    else if(key==Key::Right){if(focus<4)++focus;else if(focus==5)focus=6;else if(focus>=6&&focus<9)focus=9;}
    else if(key==Key::Left){if(focus>=8)focus=7;else if(focus==6)focus=5;else if(focus>0&&focus<5)--focus;}
    else if(key==Key::Up){if(focus>=10)--focus;else if(focus==9||focus==7)focus=8;else if(focus==5||focus==6||focus<5)focus=7;}
    else if(key==Key::Down){if(focus>=9){if(focus<int(9+RunModifierKinds)-1)++focus;}else if(focus==8)focus=9;else if(focus==7)focus=5;
        else if(focus==5||focus==6){for(unsigned i=0;i<CharacterKinds;++i)if(RunModeCharacters[i]==selected_character)focus=int16_t(i);}}
    else if(key==Key::Confirm){
        const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectDefect,
            UiAction::RunAscensionDown,UiAction::RunAscensionUp,UiAction::RunSeed,UiAction::RandomizeRun};
        if(focus<9)ui_action(actions[focus]);else run_modifier_select(unsigned(focus-9));
    }
    // NCharacterSelectButton connects FocusEntered to Select; navigating the
    // custom portrait row immediately changes the selected character.
    if(focus!=previous&&focus>=0&&focus<int(CharacterKinds))selected_character=RunModeCharacters[unsigned(focus)];
    return true;
}
