// Anchoring follows the owner's pinned combat_ui/player_hand/pile scenes.
// The integer fan, tile dimensions and preview are small-screen adaptations.
namespace {
struct HandTile {
    bool valid=false,playable=false;
    const Content* owner=nullptr;
    Card card{};
    int energy=0,stars=0,portrait=-1,frame=-1;
    char description[512]{};
    uint16_t pixels[HandTileW*HandTileH]{};
    uint8_t alpha[HandTileW*HandTileH]{};
};
static HandTile hand_tiles[MaxHand];
static bool same_text(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
}
static void invalidate_hand_tiles(){for(auto&t:hand_tiles)t.valid=false;}
static void card_title(Canvas&c,const char*name,int x,int y,int width,uint16_t ink){
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(name,x+dx,y+dy,width,10,Ink);
    c.centered(name,x,y,width,10,ink);
}
static const HandTile& hand_tile(Canvas&c,int index){
    auto&a=c.a;Card card=a.game.cards[a.game.hand.ids[index]];
    const int instance=a.game.hand.ids[index];int energy=cost(a.game,instance),stars=card_star_cost(a.game,card,instance);bool playable=can_play_card(a.game,instance);
    char description[512]{};a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,description,sizeof description,a.game.hand.ids[index]);
    int portrait=a.card_face_image(card,instance),frame=a.card_face_frame(card,instance);
    auto&t=hand_tiles[index];
    if(t.valid&&t.owner==&a.content&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded&&t.energy==energy&&t.stars==stars&&
       t.playable==playable&&t.portrait==portrait&&t.frame==frame&&same_text(t.description,description))return t;
    t.owner=&a.content;t.card=card;t.energy=energy;t.stars=stars;t.playable=playable;t.portrait=portrait;t.frame=frame;
    for(unsigned i=0;i<sizeof description;i++)t.description[i]=description[i];
    for(int i=0;i<HandTileW*HandTileH;i++){t.pixels[i]=0;t.alpha[i]=0;}
    Canvas tile{a,t.pixels,HandTileW,HandTileW,HandTileH,t.alpha};
    const bool star_card=card_star_cost(a.game,card)!=0;const int header=star_card?13:0;
    tile.picture(frame,3,5,74,104);
    tile.picture(portrait,8,24+header,64,44-header);
    tile.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,1,5+header,78,17);
    card_title(tile,a.content.card_string(card.kind),10,6+header,63,card.upgraded?Green:White);
    tile.picture(a.content.energy_image(card),0,0,18,18);tile.energy_label(energy,5,0,12);
    if(star_card)tile.star_label(stars,23,0,12);
    tile.centered(card_type_label(a.card_face_type(card,instance)),7,67,66,10,Muted);
    tile.paragraph(description,9,80,62,2,0,10,White);
    if(tile.paragraph(description,9,80,62,0,0,10)>2)tile.text("…",60,96,10,Gold,13);
    if(card.upgraded)tile.text("+",66,star_card?0:16,10,Green);
    if(!playable)for(int i=0;i<HandTileW*HandTileH;i++)if(t.alpha[i])t.pixels[i]=blend(t.pixels[i],Ink,85);
    t.valid=true;return t;
}
static void draw_hand_tile(Canvas&c,const HandTile&t,HandPose p,bool focused){
    int sn=p.sine<0?-p.sine:p.sine;
    int half_w=((p.cosine*HandTileW+sn*HandTileH)*p.scale)/(2*1024*1024)+2;
    int half_h=((sn*HandTileW+p.cosine*HandTileH)*p.scale)/(2*1024*1024)+2;
    int left=mx(0,p.cx-half_w),right=mn(320,p.cx+half_w);
    int top=mx(0,p.cy-half_h),bottom=mn(240,p.cy+half_h);
    // Inverse affine sampling: only integer additions in the inner loop.
    int du=p.cosine*1024/p.scale,dv=-p.sine*1024/p.scale;
    if(focused){c.box(p.cx-HandTileW/2+1,p.cy-HandTileH/2+3,78,107,t.playable?Gold:Muted);}
    for(int y=top;y<bottom;y++){
        int u=HandTileW*512+((left-p.cx)*p.cosine+(y-p.cy)*p.sine)*1024/p.scale;
        int v=HandTileH*512+(-(left-p.cx)*p.sine+(y-p.cy)*p.cosine)*1024/p.scale;
        for(int x=left;x<right;x++,u+=du,v+=dv){
            if(u<0||v<0||u>=HandTileW*1024||v>=HandTileH*1024)continue;
            int at=(v/1024)*HandTileW+u/1024;
            c.paint(y*c.pitch+x,t.pixels[at],t.alpha[at]);
        }
    }
}
static void combat_pile(Canvas&c,int image,int count,int x,int y,int tab){
    c.picture(image,x,y,23,23);
    char label[16]{};TextBuffer b{label,sizeof label,0};b.number(count);
    card_title(c,label,x-2,y+13,27,Gold);
    c.hit(x-3,y-3,29,28,HitKind::OpenPile,tab);
}
static void combat_actors(Canvas&c){
    auto&a=c.a;const auto&s=a.game;c.actor(0,60,124);
    const bool pet=s.character==Character::Necrobinder||s.expansion.osty_max_hp>0;
    if(pet){const auto&m=a.osty_motion;const auto clip=a.content.clip(m.clip);const unsigned elapsed=(a.animation-m.start)*33;
        if(m.clip>=0&&elapsed<(unsigned)clip.count*clip.frame_ms)c.clip(m.clip,112,123,elapsed,true,false);
        else if(s.expansion.osty_hp>0)c.clip(AnimOstyIdle,112,123,a.animation*33);
        else c.ui(U_OstyPortrait,103,109,18,18,100);
    }
    int shared=-1;bool shared_action=false;
    for(int i=0;i<s.enemy_count;i++){
        const auto*v=s.enemy_world.active?world_enemy_visual(unsigned(s.enemy_world.models[i].kind)):nullptr;
        if(v&&v->shared_group){const auto&m=a.enemy_presentation[i];bool action=!m.loop&&int32_t(a.animation-m.start)>=0&&int32_t(m.until-a.animation)>0;
            if(shared<0||(action&&(!shared_action||int32_t(m.start-a.enemy_presentation[shared].start)>=0))||(!shared_action&&!action&&s.enemies[i].hp>0)){shared=i;shared_action=action;}continue;}
        if(s.enemies[i].hp>0||s.enemy_world.active){auto p=enemy_pose(s.enemy_count,i);c.actor(i+1,p.x,p.feet,p.scale);}
    }
    if(shared>=0){int clip;uint32_t elapsed;bool loop;
        // The original Kaiser actor shares one background skeleton. Its baked
        // 320 x 218 frame is already in scene coordinates; draw it only once.
        if(a.enemy_frame(shared,clip,elapsed,loop))c.clip(clip,0,22,elapsed,false,loop);
        c.picture(a.content.world_image("background_front","KAISER_CRAB_BOSS"),0,22,320,218);
    }
}
static void combat_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(a.combat_background(),0,22,320,218);
    c.hit(0,44,320,140,HitKind::Battlefield);
    combat_actors(c);c.hp(ui_health_values(s,0),a.health_bars[0],29,129,62);
    if(s.expansion.orb_slots){
        const int count=s.expansion.orb_slots,left=mx(3,60-count*7);
        for(int i=0;i<count;i++){
            int x=left+(count-1-i)*14;
            if(i<s.expansion.orb_count)c.ui(orb_image(s.defect.orb_kind[i]),x,46,13,13);
            else c.box(x+2,48,9,9,Muted);
        }
        c.action(left-2,44,count*14+4,19,UiAction::ShowOrbs);
    }
    const bool pet=s.character==Character::Necrobinder||s.expansion.osty_max_hp>0;
    if(pet){
        c.hp(ui_health_values(s,MaxEnemies+1),a.health_bars[MaxEnemies+1],99,131,34);c.action(96,107,40,41,UiAction::ShowOsty);
    }
    const bool stars=s.character==Character::Regent||s.expansion.stars>0;
    if(stars){c.star_label(s.expansion.stars,3,149,10);c.action(0,147,48,20,UiAction::ShowStars);}
    status_strip(c,0,stars?51:24,149,stars?69:96);
    for(int i=0;i<s.enemy_count;i++){
        const auto&e=s.enemies[i];const auto pos=enemy_pose(s.enemy_count,i);int x=pos.x;
        if(!e.hp){if(!s.enemy_world.active)c.centered("已击败",x-32,105,64,10,Muted);continue;}
        c.hit(x-pos.width/2-2,pos.intent,pos.width+4,pos.status-pos.intent+15,HitKind::Enemy,i);
        if(s.enemy_world.active){native_enemy_intent(c,i,pos);if(a.target==i)c.text("›",x-pos.width/2-7,pos.intent+3,10,Gold);}
        else{
            const bool attacking=e.move!=2;c.picture(attacking?A_AttackIntent:A_BuffIntent,x-pos.width/2,pos.intent,13,13);
            char label[32]{};TextBuffer tb{label,sizeof label,0};if(attacking)tb.number(enemy_damage(s,i));else tb.text("强化");
            c.text(label,x-pos.width/2+14,pos.intent,10,attacking?Gold:Green,pos.width-11);
            if(a.target==i)c.text("▼",x-5,pos.intent+14,8,Gold);
        }
        c.hp(ui_health_values(s,i+1),a.health_bars[i+1],x-pos.width/2,pos.hp,pos.width);status_strip(c,i+1,x-pos.width/2,pos.status,pos.width);

    }
    for(int i=0;i<=s.enemy_count;i++){
        int age=(int)a.animation-a.hit_start[i];
        if(a.last_hit[i]&&age>=0&&age<24){auto p=enemy_pose(s.enemy_count,i?i-1:0);int x=i?p.x-12:48;c.num(a.last_hit[i],x,(i?p.feet-30:91)-age/2,s.enemy_count>4?10:16,i?Gold:Red);}
    }
    int selected=mn(mx(a.cursor,0),mx(0,s.hand.count-1));
    const bool focused=a.hand_focus&&a.hand_can_focus();
    // At rest every card remains on the arc. Focus is earned by hover or keys,
    // not by the cursor index merely existing. The battlefield stays visible.
    for(int i=0;i<s.hand.count;i++){
        auto p=hand_pose(s.hand.count,i,false);
        if(!focused||i!=selected)draw_hand_tile(c,hand_tile(c,i),p,false);
        auto r=hand_rail(s.hand.count,i);
        int top=p.cy-HandTileH*p.scale/(2*1024);
        c.hit(r.x,top,r.w,mx(0,r.y-top),HitKind::Hand,s.hand.ids[i],!a.busy&&can_play_card(s,s.hand.ids[i]));
    }
    if(s.hand.count){
        if(focused){
            draw_hand_tile(c,hand_tile(c,selected),hand_pose(s.hand.count,selected,true),true);
            auto p=hand_pose(s.hand.count,selected,true);
            c.hit(p.cx-HandTileW/2,p.cy-HandTileH/2,HandTileW,68,HitKind::Hand,s.hand.ids[selected],!a.busy&&can_play_card(s,s.hand.ids[selected]));
        }
        // A stable bottom selection rail prevents the raised preview from
        // stealing its neighbours when the pointer crosses from card to card.
        for(int i=0;i<s.hand.count;i++){
            auto r=hand_rail(s.hand.count,i);
            c.hit(r.x,r.y,r.w,r.h,HitKind::Hand,s.hand.ids[i],!a.busy&&can_play_card(s,s.hand.ids[i]));
        }
    }else c.centered("没有手牌",80,197,160,12,Muted);
    c.picture(character_energy_image(a.content,s.character),6,174,42,42);
    c.num(s.energy,18,182,20,White);
    native_end_turn(c,s.phase==Phase::Combat&&!a.busy);
    combat_pile(c,A_DrawPile,s.draw.count,8,215,1);
    combat_pile(c,A_DiscardPile,s.discard.count,291,215,2);
    if(s.exhaust.count)combat_pile(c,A_ExhaustPile,s.exhaust.count,293,106,3);
    c.text("I 详情",stars?268:3,160,10,Muted,44);c.hit(stars?265:0,157,49,17,HitKind::Command,(int)Key::Detail,s.hand.count>0);
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),150+age*4,163-age*5,26,19,(unsigned)(a.card_fly*18));}
}
