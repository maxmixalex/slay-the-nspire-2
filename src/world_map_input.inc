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
            if(!panel){uint8_t ids[reference::Columns];const unsigned count=world_available_points(game,ids,reference::Columns);const auto*m=world_current_map(game);
                if(m&&count){int found=-1;for(unsigned i=0;i<count;++i)if(m->points[ids[i]].col==cursor)found=int(i);
                    const unsigned selected=found<0?0:unsigned((found+int(count)+(key==Key::Left?-1:1))%int(count));cursor=m->points[ids[selected]].col;}
            }return true;
        }
    }
    if(key==Key::Map||key==Key::Inspect||key==Key::Log||key==Key::Potions||key==Key::Relics||key==Key::Collection)map_legend=false;
    return false;
}
