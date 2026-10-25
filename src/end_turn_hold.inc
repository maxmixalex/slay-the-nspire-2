static bool end_turn_hold_surface(const App&a){
    // A covered Combat can use the same semantic key for a pile/selection
    // control. Only the visible combat button captures long-press edges.
    return a.game.phase==Phase::Combat&&!a.title&&!a.paused&&!a.settings_open&&!a.frontend_confirm&&
        !a.help&&!a.panel&&!a.library&&!a.character_select&&!a.map_preview.open&&!a.map_window.is_open&&!a.bestiary.open&&
        a.auxiliary.screen==AuxiliaryScreen::None&&a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&
        !a.potion_targeting&&!a.ftue.active();
}
bool App::end_turn_hold_available()const{
    return long_press&&end_turn_hold_surface(*this)&&!combat_actions_locked()&&
        !combat_preparation.active&&int32_t(combat_linger_until-animation)<=0;
}
void App::start_end_turn_hold(uint16_t owner){
    if(!end_turn_hold_available())return;
    if(owner!=EndTurnHold::Mouse){pointer_cancel(true);pointer.visible=false;pointer.motion_time=map_clock_ms;}
    end_turn_hold.start(owner,input_context(),game.turn);
}
void App::release_end_turn_hold(uint16_t owner){
    const bool tutorial=end_turn_hold.owner==owner&&end_turn_hold.pressed&&
        end_turn_hold_available()&&end_turn_hold.context==input_context()&&end_turn_hold.turn==game.turn;
    end_turn_hold.cancel(owner);
    // NEndTurnButton checks its playable-cards tutorial on a short release.
    // The bar's completed hold calls CallReleaseLogic directly instead.
    if(tutorial)ftue_before_end_turn();
}
void App::advance_end_turn_hold(unsigned ms){
    auto&hold=end_turn_hold;
    if(hold.pressed&&(!end_turn_hold_available()||hold.context!=input_context()||hold.turn!=game.turn))hold.unfocus();
    if(!hold.advance(ms))return;
    hold.dispatching=true;
    if(hold.owner==EndTurnHold::Mouse)pointer_key(Key::EndTurn);else input(Key::EndTurn);
    hold.dispatching=false;
}
void App::command_key(Key key,bool pressed){
    if(pressed){
        if(command_held==unsigned(key))return;
        end_turn_hold.cancel(EndTurnHold::Semantic);
        command_held=uint8_t(key);command_hold=key==Key::EndTurn&&long_press&&end_turn_hold_surface(*this);
        if(command_hold){sync_ftues();start_end_turn_hold(EndTurnHold::Semantic);}
        else end_turn_hold.unfocus();
        return;
    }
    if(command_held!=unsigned(key))return;
    const bool held=command_hold;command_held=255;command_hold=false;
    if(held){release_end_turn_hold(EndTurnHold::Semantic);return;}
    input(key);map_drawing_key_release(key);
}
void App::command_keys_cancel(){
    end_turn_hold.cancel(EndTurnHold::Semantic);command_held=255;command_hold=false;
}
