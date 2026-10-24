// Offers are model tokens in their own namespace. Pile choices retain instance
// IDs across paging, details and save/load; viewing never consumes game RNG.
static const char* choice_title(const State&s){
    if(s.phase==Phase::ReturnChoice)return "从弃牌堆选择一张牌";
    if(s.phase==Phase::DiscardChoice)return "选择要丢弃的牌";
    if(s.phase!=Phase::ExtendedChoice)return "选择要消耗的牌";
    switch(s.expansion.choice){
    case XChoice::Offer:case XChoice::EventAbundance:return "选择一张牌";
    case XChoice::EnemyKnowledge:return "选择知识的代价";
    case XChoice::EventDualWield:return "选择要复制的牌";
    case XChoice::EventNeowsFury:return "选择要消耗的手牌";
    case XChoice::EventWish:return "选择一张牌加入手牌";
    case XChoice::Draw:return "从抽牌堆选择";
    case XChoice::Discard:return "从弃牌堆选择";
    case XChoice::Top:return "放回抽牌堆顶";
    case XChoice::DiscardRedraw:return "丢弃并重新抽牌";
    case XChoice::Free:return "选择本场固定费用归零的牌";
    case XChoice::Transform:return "选择要变化的牌";
    case XChoice::IUpgrade:return "选择要升级的牌";
    case XChoice::SlySkill:return "选择赋予奇巧的技能牌";
    case XChoice::NightmareCard:return "选择下回合复制的牌";
    case XChoice::RTransformHand:return "将一张手牌变为仆从打击";
    case XChoice::RTransformDraw:return "选择变为仆从俯冲的抽牌";
    case XChoice::RTransformGuards:return "将任意手牌变为仆从捐躯";
    case XChoice::RDecisions:return "选择一张技能免费打出三次";
    case XChoice::RHeirloom:return "选择一张无色牌复制";
    case XChoice::RDiscardTop:return "将一张弃牌放回抽牌堆顶";
    case XChoice::RForegone:return "从抽牌堆选择加入手牌";
    case XChoice::RTyranny:return "暴政：选择要消耗的手牌";
    case XChoice::NCleanse:return "选择要消耗的抽牌堆卡牌";
    case XChoice::NDredge:return "将弃牌堆的牌放回手牌";
    case XChoice::NEthereal:return "选择赋予虚无的手牌";
    case XChoice::NRetain:return "选择赋予保留的手牌";
    case XChoice::NSeance:return "将一张抽牌变化为灵魂";
    case XChoice::NTransfigure:return "选择增费并赋予重放的牌";
    default:return "选择要消耗的牌";
    }
}
static int choice_display_cost(const State&s,int token){
    const int base=cost(choice_card(s,token));if(base<0)return base;
    if(choice_free_energy(s))return card_free_preview_cost(s,choice_card(s,token),token<s.card_count?token:-1);
    return token<s.card_count?cost(s,token):base;
}
static int choice_display_stars(const State&s,int token){
    const int stars=card_star_cost(s,choice_card(s,token),token<s.card_count?token:-1);
    return stars>=0&&choice_free_energy(s)?0:stars;
}
static void card_choice_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.sync_selection();a.hit_count=0;
    const auto&source=choice_pile(s);const unsigned limit=choice_max(s);
    const bool offer=s.phase==Phase::ExtendedChoice&&(s.expansion.choice==XChoice::Offer||s.expansion.choice==XChoice::EnemyKnowledge||s.expansion.choice==XChoice::EventAbundance);
    const bool optional=choice_min(s)==0;
    a.cursor=mn(mx(0,a.cursor),mx(0,source.count-1));const int first=offer?0:a.cursor/10*10;
    c.dim(0,0,320,240,215);c.centered(choice_title(s),0,8,320,16,Gold);
    char label[96]={};TextBuffer text{label,sizeof label,0,&c.a.content};
    if(offer&&choice_free_energy(s))text.text("本回合减免固定费用 · ");
    text.text("已选择 ");text.number(a.choice_selected_count);text.text(optional?" / 最多 ":" / ");text.number(limit);
    c.centered(label,0,31,320,10,White);
    int portraits[10];unsigned portrait_count=0;
    for(int i=first;i<mn(source.count,first+(offer?3:10));++i){const int id=source.ids[i];
        portraits[portrait_count++]=a.card_face_image(choice_card(s,id),id<s.card_count?id:-1);}
    a.content.prepare_images(portraits,portrait_count);
    for(int i=first;i<mn(source.count,first+(offer?3:10));i++){
        const int id=source.ids[i];Card card=choice_card(s,id);bool selected=false;
        for(unsigned j=0;j<a.choice_selected_count;j++)if(a.choice_selected[j]==id)selected=true;
        if(offer){
            const int x=(320-source.count*94)/2+7+i*94,y=65;
            large_card(c,card,x,y,selected||a.cursor==i,choice_display_cost(s,id),choice_display_stars(s,id),id<s.card_count?id:-1);
            if(selected)c.box(x-4,y-4,88,120,Green);
            c.hit(x-5,y-5,90,126,HitKind::CombatChoice,id);
        }else{
            const int x=26+(i-first)%5*56,y=48+(i-first)/5*73;
            small_card(c,card,x,y,i==a.cursor,true,0,choice_display_cost(s,id),choice_display_stars(s,id),id<s.card_count?id:-1);
            if(selected){c.box(x-3,y-3,53,72,Green);c.rect(x+32,y+46,14,17,Panel);c.text("✓",x+33,y+46,12,Green);}
            c.hit(x-3,y-3,53,72,HitKind::CombatChoice,id);
        }
    }
    if(!offer&&source.count>10){
        c.text("‹",12,191,16,Gold);c.action(0,187,42,21,UiAction::PreviousChoicePage,first>0);
        c.text("›",295,191,16,Gold);c.action(278,187,42,21,UiAction::NextChoicePage,first+10<source.count);
        char page[32]={};TextBuffer text{page,sizeof page,0,&c.a.content};text.number(first/10+1);text.text(" / ");text.number((source.count+9)/10);c.centered(page,43,194,234,10,Muted);
    }else c.centered(optional?"可不选 · Enter 选择 · 空格确认":"Enter 选择 · 空格确认 · I 详情",0,192,320,10,Muted);
    c.button("清除",4,213,77,false,24);c.action(1,209,83,31,UiAction::ClearChoice,a.choice_selected_count!=0);
    c.button("详情",85,213,77,false,24);c.hit(82,209,83,31,HitKind::Command,(int)Key::Detail,source.count>0);
    const bool ready=a.choice_selected_count>=choice_min(s)&&a.choice_selected_count<=limit;
    c.button("确认",185,210,129,ready,28);if(!ready)c.dim(185,210,129,28,95);
    c.action(181,208,138,32,UiAction::ConfirmChoice,ready);
}
