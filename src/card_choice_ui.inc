// The source keeps generated offers, combat piles and retained-hand selection
// as distinct screens. Canonical tokens are never reordered or rewritten.
static int choice_display_cost(const State&s,int token){
    const int base=cost(choice_card(s,token));if(base<0)return base;
    if(choice_free_energy(s))return card_free_preview_cost(s,choice_card(s,token),token<s.card_count?token:-1);
    return token<s.card_count?cost(s,token):base;
}
static int choice_display_stars(const State&s,int token){
    const int stars=card_star_cost(s,choice_card(s,token),token<s.card_count?token:-1);
    return stars>=0&&choice_free_energy(s)?0:stars;
}
struct CombatPromptContext {WorldTextContext model;unsigned minimum,maximum;};
static LocalizedValue combat_prompt_value(const void*opaque,const char*name){
    const auto&context=*static_cast<const CombatPromptContext*>(opaque);
    if(wt_equal(name,"Amount")||wt_equal(name,"MaxCount"))return wt_number(int(context.maximum));
    if(wt_equal(name,"MinCount"))return wt_number(int(context.minimum));
    return wt_resolve(&context.model,name);
}
static LocalizedValue combat_prompt_catalog(const void*opaque,const char*name,LocalizedValue value){
    return wt_catalog_value(&static_cast<const CombatPromptContext*>(opaque)->model,name,value);
}
static void combat_selection_text(const App&a,char*out,size_t capacity){
    const auto prompt=a.combat_selection_prompt();const auto&s=a.game;
    const int instance=s.playing>=0&&s.playing<s.card_count?s.playing:-1;
    Card model=instance>=0?s.cards[instance]:Card{Strike,0};
    // The prompt carries the original model's dynamic values, independently
    // of the currently highlighted instance in the selection pool.
    if(wt_equal(prompt.table,"cards"))for(unsigned i=0;i<CardKinds;++i){const char*key=definitions[i].key;unsigned n=0;
        while(key[n]&&key[n]==prompt.key[n])++n;if(!key[n]&&prompt.key[n]=='.'){model.kind=CardKind(i);if(instance<0)model.upgraded=0;break;}}
    CombatPromptContext context{{&a,prompt.table,{},int(model.kind),-1,int(a.combat_selection_max()),model,instance},a.combat_selection_min(),a.combat_selection_max()};
    wt_model_key(context.model,prompt.key);
    // CardSelectorPrefs adds the requested values before filtering. In
    // manual ranges, a small eligible pool must not rewrite the source text.
    if(s.phase==Phase::ExhaustManyChoice)context.maximum=3+2*unsigned(model.upgraded);
    else if(s.phase==Phase::WorldRelicChoice&&wt_equal(prompt.key,"GAMBLING_CHIP.selectionScreenPrompt"))context.maximum=999999999;
    else if(s.phase==Phase::ExtendedChoice){
        if(s.expansion.choice==XChoice::DiscardRedraw||s.expansion.choice==XChoice::Exhaust||s.expansion.choice==XChoice::RTransformGuards)context.maximum=999999999;
        else if(s.expansion.choice==XChoice::EventNeowsFury)context.maximum=unsigned(mn(2+int(model.upgraded),MaxHand-int(s.hand.count)));
    }
    const char*text=a.content.locale_string(prompt.table,prompt.key);
    if(!*text)text=a.content.world_string(prompt.table,prompt.key);
    if(!*text)text=a.content.ui_text("选择一张牌"); // pre-locale resource compatibility
    char raw[512]{};TextBuffer source{raw,sizeof raw,0};source.text(text);
    LocalizedTemplate formatted(out,capacity,combat_prompt_value,&context,a.content.language()!=2?"Energy":"能量",a.content.language()!=2?"Stars":"辉星",a.content.english(),combat_prompt_catalog);
    formatted.for_language(a.content.language()).format(raw);
}
static bool combat_choice_selected(const App&a,int id){for(unsigned i=0;i<a.choice_selected_count;++i)if(a.choice_selected[i]==id)return true;return false;}
static Card combat_selection_card(const App&a,int id,const EventCardFacts**facts=nullptr,int*instance=nullptr){
    if(instance)*instance=-1;if(facts)*facts=nullptr;
    if(a.game.phase==Phase::WorldRelicChoice&&(a.game.world_relics.choice==WorldRelicChoice::CombatOffer||a.game.world_relics.choice==WorldRelicChoice::OfferCard)){
        if(id<0||id>=a.game.world_relics.offer_count)return {Strike,0};
        if(facts)*facts=&a.game.world_relics.offer_facts[id];return card_from_token(a.game.world_relics.offers[id]);
    }
    if(id>=0&&id<a.game.card_count){if(instance)*instance=id;return a.game.cards[id];}
    return choice_card(a.game,id);
}
static HitKind combat_selection_hit_kind(const App&a){return a.game.phase==Phase::WorldRelicChoice?HitKind::WorldChoice:HitKind::CombatChoice;}
static void combat_choose_offer_screen(Canvas&c,const uint16_t*ids,unsigned count){
    auto&a=c.a;auto&motion=a.combat_selection.row;const uint32_t now=a.map_clock_ms;
    Canvas banner=c;banner.opacity=uint8_t(div255(banner.opacity*motion.banner_alpha(now)));banner.font_face=5;
    const int by=motion.banner_y(now);
    banner.ui(a.content.version()>=Ui6ContentVersion?I6_CommonBanner:U_RewardBanner,80,by-5,160,40);
    banner.fitted(card_reward_label(a,"gameplay_ui","CHOOSE_CARD_HEADER","选择一张牌"),100,by,120,22,10,8,rgb(255,246,226),true,true,true,false);
    const auto contains=[&](unsigned i){const int scale=motion.scale(i,now),w=CardGridPainter::face_width(scale),h=CardGridPainter::face_height(scale),x=a.combat_selection.offer_x(i,now);
        return a.pointer.x>=x-w/2-2&&a.pointer.x<x+(w+1)/2+2&&a.pointer.y>=131-h/2-2&&a.pointer.y<131+(h+1)/2+2;};
    int focused=a.pointer.visible?-1:a.cursor;
    if(a.pointer.visible){if(motion.focus>=0&&unsigned(motion.focus)<count&&contains(unsigned(motion.focus)))focused=motion.focus;
        else for(int i=int(count)-1;i>=0;--i)if(contains(unsigned(i))){focused=i;break;}}
    motion.set_focus(focused,now);
    for(unsigned order=0;order<count;++order){const unsigned i=focused<0?order:order==count-1?unsigned(focused):order>=unsigned(focused)?order+1:order;
        const EventCardFacts*facts=nullptr;int instance=-1;const Card card=combat_selection_card(a,ids[i],&facts,&instance);
        const int scale=motion.scale(i,now),w=CardGridPainter::face_width(scale),h=CardGridPainter::face_height(scale),x=a.combat_selection.offer_x(i,now)-w/2,y=131-h/2;
        Canvas face=c;face.brightness=uint8_t(div255(face.brightness*motion.shade(now)));
        const int fee=a.game.phase==Phase::WorldRelicChoice?-2:choice_display_cost(a.game,ids[i]),stars=a.game.phase==Phase::WorldRelicChoice?-2:choice_display_stars(a.game,ids[i]);
        draw_card_face(face,card,x,y,w,h,fee,stars,instance,-1,facts);
        c.hit(x-2,y-2,w+4,h+4,combat_selection_hit_kind(a),ids[i],a.combat_selection.offer_clickable(now));
    }
    if(!a.combat_selection_min()){
        const int x=135,y=199-CardRewardPresentation::rounded(10*(4096-CardRewardPresentation::back(now-motion.opened,500)),4096);
        const char*label=card_reward_label(a,"gameplay_ui","CHOOSE_CARD_SKIP_BUTTON","跳过");
        const auto visual=c.control_visual(label,x,y,53,20,ButtonFamily::RewardAlternative,false,true);
        const int width=53*visual.scale/4096,height=20*visual.scale/4096;
        card_reward_alt_picture(c,x+(53-width)/2,y+(20-height)/2,width,height,unsigned(visual.shade));
        Canvas text=c;text.font_face=5;text.fitted(label,x+2,y+1,49,18,8,8,rgb(253,244,227),true,true,true,false);
        c.action(x,y,53,20,UiAction::SkipCombatChoice);
        if(!a.pointer.visible)card_reward_keycap(c,InputCommand::Cancel,x+48,y+8);
    }
    if(!a.pointer.visible){card_reward_keycap(c,InputCommand::Confirm,0,227,10);Canvas prompt=c;prompt.font_face=5;
        prompt.text(card_reward_label(a,"gameplay_ui","TO_INSPECT_PROMPT","查看详情"),15,226,8,Gold,104);}
}
static void combat_choose_hand_screen(Canvas&c,const uint16_t*ids,unsigned count){
    auto&a=c.a;auto&motion=a.combat_selection;const uint32_t now=a.map_clock_ms;
    const auto previous=a.combat_hand();
    for(unsigned i=0;i<count&&i<MaxHand;++i){bool tracked=false;for(const auto&h:motion.holders)if(h.id==ids[i])tracked=true;if(tracked)continue;
        for(auto&h:motion.holders)if(h.id<0){HandPose pose=hand_pose(int(count),int(i),false);
            for(unsigned j=0;j<previous.count;++j)if(previous.ids[j]==ids[i]){pose=a.combat_hand_pose(j);break;}
            h.id=int16_t(ids[i]);h.current=h.from=h.target=pose;h.at=h.last=now;break;}}
    uint16_t remaining[MaxHand];unsigned remaining_count=0;int hand_focus=-1;
    for(unsigned i=0;i<count&&i<MaxHand;++i)if(!combat_choice_selected(a,ids[i])){remaining[remaining_count]=ids[i];if(!a.pointer.visible&&int(i)==a.cursor)hand_focus=int(remaining_count);++remaining_count;}
    if(a.pointer.visible){for(unsigned i=0;i<remaining_count;++i)for(const auto&h:motion.holders)if(h.id==remaining[i]&&hand_contains(h.current,a.pointer.x,a.pointer.y))hand_focus=int(i);
        if(hand_focus<0)for(int i=int(remaining_count)-1;i>=0;--i)if(hand_contains(hand_pose(int(remaining_count),i,false),a.pointer.x,a.pointer.y)){hand_focus=i;break;}}
    const auto draw=[&](unsigned i){const int id=remaining[i];const bool focused=int(i)==hand_focus;
        const auto pose=motion.hand_pose_for(id,hand_pose(int(remaining_count),int(i),focused,hand_focus),false,now);
        draw_hand_tile(c,hand_tile(c,id),pose,focused);combat_hand_index(c,i,pose);
        const auto r=hand_bounds(pose);c.hit(r.x,r.y,r.w,r.h,combat_selection_hit_kind(a),id);
    };
    for(unsigned i=0;i<remaining_count;++i)if(int(i)!=hand_focus)draw(i);if(hand_focus>=0)draw(unsigned(hand_focus));
    motion.select_count(a.choice_selected_count,now);
    if(motion.kind==CombatSelectionKind::HandUpgrade&&a.choice_selected_count){
        const int id=a.choice_selected[0];int scale=1000+(motion.row.scale(0,now)-800)/2;
        int width=CardRewardPresentation::rounded(51*scale,1000),height=CardRewardPresentation::rounded(72*scale,1000),x=86+(51-width)/2,y=74+(72-height)/2;
        const bool focus=a.pointer.visible?a.pointer.x>=x&&a.pointer.x<x+width&&a.pointer.y>=y&&a.pointer.y<y+height:a.cursor>=0&&unsigned(a.cursor)<count&&ids[a.cursor]==id;
        motion.row.set_focus(focus?0:-1,now);scale=1000+(motion.row.scale(0,now)-800)/2;
        width=CardRewardPresentation::rounded(51*scale,1000);height=CardRewardPresentation::rounded(72*scale,1000);x=86+(51-width)/2;y=74+(72-height)/2;
        card_selection_upgrade_preview(c,a.game.cards[id],id,-1,900,110,scale);
        // Only the original "before" preview returns the selected card.
        // The upgraded preview supplies hover tips but has no click callback.
        c.hit(x,y,width,height,combat_selection_hit_kind(a),id);
        c.hit(183,74,51,72,combat_selection_hit_kind(a),id,false);
    }else{
        const int container_scale=motion.selected_scale(now),cy=motion.selected_y(now);
        for(unsigned i=0;i<a.choice_selected_count;++i){const int id=a.choice_selected[i];
            const int cx=160+CardRewardPresentation::rounded((int(i)*2-int(a.choice_selected_count)+1)*300*container_scale*2,21000);
            bool focused=false;if(a.pointer.visible){for(const auto&h:motion.holders)if(h.id==id)focused=hand_contains(h.current,a.pointer.x,a.pointer.y);}
            else focused=a.cursor>=0&&unsigned(a.cursor)<count&&ids[a.cursor]==id;
            const int scale=HandFocusScale*(focused?1000:800)*container_scale/1000000;
            const auto pose=motion.hand_pose_for(id,{cx,cy,scale,0,1024},true,now);draw_hand_tile(c,hand_tile(c,id),pose,focused);
            const auto r=hand_bounds(pose);c.hit(r.x,r.y,r.w,r.h,combat_selection_hit_kind(a),id);
        }
    }
    char prompt[512]{};combat_selection_text(a,prompt,sizeof prompt);Canvas label=c;label.font_face=5;label.bold_face=5;
    label.fitted(prompt,9,43,302,22,8,8,rgb(255,246,226),true,true,true,false);
    native_original_confirm(c,UiAction::ConfirmChoice,motion.row.opened,a.combat_selection_confirmable());
}
static void card_choice_screen(Canvas&c){
    auto&a=c.a;a.sync_selection();a.sync_combat_selection();a.hit_count=0;
    const auto kind=a.combat_selection_kind();if(kind==CombatSelectionKind::None)return;
    uint16_t ids[MaxWorldUiChoices];const unsigned count=a.combat_selection_items(ids,MaxWorldUiChoices);if(count>MaxWorldUiChoices)return;
    a.cursor=mn(mx(0,a.cursor),mx(0,int(count)-1));
    const unsigned age=a.map_clock_ms-a.combat_selection.row.opened;
    // Both the hand's SelectModeBackstop and the run overlay are authored
    // pure black. Canvas::dim uses the unrelated general-purpose Ink color.
    Canvas backstop=c;backstop.opacity=uint8_t(div255(unsigned(c.opacity)*(combat_selection_hand(kind)?unsigned(room_curve::linear(age,200)*191/4096):unsigned(room_curve::cubic_out(age,500)*217/4096))));backstop.rect(0,0,320,240,0);
    Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*int(room_curve::linear(age,200)*255/4096)));
    if(kind==CombatSelectionKind::Offer){combat_choose_offer_screen(layer,ids,count);return;}
    if(combat_selection_hand(kind)){combat_choose_hand_screen(layer,ids,count);return;}
    auto&grid=a.configure_card_grid(CardGridSurface::CombatChoice,int(count),a.game.revision);CardGridPainter painter{layer,grid};
    int portraits[24];unsigned portrait_count=0;
    for(int i=grid.first();i<grid.last()&&portrait_count<24;++i){int instance=-1;const EventCardFacts*facts=nullptr;const Card card=combat_selection_card(a,ids[i],&facts,&instance);portraits[portrait_count++]=a.card_face_image(card,instance,-1,facts);}
    a.content.prepare_images(portraits,portrait_count);
    for(int i=grid.first();i<grid.last();++i){int instance=-1;const EventCardFacts*facts=nullptr;const Card card=combat_selection_card(a,ids[i],&facts,&instance);
        painter.card(i,card,!a.pointer.visible&&i==a.cursor,true,instance>=0?choice_display_cost(a.game,ids[i]):-2,instance>=0?choice_display_stars(a.game,ids[i]):-2,instance,-1,facts,combat_choice_selected(a,ids[i]));
        painter.hit(i,combat_selection_hit_kind(a),ids[i]);}
    painter.finish();card_grid_scrollbar(layer,grid,CardGridSurface::CombatChoice);
    char prompt[512]{};combat_selection_text(a,prompt,sizeof prompt);card_selection_prompt(layer,prompt);
    native_original_confirm(layer,UiAction::ConfirmChoice,a.combat_selection.row.opened,a.combat_selection_confirmable());
}
