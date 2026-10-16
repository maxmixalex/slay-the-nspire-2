// Instance IDs for real cards, reserved tokens for generated offers.
static const char* extended_choice_title(XChoice kind){
    switch(kind){
    case XChoice::Offer:return "选择一张牌加入手牌";
    case XChoice::Draw:return "从抽牌堆选择";
    case XChoice::Discard:return "从弃牌堆选择";
    case XChoice::Top:return "选择放回牌顶的牌";
    case XChoice::DiscardRedraw:return "丢弃并重新抽牌";
    case XChoice::Exhaust:return "选择要消耗的牌";
    case XChoice::Free:return "选择本场战斗免费的牌";
    case XChoice::Transform:return "选择要变化的牌";
    default:return "选择卡牌";
    }
}
static void card_choice_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.sync_selection();a.hit_count=0;
    bool extended=s.phase==Phase::ExtendedChoice;
    bool discarding=s.phase==Phase::DiscardChoice,returning=s.phase==Phase::ReturnChoice;
    bool optional=choice_min(s)==0,offer=extended&&s.expansion.choice==XChoice::Offer;
    const auto&source=choice_pile(s);unsigned limit=choice_max(s);
    a.cursor=mn(mx(0,a.cursor),mx(0,source.count-1));int first=offer?0:a.cursor/10*10;
    c.dim(0,0,320,240,230);
    c.centered(extended?extended_choice_title(s.expansion.choice):returning?"从弃牌堆选择一张牌":discarding?"选择要丢弃的牌":"选择要消耗的牌",0,3,320,16,Gold);
    char label[96]={};TextBuffer t{label,sizeof label,0};
    if(extended&&s.expansion.choice_free)t.text("本回合免费 · ");
    else if(returning)t.text("放回抽牌堆顶部 · ");
    t.text("已选择 ");t.number(a.choice_selected_count);t.text(optional?" / 最多 ":" / ");t.number(limit);
    c.centered(label,0,26,320,10,White);
    for(int i=first;i<mn(source.count,first+(offer?3:10));i++){
        int id=source.ids[i],x=offer?23+i*96:26+(i-first)%5*56,y=offer?60:46+(i-first)/5*73;
        bool selected=false;for(unsigned j=0;j<a.choice_selected_count;j++)if(a.choice_selected[j]==id)selected=true;
        Card card=choice_card(s,id);
        if(offer)large_card(c,card,x,y,i==a.cursor);
        else small_card(c,card,x,y,i==a.cursor,true,0);
        int w=offer?86:53,h=offer?118:72;
        if(selected){c.box(x-3,y-3,w,h,Green);c.rect(x+w-21,y+h-24,14,17,Panel);c.text("✓",x+w-20,y+h-24,12,Green);}
        if(extended&&s.expansion.choice_free){c.picture(a.content.energy_image(card),x-1,y-1,18,18);c.energy_label(0,x+4,y-2,12);}
        c.hit(x-3,y-3,w,h,HitKind::CombatChoice,id);
    }
    if(source.count>10){
        c.text("‹",12,188,16,Gold);c.action(0,187,42,21,UiAction::PreviousChoicePage,first>0);
        c.text("›",295,188,16,Gold);c.action(278,187,42,21,UiAction::NextChoicePage,first+10<source.count);
        char page[40]={};TextBuffer t{page,sizeof page,0};t.number(first/10+1);t.text(" / ");t.number((source.count+9)/10);t.text(" · I 详情");c.centered(page,43,191,234,10,Muted);
    }else c.centered(optional?"可不选 · Enter 选择 · 空格确认":"Enter 选择 · 空格确认 · I 详情",0,191,320,10,Muted);
    c.button("清除",4,213,77,false,24);c.action(1,209,83,31,UiAction::ClearChoice,a.choice_selected_count!=0);
    c.button("详情",85,213,77,false,24);c.hit(82,209,83,31,HitKind::Command,(int)Key::Detail);
    bool ready=a.choice_selected_count>=choice_min(s)&&a.choice_selected_count<=limit;
    c.button(extended?"确认选择":returning?"放回牌顶":discarding?"确认丢弃":"确认消耗",185,210,129,ready,28);
    if(!ready)c.dim(185,210,129,28,95);
    c.action(181,208,138,32,UiAction::ConfirmChoice,ready);
}
