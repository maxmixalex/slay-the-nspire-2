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
static void invalidate_hand_tiles(){for(auto&t:hand_tiles)t.valid=false;invalidate_card_face_cache();}
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
    // Left inset contains the original star cost below the energy icon;
    // neither resource adds a second title row or moves the card portrait.
    draw_card_face(tile,card,8,6,69,97,energy,stars,instance,-1,nullptr,description);
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
    if(focused){
        // The outline belongs to the card face, not to its transparent cache
        // allocation. Transform the same 69 x 97 face and its 2px inset with
        // the hand pose so focus remains aligned after scale/rotation changes.
        const auto bounds=card_face::focus_bounds(8,6,69,97);
        const int xs[]={bounds.x,bounds.x+bounds.w-1,bounds.x+bounds.w-1,bounds.x};
        const int ys[]={bounds.y,bounds.y,bounds.y+bounds.h-1,bounds.y+bounds.h-1};
        int px[4],py[4];
        for(int i=0;i<4;++i){const int dx=xs[i]-HandTileW/2,dy=ys[i]-HandTileH/2;
            px[i]=p.cx+(dx*p.cosine-dy*p.sine)*p.scale/(1024*1024);
            py[i]=p.cy+(dx*p.sine+dy*p.cosine)*p.scale/(1024*1024);}
        for(int i=0;i<4;++i)c.line(px[i],py[i],px[(i+1)%4],py[(i+1)%4],t.playable?Gold:Muted);
    }
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
        else if(a.combat_health(MaxEnemies+1).hp>0)c.clip(AnimOstyIdle,112,123,a.animation*33);
        else c.ui(U_OstyPortrait,103,109,18,18,100);
    }
    int shared=-1;bool shared_action=false;
    for(int i=0;i<s.enemy_count;i++){
        if(s.enemy_world.active&&!a.enemy_opacity(i))continue;
        const auto*v=s.enemy_world.active?world_enemy_visual(a.enemy_presentation[i].model):nullptr;
        if(v&&v->shared_group){const auto&m=a.enemy_presentation[i];bool action=!m.loop&&int32_t(a.animation-m.start)>=0&&int32_t(m.until-a.animation)>0;
            if(shared<0||(action&&(!shared_action||int32_t(m.start-a.enemy_presentation[shared].start)>=0))||(!shared_action&&!action&&s.enemies[i].hp>0)){shared=i;shared_action=action;}continue;}
        if(s.enemies[i].hp>0||s.enemy_world.active){auto p=enemy_pose(s.enemy_count,i);Canvas actor=c;
            if(s.enemy_world.active)actor.opacity=uint8_t(a.enemy_opacity(i));actor.actor(i+1,p.x,p.feet,p.scale);}
    }
    if(shared>=0){int clip;uint32_t elapsed;bool loop;
        // The original Kaiser actor shares one background skeleton. Its baked
        // 320 x 218 frame is already in scene coordinates; draw it only once.
        if(a.enemy_frame(shared,clip,elapsed,loop)){Canvas actor=c;actor.opacity=uint8_t(a.enemy_opacity(shared));actor.clip(clip,0,22,elapsed,false,loop);}
        c.picture(a.content.world_image("background_front","KAISER_CRAB_BOSS"),0,22,320,218);
    }
}
static int energy_sine(int angle){
    static const int16_t quarter[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
    unsigned phase=unsigned(angle%360)*256/360;const int sign=phase>=128?-1:1;phase%=128;if(phase>64)phase=128-phase;
    const unsigned i=phase/4,j=phase%4;return sign*(i==16?quarter[i]:(quarter[i]*(4-j)+quarter[i+1]*j)/4);
}
static void combat_energy_layer(Canvas&c,int image,int angle,bool empty){
    // TextureRects share the 128px scene pivot. Resampling the two rotating
    // layers needs neither an intermediate framebuffer nor baked rotations.
    if(!angle&&!empty){c.picture(image,6,174,42,42);return;}
    const auto p=c.a.content.image(image);if(!p.pixels||p.format>3)return;
    const int sn=energy_sine(angle),cs=energy_sine((angle+90)%360),half=(21*((sn<0?-sn:sn)+(cs<0?-cs:cs))+1023)/1024;
    for(int y=mx(0,195-half);y<mn(c.clip_h,195+half);++y)for(int x=mx(0,27-half);x<mn(c.clip_w,27+half);++x){
        const int dx=x-27,dy=y-195,u=(dx*cs+dy*sn)+21*1024,v=(dy*cs-dx*sn)+21*1024;
        if(u<0||v<0||u>=42*1024||v>=42*1024)continue;
        const unsigned at=(v*p.h/(42*1024))*p.w+u*p.w/(42*1024),alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
        const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;uint16_t color=uint16_t(pixel[0]|unsigned(pixel[1])<<8);
        if(empty){
            // energy_orb_dark.tres: YIQ chroma x0.5/value x0.85, then
            // NEnergyCounter.RefreshLabel's DarkGray (169/255) modulation.
            const unsigned r=(color>>11)*255/31,g=((color>>5)&63)*255/63,b=(color&31)*255/31,gray=(299*r+587*g+114*b)/1000;
            color=rgb(int((r+gray)*563/2000),int((g+gray)*563/2000),int((b+gray)*563/2000));
        }
        c.paint(y*c.pitch+x,color,alpha);
    }
}
static void combat_energy(Canvas&c){
    auto&a=c.a;const auto&s=a.game;const int energy=a.combat_energy();const bool empty=energy==0;
    const uint32_t delta=a.map_clock_ms-a.energy_rotation_tick;
    a.energy_rotation_mdeg=(a.energy_rotation_mdeg+(delta%72000u)*(a.energy_rotation_zero?5u:30u))%360000u;
    a.energy_rotation_tick=a.map_clock_ms;a.energy_rotation_zero=empty;
    const int first=U_EnergyLayers+int(s.character)*5;
    if(a.content.ui_image(first)>=0){
        const int layers=s.character==Character::Necrobinder?3:5;
        for(int i=0;i<layers;i++){const bool rotates=i==1||(i==2&&s.character!=Character::Necrobinder);
            combat_energy_layer(c,a.content.ui_image(first+i),rotates?int((a.energy_rotation_mdeg*unsigned(i)/1000)%360):0,empty);}
    }else c.picture(character_energy_image(a.content,s.character),6,174,42,42);
    const uint16_t outline[]={rgb(77,19,13),rgb(0,79,4),rgb(22,62,100),rgb(120,64,0),rgb(112,45,111)};
    char label[24]{};TextBuffer b{label,sizeof label,0};b.number(energy);b.ch('/');b.number(a.combat_energy(true));
    const int size=c.width(label,12)<=37?12:10;const uint16_t edge=empty?rgb(55,24,26):outline[unsigned(s.character)];
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy)c.centered(label,6+dx,187+dy,42,size,edge);
    c.centered(label,6,187,42,size,empty?rgb(255,79,67):rgb(255,246,226));
}
static void combat_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(a.combat_background(),0,22,320,218);
    c.hit(0,44,320,140,HitKind::Battlefield);
    combat_actors(c);c.hp(a.combat_health(0),a.health_bars[0],29,129,62);
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
        c.hp(a.combat_health(MaxEnemies+1),a.health_bars[MaxEnemies+1],99,131,34);c.action(96,107,40,41,UiAction::ShowOsty);
    }
    const bool stars=s.character==Character::Regent||s.expansion.stars>0;
    if(stars){c.star_label(s.expansion.stars,3,149,10);c.action(0,147,48,20,UiAction::ShowStars);}
    status_strip(c,0,stars?51:24,149,stars?69:96);
    for(int i=0;i<s.enemy_count;i++){
        const auto&e=s.enemies[i];const auto pos=enemy_pose(s.enemy_count,i);int x=pos.x;
        if(a.combat_health(i+1).hp<=0){if(!s.enemy_world.active)c.centered("已击败",x-32,105,64,10,Muted);continue;}
        c.hit(x-pos.width/2-2,pos.intent,pos.width+4,pos.status-pos.intent+15,HitKind::Enemy,i);
        if(s.enemy_world.active){native_enemy_intent(c,i,pos);if(a.target==i)c.text("›",x-pos.width/2-7,pos.intent+3,10,Gold);}
        else{
            const bool attacking=e.move!=2;c.picture(attacking?A_AttackIntent:A_BuffIntent,x-pos.width/2,pos.intent,13,13);
            char label[32]{};TextBuffer tb{label,sizeof label,0};if(attacking)tb.number(enemy_damage(s,i));else tb.text("强化");
            c.text(label,x-pos.width/2+14,pos.intent,10,attacking?Gold:Green,pos.width-11);
            if(a.target==i)c.text("▼",x-5,pos.intent+14,8,Gold);
        }
        c.hp(a.combat_health(i+1),a.health_bars[i+1],x-pos.width/2,pos.hp,pos.width);status_strip(c,i+1,x-pos.width/2,pos.status,pos.width);

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
    combat_energy(c);
    native_end_turn(c,s.phase==Phase::Combat&&!a.busy);
    combat_pile(c,A_DrawPile,s.draw.count,8,215,1);
    combat_pile(c,A_DiscardPile,s.discard.count,291,215,2);
    if(s.exhaust.count)combat_pile(c,A_ExhaustPile,s.exhaust.count,293,106,3);
    // Dense power rows own both their pixels and pointer targets. Prefer the
    // usual side, then the opposite gutter; omit this redundant shortcut
    // label if neither fits. Keyboard Detail and card clicks still work.
    for(int side=0;side<2;++side){
        const int left=(stars!=bool(side))?265:0;bool blocked=false;
        for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
            if(h.enabled&&h.kind==HitKind::BattleStatus&&
               left<h.x+h.w&&h.x<left+49&&157<h.y+h.h&&h.y<174){blocked=true;break;}
        }
        if(blocked)continue;
        c.text("I 详情",left+3,160,10,Muted,44);c.hit(left,157,49,17,HitKind::Command,(int)Key::Detail,s.hand.count>0);break;
    }
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),150+age*4,163-age*5,26,19,(unsigned)(a.card_fly*18));}
}
