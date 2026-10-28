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
struct HudTipContext {UiTextContext ui;const char*boss="";const char*second="";const char*character="";};
static LocalizedValue hud_tip_value(const void*opaque,const char*name){
    const auto&v=*static_cast<const HudTipContext*>(opaque);
    if(wt_equal(name,"BossName")||wt_equal(name,"BossName1"))return wt_string(v.boss);
    if(wt_equal(name,"BossName2"))return wt_string(v.second);
    if(wt_equal(name,"character"))return wt_string(v.character);
    return ui_text_value(&v.ui,name);
}
static const char* hud_boss_name(const Content&content,EncounterId id){
    const auto*definition=encounter_definition(id);return definition?content.world_title("encounters",definition->key):"";
}
static void hud_static_tip(Canvas&c,const char*key,int x,int y=27,int width=SourceHoverWidth){
    char title_key[64]{},body_key[64]{};TextBuffer title_name{title_key,sizeof title_key,0},body_name{body_key,sizeof body_key,0};
    title_name.text(key);title_name.text(".title");body_name.text(key);body_name.text(".description");
    const auto*title=c.a.content.world_string("static_hover_tips",title_key);const auto*body=c.a.content.world_string("static_hover_tips",body_key);
    if(!*title&&!*body)return;
    const InputCommand command=wt_equal(key,"DECK")?InputCommand::Deck:wt_equal(key,"MAP")?InputCommand::Map:
        wt_equal(key,"DRAW_PILE")?InputCommand::Draw:wt_equal(key,"DISCARD_PILE")?InputCommand::Discard:wt_equal(key,"EXHAUST_PILE")?InputCommand::Exhaust:InputCommand::Count;
    char binding[24]{};const char*hotkey="None";
    if(command!=InputCommand::Count){input_key_label(c.a.input_bindings.keys[c.a.keyboard_only?1:0][unsigned(command)],binding,sizeof binding,c.a.calculator_keyboard);hotkey=binding;}
    HudTipContext context{{&c.a,hotkey}};
    if(wt_equal(key,"BOSS")||wt_equal(key,"DOUBLE_BOSS")){const auto info=hud_room_info(c.a.game);
        context.boss=hud_boss_name(c.a.content,info.boss);context.second=hud_boss_name(c.a.content,info.second_boss);}
    char formatted_title[256],formatted_body[1024];
    LocalizedTemplate format_title(formatted_title,sizeof formatted_title,hud_tip_value,&context);
    LocalizedTemplate format_body(formatted_body,sizeof formatted_body,hud_tip_value,&context);
    format_title.for_language(c.a.content.language()).format(title);format_body.for_language(c.a.content.language()).format(body);
    if(!format_title.truncated())title=formatted_title;if(!format_body.truncated())body=formatted_body;
    source_hover_box(c,title,body,x,y,width);
}
static void hud_portrait_tip(Canvas&c){
    if(!hud_portrait_enabled(c.a.game))return;
    const auto&definition=character_definitions[unsigned(c.a.game.character)];
    const char*name=c.a.content.world_title("characters",definition.key);
    HudTipContext context{{&c.a,"None"},"","",*name?name:definition.name};
    const unsigned level=world_ascension(c.a.game);char title[256]{},body[2048]{};
    LocalizedTemplate formatter(title,sizeof title,hud_tip_value,&context);
    formatter.for_language(c.a.content.language()).format(c.a.content.world_string("ascension",level?"PORTRAIT_TITLE":"PORTRAIT_TITLE_NO_ASCENSION"));
    TextBuffer description{body,sizeof body,0};
    if(level){for(unsigned i=1;i<=level;++i){char key[32]{};TextBuffer k{key,sizeof key,0};k.text("LEVEL_");if(i<10)k.ch('0');k.number(i);k.text(".title");
        // PORTRAIT_DESCRIPTION is {ascensions:list: +{}|\n} in every one
        // of the sixteen pinned locales. Titles remain fully localized.
        if(i>1)description.ch('\n');description.text(" +");description.text(c.a.content.world_string("ascension",key));}}
    if(c.a.game.run_options.mode!=RunMode::Standard){if(level)description.ch('\n');description.text(c.a.content.world_string("gameplay_ui","ACHIEVEMENTS_LOCKED"));}
    source_hover_box(c,*title?title:context.character,body,3,26);
}
static const HitRegion* hud_focused_hit(const App&a){
    if(a.hud_focus<0||a.pointer.visible)return nullptr;
    const int slots=int(potion_slot_count(a.game));HitKind kind=HitKind::HudInfo;int value=a.hud_focus;
    if(a.hud_focus>=HudRelicFocusFirst){kind=HitKind::OwnedRelic;value=a.hud_focus-HudRelicFocusFirst;}
    else if(a.hud_focus>=2&&a.hud_focus<slots+2){kind=HitKind::PotionSlot;value=a.hud_focus-2;}
    else if(a.hud_focus>=slots+2){static const int info[]={HudRoom,HudFloor,HudBoss,HudPortrait,HudTimer};const unsigned at=unsigned(a.hud_focus-slots-2);if(at>=5)return nullptr;value=info[at];}
    for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return &a.hits[i];
    return nullptr;
}
static void hud_hover_tip(Canvas&c){
    const auto&a=c.a;if(a.title||a.paused||a.help||a.library||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0)return;
    const auto*h=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):hud_focused_hit(a);if(!h||!h->enabled)return;
    if(h->kind==HitKind::HudInfo){
        static const char*keys[]={"HIT_POINTS","MONEY_POUCH","FLOOR"};
        if(unsigned(h->value)<3)hud_static_tip(c,keys[h->value],h->x);
        else if(h->value==HudRoom)hud_static_tip(c,hud_room_info(a.game).room_tip,h->x);
        else if(h->value==HudBoss){const auto info=hud_room_info(a.game);hud_static_tip(c,info.second_boss==EncounterId::None?"BOSS":"DOUBLE_BOSS",h->x);}
        else if(h->value==HudPortrait)hud_portrait_tip(c);
        else if(h->value==HudTimer){char elapsed[24]{};a.run_timer_label(elapsed,sizeof elapsed);source_hover_box(c,a.content.world_string("stats_screen","TIP_PLAYTIME.header"),elapsed,h->x,27);}
    }else if(h->kind==HitKind::Command&&h->y==0){const auto key=Key(h->value);const char*name=key==Key::Inspect?"DECK":key==Key::Map?"MAP":key==Key::Back?"SETTINGS":nullptr;if(name)hud_static_tip(c,name,h->x+h->w-SourceHoverWidth);}
    else if(h->kind==HitKind::OpenPile){const char*name=h->value==1?"DRAW_PILE":h->value==2?"DISCARD_PILE":h->value==3?"EXHAUST_PILE":nullptr;if(name)hud_static_tip(c,name,h->x<160?h->x+h->w+2:h->x-SourceHoverWidth-2,h->y);}
}
static void hud_room_picture(Canvas&c,int auxiliary,int x,int y,int w,int h,bool placeholder=false,uint16_t tint=0){
    const auto picture=c.a.content.image(c.a.content.ui_image(auxiliary));if(!picture.pixels||!picture.w||!picture.h)return;
    const int width=mn(w,h*picture.w/picture.h),height=mx(1,width*picture.h/picture.w);
    x+=(w-width)/2;y+=(h-height)/2;
    if(!placeholder){c.picture(picture,x,y,width,height);return;}
    // second_boss_icon.tscn's original placeholder material, on an icon-sized
    // temporary tile. Its gray-to-act tint and 0.75 alpha are not a map sprite.
    if(width>18||height>20)return;uint16_t pixels[18*20];uint8_t alpha[18*20]{};
    Canvas tile{c.a,pixels,18,width,height,alpha};tile.picture(picture,0,0,width,height);
    const unsigned tr=(tint>>11)*255/31,tg=((tint>>5)&63)*255/63,tb=(tint&31)*255/31;
    for(int yy=0;yy<height;++yy)for(int xx=0;xx<width;++xx){const int at=yy*18+xx;if(!alpha[at])continue;
        const unsigned color=pixels[at],gray=(299*(color>>11)*255/31+587*((color>>5)&63)*255/63+114*(color&31)*255/31+500)/1000;
        const unsigned factor=1785+3*gray;
        if(x+xx>=0&&x+xx<c.clip_w&&y+yy>=0&&y+yy<c.clip_h)c.paint((y+yy)*c.pitch+x+xx,rgb(tr*factor/2550,tg*factor/2550,tb*factor/2550),alpha[at]*3/4);
    }
}
static void hud_room_icon(Canvas&c,const char*key,int x,int y,int w,int h,bool placeholder=false,uint16_t tint=0){
    if(!key)return;const auto*asset=hud_ui7_icon(key);if(!asset)return;
    Canvas outline=c;outline.tint_red=outline.tint_green=outline.tint_blue=0;outline.opacity=uint8_t(div255(outline.opacity*45));
    hud_room_picture(outline,asset->outline,x,y,w,h,placeholder,tint);hud_room_picture(c,asset->icon,x,y,w,h,placeholder,tint);
}
static void hud_room_nodes(Canvas&c){
    auto&a=c.a;const auto info=hud_room_info(a.game);const auto p=hud_room_geometry(a.game);
    if(info.room_enabled){hud_room_icon(c,info.room_key,p.room,2,10,17);c.hit(p.room,0,10,22,HitKind::HudInfo,HudRoom);}
    c.ui(U_TopFloor,p.floor,4,10,14);char floor[16]{};TextBuffer text{floor,sizeof floor,0};text.number(a.game.floor);
    const int number_width=mn(12,p.boss-p.floor-11);
    c.text_fitted(floor,p.floor+10,4,number_width,10,rgb(197,218,206),6);c.hit(p.floor,0,10+number_width,22,HitKind::HudInfo,HudFloor);
    if(info.boss_enabled&&info.second_boss!=EncounterId::None){
        static const uint16_t act_tints[]={rgb(135,114,86),rgb(83,74,98),rgb(110,119,80),rgb(96,113,124)};
        const unsigned region=a.game.world.rooms[a.game.world.act_index].region;
        const auto*second=encounter_definition(info.second_boss);
        if(second)hud_room_icon(c,second->key,p.boss+9,6,8,8,info.second_placeholder,act_tints[region<4?region:0]);
    }
    if(info.boss_enabled){const auto*boss=encounter_definition(info.boss);if(boss)hud_room_icon(c,boss->key,p.boss,2,11,17);
        c.hit(p.boss,0,info.second_boss==EncounterId::None?11:17,22,HitKind::HudInfo,HudBoss);}
    if(a.run_timer_visible()){
        c.ui(I9_RunTimerIcon,p.timer,7,7,7);
        c.hit(p.timer,0,257-p.timer,22,HitKind::HudInfo,HudTimer);
    }
}
static void hud_deck_count(Canvas&c,int right=280){
 auto&a=c.a;char label[16]{};TextBuffer b{label,sizeof label,0};b.number(a.game.deck_count);
 const int scale=a.deck_count_pulse.observe(a.game.deck_count,a.map_clock_ms),width=c.width(label,8),x=right-width;
 if(scale==4096){
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)c.text(label,x+dx,12+dy,8,Ink);
  c.text(label,x,12,8,White);return;
 }
 uint16_t pixels[32*14]{};uint8_t alpha[32*14]{};Canvas tile{a,pixels,32,32,14,alpha};
 const int tx=(32-width)/2;
 for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)tile.text(label,tx+dx,2+dy,8,Ink);
 tile.text(label,tx,2,8,White);
 const Picture picture{reinterpret_cast<const uint8_t*>(pixels),alpha,nullptr,32,14,1};
 ui_picture_transform(c,picture,x+width/2,17,32*scale/4096,14*scale/4096);
}
static int hud_potion_shake(const App&a){
 const auto&m=a.potion_belt_motion;if(!m.failed)return 0;const unsigned age=a.map_clock_ms-m.failed_at;if(age>=500)return 0;
 const int phase=int(age)*4096/500;
 const int value=2*CombatCardMotion::sine(phase*5)*CombatCardMotion::sine(phase/2),unit=3*1024*1024;
 return value<0?-((-value+unit/2)/unit):(value+unit/2)/unit;
}
static void hud_potion_container(Canvas&c){
    using namespace hud_potion_layout;auto&s=c.a.game;
    const int slots=potion_slot_count(s),holder_width=cell(slots),belt_width=width(slots);
    c.nine_ui(U_TopBarBackdrop,Left,1,belt_width,20,7,6,7,6);
    const unsigned error=c.a.potion_belt_motion.error_alpha(c.a.map_clock_ms);
    if(error&&c.a.content.version()>=Ui7ContentVersion)c.nine_ui(I7_PotionErrorBackground,Left,1,belt_width,20,7,6,7,6,error);
    const int shake=hud_potion_shake(c.a);
    const int width=holder_width;
    c.a.potion_page=uint8_t(mn(int(c.a.potion_page),(slots-1)/Visible));
    const int first=int(c.a.potion_page)*Visible,visible=mn(slots-first,Visible);
    for(int i=0;i<visible;i++){
        int x=Left+i*width+shake;const int slot=first+i;Potion potion=potion_at(s,slot);
        const bool focused=hud_pointer_inside(c.a,x,width)||(!c.a.pointer.visible&&c.a.hud_focus==slot+2&&!c.a.paused&&!c.a.settings_open&&!c.a.frontend_confirm&&c.a.detail_card<0&&c.a.detail_item<0);
        auto&holder=c.a.potion_belt_motion.holders[slot];const int scale=holder.observe(focused,potion!=Potion::Empty,c.a.map_clock_ms);
        int bounce=0;if(holder.bouncing){const unsigned t=c.a.map_clock_ms-holder.bounce;const int phase=t<125?int(t)*1024/125:1024+int(t-125)*1024/125;bounce=(8*CombatCardMotion::sine(phase)+1536)/(3*1024);}
        const int pw=mx(1,(width-3)*scale/4096),ph=mx(1,18*scale/4096);
        const unsigned age=c.a.map_clock_ms-c.a.potion_removed_start;
        const bool removing=c.a.potion_removed!=Potion::Empty&&c.a.potion_removed_slot==slot&&age<(c.a.potion_removed_discard?400u:200u);
        if(potion!=Potion::Empty){if(!c.a.reward_pickup_hidden(RewardPickupKind::Potion,slot))c.picture(c.a.content.potion_image(potion),x+width/2-pw/2,11-ph/2-bounce,pw,ph);}
        else if(c.a.content.ui_image(U_PotionPlaceholder)>=0){const unsigned alpha=age<200&&c.a.potion_removed_slot==slot?0:age<400&&c.a.potion_removed_slot==slot?(age-200)*255/200:255;c.ui(U_PotionPlaceholder,x+width/2-pw/2,10-(15*scale/4096)/2,pw,mx(1,15*scale/4096),alpha);}
        else{c.box(x+4,5,7,12,rgb(92,90,83));c.rect(x+5,2,5,3,Muted);}
        if(removing){
            const int duration=c.a.potion_removed_discard?400:200,t=int(age)*4096/duration;
            const int back=int((int64_t(t)*t/4096)*(int64_t(11066)*t/4096-6970)/4096);
            const int scale=c.a.potion_removed_discard?4096:4096-back,w=mx(1,(width-3)*scale/4096),h=mx(1,18*scale/4096);
            c.picture(c.a.content.potion_image(c.a.potion_removed),x+width/2-w/2,11-h/2-(c.a.potion_removed_discard?back*20/4096:0),w,h);
        }
        c.hit(x,0,width,22,HitKind::PotionSlot,slot,potion!=Potion::Empty);
    }
    if(slots>Visible){const int page=page_x(slots);c.text("›",page,5,10,Gold);c.hit(page,0,PageWidth,22,HitKind::PotionPage,1);}
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
        c.ui(U_TopBarAscension,12,7,12,15);
        char level[8]{};TextBuffer text{level,sizeof level,0};text.number(int(ascension));
        health_label(c,level,12,11,12,8,Gold,Ink,true);
    }
    c.hit(0,0,26,22,HitKind::HudInfo,HudPortrait,hud_portrait_enabled(s));
    const auto health=c.a.combat_health(0);
    c.picture(A_Heart,27,4,10,13);char b[32]={};TextBuffer t{b,sizeof b,0,&c.a.content};t.number(health.hp);t.ch('/');t.number(health.max_hp);
    c.text_fitted(b,38,4,40,10,rgb(255,85,85));
    c.hit(26,0,52,22,HitKind::HudInfo,HudHp);
    c.picture(A_Coin,79,4,10,13);b[0]=0;t.n=0;t.number(c.a.reward_gold());c.text_fitted(b,90,4,24,10,Gold);
    c.hit(78,0,36,22,HitKind::HudInfo,HudGold);
    hud_potion_container(c);
    hud_room_nodes(c);
    hud_navigation(c,0,U_TopMap,267,257,20,Key::Map,!s.world.active||s.phase!=Phase::Map,c.a.panel==3);
    hud_navigation(c,1,U_TopDeck,287,277,20,Key::Inspect,true,c.a.panel==1&&c.a.pile_tab==0);
    hud_deck_count(c,296);
    hud_navigation(c,2,U_TopSettings,307,297,23,Key::Back,true,c.a.paused);
    if((!c.a.panel||c.a.panel==3)&&s.phase!=Phase::Complete&&s.phase!=Phase::Dead&&s.phase!=Phase::Error)owned_relic_strip(c);

}
