#pragma once
#include "app.hpp"

namespace spire_test {
inline void discover_card(spire::App&a,spire::CardKind kind){
    const unsigned index=unsigned(kind)/8;const auto bit=uint8_t(1u<<(unsigned(kind)%8));
    a.progress.discovered_cards[index]|=bit;
    // Census fixtures establish a catalog's initial observations explicitly.
    // Runtime observation tests use App::discover_card and keep the snapshot.
    a.card_library.seen[index]|=bit;a.card_library.invalidate();
}
inline void discover_all_cards(spire::App&a){for(unsigned i=0;i<spire::CardKinds;++i)discover_card(a,spire::CardKind(i));}
inline bool choose_library_pool(spire::App&a,int filter){
    static constexpr int filters[]={1,2,5,6,4,3,7,8};
    for(int i=0;i<8;++i)if(filter==filters[i]){if(a.library_filter!=filter)a.library_control(spire::LibraryPoolFirst+i);return a.library_filter==filter;}return false;
}
// Census fixtures create observation metadata explicitly, then locate the
// model through one of the original eight pool filters. There is no All tab.
inline int library_model_index(spire::App&a,spire::CardKind kind){
    if(!a.library||a.library_tab)a.open_library(0);discover_card(a,kind);
    static constexpr int filters[]={1,2,5,6,4,3,7,8};spire::CardKind ids[spire::CardKinds];
    for(const int filter:filters){if(!choose_library_pool(a,filter))return -1;const unsigned count=a.library_items(ids);for(unsigned i=0;i<count;++i)if(ids[i]==kind)return int(i);}return -1;
}
// Content tests may position an isolated model before exercising its real
// click. Scrolling/navigation itself is covered separately through pointer
// wheel, content drag, draggable thumb and directional input.
inline bool settle_card_grid(spire::App&a){
    a.render();const int index=a.active_card_grid();if(index<0)return false;
    auto&grid=a.card_grids[index];
    for(unsigned i=0;i<48&&(grid.position!=grid.target||grid.target<grid.minimum()||grid.target>grid.maximum());++i)a.advance(33);
    a.render();return grid.position==grid.target&&grid.target>=grid.minimum()&&grid.target<=grid.maximum();
}
inline bool focus_card_grid(spire::App&a){a.render();a.focus_card_grid();return settle_card_grid(a);}
inline bool drag_grid_thumb(spire::App&a,bool bottom){
    a.render();const int index=a.active_card_grid();if(index<0)return false;
    auto&grid=a.card_grids[index];if(!grid.has_scrollbar())return false;
    a.pointer_move(305,grid.thumb_y());a.pointer_button(true);
    if(a.pointer.captured.kind!=spire::HitKind::GridScroll)return false;
    a.pointer_move(305,bottom?grid.bar_bottom():grid.bar_top());a.pointer_button(false);
    return settle_card_grid(a)&&grid.position==(bottom?grid.maximum():grid.minimum());
}
}
