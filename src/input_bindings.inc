// NInputManager keeps independent mouse/keyboard and keyboard-only maps.
// Physical keys enter here once; semantic UI buttons keep the existing API.
const char* App::input_command_title(unsigned command)const{
    char key[72]{};TextBuffer text{key,sizeof key,0};text.text("INPUT_SETTINGS.INPUT_TITLE.");text.text(input_command_key(command));
    const char*localized=content.world_string("settings_ui",key);if(*localized)return localized;
    static const char*const english[]={"Select","Cancel / Exit","View Map","Top Panel","View Deck / Page Left","View Draw Pile","View Discard Pile","View Exhaust / Page Right","Confirm","End Turn","Peek","Up","Down","Left","Right","Select Card 1","Select Card 2","Select Card 3","Select Card 4","Select Card 5","Select Card 6","Select Card 7","Select Card 8","Select Card 9","Select Card 10"};
    static const char*const chinese[]={"选择","取消 / 退出","查看地图","顶部面板","查看牌组 / 上一页","查看抽牌堆","查看弃牌堆","查看消耗堆 / 下一页","确认","结束回合","查看战斗","向上","向下","向左","向右","选择卡牌 1","选择卡牌 2","选择卡牌 3","选择卡牌 4","选择卡牌 5","选择卡牌 6","选择卡牌 7","选择卡牌 8","选择卡牌 9","选择卡牌 10"};
    return command<InputCommandCount?(content.english()?english[command]:chinese[command]):"";
}
void App::begin_key_rebind(unsigned command){
    if(!settings_open||!settings_tab||binding_listening>=0||command>=InputCommandCount)return;
    const unsigned bank=keyboard_only&&!pointer.visible?1:0;
    if(!input_remappable(bank,command))return;
    settings_item=uint8_t(command+1);binding_bank=uint8_t(bank);binding_listening=int8_t(command);pointer_cancel();
}
void App::reset_input_bindings(){
    physical_keys_cancel();input_bindings=default_input_bindings(calculator_keyboard);binding_listening=-1;persist_preferences();
}
void App::physical_keys_cancel(){
    for(auto&held:held_inputs){if(held.code&&held.key<=unsigned(Key::Debug))map_drawing_key_release(Key(held.key));held={};}
}
static bool title_patch_notes_available(const App&a){
    return a.title&&!a.paused&&!a.settings_open&&!a.frontend_confirm&&!a.help&&!a.panel&&!a.library&&
        !a.character_select&&!a.map_preview.open&&!a.compendium_open&&!a.statistics_open&&!a.history_open&&
        !a.profiles_open&&!a.bestiary.open&&a.run_mode.screen==RunMenu::Closed&&
        a.auxiliary.screen==AuxiliaryScreen::None&&a.detail_card<0&&a.detail_item<0;
}
bool App::physical_key(uint16_t code,bool pressed){
    code=normalize_input_key(code);if(!valid_input_key(code))return false;
    HeldInput*held=nullptr;for(auto&entry:held_inputs)if(entry.code==code){held=&entry;break;}
    if(!pressed){if(!held)return false;const unsigned released=held->key;const uint32_t context=held->context;*held={};
        if(released<=unsigned(Key::Debug))map_drawing_key_release(Key(released));
        else if(released==254)toggle_combat_peek();
        else if(released==253&&run_mode.screen==RunMenu::Custom)ui_action(UiAction::RandomizeRun);
        else if(released==252&&title_patch_notes_available(*this))open_auxiliary(AuxiliaryScreen::PatchNotes);
        else if(released==251&&auxiliary.screen==AuxiliaryScreen::PatchNotes)close_auxiliary();
        else if((released==248||released==247)&&auxiliary.screen==AuxiliaryScreen::PatchNotes)auxiliary_control(released==248?AuxiliaryPrevious:AuxiliaryNext);
        else if(released==246&&auxiliary.screen!=AuxiliaryScreen::None)auxiliary_input(Key::Back);
        else if((released==245||released==244)&&game_over_available()&&game_over_button_ready()&&context==input_context()){
            if(released==245){if(game_over_ui::ViewRunEnabled)open_game_over_history();}
            else ui_action(game_over_summary?UiAction::GameOverMainMenu:UiAction::GameOverSummary);
        }
        else if((released==250||released==249)&&card_reward_interactive()){
            sync_card_reward_presentation();if(context==reward_presentation.cards.key)card_reward_alternative(released==249);
        }
        return true;}
    if(held){
        // OS repeat navigates lists, but a held selection key must never play
        // another card or accept a newly exposed modal.
        if(held->key<=unsigned(Key::Down))input(Key(held->key));return true;
    }
    for(auto&entry:held_inputs)if(!entry.code){held=&entry;break;}if(!held)return true;
    held->code=code;held->key=255;
    sync_game_over_presentation();
    if(settings_open&&binding_listening>=0){
        if(rebind_input(input_bindings,binding_bank,unsigned(binding_listening),code)){binding_listening=-1;persist_preferences();}
        return true;
    }
    const unsigned bank=keyboard_only?1:0;unsigned command=InputCommandCount;
    for(unsigned i=0;i<InputCommandCount;++i)if(input_bindings.keys[bank][i]==code){command=i;break;}
    const auto dispatch=[&](Key key){held->key=uint8_t(key);input(key);return true;};
    if(auxiliary.screen!=AuxiliaryScreen::None){
        if(command==unsigned(InputCommand::Cancel)){held->key=246;return true;}
        if(auxiliary.screen==AuxiliaryScreen::PatchNotes&&(command==unsigned(InputCommand::Left)||command==unsigned(InputCommand::Right))){held->key=command==unsigned(InputCommand::Left)?248:247;return true;}
    }
    if(command>=unsigned(InputCommand::Card1)&&command<InputCommandCount){
        if(auxiliary.screen!=AuxiliaryScreen::None)return true;
        const unsigned card=command-unsigned(InputCommand::Card1);
        if(hand_can_focus()&&!combat_actions_locked()&&card<unsigned(game.hand.count)){
            if(!bank&&keyboard_card==game.hand.ids[card]){keyboard_card=-1;hand_focus=true;return true;}
            // The source cancels a previous card play and selects the newly
            // requested holder. The first press never spends energy.
            keyboard_card=-1;cursor=int(card);dispatch(Key::Confirm);
        }else if(!title&&!paused&&!settings_open&&!frontend_confirm&&!panel&&!library&&choice_phase(game.phase)&&combat_choice_ready()){
            const auto&pile=choice_pile(game);if(card<pile.count)toggle_choice(pile.ids[card]);
        }
        return true;
    }
    switch(InputCommand(command)){
    case InputCommand::Select:
        if(game_over_available()){if(game_over_button_ready()){held->key=244;held->context=input_context();}return true;}
        return dispatch(Key::Confirm);
    case InputCommand::Cancel:
        if(card_reward_interactive()){sync_card_reward_presentation();held->key=249;held->context=reward_presentation.cards.key;return true;}
        return dispatch(Key::Back);
    case InputCommand::Map:return dispatch(Key::Map);
    case InputCommand::TopPanel:
        if(auxiliary.screen==AuxiliaryScreen::None&&!title&&!paused&&!settings_open&&!frontend_confirm&&!library&&!character_select&&!map_preview.open){pointer_cancel();pointer.visible=false;hud_focus=hud_focus<0?0:-1;}return true;
    case InputCommand::Deck:return dispatch(Key::Inspect);
    case InputCommand::Draw:return dispatch(Key::DrawPile);
    case InputCommand::Discard:return dispatch(Key::DiscardPile);
    case InputCommand::Exhaust:
        if(card_reward_interactive()){sync_card_reward_presentation();held->key=250;held->context=reward_presentation.cards.key;return true;}
        return dispatch(Key::ExhaustPile);
    case InputCommand::Confirm:
        if(game_over_available()){if(game_over_ui::ViewRunEnabled&&game_over_summary&&game_over_button_ready()){held->key=245;held->context=input_context();}return true;}
        // NPatchNotesButton listens to MegaInput.confirm's release. Its
        // title shortcut stays distinct from Select and the Enter fallback.
        if(title_patch_notes_available(*this)){held->key=252;return true;}
        if(auxiliary.screen==AuxiliaryScreen::PatchNotes){held->key=251;return true;}
        if(card_reward_interactive())return dispatch(Key::Detail);
        if(!title&&!paused&&!settings_open&&!frontend_confirm&&!help&&!panel&&!library&&detail_card<0&&detail_item<0){
            if(choice_phase(game.phase)||((game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice)&&!world_choice_is_card_reward()&&
               !(game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Crystal)))return dispatch(Key::EndTurn);
            if(!bank&&game.phase==Phase::Combat)return dispatch(Key::EndTurn);
            if(bank&&game.phase==Phase::Combat&&hud_focus<0)return true;
        }
        return dispatch(Key::Confirm);
    case InputCommand::EndTurn:return dispatch(Key::EndTurn);
    case InputCommand::Peek:
        if(auxiliary.screen!=AuxiliaryScreen::None)return true;
        held->key=run_mode.screen==RunMenu::Custom?253:254;
        return true;
    case InputCommand::Up:return dispatch(Key::Up);
    case InputCommand::Down:return dispatch(Key::Down);
    case InputCommand::Left:return dispatch(Key::Left);
    case InputCommand::Right:return dispatch(Key::Right);
    default:break;
    }
    // Enter remains the desktop UI acceptance key in mouse/keyboard mode;
    // calculator 5 is its physical select button. Explicit mappings win.
    if(!bank&&(code==13||(calculator_keyboard&&code=='5'))){
        if(game_over_available()){if(game_over_button_ready()){held->key=244;held->context=input_context();}return true;}
        return dispatch(Key::Confirm);
    }
    switch(code){
    case 'L':return dispatch(Key::Log);case 'O':return dispatch(Key::Load);
    case 'P':return dispatch(Key::Potions);case 'R':return dispatch(Key::Relics);
    case 'I':return dispatch(Key::Detail);case 'C':return dispatch(Key::Collection);case 'V':return dispatch(Key::Statuses);
#if SPIRE_DEBUG_COMMANDS
    case InputF1:case 'G':return dispatch(Key::Debug);
#endif
    default:*held={};return false;
    }
}
