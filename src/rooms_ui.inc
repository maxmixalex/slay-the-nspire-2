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
#include "events_ui.inc"

static void deck_select(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.dim(0,20,320,220,225);
    const bool grimoire=a.grimoire_select&&s.phase==Phase::Reward&&s.necro.grimoire_pending;
    const char* title=grimoire?"禁忌魔典：永久移除一张牌":s.phase==Phase::Upgrade?"选择一张牌升级":s.phase==Phase::Remove?"移除一张牌":"选择一张牌变化";
    c.centered(title,0,25,320,16,Gold);
    a.cursor=mn(mx(0,a.cursor),mx(0,s.deck_count-1));
    auto&grid=a.configure_card_grid(CardGridSurface::DeckChoice,s.deck_count,s.revision);CardGridPainter painter{c,grid};const int first=grid.first();
    int portraits[16];unsigned portrait_count=0;
    for(int i=first;i<grid.last();++i)portraits[portrait_count++]=a.card_face_image(s.deck[i],-1,i);
    a.content.prepare_images(portraits,portrait_count);
    for(int i=first;i<grid.last();i++){
        Card card=s.deck[i];int x=grid.card_x(i),y=grid.card_y(i);
        bool allowed=s.phase==Phase::Upgrade?card_upgradeable(card):s.phase==Phase::Transform?(i!=s.event_first&&card_transformable(card)):(card_removable(card)&&s.deck_count>1);
        const bool hover=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+55&&a.pointer.y>=y-2&&a.pointer.y<y+77&&a.pointer.y>=grid.top&&a.pointer.y<grid.bottom;
        painter.card(i,card,hover||(!a.pointer.visible&&a.cursor==i),allowed,-2,-2,-1,i);
        painter.hit(i,HitKind::Choice,i,allowed);
    }
    if(grimoire){char text[96]={};TextBuffer b{text,sizeof text,0,&c.a.content};b.text("剩余 ");b.number(s.necro.grimoire_pending);b.text(a.content.english()?"":" 次");c.centered(text,0,196,320,10,Muted);}
    else if(s.phase==Phase::Transform){char text[64]={};TextBuffer b{text,sizeof text,0,&c.a.content};b.text("还需变化 ");b.number(s.event_remaining);b.text(a.content.english()?" cards":" 张");c.centered(text,0,196,320,10,Muted);}
    painter.finish();card_grid_scrollbar(c,grid,CardGridSurface::DeckChoice);
    if(grimoire){c.button("返回",44,213,78,false,24);c.action(42,210,82,30,UiAction::Back);
        c.button("放弃一次",194,213,81,false,24);c.action(192,210,83,30,UiAction::SkipGrimoire);}
}
