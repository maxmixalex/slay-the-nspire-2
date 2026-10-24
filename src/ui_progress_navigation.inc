void App::open_compendium(){
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
    compendium_open=true;compendium_selection=5;history_open=true;statistics_open=library=false;history_index=0;history_selected=-1;
    progress_page_started=map_clock_ms;progress_page_scroll.reset();pointer_cancel();
}
void App::history_select(int direction){
    if(!history_open||!progress.count)return;
    const int next=int(history_index)+direction;
    if(next<0||next>=int(progress.count))return;
    history_index=uint8_t(next);history_selected=-1;progress_page_scroll.reset();pointer_cancel();
}
bool App::progress_navigation_input(Key key){
    if(library||bestiary.open||detail_card>=0||detail_item>=0)return false;
    if(statistics_open||history_open){
        if(key==Key::Back||key==Key::Collection){statistics_open=history_open=false;pointer_cancel();return true;}
        if(key==Key::Up||key==Key::Down)progress_page_scroll.wheel(key==Key::Up?-4:4);
        else if(history_open&&(key==Key::Left||key==Key::Right))history_select(key==Key::Left?1:-1);
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
    if((!statistics_open&&!history_open)||detail_card>=0||detail_item>=0||settings_open||frontend_confirm)return false;
    auto&scroller=progress_page_scroll;
    if(down){
        if(hit.kind==HitKind::ProgressThumb||hit.kind==HitKind::ProgressScroll){
            pointer.held=true;pointer.dragging=false;pointer.captured=hit;pointer.down_x=pointer.x;pointer.down_y=pointer.y;
            scroller.dragging=true;scroller.thumb=hit.kind==HitKind::ProgressThumb;scroller.last_y=pointer.y;
            if(scroller.thumb)scroller.set_thumb(pointer.y);return true;
        }
        return false;
    }
    if(scroller.dragging){scroller.dragging=scroller.thumb=false;pointer_cancel();return true;}
    return false;
}
void App::history_pointer(const HitRegion&hit){
    if(!history_open)return;const auto*record=history_at(history_index);if(!record)return;
    if(hit.kind==HitKind::HistoryCard&&hit.value>=0&&hit.value<record->deck_count){
        const auto&card=record->deck[hit.value];history_selected=hit.value;open_card_offer(card.card,card.facts,card.genetic,card.scythe);
    }else if(hit.kind==HitKind::HistoryRelic&&hit.value>=0&&hit.value<record->relic_count){
        detail_owned=-1;detail_item=record->relics[hit.value];detail_scroll=0;pointer_cancel();
    }
}
