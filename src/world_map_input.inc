// Godot 4.5.1 SINE_OUT, sampled at 33ms on the real map clock.
// Original reticle tweens do not accelerate with the run's Fast setting.
// The terminal value is explicit, and unsigned tick differences are clamped
// before any multiplication so a long-open screen cannot restart its tween.
static unsigned map_reticle_sine(uint32_t ticks){
    static const uint16_t values[]={0,16796,32470,45975,56409,63075,65527};
    return ticks<sizeof values/sizeof values[0]?values[ticks]:65535;
}
void App::reset_map_reticles(){
    for(auto&r:map_reticles)r=MapReticleMotion{};
    map_reticle_seed=0;map_reticle_act=255;map_reticle_current=reference::NoPoint;
    // Input mode, like the original global controller manager, survives a
    // room reset or saved-run load. Only actual input changes that mode.
}
unsigned App::map_reticle_opacity(unsigned point)const{
    if(point>=reference::MaxPoints)return 0;
    const auto&r=map_reticles[point];
    if(r.mode==1||r.mode==3)return 255;
    if(r.mode!=2)return 0;
    return (unsigned(r.from_alpha)*(65535-map_reticle_sine(uint32_t(map_clock_ms-r.start)/33u))+32767)/65535;
}
unsigned App::map_reticle_scale(unsigned point)const{
    if(point>=reference::MaxPoints)return 4096;
    const auto&r=map_reticles[point];
    if(r.mode==1){
        // OnSelect immediately writes Scale=One, then its explicit From(.9)
        // drives the next tween update. EXPO_OUT uses Godot's 1.001 factor.
        static const uint16_t values[]={4096,3837,3932,3992,4031,4055,4070,4080,4086,4090,4092,4094,4095,4095,4096,4096};
        const uint32_t ticks=uint32_t(map_clock_ms-r.start)/33u;return ticks<sizeof values/sizeof values[0]?values[ticks]:4096;
    }
    if(r.mode==2){const unsigned weight=map_reticle_sine(uint32_t(map_clock_ms-r.start)/33u);
        return (unsigned(r.from_scale)*(65535-weight)+4301u*weight+32767)/65535;}
    return 4096;
}
void App::sync_map_reticles(){
    const auto*m=world_current_map(game);
    if(!m||m->count>reference::MaxPoints){if(map_reticle_act!=255)reset_map_reticles();return;}
    if(map_reticle_act!=game.world.act_index||map_reticle_seed!=game.world.run_seed){
        reset_map_reticles();map_reticle_act=game.world.act_index;map_reticle_seed=game.world.run_seed;
    }
    for(auto&r:map_reticles){
        if(r.mode==1&&uint32_t(map_clock_ms-r.start)>=500)r.mode=3;
        else if(r.mode==2&&uint32_t(map_clock_ms-r.start)>=200)r.mode=0;
    }
    unsigned next=reference::NoPoint;
    if(map_directional&&!map_travel.pending&&map_routes_active()&&!title&&!paused&&!help&&!library&&!character_select&&
       !map_preview.open&&!map_legend&&!event_reader&&!frontend_confirm&&!settings_open&&
       detail_card<0&&detail_item<0&&status_actor<0&&deck_candidate<0){
        uint8_t ids[reference::Columns];const unsigned count=world_available_points(game,ids,reference::Columns);
        if(count){next=ids[0];for(unsigned i=0;i<count;++i)if(m->points[ids[i]].col==cursor){next=ids[i];break;}}
    }
    if(next==map_reticle_current)return;
    if(map_reticle_current<reference::MaxPoints){
        auto&r=map_reticles[map_reticle_current];
        r.from_scale=uint16_t(map_reticle_scale(map_reticle_current));
        r.from_alpha=uint8_t(map_reticle_opacity(map_reticle_current));
        r.start=map_clock_ms;r.mode=r.from_alpha?2:0;
    }
    map_reticle_current=uint8_t(next);
    if(next<reference::MaxPoints){auto&r=map_reticles[next];r.start=map_clock_ms;r.from_scale=4096;r.from_alpha=255;r.mode=1;}
}
struct MapReticleInputSync {App&a;~MapReticleInputSync(){a.sync_map_window();a.sync_map_reticles();}};

bool App::map_input(Key key){
    if(title||paused||help||library||character_select||map_preview.open||status_actor>=0||detail_card>=0||detail_item>=0||deck_candidate>=0)return false;
    const bool visible=game.world.active&&(panel==3||(!panel&&game.phase==Phase::Map));
    if(!visible)return false;
    if(key==Key::Detail){ui_action(UiAction::ToggleMapLegend);return true;}
    if(map_legend){
        if(key==Key::Back||key==Key::Confirm){map_legend=false;pointer_cancel();return true;}
        if(key==Key::Up||key==Key::Left){map_legend_item=uint8_t((map_legend_item+5)%6);return true;}
        if(key==Key::Down||key==Key::Right){map_legend_item=uint8_t((map_legend_item+1)%6);return true;}
    }else{
        if(key==Key::Up||key==Key::Down){map_scroll=mx(-reference::MaxRows,mn(reference::MaxRows,map_scroll+(key==Key::Up?1:-1)));return true;}
        if(key==Key::Left||key==Key::Right){
            if(map_routes_active()){uint8_t ids[reference::Columns];const unsigned count=world_available_points(game,ids,reference::Columns);const auto*m=world_current_map(game);
                if(m&&count){int found=-1;for(unsigned i=0;i<count;++i)if(m->points[ids[i]].col==cursor)found=int(i);
                    const unsigned selected=found<0?0:unsigned((found+int(count)+(key==Key::Left?-1:1))%int(count));cursor=m->points[ids[selected]].col;}
            }return true;
        }
    }
    if(key==Key::Confirm&&map_routes_active()){
        if(map_scroll){map_scroll=0;message("已返回可选楼层，再次确认进入");}
        else if(!begin_map_column(cursor))message("这条路线不相连，请选择亮起的节点");
        return true;
    }
    if(key==Key::Map||key==Key::Inspect||key==Key::Log||key==Key::Potions||key==Key::Relics||key==Key::Collection)map_legend=false;
    return false;
}
