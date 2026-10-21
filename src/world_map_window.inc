// Map windows retain the live room underneath, including a completed room
// whose controller permits travel. Only destination commit exits that room.
// Phase::Map remains the later acts' standalone map room.
void App::reset_map_window(){
    ++map_window_generation;
    map_window.reset();map_window_seed=0;map_window_act=255;map_window_point=reference::NoPoint;
    map_window_step_ms=map_clock_ms;map_window_y_q12=map_window_camera_base_q12=0;map_window_y_tween_finished=false;
    map_dragging=false;
    map_room_cursor=map_room_scroll=0;
}
bool App::map_routes_active()const{
    return world_can_travel(game)&&!treasure_empty_waiting()&&((game.phase==Phase::Map&&!panel)||
        (map_window.is_open&&panel==3));
}
void App::proceed_map_window(){
    // Boss/act/final-event transitions replace the room. Ordinary Proceed
    // leaves its actual live state available when the window is closed.
    if(world_can_travel(game)&&game.phase!=Phase::Map)open_map_window();
    else reset_presentation(true);
}
void App::open_map_window(bool from_top_bar){
    if(!game.world.active||title||character_select||map_preview.open)return;
    sync_map_act_intro();
    if(!map_window.open(map_clock_ms))return;
    map_window_seed=game.world.run_seed;map_window_act=game.world.act_index;
    map_window_point=game.world.current_point;
    map_window_step_ms=map_clock_ms;map_window_y_q12=0;map_window_y_tween_finished=false;
    map_room_cursor=cursor;map_room_scroll=scroll;
    panel=game.phase==Phase::Map?0:3;map_scroll=map_act_scroll=0;map_legend=false;
    // Opening an already-used map snaps its position even if an old act
    // tween is paused. The surviving tween may write again next frame.
    map_act_intro.snap(map_camera_target_q12());
    if(map_intro_at_entry()&&!map_act_intro.played){
        // No original FTUE profile exists in this frontend. Its established
        // no-tutorial flow is explicitly treated as "map_select_ftue seen".
        map_act_intro.open_intro(!from_top_bar&&!fast_mode,game.world.act_index,
            game.world.rooms[game.world.act_index].region,fast_mode);
    }
    if(game.phase==Phase::Map&&!map_act_room_entered){
        map_act_room_entered=true;
        // NMapRoom._Ready adds its own banner after Open has added one.
        if(!map_act_restore_pending)map_act_intro.add_banner(game.world.act_index,
            game.world.rooms[game.world.act_index].region,fast_mode);
    }
    if(map_act_restore_pending){map_window.settle();map_act_restore_pending=false;}
    if(world_can_travel(game)){
        uint8_t ids[reference::Columns];const unsigned n=world_available_points(game,ids,reference::Columns);
        const auto*m=world_current_map(game);if(m&&n)cursor=m->points[ids[0]].col;
    }
    map_visuals.set_open(game,true,map_clock_ms);
    pointer_cancel();hit_count=0;
}
void App::close_map_window(bool animate){
    sync_map_act_intro();
    map_window_camera_base_q12=map_camera_target_q12();
    if(!map_window.close(map_clock_ms,animate,map_act_intro.y_q12-map_window_camera_base_q12))return;
    map_window_step_ms=map_clock_ms;
    map_window_y_q12=map_window.sample(map_clock_ms).map_y_q12;
    map_window_y_tween_finished=false;
    if(panel==3)panel=0;
    cursor=map_room_cursor;scroll=map_room_scroll;
    map_visuals.set_open(game,false,map_clock_ms);
    pointer_cancel();hit_count=0;
}
void App::sync_map_window(){
    sync_map_act_intro();
    if(map_window.closing()){
        const uint32_t elapsed=uint32_t(map_clock_ms-map_window.start);
        const uint32_t delta=uint32_t(map_clock_ms-map_window_step_ms);
        if(delta){
            // Godot processes nodes before tweens. Until the first frame that
            // completes the 250 ms Y tween, its result overwrites the scroll
            // lerp. Later frames retain UpdateScrollPosition(delta * 15),
            // including its target snap on an overshoot or <0.5 source pixel.
            if(map_window_y_tween_finished){
                // The earlier act tween can resume writing after the later
                // close-Y tweener has finished. Otherwise _Process lerps.
                map_window_y_q12=map_act_intro.y_q12-map_window_camera_base_q12;
            }else{
                map_window_y_q12=map_window.sample(map_clock_ms).map_y_q12;
                map_act_intro.y_q12=map_window_camera_base_q12+map_window_y_q12;
                if(elapsed>=250)map_window_y_tween_finished=true;
            }
        }
    }
    map_window_step_ms=map_clock_ms;
    map_window.update(map_clock_ms);
    if(!map_window.visible&&map_window_act!=255)map_legend=false;
    const bool identity=game.world.active&&game.world.run_seed==map_window_seed&&
        game.world.act_index==map_window_act&&game.world.current_point==map_window_point;
    if(title||quit||character_select||map_preview.open||!game.world.active||
       (map_window_act!=255&&!identity)){
        const bool had_window=map_window_act!=255;
        reset_map_window();if(had_window&&panel==3)panel=0;
        if(title||quit||character_select||map_preview.open||!game.world.active)return;
    }
    if(game.phase==Phase::Map){
        if(!map_window.visible)open_map_window();
        return;
    }
    // Deck and other capstones cover a map without closing it. Closing the
    // covering panel returns to that same window, with no new Open tween.
    if(map_window.is_open&&!panel)panel=3;
    // Keep explicit host/test panel entry consistent with the real hotkey.
    if(panel==3&&!map_window.visible)open_map_window();
}
