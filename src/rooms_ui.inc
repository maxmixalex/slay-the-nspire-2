static int room_asset(Room r){switch(r){case Room::Combat:return A_MapCombat;case Room::Event:return A_MapEvent;case Room::Shop:return A_MapShop;case Room::Camp:return A_MapCamp;case Room::Treasure:return A_MapTreasure;default:return A_MapFinal;}}
static const char* room_name(Room r){switch(r){case Room::Combat:return "战斗";case Room::Event:return "未知事件";case Room::Shop:return "商店";case Room::Camp:return "休息处";case Room::Treasure:return "宝藏";default:return "最后的战斗";}}
static void map_screen(Canvas&c,bool inspect=false){
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
            if(node.visited)c.picture(s.character==Character::Silent?a.content.ui_image(U_SilentMapMarker):A_MapMarker,xs[col]-12,y-11,23,23);
            else c.picture(room_asset(node.room),xs[col]-12,y-12,24,24,allowed?255:row<(int)s.next_row?70:155);
            if(selected)c.text("▼",xs[col]-5,y-31,10,Ink);
            if(allowed&&!inspect)c.hit(xs[col]-19,y-19,38,38,HitKind::Choice,col);
        }
    }
    c.rect(0,207,320,33,Panel);
    c.text("↑",299,45,12,Ink);c.hit(294,41,26,25,HitKind::Scroll,-1);
    c.text("↓",299,180,12,Ink);c.hit(294,179,26,25,HitKind::Scroll,1);
    if(inspect)c.centered("路线预览 · ↑↓ 滚动 · M / Esc 返回",0,214,320,12,Gold);
    else{char b[64]={};TextBuffer t{b,sizeof b,0};t.text("第 ");t.number(s.next_row+1);t.text(" 层  ");t.text(room_name(s.map[s.next_row*3+a.cursor].room));c.centered(b,0,207,320,12,Gold);c.centered("←→ 选路   Enter 进入   ↑↓ 查看远方",0,225,320,10,Muted);}
}
static int event_text(const State&s){
    if(s.event_id==EventId::Baths){if(s.event_page==0)return 45;if(s.event_page==2)return 47;if(s.event_page==3)return 48;return s.event_count<=1?46:48+mn(9,s.event_count-1);}
    if(s.event_id==EventId::Vegetation)return s.event_page==0?59:s.event_page==4?61:60;
    return s.event_page==0?63:s.event_page==2?64:65;
}
static void event_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;int e=(int)s.event_id;
    c.picture(A_EventBaths+e,0,20);if(e)c.picture(A_EventVegetationForeground+e-1,0,20);
    c.dim(0,20,320,27,110);c.centered(a.content.string(e==0?44:e==1?58:62),0,23,320,16,Gold);
    c.dim(4,73,312,99,210);int total=c.paragraph(a.content.string(event_text(s)),12,78,293,0);
    a.scroll=mn(a.scroll,mx(0,total-6));c.paragraph(a.content.string(event_text(s)),12,78,293,6,a.scroll);
    c.hit(298,74,21,23,HitKind::Scroll,-1);c.hit(298,146,21,23,HitKind::Scroll,1);
    if(total>6){c.rect(313,79,2,84,rgb(77,74,63));c.rect(313,79+76*a.scroll/mx(1,total-6),2,8,Gold);}
    if(s.event_page>=2){c.button(e==1&&s.event_page==4?"战！":"继续前进",35,201,250,true,29);c.hit(35,201,250,29,HitKind::Choice,0);return;}
    for(int i=0;i<2;i++){
        int y=177+i*31;bool selected=a.cursor==i;char b[150]={};TextBuffer t{b,sizeof b,0};const char*title="";
        if(e==0){title=i?(s.event_page?"离开浴场":"敬而远之"):(s.event_page?"沉溺":"投身其中");
            if(i)t.text(s.event_page?"保留已获得的最大生命":"[green]回复 10 点生命[/green]");else{t.text("[green]最大生命 +2[/green]  [red]伤害 ");t.number(3+s.event_count);t.text("[/red]");}}
        else if(e==1){title=i?"休息":"坚持跋涉";if(i){t.text("[green]回复 ");t.number((s.max_hp*3)/10);t.text(" 生命[/green]，进入战斗");}else{t.text("[gold]金币 +");t.number(s.event_value);t.text("[/gold]  [red]生命 -8[/red]");}}
        else{title=i?"落单变形灵":"大群变形灵";t.text(i?"[green]最大生命 +5[/green]":"[red]失去全部金币[/red]，变化 2 张牌");}
        c.hit(5,y,310,29,HitKind::Choice,i);c.rect(5,y,310,29,selected?rgb(65,52,37):Panel);c.box(5,y,310,29,selected?Gold:rgb(77,71,61));
        if(selected){c.rect(5,y,3,29,Gold);c.text("›",11,y+4,16,Gold);}
        c.text(title,25,y,12,selected?Gold:White,274);c.paragraph(b,25,y+14,280,1,0,10);
    }

}
static const char* relic_name(int r){auto*d=relic_definition(r);return d?d->name:"售罄";}
static const char* relic_desc(int r){auto*d=relic_definition(r);return d?d->description:"没有剩余的已实现遗物。";}
static const char* potion_name(int p){auto*d=potion_definition((Potion)p);return d?d->name:"空药水槽";}
static const char* potion_desc(int p){auto*d=potion_definition((Potion)p);return d?d->description:"药水栏为空。可在商店购买或领取战斗掉落。";}
static void shop_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_ShopBackground,0,22,320,218);c.dim(0,22,320,218,35);
    c.ui(U_ShopRug,4,39,312,182);
    c.clip(AnimMerchantTop,258,26,a.animation*33,false);c.text("商店",10,42,16,Gold);
    // Five class cards on the upper shelf, two colorless cards below left.
    for(int i=0;i<ShopCardCount;i++){
        int x=i<5?16+i*56:16+(i-5)*56,y=i<5?65:143;
        bool sold=s.shop_sold&(1u<<i);
        small_card(c,{s.shop_cards[i],0},x,y,a.cursor==i,!sold,0);
        if(sold)c.centered("售罄",x,y+62,47,10,Muted);
        else {c.picture(A_Coin,x+2,y+64,10,10);c.num(s.shop_prices[i],x+15,y+62,10,s.gold>=s.shop_prices[i]?Gold:Red);}
        c.hit(x-2,y-2,51,79,HitKind::Choice,i,!sold);
    }
    for(int i=0;i<3;i++){
        int x=145+i*55,item=ShopRelicStart+i;unsigned id=s.shop_relics[i];int index=-1;
        for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==id)index=r;
        bool sold=s.shop_sold&(1u<<item);
        if(a.cursor==item)c.box(x-4,143,43,37,Gold);
        if(index>=0)c.picture(a.content.relic_image(index),x+4,143,25,25,sold?60:255);
        if(sold)c.text("售罄",x+4,168,10,Muted);else c.num(s.shop_prices[item],x+3,166,10,s.gold>=s.shop_prices[item]?Gold:Red);
        c.hit(x-4,142,46,38,HitKind::Choice,item,!sold);
        item=ShopPotionStart+i;sold=s.shop_sold&(1u<<item);
        if(a.cursor==item)c.box(x-4,183,43,36,Gold);
        c.picture(a.content.potion_image((Potion)s.shop_potions[i]),x+6,182,20,24,sold?60:255);
        if(sold)c.text("售罄",x+4,207,10,Muted);else c.num(s.shop_prices[item],x+6,205,10,s.gold>=s.shop_prices[item]?Gold:Red);
        c.hit(x-4,181,46,38,HitKind::Choice,item,!sold);
    }
    c.rect(0,220,320,20,Panel);
    char remove[48]={};TextBuffer r{remove,sizeof remove,0};r.text("移除卡牌  ");r.number(remove_price(s));
    c.text((s.shop_sold&(1u<<ShopRemove))?"已移除卡牌":remove,8,221,10,a.cursor==ShopRemove?Gold:White,149);
    c.hit(0,220,163,20,HitKind::Choice,ShopRemove,!(s.shop_sold&(1u<<ShopRemove)));
    c.text("详情 I",181,221,10,Gold);c.hit(168,220,76,20,HitKind::Command,(int)Key::Detail,a.cursor<ShopRemove);
    c.text("离开 ›",266,221,10,a.cursor==ShopLeave?Gold:White);c.hit(250,220,70,20,HitKind::Choice,ShopLeave);
    // Item tooltip appears over the empty header, never over the target itself.
    const char* name="";int price=0;
    if(a.cursor<ShopCardCount){name=a.content.card_string(s.shop_cards[a.cursor]);price=s.shop_prices[a.cursor];}
    else if(a.cursor<ShopPotionStart){name=relic_name(s.shop_relics[a.cursor-ShopRelicStart]);price=s.shop_prices[a.cursor];}
    else if(a.cursor<ShopRemove){auto*d=potion_definition((Potion)s.shop_potions[a.cursor-ShopPotionStart]);name=d?d->name:"";price=s.shop_prices[a.cursor];}
    else name=a.cursor==ShopRemove?"永久移除一张牌":"继续前进";
    char info[100]={};TextBuffer t{info,sizeof info,0};t.text(name);if(price){t.text("  ");t.number(price);t.text(" 金币");}
    c.dim(68,41,226,20,200);c.centered(info,68,42,226,10,Gold);
    if(a.pointer.visible&&!a.pointer.held&&a.cursor>=ShopRelicStart&&a.cursor<ShopRemove){
        const char*desc=a.cursor<ShopPotionStart?relic_desc(s.shop_relics[a.cursor-ShopRelicStart]):potion_desc(s.shop_potions[a.cursor-ShopPotionStart]);
        c.rect(12,65,296,59,Panel);c.box(12,65,296,59,Gold);c.text(name,21,68,12,Gold,280);c.paragraph(desc,21,87,278,2,0,10);
    }
}

