// A mandatory card choice is a separate interaction layer, not a hand-hover mode.
static void card_choice_screen(Canvas& c){
    auto& a=c.a;auto& s=a.game;a.sync_selection();a.hit_count=0;
    const bool discarding=s.phase==Phase::DiscardChoice;
    const unsigned required=choice_max(s);const bool optional=s.phase==Phase::ExhaustManyChoice;
    c.dim(0,0,320,240,215);
    c.centered(discarding?"选择要丢弃的牌":"选择要消耗的牌",0,3,320,16,Gold);
    char label[96]={};TextBuffer t{label,sizeof label,0};t.text("已选择 ");t.number(a.choice_selected_count);t.text(optional?" / 最多 ":" / ");t.number(required);
    c.centered(label,0,26,320,10,White);
    for(unsigned i=0;i<s.hand.count;i++){
        const int id=s.hand.ids[i],x=26+(i%5)*56,y=46+(i/5)*73;
        bool selected=false;for(unsigned j=0;j<a.choice_selected_count;j++)if(a.choice_selected[j]==id)selected=true;
        small_card(c,s.cards[id],x,y,(int)i==a.cursor,true,0);
        if(selected){c.box(x-3,y-3,53,72,Green);c.rect(x+32,y+46,14,17,Panel);c.text("✓",x+33,y+46,12,Green);}
        c.hit(x-3,y-3,53,72,HitKind::CombatChoice,id);
    }
    c.centered(optional?"可不选 · Enter 选择 · 空格确认":"Enter 选择 · 空格确认 · I 详情",0,191,320,10,Muted);
    c.button("清除",4,213,77,false,24);c.action(1,209,83,31,UiAction::ClearChoice,a.choice_selected_count!=0);
    c.button("详情",85,213,77,false,24);c.hit(82,209,83,31,HitKind::Command,(int)Key::Detail);
    const bool ready=a.choice_selected_count>=choice_min(s)&&a.choice_selected_count<=required;
    c.button(discarding?"确认丢弃":"确认消耗",185,210,129,ready,28);
    if(!ready)c.dim(185,210,129,28,95);
    c.action(181,208,138,32,UiAction::ConfirmChoice,ready);
}
