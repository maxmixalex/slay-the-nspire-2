// Original map travel is asynchronous presentation followed by exactly one
// controller operation. No pending destination or UI clock enters State/save.
static bool map_travel_scene(const App&a){
    return a.game.world.active&&a.game.phase==Phase::Map&&!a.title&&!a.paused&&
        !a.help&&!a.library&&!a.character_select&&!a.map_preview.open&&!a.panel&&
        !a.map_legend&&!a.event_reader&&!a.frontend_confirm&&!a.settings_open&&
        a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&a.deck_candidate<0;
}
static bool map_travel_run(const App&a){
    // Original RoomFadeOut leaves the top bar's pointer controls enabled.
    // Opening a deck, pause or settings overlay does not leave this run.
    return a.game.world.active&&a.game.phase==Phase::Map&&!a.title&&!a.quit&&
        !a.character_select&&!a.map_preview.open;
}
bool App::begin_map_travel(unsigned point){
    // Keyboard dispatch already observes its longer transition gate. A
    // pointer may select another route after an immediately completed room
    // while Fast fade-in still blocks keyboard/controller hotkeys.
    if(map_travel.pending||map_transition_blocks_pointer()||!map_travel_scene(*this)||
       !world_reachable(game,point))return false;
    SaveCheckpoint checkpoint{};if(!checkpoint_of(game,checkpoint))return false;
    const auto*m=world_current_map(game);if(!m||point>=m->count)return false;
    const unsigned from=game.world.current_point;
    const bool connected=from<m->count&&m->points[from].children.contains(uint8_t(point));
    const auto speed=fast_mode?world_map_transition::Speed::Fast:world_map_transition::Speed::Normal;
    map_visuals.bind(game,map_clock_ms);
    map_visuals.begin(game,from,point,map_clock_ms,
        connected&&world_map_path_dot_count(game,from,point)?(fast_mode?300u:800u):0u);
    if(!map_visuals.traveling()||map_visuals.target()!=point)return false;
    map_travel={};map_travel.checkpoint=checkpoint;map_travel.seed=game.world.run_seed;
    map_travel.start=map_clock_ms;map_travel.revision=game.revision;
    map_travel.act=game.world.act_index;map_travel.from=uint8_t(from);map_travel.to=uint8_t(point);
    map_travel.speed=speed;map_travel.pending=true;
    pointer_cancel();hit_count=0;
    return true;
}
bool App::begin_map_column(int column){
    if(column<0||column>=reference::Columns)return false;
    const auto*m=world_current_map(game);if(!m)return false;
    for(unsigned i=0;i<m->count;++i)
        if(m->points[i].col==column&&world_reachable(game,i))return begin_map_travel(i);
    return false;
}
void App::cancel_map_travel(){
    map_travel={};map_visuals.cancel(game,map_clock_ms);pointer_cancel();
}
bool App::map_transition_blocks_keyboard()const{
    return map_travel.pending||(map_travel.fading_in&&
        uint32_t(map_clock_ms-map_travel.fade_start)<world_map_transition::fade_in_keyboard_delay_ms(map_travel.speed));
}
bool App::map_transition_blocks_pointer()const{
    // NGameTransition becomes MouseFilter.Stop only when fade-out finishes.
    // Map points have their separate IsTraveling gate throughout the wait.
    return map_travel.fading_in&&
        uint32_t(map_clock_ms-map_travel.fade_start)<world_map_transition::fade_in_pointer_delay_ms(map_travel.speed);
}
unsigned App::map_transition_alpha(int y)const{
    if(map_travel.pending)return world_map_transition::fade_out_alpha(
        map_travel.speed,uint32_t(map_clock_ms-map_travel.start),y,ScreenH);
    return map_travel.fading_in?world_map_transition::fade_in_alpha(
        map_travel.speed,uint32_t(map_clock_ms-map_travel.fade_start)):0;
}
void App::update_map_travel(){
    map_visuals.update(game,map_clock_ms);
    if(map_travel.fading_in&&uint32_t(map_clock_ms-map_travel.fade_start)>=
       world_map_transition::fade_in_duration_ms(map_travel.speed))map_travel.fading_in=false;
    if(!map_travel.pending)return;
    // Cheap identity checks on every update; the full canonical snapshot is
    // compared only at submission, including mutations that omit revision.
    if(!map_travel_run(*this)||game.world.run_seed!=map_travel.seed||
       game.world.act_index!=map_travel.act||game.world.current_point!=map_travel.from||
       game.revision!=map_travel.revision||!world_reachable(game,map_travel.to)){
        cancel_map_travel();return;
    }
    if(uint32_t(map_clock_ms-map_travel.start)<world_map_transition::fade_out_duration_ms(map_travel.speed))return;
    SaveCheckpoint current{};
    if(!checkpoint_of(game,current)||!same_checkpoint(current,map_travel.checkpoint)){
        cancel_map_travel();return;
    }
    const unsigned destination=map_travel.to;const auto speed=map_travel.speed;
    // Clear first: callbacks, failure, death on entry and repeated updates
    // cannot submit the same route again or consume another travel charge.
    map_travel.pending=false;
    // Clear the old room before observing entry. Clearing afterwards would
    // discard newly emitted enemy animation commands and relic feedback.
    // RunManager.ClearScreens also closes overlays opened during the wait.
    paused=help=library=settings_open=false;frontend_confirm=frontend_choice=0;
    confirmed_checkpoint={};
    reset_presentation(true);
    {
        UiCoreObserver observer(*this);
        const bool entered=world_choose_point(game,destination);
        if(entered){
            map_visuals.committed(game,map_clock_ms);choice_revision=game.revision;
            target=0;for(int i=0;i<game.enemy_count;++i)if(game.enemies[i].hp){target=i;break;}
            map_travel={};map_travel.speed=speed;map_travel.fade_start=map_clock_ms;
            map_travel.fading_in=true;
        }else{
            cancel_map_travel();reset_presentation(true);
            message("无法进入这条路线，请重新选择");
        }
    }
}
