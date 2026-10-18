// Offers are model tokens in their own namespace. Pile choices retain instance
// IDs across paging, details and save/load; viewing never consumes game RNG.
static const char* choice_title(const State&s){
    if(s.phase==Phase::ReturnChoice)return "从弃牌堆选择一张牌";
    if(s.phase==Phase::DiscardChoice)return "选择要丢弃的牌";
    if(s.phase!=Phase::ExtendedChoice)return "选择要消耗的牌";
    switch(s.expansion.choice){
    case XChoice::Offer:return "选择一张牌";
    case XChoice::Draw:return "从抽牌堆选择";
    case XChoice::Discard:return "从弃牌堆选择";
    case XChoice::Top:return "放回抽牌堆顶";
    case XChoice::DiscardRedraw:return "丢弃并重新抽牌";
    case XChoice::Free:return "选择一张牌免费打出";
    case XChoice::Transform:return "选择要变化的牌";
    default:return "选择要消耗的牌";
    }
}
static int choice_display_cost(const State&s,int token){
    const int base=cost(choice_card(s,token));if(base<0)return base;
    if(s.phase==Phase::ExtendedChoice&&s.expansion.choice_free)return 0;
    return token<s.card_count?cost(s,token):base;
}
static void card_choice_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.sync_selection();a.hit_count=0;
    const auto&source=choice_pile(s);const unsigned limit=choice_max(s);
    const bool offer=s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Offer;
    const bool optional=choice_min(s)==0;
    a.cursor=mn(mx(0,a.cursor),mx(0,source.count-1));const int first=offer?0:a.cursor/10*10;
    c.dim(0,0,320,240,215);c.centered(choice_title(s),0,8,320,16,Gold);
    char label[96]={};TextBuffer text{label,sizeof label,0};
    if(offer&&s.expansion.choice_free)text.text("本回合免费 · ");
    text.text("已选择 ");text.number(a.choice_selected_count);text.text(optional?" / 最多 ":" / ");text.number(limit);
    c.centered(label,0,31,320,10,White);
    for(int i=first;i<mn(source.count,first+(offer?3:10));i++){
        const int id=source.ids[i];Card card=choice_card(s,id);bool selected=false;
        for(unsigned j=0;j<a.choice_selected_count;j++)if(a.choice_selected[j]==id)selected=true;
        if(offer){
            const int x=23+i*97,y=65;
            large_card(c,card,x,y,selected||a.cursor==i,choice_display_cost(s,id));
            if(selected)c.box(x-4,y-4,88,120,Green);
            c.hit(x-5,y-5,90,126,HitKind::CombatChoice,id);
        }else{
            const int x=26+(i-first)%5*56,y=48+(i-first)/5*73;
            small_card(c,card,x,y,i==a.cursor,true,0,choice_display_cost(s,id));
            if(selected){c.box(x-3,y-3,53,72,Green);c.rect(x+32,y+46,14,17,Panel);c.text("✓",x+33,y+46,12,Green);}
            c.hit(x-3,y-3,53,72,HitKind::CombatChoice,id);
        }
    }
    if(!offer&&source.count>10){
        c.text("‹",12,191,16,Gold);c.action(0,187,42,21,UiAction::PreviousChoicePage,first>0);
        c.text("›",295,191,16,Gold);c.action(278,187,42,21,UiAction::NextChoicePage,first+10<source.count);
        char page[32]={};TextBuffer text{page,sizeof page,0};text.number(first/10+1);text.text(" / ");text.number((source.count+9)/10);c.centered(page,43,194,234,10,Muted);
    }else c.centered(optional?"可不选 · Enter 选择 · 空格确认":"Enter 选择 · 空格确认 · I 详情",0,192,320,10,Muted);
    c.button("清除",4,213,77,false,24);c.action(1,209,83,31,UiAction::ClearChoice,a.choice_selected_count!=0);
    c.button("详情",85,213,77,false,24);c.hit(82,209,83,31,HitKind::Command,(int)Key::Detail,source.count>0);
    const bool ready=a.choice_selected_count>=choice_min(s)&&a.choice_selected_count<=limit;
    c.button("确认",185,210,129,ready,28);if(!ready)c.dim(185,210,129,28,95);
    c.action(181,208,138,32,UiAction::ConfirmChoice,ready);
}
