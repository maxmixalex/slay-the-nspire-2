static int potion_belt_x(const App&a,int slot){
    const int width=potion_slot_count(a.game)>3?14:17;
    return 128+(slot-int(a.potion_page)*3)*width;
}
static void potion_tip(Canvas&c,int slot,int x,int y,int width){
    const Potion bottle=potion_at(c.a.game,unsigned(slot));if(bottle==Potion::Empty){hud_static_tip(c,"POTION_SLOT",x,y,width);return;}if(!potion_definition(bottle))return;
    c.a.discover_potion(bottle);
    char body[2048]{};ui_potion_description(c.a,int(bottle),body,sizeof body);
    const int lines=c.paragraph(body,0,0,width-16,0,0,10),height=mn(209-y,30+mn(lines,12)*13);
    if(!c.nine_ui(U_HoverTip,x,y,width,height,7,7,7,7)){c.rect(x,y,width,height,Panel);c.box(x,y,width,height,Gold);}
    c.text_fitted(potion_name(int(bottle),&c.a.content),x+8,y+6,width-16,10,Gold,8);
    c.paragraph(body,x+8,y+24,width-16,mx(0,(height-28)/13),0,10,White);
}
static void potion_popup_screen(Canvas&c){
    auto&a=c.a;const auto&s=a.game;
    // Retain only actual creature/merchant hit geometry from the underlying
    // room. No target grid and no default-target consumption on popup click.
    HitRegion targets[MaxEnemies*2+1]{};unsigned count=0;
    if(a.potion_targeting){
        for(int i=0;i<a.hit_count&&count<MaxEnemies*2+1;++i){const auto&h=a.hits[i];
            if(s.phase==Phase::Combat&&(h.kind==HitKind::Enemy||h.kind==HitKind::EnemyIntent)&&can_use_potion(s,a.scroll,h.value))targets[count++]=h;
            if(s.phase==Phase::Shop&&h.kind==HitKind::UiAction&&h.value==int(UiAction::OpenShop)&&!a.shop_open){targets[count]=h;targets[count++].kind=HitKind::PotionMerchant;}
        }
    }
    a.hit_count=0;c.hit(0,22,320,218,HitKind::Close);
    if(a.potion_targeting){
        for(unsigned i=0;i<count;++i){const auto&h=targets[i];c.hit(h.x,h.y,h.w,h.h,h.kind,h.value,h.enabled);}
        // fake_merchant_button.tscn: center anchor + (174,-195,504,275)
        // in its 1920x1080 scene, mapped by the event backdrop's cover fit.
        if(original_event_can_use_foul(s))c.hit(195,92,67,95,HitKind::PotionMerchant,0);
        return;
    }
    if(a.scroll<0||a.scroll>=int(potion_slot_count(s)))return;
    if(a.potion_popup_slot!=a.scroll){a.potion_popup_slot=a.scroll;a.potion_popup_start=a.map_clock_ms;}
    a.potion_page=uint8_t(a.scroll/3);
    const unsigned age=a.map_clock_ms-a.potion_popup_start;
    // NPotionPopup: alpha 100 ms; position -25 -> 0 in 150 ms Sine/In.
    const int elapsed=mn(int(age),150);
    const int x=mx(6,mn(235,potion_belt_x(a,a.scroll)-29)),y=25-5*CombatCardMotion::sine(1024+elapsed*1024/150)/1024;
    Canvas layer=c;layer.opacity=uint8_t(mn(255,int(age)*255/100));
    const bool original=a.content.ui_image(F_PotionPopup)>=0;
    if(original)layer.ui(F_PotionPopup,x,y,78,72);else{layer.rect(x,y,78,72,Panel);layer.box(x,y,78,72,Gold);}
    const bool use=a.potion_usable(),discard=!a.combat_actions_locked()&&can_discard_potion(s,a.scroll);
    const bool pointer_use=a.pointer.visible&&a.pointer.x>=x+3&&a.pointer.x<x+75&&a.pointer.y>=y+14&&a.pointer.y<y+42;
    const bool pointer_discard=a.pointer.visible&&a.pointer.x>=x+3&&a.pointer.x<x+75&&a.pointer.y>=y+43&&a.pointer.y<y+69;
    if(original&&(pointer_use||(!a.pointer.visible&&!a.potion_menu_item)))layer.ui(F_PotionPopupTop,x+3,y+3,72,39,use?64:90);
    if(original&&(pointer_discard||(!a.pointer.visible&&a.potion_menu_item)))layer.ui(F_PotionPopupBottom,x+3,y+43,72,28,discard?64:90);
    const char*verb=a.content.world_string("gameplay_ui",a.potion_is_thrown()?"POTION_POPUP.throw":"POTION_POPUP.drink");
    const char*discard_text=a.content.world_string("gameplay_ui","POTION_POPUP.discard");
    layer.centered(*verb?verb:a.potion_is_thrown()?"Throw":"Drink",x+5,y+23,68,10,use?White:Muted);
    layer.centered(*discard_text?discard_text:"丢弃",x+5,y+49,68,10,discard?rgb(244,122,102):Muted);
    c.hit(x+3,y+14,72,28,HitKind::Potion,a.scroll,use);
    c.action(x+3,y+43,72,26,UiAction::DiscardPotion,discard);
    const int tx=x+80,tw=320-tx-5;
    if(tw>=95)potion_tip(layer,a.scroll,tx,28,tw);else potion_tip(layer,a.scroll,mx(4,x-124),28,120);
}
static void potion_hover_tip(Canvas&c){
    auto&a=c.a;if((a.panel&&a.panel!=1&&a.panel!=3)||a.title||a.paused||a.help||a.library||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0)return;
    const auto*h=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):hud_focused_hit(a);if(!h||h->kind!=HitKind::PotionSlot)return;
    potion_tip(c,h->value,mn(171,mx(4,h->x)),33,145);
}
