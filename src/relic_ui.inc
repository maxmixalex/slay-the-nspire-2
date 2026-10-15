// Original-derived relic art and panels, adapted to the calculator's resolution.
static void relic_progress(const State&s,unsigned id,char*out,size_t size){
    if(!size)return;out[0]=0;TextBuffer b{out,size,0};unsigned limit=relic_counter_limit(id);
    if(!limit)return;
    if(id==CentennialPuzzle){b.text(relic_counter(s,id)?"本场已触发":"本场尚未触发");return;}
    b.text(relic_counter_is_turn_scoped(id)?"本回合 ":"累计 ");b.number(relic_counter(s,id));b.text(" / ");b.number(limit);
}
static void relic_badge(Canvas&c,unsigned id,int x,int y){
    unsigned limit=relic_counter_limit(id);if(!limit)return;
    if(relic_counter_is_turn_scoped(id)&&!combat_phase(c.a.game.phase))return;
    unsigned value=relic_counter(c.a.game,id);
    bool active=c.a.relic_flash_ticks>0&&(c.a.relic_flash_mask&id);
    if(active)value=limit;
    c.rect(x,y,13,13,Panel);c.num(value,x+1,y-1,10,active?Green:Gold);
}
static void owned_relic_strip(Canvas&c){
    auto&a=c.a;auto&s=a.game;
    for(int i=0;i<s.relic_count&&i<15;i++){
        int x=3+i*19;unsigned index=s.relic_order[i],id=relic_definitions[index].id;
        bool active=a.relic_flash_ticks>0&&(a.relic_flash_mask&id);
        if(active){c.box(x-1,23,19,20,Gold);c.rect(x+2,42,13,1,Green);}
        c.picture(a.content.relic_image(index),x,24,17,17);
        relic_badge(c,id,x+7,31);c.hit(x-1,22,19,22,HitKind::OwnedRelic,i);
    }
    if(s.relic_count>15){char more[16]={};TextBuffer b{more,sizeof more,0};b.ch('+');b.number(s.relic_count-15);
        c.text(more,290,25,10,Gold);c.hit(287,22,33,22,HitKind::Command,(int)Key::Relics);}
}
static void owned_relic_panel(Canvas&c){
    auto&a=c.a;auto&s=a.game;const int count=s.relic_count;
    a.scroll=mn(mx(0,a.scroll),mx(0,count-1));int first=a.scroll/18*18;
    c.centered("遗物",5,28,310,16,Gold);
    for(int i=first;i<mn(count,first+18);i++){
        int j=i-first,x=20+j%6*49,y=55+j/6*35;
        unsigned index=s.relic_order[i],id=relic_definitions[index].id;
        if(i==a.scroll){c.rect(x-4,y-2,38,33,rgb(75,58,37));c.box(x-4,y-2,38,33,Gold);}
        c.picture(a.content.relic_image(index),x,y,25,25);relic_badge(c,id,x+20,y+16);
        c.hit(x-5,y-3,46,34,HitKind::Relic,i);
    }
    if(count){const auto&d=relic_definitions[s.relic_order[a.scroll]];
        c.centered(d.name,15,161,290,12,Gold);
        char progress[96]={};relic_progress(s,d.id,progress,sizeof progress);
        if(*progress)c.centered(progress,15,180,290,10,Green);
        c.paragraph(d.description,20,*progress?197:182,280,*progress?1:2,0,10,White);
    }else c.centered("没有遗物",0,106,320,12,Muted);
    c.text("‹",15,216,16,Gold);c.hit(7,211,35,24,HitKind::Scroll,-18,first>0);
    c.text("›",292,216,16,Gold);c.hit(278,211,35,24,HitKind::Scroll,18,first+18<count);
    char page[64]={};TextBuffer b{page,sizeof page,0};b.number(first/18+1);b.text(" / ");b.number(mx(1,(count+17)/18));b.text(" · 点击 / I 查看详情");
    c.centered(page,41,219,236,10,Muted);
}
static void owned_relic_tooltip(Canvas&c){
    auto&a=c.a;
    if(a.event_reader||!a.pointer.visible||a.pointer.held||a.title||a.paused||a.help||a.panel||a.library||
       a.character_select||a.map_preview.open||a.status_actor>=0||a.detail_card>=0||a.detail_item>=0||
       a.deck_candidate>=0||choice_phase(a.game.phase))return;
    const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);
    if(!hit||hit->kind!=HitKind::OwnedRelic||hit->value<0||hit->value>=a.game.relic_count)return;
    const auto&d=relic_definitions[a.game.relic_order[hit->value]];
    int x=mn(mx(4,a.pointer.x-66),150),y=47;
    c.rect(x,y,166,87,Panel);c.box(x,y,166,87,Gold);
    c.text(d.name,x+8,y+5,12,Gold,150);
    char progress[80]={};relic_progress(a.game,d.id,progress,sizeof progress);
    if(*progress)c.text(progress,x+8,y+24,10,Green,150);
    c.paragraph(d.description,x+8,y+(*progress?40:25),150,3,0,10,White);
    // Read-only tooltip: do not add a hit region or alter the selected card.
}
static void treasure_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_ChestBackground,0,20);
    if(!s.treasure_state){
        c.picture(A_Chest,128,91);c.centered("发现宝藏",0,37,320,20,Gold);
        c.button("打开宝箱",35,198,250,true,28);c.hit(35,198,250,28,HitKind::Choice,0);return;
    }
    c.dim(0,22,320,218,180);c.ui(U_RewardBanner,34,29,252,48);
    const auto*d=relic_definition(s.treasure_relic);bool taken=s.treasure_state==2;
    c.centered(taken?"已领取":"宝藏",47,40,226,20,Gold);
    unsigned opacity=taken?120:220+(a.animation%16<8?a.animation%8:15-a.animation%16)*4;
    c.picture(A_MapHalo,118,69,84,84,opacity);
    int image=d?a.content.relic_image((int)(d-relic_definitions)):A_Coin;
    c.picture(image,138,88,44,44,taken?135:255);
    c.centered(d?d->name:"50 金币",13,143,294,16,Gold);
    c.paragraph(d?d->description:"已获得全部可用遗物，改为获得 [gold]50[/gold] 金币。",31,165,258,2,0,12,White);
    c.hit(119,77,82,77,HitKind::Choice,0,!taken);
    c.button(taken?"已领取":"领取",7,209,99,a.cursor==0&&!taken,27);
    c.hit(4,207,105,31,HitKind::Choice,0,!taken);
    c.button("详情 I",112,211,91,false,24);c.hit(109,207,96,31,HitKind::Command,(int)Key::Detail,d!=nullptr);
    c.ui(U_RewardSkip,214,210,100,27);c.centered(taken?"前进 ›":"跳过 ›",214,214,100,12,a.cursor==1?Gold:White);
    c.hit(208,207,112,31,HitKind::Choice,1);
}
