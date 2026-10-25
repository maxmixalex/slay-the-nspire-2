bool App::map_drawing_ready()const{
    if(ftue.active())return false;
    return game.world.active&&map_window.is_open&&(panel==3||(!panel&&game.phase==Phase::Map))&&
        !title&&!paused&&!help&&!library&&!character_select&&!settings_open&&!frontend_confirm&&
        !map_preview.open&&!event_reader&&detail_card<0&&detail_item<0&&status_actor<0&&deck_candidate<0&&
        !map_transition_blocks_pointer()&&!map_act_intro.camera_running&&map_can_scroll();
}
void App::sync_map_drawing(){
    if(!map_drawing_ready())map_drawing.cancel();
    if(title||!game.world.active||!map_window.visible)return;
    auto&document=progress.map_drawing;
    if(!document.matches(game.world.run_seed,progress.current.serial,game.world.act_index)){
        if(document.count)progress_dirty=true;
        document.bind(game.world.run_seed,progress.current.serial,game.world.act_index);
        map_drawing.cancel();map_drawing.transform_valid=false;
    }
}
void App::map_drawing_tool(unsigned tool,bool keyboard){
    if(tool>2||!map_drawing_ready())return;
    sync_map_drawing();
    const MapDrawingMode next=tool==0?MapDrawingMode::Draw:MapDrawingMode::Erase;
    const bool same=map_drawing.mode==next;
    pointer_cancel();map_drawing.cancel();map_legend=false;
    if(tool==2){
        if(progress.map_drawing.count){progress.map_drawing.count=0;progress_dirty=map_drawing.dirty=true;}
        if(keyboard)map_drawing.tool_focus=2;
    }else if(!same){
        map_drawing.mode=next;map_drawing.keyboard=keyboard;
        map_drawing.cursor_x=pointer.x*256;map_drawing.cursor_y=mx(WorldMapTop,mn(239,pointer.y))*256;
    }
    if(keyboard)pointer.visible=false;
}
bool App::map_drawing_point(bool begin){
    auto&input=map_drawing;auto&document=progress.map_drawing;
    if(!map_drawing_ready()||input.mode==MapDrawingMode::None)return false;
    const int x=input.keyboard?input.cursor_x/256:pointer.x,y=input.keyboard?input.cursor_y/256:pointer.y;
    if(y<WorldMapTop||y>=WorldMapBottom){input.stop_line();return false;}
    if(!input.transform_valid){WorldMapLayout layout;if(!world_map_layout(game,layout))return false;
        input.paper_origin=map_view(layout).origin;input.transform_valid=true;
    }
    const MapDrawingTransform transform{input.paper_origin};
    if(document.count>=MapDrawingCapacity){
        if(begin||input.pressed)message(content.language()!=2?"Map notes are full. Clear them to draw again.":"地图标记已满，请先清空后继续绘制。");
        input.stop_line();return false;
    }
    const bool changed=document.append(transform.source_x(x),transform.source_y(y),input.mode,begin);
    if(begin)input.pressed=changed;
    if(changed){++input.revision;input.dirty=progress_dirty=true;}
    return changed;
}
bool App::map_drawing_key(Key key){
    if(!map_drawing_ready())return false;
    auto&input=map_drawing;
    // The original view-exhaust/tab-right action focuses DrawingTools on a
    // map. It never opens an unrelated card pile behind this screen.
    if(key==Key::ExhaustPile){
        input.stop_line();input.held_directions=0;input.keyboard=true;
        input.tool_focus=input.mode==MapDrawingMode::Erase?1:0;pointer.visible=false;return true;
    }
    if(input.tool_focus>=0){
        if(key==Key::Back){input.tool_focus=-1;return true;}
        if(key==Key::Left||key==Key::Right){input.tool_focus=int8_t(mn(2,mx(0,int(input.tool_focus)+(key==Key::Left?-1:1))));return true;}
        if(key==Key::Confirm){const unsigned tool=unsigned(input.tool_focus);input.tool_focus=-1;map_drawing_tool(tool,true);return true;}
        if(key==Key::Up||key==Key::Down){input.tool_focus=-1;return true;}
    }
    if(input.mode==MapDrawingMode::None)return false;
    if(key==Key::Back){input.cancel();pointer_cancel();return true;}
    // Switching from mouse to directional navigation stops the old stroke.
    // Keyboard drawing uses the dedicated focus action above, as in source.
    if(!input.keyboard){input.cancel();return false;}
    if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){
        input.held_directions|=uint8_t(1u<<unsigned(key));return true;
    }
    if(key==Key::Confirm){if(!input.pressed)map_drawing_point(true);return true;}
    if(key==Key::Save)return false;
    input.cancel();return false;
}
void App::map_drawing_key_release(Key key){
    if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)
        map_drawing.held_directions&=uint8_t(~(1u<<unsigned(key)));
    if(key==Key::Confirm&&map_drawing.keyboard)map_drawing.stop_line();
}
void App::map_drawing_advance(uint32_t milliseconds){
    auto&input=map_drawing;
    if(!map_drawing_ready()){if(input.mode!=MapDrawingMode::None||input.tool_focus>=0)input.cancel();return;}
    if(!input.keyboard||input.mode==MapDrawingMode::None||input.tool_focus>=0||!input.held_directions)return;
    const int dx=int(bool(input.held_directions&2))-int(bool(input.held_directions&1));
    const int dy=int(bool(input.held_directions&8))-int(bool(input.held_directions&4));
    // NControllerMapDrawingInput moves at 700 authored px/s. Normalize the
    // diagonal D-pad vector, then project by the same 0.22 as map geometry.
    int distance=int(milliseconds)*154*256/1000;if(dx&&dy)distance=distance*181/256;
    input.cursor_x=mx(0,mn(319*256,input.cursor_x+dx*distance));
    input.cursor_y=mx(WorldMapTop*256,mn(239*256,input.cursor_y+dy*distance));
    if(input.pressed)map_drawing_point();
}
void App::map_drawing_pointer_motion(){
    if(map_drawing.keyboard){map_drawing.cancel();return;}
    if(map_drawing.pressed){if(map_drawing_ready())map_drawing_point();else map_drawing.cancel();}
}
bool App::map_drawing_pointer_button(bool down,const HitRegion&hit){
    auto&input=map_drawing;
    if(!map_drawing_ready())return false;
    if(input.keyboard)input.cancel();
    if(input.mode==MapDrawingMode::None||input.temporary_button||hit.kind==HitKind::MapDrawingTool)return false;
    if(down){
        // HUD controls and the permanent drawing toolbar remain usable.
        if(pointer.y<WorldMapTop)return false;
        pointer.held=true;pointer.dragging=false;pointer.captured={};
        map_drawing_point(true);
    }else{if(input.pressed)map_drawing_point();input.stop_line();pointer.held=false;pointer.captured={};}
    return true;
}
void App::pointer_aux_button(unsigned button,bool down){
    if(startup_logo.active()){if(down)startup_logo.cancel(map_clock_ms);pointer_cancel();return;}
    if(ftue.active())return;
    if(button!=2&&button!=3)return;
    pointer.visible=true;pointer.motion_time=map_clock_ms;hud_focus=-1;
    sync_map_window();
    if(!map_drawing_ready()){if(down&&button==2)input(Key::Back);return;}
    auto&drawing=map_drawing;
    if(!down){
        if(drawing.temporary_button==button){if(drawing.pressed)map_drawing_point();drawing.cancel();}
        return;
    }
    // A right/middle press cancels a selected persistent mode. With None,
    // those buttons draw/erase only while held, exactly like NMouseHeldInput.
    if(drawing.mode!=MapDrawingMode::None){drawing.cancel();pointer_cancel();return;}
    if(pointer.y<WorldMapTop)return;
    render();sync_map_drawing();pointer_cancel();map_legend=false;
    drawing.mode=button==2?MapDrawingMode::Draw:MapDrawingMode::Erase;
    drawing.temporary_button=uint8_t(button);drawing.keyboard=false;map_drawing_point(true);
}