static void deck_select(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.dim(0,20,320,220,225);
    const char* title=s.phase==Phase::Upgrade?"选择一张牌升级":s.phase==Phase::Remove?"移除一张牌":"选择一张牌变化";
    c.centered(title,0,25,320,16,Gold);
    a.cursor=mn(mx(0,a.cursor),s.deck_count-1);int first=a.cursor/10*10;
    for(int i=first;i<mn(s.deck_count,first+10);i++){
        Card card=s.deck[i];int x=24+(i-first)%5*55,y=54+(i-first)/5*73;
        bool allowed=s.phase==Phase::Upgrade?!card.upgraded:s.phase==Phase::Transform?i!=s.event_first:true;
        small_card(c,card,x,y,a.cursor==i,allowed,0);
        c.hit(x-3,y-2,53,71,HitKind::Choice,i,allowed);
    }
    if(s.phase==Phase::Transform){char text[64]={};TextBuffer b{text,sizeof text,0};b.text("还需变化 ");b.number(s.event_remaining);b.text(" 张 · 确认前不会扣除卡牌");c.centered(text,0,196,320,10,Muted);}
    else c.centered("方向键选牌 · I 详情 · Enter 预览并确认",0,196,320,10,Muted);
    int page=first/10,last=(s.deck_count-1)/10;
    c.text("‹",12,214,16,Gold);c.action(0,210,43,30,UiAction::PreviousDeckPage,page>0);
    c.text("›",296,214,16,Gold);c.action(277,210,43,30,UiAction::NextDeckPage,page<last);
    char label[24]={};TextBuffer b{label,sizeof label,0};b.number(page+1);b.text(" / ");b.number(last+1);c.centered(label,112,218,96,10,Muted);
}
