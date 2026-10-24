// NInspectCardScreen / inspect_card_screen.tscn, projected uniformly from
// the verified 1680x1260 viewport. The original inspector has a backstop, one centered card,
// neighboring-card arrows and a bottom upgrade tickbox.
static void card_inspect_arrow(Canvas&c,unsigned index,int x,int y,UiAction action){
    auto&a=c.a;auto&motion=a.card_inspect.arrows[index];
    constexpr int box=card_inspect_layout::ArrowSize;
    const bool focused=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+box&&a.pointer.y>=y&&a.pointer.y<y+box;
    const bool pressed=focused&&a.pointer.held&&a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==int(action);
    motion.observe(pressed?2:focused?1:0,a.map_clock_ms);
    const int scale=motion.scale(a.map_clock_ms),w=(13*scale+2048)/4096,h=(box*scale+2048)/4096;
    const int image=a.content.ui_image(index?I7_SettingsArrowRight:I7_SettingsArrowLeft);
    if(image>=0){
        Canvas shade=c;const int value=motion.shade(a.map_clock_ms);shade.brightness=uint8_t(mn(255,value));
        shade.picture(image,x+(box-w)/2,y+(box-h)/2,w,h);
        if(value>255)c.picture(image,x+(box-w)/2,y+(box-h)/2,w,h,unsigned(value-255),presentation::Add);
    }
    c.action(x,y,box,box,action);
}
static void detail_screen(Canvas&c){
    auto&a=c.a;const Card card=card_from_token(a.detail_card);auto&view=a.card_inspect;
    a.hit_count=0;c.hit(0,0,320,240,HitKind::DetailClose);
    const unsigned age=a.map_clock_ms-view.opened,root_alpha=view.opacity(a.map_clock_ms);
    const unsigned linear=age>=250?255:age*255/250;
    c.dim(0,0,320,240,229*linear/255*root_alpha/255);
    Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*root_alpha));
    Canvas face=layer;face.opacity=uint8_t(div255(face.opacity*(view.direction?255:linear)));
    face.brightness=uint8_t(view.direction?255:linear);
    const int scale=view.card_scale(a.map_clock_ms),w=(300*scale+10752)/21504,h=(422*scale+10752)/21504;
    const int x=view.card_x(a.map_clock_ms)-(w+1)/2,y=card_inspect_layout::CenterY-(h+1)/2;
    char description[2048]{};const auto*facts=a.detail_has_facts?&a.detail_facts:nullptr;
    a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,description,sizeof description,a.detail_instance,a.detail_deck_index,facts);
    draw_card_face(face,card,x,y,w,h,a.detail_energy(),a.detail_stars(),a.detail_instance,a.detail_deck_index,facts,description,&view.description_truncated);
    if(!a.library&&combat_phase(a.game.phase)&&a.detail_instance>=0&&a.detail_instance<a.game.card_count&&
       a.game.cards[a.detail_instance].kind==card.kind&&a.game.silent.inky[a.detail_instance])face.ui(U_Inky,151,y+h-7,18,18);
    if(card.upgraded||card_upgradeable(card)){
        const char*label=a.content.world_string("card_selection","VIEW_UPGRADES");
        if(!*label)label=a.content.english()?"View Upgrades":"查看升级";
        constexpr int icon=card_inspect_layout::UpgradeH,gap=reference_viewport::project(8),top=card_inspect_layout::UpgradeY;
        const int width=mn(150,layer.width(label,8)+icon+gap),left=160-width/2;
        layer.ui(card.upgraded?F_SettingsTicked:F_SettingsUnticked,left,top,icon,icon);
        layer.text_fitted(label,left+icon+gap,top+1,width-icon-gap,8,Gold,8);
        c.action(left,top,width,icon,UiAction::ToggleCardUpgrade);
    }
    if(view.count>1){
        const unsigned arrow_age=age>100?age-100:0;
        Canvas arrows=layer;arrows.opacity=uint8_t(div255(arrows.opacity*(arrow_age>=250?255:arrow_age*255/250)));
        const int q=room_curve::back_out(room_curve::linear(arrow_age,250)),shift=card_inspect_layout::Travel*(4096-q)/4096;
        if(view.index>0)card_inspect_arrow(arrows,0,card_inspect_layout::LeftArrowX+shift,card_inspect_layout::ArrowY,UiAction::InspectPreviousCard);
        if(view.index+1<view.count)card_inspect_arrow(arrows,1,card_inspect_layout::RightArrowX-shift,card_inspect_layout::ArrowY,UiAction::InspectNextCard);
    }
#if SPIRE_DEBUG_COMMANDS
    if(a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat){
        const bool room=a.game.card_count<MaxCards;
        c.button(room?(a.game.hand.count<MaxHand?"加入手牌":"加入弃牌堆"):"卡牌已满",204,4,112,false,23,room);
        c.action(204,4,112,23,UiAction::LabCard,room);
    }
#endif
}
