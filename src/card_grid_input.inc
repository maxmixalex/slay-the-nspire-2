CardGridScroll& App::configure_card_grid(CardGridSurface surface,int count,uint32_t identity){
    auto&grid=card_grids[unsigned(surface)];int top=43,bottom=207,inset=8,left=CardGridScroll::Left;
    if(surface==CardGridSurface::Pile){top=22;bottom=240;inset=pile_tab?8:27;left=pile_tab?52:57;identity=identity*8+unsigned(pile_tab);}
    else if(surface==CardGridSurface::Library){
        if(library_tab){prepare_collection();return card_grids[unsigned(surface)];}
        grid.configure(card_library.epoch,pile_sort_epoch,count,0,240,15,4,75,false);return grid;
    }
    else if(surface==CardGridSurface::DeckChoice){top=48;bottom=208;inset=6;identity=identity*32+unsigned(game.phase)+unsigned(grimoire_select)*256;}
    grid.bar_inset=surface==CardGridSurface::Pile?18:25;
    grid.configure(identity,pile_sort_epoch,count,top,bottom,inset,CardGridScroll::Columns,left);return grid;
}
void App::reset_card_grids(){for(auto&grid:card_grids)grid.reset();card_grid_drag=-1;}
int App::active_card_grid()const{
    if(paused||help||settings_open||frontend_confirm||detail_card>=0||detail_item>=0||deck_candidate>=0||event_reader||status_actor>=0)return -1;
    if(library)return int(CardGridSurface::Library);
    if(title||character_select)return -1;
    if(panel)return panel==1?int(CardGridSurface::Pile):-1;
    if(!combat_choice_ready()&&(choice_phase(game.phase)||game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice))return -1;
    if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform||grimoire_select)return int(CardGridSurface::DeckChoice);
    if(choice_phase(game.phase)&&game.phase!=Phase::WorldRelicChoice&&game.phase!=Phase::OriginalEventChoice){
        if(game.phase==Phase::ExtendedChoice&&(game.expansion.choice==XChoice::Offer||game.expansion.choice==XChoice::EnemyKnowledge||game.expansion.choice==XChoice::EventAbundance))return -1;
        return int(CardGridSurface::CombatChoice);
    }
    if(game.phase==Phase::WorldRelicChoice){
        const auto choice=game.world_relics.choice;
        if(choice==WorldRelicChoice::Potion||choice==WorldRelicChoice::OfferReward||choice==WorldRelicChoice::OfferBundle||world_choice_is_card_reward())return -1;
        if((choice==WorldRelicChoice::OfferCard||choice==WorldRelicChoice::CombatOffer)&&game.world_relics.offer_count<=3)return -1;
        return int(CardGridSurface::WorldChoice);
    }
    if(game.phase==Phase::OriginalEventChoice){
        const auto choice=game.original_events.selection;
        if(choice==OriginalEventSelection::Potion||choice==OriginalEventSelection::Rewards||choice==OriginalEventSelection::Relic||choice==OriginalEventSelection::Crystal||world_choice_is_card_reward())return -1;
        if(choice==OriginalEventSelection::OfferCard&&game.original_events.offer_count<=3)return -1;
        return int(CardGridSurface::WorldChoice);
    }
    return -1;
}
void App::focus_card_grid(){
    if(library&&library_tab){focus_collection();return;}
    const int surface=active_card_grid();if(surface<0)return;
    const int selected=surface==int(CardGridSurface::Pile)?inspect_cursor:surface==int(CardGridSurface::Library)?library_selected:cursor;
    card_grids[surface].focus(selected);
}
bool App::card_grid_input(Key key){
    if(library)return library_input(key);
    if(pointer.dispatching||(key!=Key::Left&&key!=Key::Right&&key!=Key::Up&&key!=Key::Down))return false;
    const int surface=active_card_grid();if(surface<0)return false;
    if(surface==int(CardGridSurface::Pile)&&pile_control_focus>=0)return false;
    int count=0;
    if(surface==int(CardGridSurface::Pile))count=int(pile_items());
    else if(surface==int(CardGridSurface::Library))count=int(library_items());
    else if(surface==int(CardGridSurface::DeckChoice))count=game.deck_count;
    else if(surface==int(CardGridSurface::CombatChoice))count=choice_pile(game).count;
    else count=int(world_choice_options(nullptr,0));
    auto&grid=configure_card_grid(CardGridSurface(surface),count,game.revision);
    int&selected=surface==int(CardGridSurface::Pile)?inspect_cursor:surface==int(CardGridSurface::Library)?library_selected:cursor;
    if(!count){selected=0;return true;}selected=CardGridScroll::clamp(selected,0,count-1);
    const int first=selected/grid.columns*grid.columns,last=CardGridScroll::clamp(first+grid.columns,0,count);
    // NCardGrid wraps horizontally within its row. Vertical focus stays in
    // its column, including at a short final row.
    if(key==Key::Left)selected=selected>first?selected-1:last-1;
    else if(key==Key::Right)selected=selected+1<last?selected+1:first;
    else if(key==Key::Up){
        if(selected<grid.columns&&surface==int(CardGridSurface::Pile)&&!pile_tab){pile_control_focus=selected;return true;}
        if(selected>=grid.columns)selected-=grid.columns;
    }else if(key==Key::Down&&selected+grid.columns<count)selected+=grid.columns;
    grid.focus(selected);return true;
}
