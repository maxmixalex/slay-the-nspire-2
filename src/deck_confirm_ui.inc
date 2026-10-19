// A larger original-framed card keeps upgrade text inside the card itself.
static void comparison_card(Canvas&c,Card card,int x,int y,int deck_index=-1){
    const int stars=card_star_cost(c.a.game,card),header=stars?16:0;
    c.picture(c.a.content.frame_image(card),x,y,118,166);
    c.picture(c.a.content.card_image(card.kind),x+6,y+27+header,106,77-header);
    c.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,x,y+1+header,118,24);
    char title[128]={};TextBuffer t{title,sizeof title,0};t.text(c.a.content.card_string(card.kind));if(card.upgraded)t.ch('+');
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(title,x+5+dx,y+3+header+dy,108,12,Ink);
    c.centered(title,x+5,y+3+header,108,12,card.upgraded?Green:White);
    c.picture(c.a.content.energy_image(card),x-4,y-6,24,24);c.energy_label(cost(card),x+4,y-4,16);
    if(stars)c.star_label(stars,x+24,y-4,12);
    char text[512]={};c.a.card_description(card,-1,text,sizeof text,-1,deck_index);
    c.dim(x+6,y+106,106,53,90);c.paragraph(text,x+10,y+108,98,4,0,10,White);
    if(c.paragraph(text,0,0,98,0,0,10)>4)c.text("…",x+101,y+150,10,Gold);
}
// Confirm irreversible deck operations with the captured revision still valid.
static void deck_confirm_screen(Canvas&c){
    auto&a=c.a;if(!a.deck_candidate_valid()){a.deck_candidate=-1;return;}
    auto&s=a.game;Card before=s.deck[a.deck_candidate];a.hit_count=0;
    c.dim(0,0,320,240,235);
    const bool upgrade=s.phase==Phase::Upgrade,remove=s.phase==Phase::Remove||a.grimoire_select;
    c.centered(upgrade?"确认升级":remove?"确认永久移除":"确认变化",0,5,320,20,Gold);
    if(upgrade){
        comparison_card(c,before,18,35,a.deck_candidate);Card after=before;after.upgraded=1;comparison_card(c,after,185,35,a.deck_candidate);
        c.text("→",147,104,20,Gold);
    }else{
        large_card(c,before,120,49,true,-2,-2,-1,a.deck_candidate);
        char text[128]={};TextBuffer b{text,sizeof text,0};
        if(remove){
            if(a.grimoire_select)b.text("禁忌魔典 · 不消耗金币");
            else if(s.event_deck_action==EventDeckAction::Remove)b.text("事件移除 · 不消耗金币");
            else{b.text("永久移除 · 花费 ");b.number(remove_price(s));b.text(" 金币");}
        }
        else b.text("替换为本职业牌池中的另一张牌");
        c.centered(text,4,176,312,10,White);
    }
    c.button("取消",13,207,121,false,28);c.action(8,202,130,38,UiAction::CancelDeckChoice);
    c.button(upgrade?"确认升级":remove?"确认移除":"确认变化",180,207,127,true,28);
    c.action(176,202,137,38,UiAction::ConfirmDeckChoice);
}
