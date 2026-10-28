// Presentation coordinates use NMapScreen's Y relative to its -600 first-floor
// target. Map points still use the calculator's existing bounded layout.
static int map_intro_screen_offset(int32_t source_q12){
    return world_map_act_intro::scaled(source_q12,22,409600);
}
static int32_t map_intro_source_offset(int pixels){
    return world_map_act_intro::scaled(pixels,409600,22);
}
int32_t App::map_camera_reference_q12()const{
    const auto*m=world_current_map(game);if(!m||m->config.rows<2)return 0;
    const unsigned row=game.world.current_point<m->count?m->points[game.world.current_point].row:0;
    const unsigned factor=m->config.second_boss?9:10;
    const unsigned denominator=(m->config.rows-1)*10;
    return int32_t((uint64_t(row)*2325*4096*factor+denominator/2)/denominator);
}
int32_t App::map_camera_target_q12()const{
    WorldMapLayout layout;if(!world_map_layout(game,layout))return 0;
    const auto view=world_map_view(game,layout,map_scroll);
    return map_camera_reference_q12()+map_intro_source_offset(view.origin-view.natural);
}
WorldMapView App::map_view(const WorldMapLayout&layout)const{
    auto view=world_map_view(game,layout,map_scroll);
    // Apply the intro after ordinary scroll clamping. Clamping the camera
    // again would remove its 1800 -> -600 traversal of the original map.
    if(map_act_intro.position_valid)
        view.origin=view.natural+map_intro_screen_offset(map_act_intro.y_q12-map_camera_reference_q12());
    return view;
}
bool App::map_node_on_screen(unsigned point)const{
    const auto*m=world_current_map(game);WorldMapLayout layout;
    if(!m||point>=m->count||!world_map_layout(game,layout))return false;
    const auto view=map_view(layout);const int y=layout.points[point].y+view.origin;
    // Directional selection follows the node's visible position; it is not
    // a global camera-animation lock. The native HUD reserves these bounds.
    return y>WorldMapTop&&y<WorldMapBottom;
}
bool App::map_intro_at_entry()const{
    const auto*m=world_current_map(game);if(!m)return false;
    unsigned visited=0;for(unsigned i=0;i<m->count;++i)visited+=game.world.visited[game.world.act_index][i]!=0;
    // Original runs here always start with Neow. Later acts first enter the
    // standalone MapRoom at ActFloor zero, before choosing their Ancient.
    return game.world.act_index==0?visited==1&&game.world.current_point==m->root:
        visited==0&&game.world.current_point==reference::NoPoint;
}
void App::reset_map_act_intro(bool restored){
    map_act_intro.reset(fast_mode);map_act_clock_ms=map_clock_ms;
    map_act_seed=0;map_act_index=map_act_variant=255;
    map_act_room_entered=false;map_act_restore_pending=restored;map_dragging=false;map_act_scroll=map_scroll;
    if(const auto*m=world_current_map(game)){
        map_act_seed=game.world.run_seed;map_act_index=game.world.act_index;map_act_variant=m->config.variant;
        map_act_intro.played=restored;
    }
}
void App::sync_map_act_intro(){
    if(map_scroll!=map_act_scroll&&map_window.is_open&&!map_act_intro.camera_running&&!map_act_intro.input_guard)
        snap_map_camera();
    const uint32_t delta=uint32_t(map_clock_ms-map_act_clock_ms);map_act_clock_ms=map_clock_ms;
    // Capstones only hide Backstop in the original. They do not stop the
    // MapScreen node. Its close tail remains processable for all 350 ms.
    map_act_intro.update(delta,map_clock_ms,map_window.visible&&!title&&!quit&&!character_select&&!map_preview.open);
    const auto*m=world_current_map(game);
    if(!m||title||quit||character_select||!game.world.active){
        if(map_act_index!=255)reset_map_act_intro();return;
    }
    if(map_act_index==255||map_act_seed!=game.world.run_seed){reset_map_act_intro();return;}
    if(map_act_index!=game.world.act_index||map_act_variant!=m->config.variant){
        map_act_intro.install_map();map_act_index=game.world.act_index;map_act_variant=m->config.variant;
        map_act_room_entered=false;map_act_restore_pending=false;
    }
}
bool App::map_can_scroll()const{return map_act_intro.can_scroll();}
bool App::cancel_map_act_intro(){
    if(!map_act_intro.cancel(map_clock_ms))return false;
    map_scroll=map_act_scroll=0;map_dragging=false;pointer_cancel();return true;
}
void App::snap_map_camera(){
    WorldMapLayout layout;if(world_map_layout(game,layout))map_scroll=world_map_view(game,layout,map_scroll).scroll;
    map_act_scroll=map_scroll;
    const int32_t target=map_camera_target_q12();map_act_intro.target_q12=target;
    if(!map_act_intro.camera_running)map_act_intro.snap(target);
}
void App::scroll_map(int rows){
    if(!rows||!map_can_scroll())return;
    // The first permitted scroll cancels the sweep and restores its original
    // target. It does not also move to another row during the 200 ms guard.
    if(cancel_map_act_intro())return;
    map_scroll=mx(-reference::MaxRows,mn(reference::MaxRows,map_scroll+rows));snap_map_camera();
}
void App::drag_map(int pixels){
    if(!pixels||!map_can_scroll()||map_act_intro.camera_running)return;
    WorldMapLayout layout;if(!world_map_layout(game,layout))return;
    const auto view=world_map_view(game,layout,map_scroll);
    const int32_t base=map_camera_reference_q12();
    const int32_t lo=base+map_intro_source_offset(view.minimum-view.natural);
    const int32_t hi=base+map_intro_source_offset(view.maximum-view.natural);
    const int32_t target=map_act_intro.target_q12+map_intro_source_offset(pixels);
    map_act_intro.target_q12=target<lo?lo:target>hi?hi:target;
    map_act_intro.position_valid=true;
    // Track the nearest native row so directional confirmation can recenter.
    const int screen=map_intro_screen_offset(map_act_intro.target_q12-base);
    const int step=layout.row_step>0?layout.row_step:1;
    map_scroll=screen<0?-((-screen+step/2)/step):(screen+step/2)/step;
    map_act_scroll=map_scroll;
}
