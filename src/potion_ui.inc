static int potion_belt_x(const App&a,int slot){
    (void)a;return hud_potion_layout::x(unsigned(slot));
}
static void potion_tip(Canvas&c,int slot,int x,int y,int width){
    const Potion bottle=potion_at(c.a.game,unsigned(slot));if(bottle==Potion::Empty){hud_static_tip(c,"POTION_SLOT",x,y,width);return;}if(!potion_definition(bottle))return;
    c.a.discover_potion(bottle);
    char body[2048]{};ui_potion_description(c.a,int(bottle),body,sizeof body);
    source_hover_box(c,potion_name(int(bottle),&c.a.content),body,x,y,width);
}
static void potion_popup_screen(Canvas&c){
    using namespace potion_popup_layout;
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
    a.potion_page=uint8_t(a.scroll/hud_potion_layout::Visible);
    const unsigned age=a.map_clock_ms-a.potion_popup_start;
    // NPotionPopup: alpha 100 ms; position -25 -> 0 in 150 ms Sine/In.
    const int elapsed=mn(int(age),150);
    const int holder_width=hud_potion_layout::Cell;
    const int x=mx(0,mn(320-W,potion_belt_x(a,a.scroll)+holder_width/2-W/2));
    const int y=33-5*CombatCardMotion::sine(1024+elapsed*1024/150)/1024;
    Canvas layer=c;layer.opacity=uint8_t(age>=100?255:(1024-CombatCardMotion::sine(1024+int(age)*2048/100))*255/2048);
    const bool original=a.content.ui_image(F_PotionPopup)>=0;
    if(original)layer.ui(F_PotionPopup,x,y,W,H);else{layer.rect(x,y,W,H,Panel);layer.box(x,y,W,H,Gold);}
    const bool use=a.potion_usable(),discard=!a.combat_actions_locked()&&can_discard_potion(s,a.scroll);
    const auto highlight=[&](bool enabled,int offset,int image,int background_y,int background_h,const char*key,bool selected){
        const auto v=c.control_visual(key,x+ControlX,y+offset,ControlW,ControlH,ButtonFamily::PotionPopup,selected,enabled);
        Canvas bg=layer;if(!enabled){bg.tint_red=bg.tint_green=bg.tint_blue=26;bg.ui(image,x+3,y+background_y,68,background_h,191);}
        else if(v.hover)bg.ui(image,x+3,y+background_y,68,background_h,unsigned(v.hover)*64/4096);
    };
    if(original){highlight(use,UseY,F_PotionPopupTop,3,38,"POTION_USE",!a.potion_menu_item);highlight(discard,DiscardY,F_PotionPopupBottom,41,27,"POTION_DISCARD",a.potion_menu_item);}
    const char*verb=a.content.world_string("gameplay_ui",a.potion_is_thrown()?"POTION_POPUP.throw":"POTION_POPUP.drink");
    const char*discard_text=a.content.world_string("gameplay_ui","POTION_POPUP.discard");
    layer.font_face=5;
    layer.text_fitted(*verb?verb:a.potion_is_thrown()?"Throw":"Drink",x+ControlX,y+UseY+(ControlH-Font)/2,ControlW,Font,rgb(255,246,226),8,true,true,0);
    layer.text_fitted(*discard_text?discard_text:"丢弃",x+ControlX,y+DiscardY+(ControlH-Font)/2,ControlW,Font,rgb(255,85,85),8,true,true,0);
    c.hit(x+ControlX,y+UseY,ControlW,ControlH,HitKind::Potion,a.scroll,use);
    c.action(x+ControlX,y+DiscardY,ControlW,ControlH,UiAction::DiscardPotion,discard);
    const int tx=x+W+4,tw=320-tx-5;
    if(tw>=SourceHoverWidth)potion_tip(c,a.scroll,tx,40,SourceHoverWidth);else potion_tip(c,a.scroll,mx(4,x-SourceHoverWidth-4),40,SourceHoverWidth);
}
static void potion_hover_tip(Canvas&c){
    auto&a=c.a;if((a.panel&&a.panel!=1&&a.panel!=3)||a.title||a.paused||a.help||a.library||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0)return;
    const auto*h=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):hud_focused_hit(a);if(!h||h->kind!=HitKind::PotionSlot)return;
    potion_tip(c,h->value,mn(317-SourceHoverWidth,mx(4,h->x)),33,SourceHoverWidth);
}
static void potion_popup_exit_screen(Canvas&c){
    using namespace potion_popup_layout;auto&v=c.a.potion_popup_exit;if(!v.active)return;
    const unsigned age=c.a.map_clock_ms-v.start;
    if(age>=200){v.active=false;return;}if(age>=100||!v.alpha)return;
    const unsigned fade=unsigned(1024+CombatCardMotion::sine(1024+int(age)*2048/100))*255/2048;
    Canvas layer=c;layer.opacity=uint8_t(div255(unsigned(layer.opacity)*v.alpha*fade/255));
    const int x=v.x,y=v.y-(v.y-28)*CombatCardMotion::sine(int(age)*1024/200)/1024;
    layer.ui(F_PotionPopup,x,y,W,H);
    Canvas disabled=layer;disabled.tint_red=disabled.tint_green=disabled.tint_blue=26;
    disabled.ui(F_PotionPopupTop,x+3,y+3,68,38,191);disabled.ui(F_PotionPopupBottom,x+3,y+41,68,27,191);
    const char*verb=c.a.content.world_string("gameplay_ui",v.thrown?"POTION_POPUP.throw":"POTION_POPUP.drink");
    const char*discard=c.a.content.world_string("gameplay_ui","POTION_POPUP.discard");layer.font_face=5;
    layer.text_fitted(verb,x+ControlX,y+UseY+(ControlH-Font)/2,ControlW,Font,rgb(255,246,226),8,true,true,0);
    layer.text_fitted(discard,x+ControlX,y+DiscardY+(ControlH-Font)/2,ControlW,Font,rgb(255,85,85),8,true,true,0);
    // Remove disconnects the original controls immediately. This retiring
    // visual never registers Use/Discard or targeting hit regions.
}
