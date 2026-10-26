static int room_asset(Room r){switch(r){case Room::Combat:return A_MapCombat;case Room::Event:return A_MapEvent;case Room::Shop:return A_MapShop;case Room::Camp:return A_MapCamp;case Room::Treasure:return A_MapTreasure;default:return A_MapFinal;}}
static const char* room_name(Room r){switch(r){case Room::Combat:return "战斗";case Room::Event:return "未知事件";case Room::Shop:return "商店";case Room::Camp:return "休息处";case Room::Treasure:return "宝藏";default:return "最后的战斗";}}
#include "world_map_ui.inc"
static void map_screen(Canvas&c,bool inspect=false){
    if(c.a.game.world.active){world_map_screen(c,inspect);return;}
    auto&a=c.a;auto&s=a.game;c.picture(A_MapBackground,0,0);c.centered("蔓生之地 · 远征路线",0,24,320,12,Ink);
    int natural=mx(0,mn(MapRows-4,(int)s.next_row-1));int base=mx(0,mn(MapRows-4,natural+a.map_scroll));
    a.map_scroll=base-natural;const int xs[3]={66,159,252};uint16_t path=rgb(99,78,55),faint=rgb(119,93,63);
    for(int row=base;row<base+3;row++)for(int col=0;col<3;col++){
        const auto&node=s.map[row*3+col];int y=183-(row-base)*44;
        for(int j=0;j<3;j++)if(node.links&(1<<j))c.line(xs[col],y-10,xs[j],y-34,node.visited&&s.map[(row+1)*3+j].visited?Ink:faint,true);
    }
    for(int row=base;row<base+4;row++){
        int y=183-(row-base)*44;c.num(row+1,15,y-8,10,path);
        for(int col=0;col<3;col++){
            int id=row*3+col;const auto&node=s.map[id];bool allowed=reachable(s,id),selected=allowed&&col==a.cursor&&!inspect;
            if(selected)c.picture(A_MapHalo,xs[col]-21,y-21,43,43,(unsigned)(210+(a.animation%30<15?a.animation%15:14-a.animation%15)*3));
            if(node.visited)c.picture(character_map_image(a.content,s.character),xs[col]-12,y-11,23,23);
            else c.picture(room_asset(node.room),xs[col]-12,y-12,24,24,allowed?255:row<(int)s.next_row?70:155);
            if(selected)c.text("▼",xs[col]-5,y-31,10,Ink);
            if(allowed&&!inspect)c.hit(xs[col]-19,y-19,38,38,HitKind::Choice,col);
        }
    }
    c.rect(0,207,320,33,Panel);
    c.text("↑",299,45,12,Ink);c.hit(294,41,26,25,HitKind::Scroll,-1);
    c.text("↓",299,180,12,Ink);c.hit(294,179,26,25,HitKind::Scroll,1);
    if(inspect)c.centered("路线预览 · ↑↓ 滚动 · M / Esc 返回",0,214,320,12,Gold);
    else{char b[64]={};TextBuffer t{b,sizeof b,0,&c.a.content};t.text("第 ");t.number(s.next_row+1);t.text(" 层  ");t.text(room_name(s.map[s.next_row*3+a.cursor].room));c.centered(b,0,207,320,12,Gold);c.centered("←→ 选路   Enter 进入   ↑↓ 查看远方",0,225,320,10,Muted);}
}
#include "merchant_ui.inc"
#include "fake_merchant_ui13.inc"
#include "events_ui.inc"

static void deck_select(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.sync_deck_selection();
    const unsigned shade=217*room_curve::cubic_out(a.map_clock_ms-a.deck_selection.opened,500)/4096;
    Canvas backstop=c;backstop.opacity=uint8_t(div255(unsigned(backstop.opacity)*shade));backstop.rect(0,0,320,240,0);
    const bool grimoire=a.grimoire_select&&s.phase==Phase::Reward&&s.necro.grimoire_pending;
    uint16_t indices[MaxDeck];const unsigned count=a.deck_choice_items(indices,MaxDeck);
    auto&grid=a.configure_card_grid(CardGridSurface::DeckChoice,int(count),s.revision);CardGridPainter painter{c,grid,a.deck_candidate<0};const int first=grid.first();
    int portraits[24];unsigned portrait_count=0;
    for(int i=first;i<grid.last();++i){const int index=indices[i];portraits[portrait_count++]=a.card_face_image(s.deck[index],-1,index);}
    a.content.prepare_images(portraits,portrait_count);
    for(int i=first;i<grid.last();i++){
        const int index=indices[i];Card card=s.deck[index];const bool preview=s.phase==Phase::Upgrade&&a.deck_selection.show_upgrades;
        if(preview)card.upgraded=1;
        painter.card(i,card,a.cursor==index,true,-2,-2,-1,index,nullptr,false,true,preview);
        painter.hit(i,HitKind::Choice,index);
    }
    painter.finish();if(a.deck_candidate<0)card_grid_scrollbar(c,grid,CardGridSurface::DeckChoice);
    if(a.deck_candidate<0&&a.deck_selection_cancelable())native_original_back(c,a.deck_selection.opened);
    if(grimoire)native_original_confirm(c,UiAction::SkipGrimoire,a.deck_selection.opened);
    deck_selection_prompt(c);deck_selection_upgrades(c);
}
