void App::open_compendium(){
    history_from_game_over=false;
    const unsigned parent=library?unsigned(library_tab):statistics_open?4:history_open?5:bestiary.open?3:0;
    compendium_open=true;statistics_open=history_open=library=false;bestiary.open=false;
    detail_card=detail_item=-1;compendium_selection=uint8_t(parent);progress_page_started=map_clock_ms;
    pointer_cancel();for(auto&motion:compendium_motion)motion={};
}
void App::open_statistics(){
    compendium_open=true;compendium_selection=4;statistics_open=true;history_open=library=false;progress_page_started=map_clock_ms;
    progress_page_scroll.reset();pointer_cancel();
}
void App::open_history(){
    if(!progress.count)return;
    history_from_game_over=false;
    compendium_open=true;compendium_selection=5;history_open=true;statistics_open=library=false;history_index=0;history_selected=-1;
    progress_page_started=map_clock_ms;progress_page_scroll.reset();history_presentation.open(map_clock_ms);pointer_cancel();
}
void App::history_select(int direction){
    if(!history_open||!progress.count)return;
    const int next=int(history_index)+direction;
    if(next<0||next>=int(progress.count))return;
    history_index=uint8_t(next);history_selected=-1;progress_page_scroll.reset();history_presentation.open(map_clock_ms,direction>0?-1:1);pointer_cancel();
}
bool App::progress_navigation_input(Key key){
    if(library||bestiary.open||detail_card>=0||detail_item>=0)return false;
    if(statistics_open||history_open){
        if(key==Key::Back||key==Key::Collection){statistics_open=history_open=false;if(history_from_game_over){history_from_game_over=false;compendium_open=false;}pointer_cancel();return true;}
        if(history_open&&(key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right||key==Key::Confirm)){
            const auto*r=history_at(history_index);if(!r)return true;
            uint16_t indices[MaxDeck]{};const unsigned groups=ui_history_groups(*r,indices);
            const UiHistoryLayout layout(*r,groups);auto&focus=history_presentation.focus;
            if(focus<0){focus=int16_t(layout.first(*r));return true;}
            if(key==Key::Confirm){
                if(focus>=UiHistoryLayout::CardFirst&&unsigned(focus-UiHistoryLayout::CardFirst)<groups)
                    history_pointer({0,0,0,0,HitKind::HistoryCard,int16_t(indices[focus-UiHistoryLayout::CardFirst]),true});
                return true;
            }
            UiHistoryLayout::Point current{};if(!layout.point(*r,focus,current)){focus=int16_t(layout.first(*r));return true;}
            int best=-1,score=0x7fffffff;
            const auto consider=[&](int id){if(id==focus)return;UiHistoryLayout::Point next{};if(!layout.point(*r,id,next))return;
                const int dx=next.x+next.w/2-current.x-current.w/2,dy=next.y+next.h/2-current.y-current.h/2;
                const bool horizontal=key==Key::Left||key==Key::Right;
                const int along=key==Key::Left?-dx:key==Key::Right?dx:key==Key::Up?-dy:dy;
                const int cross=horizontal?(dy<0?-dy:dy):(dx<0?-dx:dx);
                if(along<=0||(horizontal&&cross>8))return;
                const int rank=along*4+cross*7;if(rank<score){score=rank;best=id;}
            };
            for(unsigned i=0;i<r->floor_count;++i)consider(int(i));
            for(unsigned i=0;i<r->relic_count;++i)consider(UiHistoryLayout::RelicFirst+int(i));
            for(unsigned i=0;i<groups;++i)consider(UiHistoryLayout::CardFirst+int(i));
            if(best>=0){focus=int16_t(best);UiHistoryLayout::Point point{};layout.point(*r,best,point);
                const int top=progress_page_scroll.offset();
                if(point.y<top)progress_page_scroll.target=point.y*256;
                else if(point.y+point.h>top+UiHistoryLayout::ViewHeight)progress_page_scroll.target=(point.y+point.h-UiHistoryLayout::ViewHeight)*256;
            }else if(key==Key::Left||key==Key::Right)history_select(key==Key::Left?1:-1);
        }else if(key==Key::Up||key==Key::Down)progress_page_scroll.wheel(key==Key::Up?-4:4);
        else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
        return true;
    }
    if(!compendium_open)return false;
    if(key==Key::Back||key==Key::Collection){compendium_open=false;pointer_cancel();return true;}
    const int top=bestiary_available()?4:3;
    if(key==Key::Left){if(compendium_selection<4){if(compendium_selection)--compendium_selection;}else compendium_selection=4;}
    else if(key==Key::Right){if(compendium_selection<4){if(compendium_selection+1<top)++compendium_selection;}else if(progress.count)compendium_selection=5;}
    else if(key==Key::Down&&compendium_selection<4)compendium_selection=compendium_selection==3&&progress.count?5:4;
    else if(key==Key::Up&&compendium_selection>=4)compendium_selection=compendium_selection==5&&top==4?3:2;
    else if(key==Key::Confirm){
        if(compendium_selection<3)ui_action(UiAction(int(UiAction::LibraryCards)+compendium_selection));
        else if(compendium_selection==3)open_bestiary();else if(compendium_selection==4)open_statistics();else open_history();
    }else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
    return true;
}
bool App::progress_pointer_button(bool down,const HitRegion&hit){
    if((!statistics_open&&!history_open&&run_mode.screen!=RunMenu::Custom&&run_mode.screen!=RunMenu::Daily)||detail_card>=0||detail_item>=0||settings_open||frontend_confirm)return false;
    auto&scroller=progress_page_scroll;
    if(down){
        if(hit.kind==HitKind::ProgressThumb||hit.kind==HitKind::ProgressScroll||hit.kind==HitKind::HistoryCard||hit.kind==HitKind::HistoryRelic||hit.kind==HitKind::HistoryFloor||hit.kind==HitKind::RunModifier){
            pointer.held=true;pointer.dragging=false;pointer.captured=hit;pointer.down_x=pointer.x;pointer.down_y=pointer.y;pointer.context=input_context();pointer.revision=game.revision;
            scroller.dragging=true;scroller.thumb=hit.kind==HitKind::ProgressThumb;scroller.last_y=pointer.y;
            if(scroller.thumb)scroller.set_thumb(pointer.y,run_mode.screen==RunMenu::Custom?42:61,run_mode.screen==RunMenu::Custom?158:128);return true;
        }
        return false;
    }
    if(scroller.dragging){const bool dragged=scroller.thumb||pointer.dragging||pointer.captured.kind==HitKind::ProgressScroll;scroller.dragging=scroller.thumb=false;if(dragged){pointer_cancel();return true;}}
    return false;
}
void App::history_pointer(const HitRegion&hit){
    if(!history_open)return;const auto*record=history_at(history_index);if(!record)return;
    if(hit.kind==HitKind::HistoryCard&&hit.value>=0&&hit.value<record->deck_count){
        const auto&card=record->deck[hit.value];history_selected=hit.value;open_card_offer(card.card,card.facts,card.genetic,card.scythe);
    }else if(hit.kind==HitKind::HistoryRelic&&hit.value>=0&&hit.value<record->relic_count){
        history_presentation.focus=int16_t(UiHistoryLayout::RelicFirst+hit.value);
    }
}
