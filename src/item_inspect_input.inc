// NInspectRelicScreen copies the supplied ordered models and clamps navigation.
// Its owners are stable while the modal blocks gameplay; keep indices rather
// than duplicating an inventory or storing anything in canonical State.
void App::sync_relic_inspect(){
    if(detail_item<0||detail_item>=WorldRelicKinds)return;
    if(relic_inspect.bound&&!relic_inspect.exiting&&relic_inspect.model==detail_item&&relic_inspect.owned==detail_owned)return;
    relic_inspect.begin(map_clock_ms);auto&view=relic_inspect;
    view.model=int16_t(detail_item);view.owned=int16_t(detail_owned);
    if(library&&library_tab==1){
        prepare_collection();for(unsigned i=0;i<collection.count;++i)if(collection.ids[i]==detail_item){
            view.source=RelicInspectSource::Collection;view.index=int16_t(i);view.count=int16_t(collection.count);break;
        }
    }else if(detail_owned>=0&&detail_owned<int(ui_owned_relic_count(game))&&ui_owned_relic_id(game,detail_owned)==detail_item){
        view.source=RelicInspectSource::Owned;view.index=int16_t(detail_owned);view.count=int16_t(ui_owned_relic_count(game));
    }
    if(!library&&!history_open)discover_relic(RelicId(detail_item));
    keyword_hover={};
}
void App::open_relic_inspect(RelicId id,int owned){
    if(id>=WorldRelicKinds||detail_card>=0||detail_item>=0||settings_open||frontend_confirm||ftue.active())return;
    detail_item=int(id);detail_owned=owned;detail_scroll=0;pointer_cancel();sync_relic_inspect();
}
void App::close_relic_inspect(){
    if(detail_item<0)return;
    if(detail_item<WorldRelicKinds){sync_relic_inspect();relic_inspect.close(map_clock_ms);}
    detail_item=detail_owned=-1;detail_scroll=0;keyword_hover={};pointer_cancel();
}
void App::move_relic_inspect(int direction){
    if(detail_item<0||detail_item>=WorldRelicKinds||!direction)return;
    sync_relic_inspect();auto&view=relic_inspect;int next=view.index+(direction<0?-1:1);
    if(next<0)next=0;if(next>=view.count)next=view.count-1;
    if(view.source==RelicInspectSource::Owned){
        if(next<0||next>=int(ui_owned_relic_count(game)))return;
        detail_owned=next;detail_item=ui_owned_relic_id(game,unsigned(next));
    }else if(view.source==RelicInspectSource::Collection){
        if(next<0||next>=int(collection.count))return;detail_item=collection.ids[next];detail_owned=-1;
    }
    // The original keyboard handler also animates a clamped first/last item;
    // invisible pointer arrows have no hit region and cannot wrap around.
    view.index=int16_t(next);view.model=int16_t(detail_item);view.owned=int16_t(detail_owned);
    view.direction=int8_t(direction<0?-1:1);view.slid=map_clock_ms;keyword_hover={};pointer_cancel();
}
bool App::relic_inspect_input(Key key){
    if(detail_item<0)return false;
    sync_relic_inspect();
    if(key==Key::Back)close_relic_inspect();
    else if(key==Key::Left||key==Key::Right)move_relic_inspect(key==Key::Left?-1:1);
    else if(key==Key::Up||key==Key::Down)keyword_hover_scroll(*this,key==Key::Up?-1:1);
#if SPIRE_DEBUG_COMMANDS
    else if(key==Key::EndTurn&&library&&!title&&!paused&&!help&&detail_item>=WorldRelicKinds)ui_action(UiAction::LabPotion);
#endif
    return true;
}
