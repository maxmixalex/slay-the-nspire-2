namespace {
static int top_bar_scaled(int value,int q){
    // A rotating settings button can enter focus from almost +360 degrees
    // toward -180. The Q12 product exceeds int32; retain truncation toward
    // zero without a freestanding 64-bit division helper.
    const int64_t product=int64_t(value)*q;
    return int((product+(product<0?4095:0))>>12);
}
static int top_bar_phase(int angle){
    // Trigonometry is periodic. Reduce whole turns before the Q12 product
    // so even a repeatedly interrupted press cannot overflow this conversion.
    return (angle%360000)*4096/360000;
}
struct TopBarMotion {
    const Content*owner=nullptr;uint32_t generation=0,start=0;
    int from_angle=0,from_scale=4096;uint8_t state=0;
    int angle=0,scale=4096;
    void observe(const App&a,unsigned next,bool opened,unsigned kind){
        if(owner!=&a.content||generation!=a.content.generation()){
            *this={};owner=&a.content;generation=a.content.generation();start=a.map_clock_ms;
        }
        if(state!=next){from_angle=angle;from_scale=scale;state=uint8_t(next);start=a.map_clock_ms;}
        const unsigned elapsed=a.map_clock_ms-start;
        if(state==1){
            scale=4506;int q=4096;
            if(elapsed<500){const int u=int(elapsed*4096/500)-4096;q=4096+(u*u/4096)*((11066*u/4096)+6970)/4096;}
            const int to=kind==2?-180000:-12000;angle=from_angle+top_bar_scaled(to-from_angle,q);
        }else if(state==2){
            scale=4506;const int inverse=elapsed>=250?0:4096-int(elapsed)*4096/250,q=4096-(inverse*inverse/4096)*inverse/4096;
            angle=from_angle+top_bar_scaled(kind==2?45000:24000,q);
        }else{
            const int q=elapsed>=1000?4096:int(health_bar::expo_out(elapsed));
            scale=from_scale+(4096-from_scale)*q/4096;
            const int elastic=elapsed>=1000?4096:4096+(4096-q)*CombatCardMotion::sine(int(elapsed)*40960/3000-1024)/1024;
            angle=top_bar_scaled(from_angle,4096-elastic);
        }
        if(opened&&state==0)scale=4096;
        if(opened&&state!=2&&kind==1)angle=CombatCardMotion::sine(int((a.map_clock_ms%6283)*4096/1571))*6875/1024;
        // The maximum product is 6282*360000 = 2,261,520,000: it fits
        // uint32_t, but not int32_t. The divided angle is below 360000.
        if(opened&&state!=2&&kind==2)angle=int((a.map_clock_ms%6283u)*360000u/6283u);
    }
};
static TopBarMotion top_bar_motions[3];
}
static void invalidate_hud_motion(){
    // A new App can occupy the same address and reopen generation 1. Its
    // session clock must never resume a prior holder's hover/unfocus tween.
    for(auto&motion:top_bar_motions)motion=TopBarMotion{};
}
static bool hud_pointer_inside(const App&a,int x,int width){return a.pointer.visible&&!a.paused&&!a.settings_open&&!a.frontend_confirm&&a.detail_card<0&&a.detail_item<0&&a.pointer.x>=x&&a.pointer.x<x+width&&a.pointer.y>=0&&a.pointer.y<22;}
static void hud_navigation(Canvas&c,int index,int asset,int cx,int hit_x,int hit_width,Key key,bool enabled,bool opened){
    auto&a=c.a;const bool hover=enabled&&hud_pointer_inside(a,hit_x,hit_width);
    const bool press=hover&&a.pointer.held&&a.pointer.captured.kind==HitKind::Command&&a.pointer.captured.value==int(key);
    auto&motion=top_bar_motions[index];motion.observe(a,press?2:hover?1:0,opened,unsigned(index));
    const int size=18*motion.scale/4096,phase=top_bar_phase(motion.angle);
    Canvas layer=c;if(!enabled)layer.opacity=150;else if(press)layer.brightness=128;
    ui_picture_transform(layer,a.content.ui_image(asset),cx,11,size,size,CombatCardMotion::sine(phase),CombatCardMotion::sine(phase+1024));
    c.hit(hit_x,0,hit_width,22,HitKind::Command,int(key),enabled);
}
static void hud_static_tip(Canvas&c,const char*key,int x,int y=27,int width=145){
    char title_key[64]{},body_key[64]{};TextBuffer title_name{title_key,sizeof title_key,0},body_name{body_key,sizeof body_key,0};
    title_name.text(key);title_name.text(".title");body_name.text(key);body_name.text(".description");
    const auto*title=c.a.content.world_string("static_hover_tips",title_key);const auto*body=c.a.content.world_string("static_hover_tips",body_key);
    if(!*title&&!*body)return;
    const char*hotkey=wt_equal(key,"DECK")?"Q":wt_equal(key,"MAP")?"M":wt_equal(key,"DRAW_PILE")?"A":wt_equal(key,"DISCARD_PILE")?"D":wt_equal(key,"EXHAUST_PILE")?"F":"None";
    char formatted_title[128];UiTextContext context{&c.a,hotkey};
    LocalizedTemplate format_title(formatted_title,sizeof formatted_title,ui_text_value,&context);
    format_title.format(title);if(!format_title.truncated())title=formatted_title;
    const int lines=c.paragraph(body,0,0,width-16,0,0,10),height=mn(210-y,30+mn(lines,12)*13);
    x=mx(4,mn(316-width,x));
    if(!c.nine_ui(U_HoverTip,x,y,width,height,7,7,7,7)){c.rect(x,y,width,height,Panel);c.box(x,y,width,height,Gold);}
    c.text_fitted(title,x+8,y+6,width-16,10,Gold,8);
    c.paragraph(body,x+8,y+24,width-16,mx(0,(height-28)/13),0,10,White);
}
static void hud_hover_tip(Canvas&c){
    const auto&a=c.a;if(a.title||a.paused||a.help||a.library||a.settings_open||a.frontend_confirm||!a.pointer.visible||a.detail_card>=0||a.detail_item>=0)return;
    const auto*h=a.hit_at(a.pointer.x,a.pointer.y);if(!h||!h->enabled)return;
    if(h->kind==HitKind::HudInfo){static const char*keys[]={"HIT_POINTS","MONEY_POUCH","FLOOR"};if(unsigned(h->value)<3)hud_static_tip(c,keys[h->value],h->x);}
    else if(h->kind==HitKind::Command&&h->y==0){const auto key=Key(h->value);const char*name=key==Key::Inspect?"DECK":key==Key::Map?"MAP":key==Key::Back?"SETTINGS":nullptr;if(name)hud_static_tip(c,name,h->x+h->w-145);}
    else if(h->kind==HitKind::OpenPile){const char*name=h->value==1?"DRAW_PILE":h->value==2?"DISCARD_PILE":h->value==3?"EXHAUST_PILE":nullptr;if(name)hud_static_tip(c,name,h->x<160?8:167,h->y<150?85:122);}
}
static void hud(Canvas&c){
    auto&s=c.a.game;c.rect(0,0,320,22,Panel);c.ui(U_TopBar,0,0,320,22);
    // top_bar.tscn order: portrait, red HP, gold, recessed potion tray,
    // room/floor information, then the right-aligned navigation controls.
    c.nine_ui(U_TopBarBackdrop,1,0,24,22,7,6,7,6);
    int portrait=c.a.content.ui_image(U_TopBarIronclad+int(s.character));
    if(portrait<0)portrait=c.a.content.ui_image(character_portrait(s.character));
    c.picture(portrait,3,1,20,20);
    if(const unsigned ascension=world_ascension(s)){
        c.ui(U_TopBarAscension,13,10,7,11);c.num(int(ascension),19,10,8,Gold);
    }
    const auto health=c.a.combat_health(0);
    c.picture(A_Heart,27,4,12,13);char b[32]={};TextBuffer t{b,sizeof b,0,&c.a.content};t.number(health.hp);t.ch('/');t.number(health.max_hp);
    c.text_fitted(b,41,5,43,8,rgb(255,85,85));
    c.hit(26,0,59,22,HitKind::HudInfo,0);
    c.picture(A_Coin,86,4,12,13);b[0]=0;t.n=0;t.number(c.a.reward_gold());c.text_fitted(b,100,5,25,8,Gold);
    c.hit(85,0,42,22,HitKind::HudInfo,1);
    c.nine_ui(U_TopBarBackdrop,127,1,53,20,7,6,7,6);
    const int slots=potion_slot_count(s),width=slots>3?14:17;
    c.a.potion_page=uint8_t(mn(int(c.a.potion_page),(slots-1)/3));
    const int first=int(c.a.potion_page)*3,visible=mn(slots-first,3);
    for(int i=0;i<visible;i++){
        int x=128+i*width;const int slot=first+i;Potion potion=potion_at(s,slot);
        const unsigned age=c.a.map_clock_ms-c.a.potion_removed_start;
        const bool removing=c.a.potion_removed!=Potion::Empty&&c.a.potion_removed_slot==slot&&age<(c.a.potion_removed_discard?400u:200u);
        if(potion!=Potion::Empty){if(!c.a.reward_pickup_hidden(RewardPickupKind::Potion,slot))c.picture(c.a.content.potion_image(potion),x+1,2,width-3,18);}
        else if(c.a.content.ui_image(U_PotionPlaceholder)>=0){const unsigned alpha=age<200&&c.a.potion_removed_slot==slot?0:age<400&&c.a.potion_removed_slot==slot?(age-200)*255/200:255;c.ui(U_PotionPlaceholder,x+1,3,width-3,15,alpha);}
        else{c.box(x+4,5,7,12,rgb(92,90,83));c.rect(x+5,2,5,3,Muted);}
        if(removing){
            const int duration=c.a.potion_removed_discard?400:200,t=int(age)*4096/duration;
            const int back=int((int64_t(t)*t/4096)*(int64_t(11066)*t/4096-6970)/4096);
            const int scale=c.a.potion_removed_discard?4096:4096-back,w=mx(1,(width-3)*scale/4096),h=mx(1,18*scale/4096);
            c.picture(c.a.content.potion_image(c.a.potion_removed),x+width/2-w/2,11-h/2-(c.a.potion_removed_discard?back*20/4096:0),w,h);
        }
        c.hit(x,0,width,22,HitKind::PotionSlot,slot,potion!=Potion::Empty);
    }
    if(slots>3){c.text("›",171,5,10,Gold);c.hit(170,0,11,22,HitKind::PotionPage,1);}
    c.ui(U_TopFloor,183,3,13,16);b[0]=0;t.n=0;t.number(s.floor);c.text_fitted(b,198,5,17,8,Gold);c.hit(182,0,34,22,HitKind::HudInfo,2);
#if SPIRE_DEBUG_COMMANDS
    if(s.practice){c.text_fitted("图鉴",217,5,31,10,Gold,8,true);c.hit(216,0,33,22,HitKind::Command,(int)Key::Collection);}
    else
#endif
    hud_navigation(c,0,U_TopMap,233,216,33,Key::Map,!s.world.active||s.phase!=Phase::Map,c.a.panel==3);
    hud_navigation(c,1,U_TopDeck,266,249,34,Key::Inspect,true,c.a.panel==1&&c.a.pile_tab==0);
    b[0]=0;t.n=0;t.number(s.deck_count);const int count_x=280-c.width(b,8);
    for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)c.text(b,count_x+dx,12+dy,8,Ink);
    c.text(b,count_x,12,8,White);
    hud_navigation(c,2,U_TopSettings,300,283,37,Key::Back,true,c.a.paused);
    if(!c.a.panel&&(combat_phase(s.phase)||s.phase==Phase::Shop||s.phase==Phase::Camp||s.phase==Phase::Reward||s.phase==Phase::WorldRelicChoice||s.phase==Phase::OriginalEventChoice))owned_relic_strip(c);

}
