// Anchoring follows the owner's pinned combat_ui/player_hand/pile scenes.
// The integer fan, tile dimensions and preview are small-screen adaptations.
namespace {
struct HandTile {
    bool valid=false,playable=false;
    const Content* owner=nullptr;
    Card card{};
    uint32_t used=0;
    int energy=0,stars=0,portrait=-1,frame=-1;
    char description[512]{};
    uint16_t pixels[HandTileW*HandTileH]{};
    uint8_t alpha[HandTileW*HandTileH]{};
};
static HandTile hand_tiles[MaxHand];
static uint32_t hand_tile_clock=0;
static bool same_text(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
}
static void invalidate_hand_tiles(){for(auto&t:hand_tiles)t.valid=false;invalidate_card_face_cache();}
static void card_title(Canvas&c,const char*name,int x,int y,int width,uint16_t ink){
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(name,x+dx,y+dy,width,10,Ink);
    c.centered(name,x,y,width,10,ink);
}
static const HandTile& hand_tile(Canvas&c,int instance,const CombatCardMotion::Sprite* visual=nullptr){
    auto&a=c.a;Card card=visual?visual->card:a.game.cards[instance];
    int energy=visual?visual->energy:cost(a.game,instance),stars=visual?visual->stars:card_star_cost(a.game,card,instance);
    bool playable=visual||can_play_card(a.game,instance);
    char description[512]{};a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,description,sizeof description,instance);
    int portrait=a.card_face_image(card,instance),frame=a.card_face_frame(card,instance);
    ++hand_tile_clock;HandTile*slot=nullptr;
    // A holder index changes when a neighbour leaves the hand. Reuse the
    // visual key across hand, play and flight instead of repainting every
    // shifted slot, while keeping the same ten 80 x 112 cache allocations.
    for(auto&t:hand_tiles){
        if(t.valid&&t.owner==&a.content&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded&&t.energy==energy&&t.stars==stars&&
           t.playable==playable&&t.portrait==portrait&&t.frame==frame&&same_text(t.description,description)){t.used=hand_tile_clock;return t;}
        if(!slot||(!t.valid&&slot->valid)||(t.valid==slot->valid&&int32_t(t.used-slot->used)<0))slot=&t;
    }
    auto&t=*slot;t.used=hand_tile_clock;
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
static void draw_hand_tile(Canvas&c,const HandTile&t,HandPose p,bool focused,unsigned brightness=255){
    if(p.scale<=0)return;
    int sn=p.sine<0?-p.sine:p.sine,cs=p.cosine<0?-p.cosine:p.cosine;
    int half_w=((cs*HandTileW+sn*HandTileH)*p.scale)/(2*1024*1024)+2;
    int half_h=((sn*HandTileW+cs*HandTileH)*p.scale)/(2*1024*1024)+2;
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
            uint16_t color=t.pixels[at];
            if(brightness<255)color=CombatCardMotion::modulate(color,brightness);
            c.paint(y*c.pitch+x,color,t.alpha[at]);
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
static void combat_energy_layer(Canvas&c,int image,int angle,bool empty,int left=6,int top=174,int size=42,bool star=false,unsigned light=255){
    // TextureRects share the 128px scene pivot. Resampling the two rotating
    // layers needs neither an intermediate framebuffer nor baked rotations.
    if(!angle&&!empty&&light==255){c.picture(image,left,top,size,size);return;}
    const auto p=c.a.content.image(image);if(!p.pixels||p.format>3)return;
    const int radius=size/2,cx=left+radius,cy=top+radius;
    const int sn=energy_sine(angle),cs=energy_sine((angle+90)%360),half=(radius*((sn<0?-sn:sn)+(cs<0?-cs:cs))+1023)/1024;
    for(int y=mx(0,cy-half);y<mn(c.clip_h,cy+half);++y)for(int x=mx(0,cx-half);x<mn(c.clip_w,cx+half);++x){
        const int dx=x-cx,dy=y-cy,u=(dx*cs+dy*sn)+radius*1024,v=(dy*cs-dx*sn)+radius*1024;
        if(u<0||v<0||u>=size*1024||v>=size*1024)continue;
        const unsigned at=(v*p.h/(size*1024))*p.w+u*p.w/(size*1024),alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
        const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;uint16_t color=uint16_t(pixel[0]|unsigned(pixel[1])<<8);
        if(empty){
            // energy_orb_dark.tres: YIQ chroma x0.5/value x0.85, then
            // NEnergyCounter.RefreshLabel's DarkGray (169/255) modulation.
            const unsigned r=(color>>11)*255/31,g=((color>>5)&63)*255/63,b=(color&31)*255/31;
            // hsv.gdshader halves saturation toward the maximum channel;
            // the energy-orb material instead uses the YIQ gray luminance.
            const unsigned gray=star?unsigned(mx(int(r),mx(int(g),int(b)))):(299*r+587*g+114*b)/1000;
            const unsigned value=star?850:563;
            color=rgb(int((r+gray)*value/2000),int((g+gray)*value/2000),int((b+gray)*value/2000));
        }
        if(light>255){const unsigned r=(color>>11)*255/31,g=((color>>5)&63)*255/63,b=(color&31)*255/31;
            color=rgb(mn(255,int(r*light/255)),mn(255,int(g*light/255)),mn(255,int(b*light/255)));}
        c.paint(y*c.pitch+x,color,alpha);
    }
}
static bool combat_stars_visible(App&a){
    auto&v=a.combat_status;const auto&s=a.game;
    if(v.visual_seed!=s.seed||v.visual_floor!=s.floor||v.visual_combat!=s.world.combat_count||v.visual_encounter!=int(s.enemy_world.encounter)){
        v.reset_visuals();v.visual_seed=s.seed;v.visual_floor=s.floor;v.visual_combat=s.world.combat_count;v.visual_encounter=int(s.enemy_world.encounter);v.star_tick=a.map_clock_ms;
    }
    const int count=a.combat_status_amount(0,44);
    v.stars_visible=v.stars_visible||s.character==Character::Regent||count>0;
    if(count>v.star_observed)v.star_gain_at=a.animation;
    if(count<v.star_observed){v.star_lerp=float(count);v.star_velocity=0;v.star_gain_at=a.animation-7;}
    const uint32_t delta=a.map_clock_ms-v.star_tick;
    v.star_rotation_mdeg=(v.star_rotation_mdeg+(delta%72000u)*(v.star_zero?5u:30u))%360000u;
    v.star_tick=a.map_clock_ms;v.star_zero=count==0;v.star_observed=count;
    if(delta&&v.star_lerp!=float(count)){
        // NStarCounter uses MathHelper.SmoothDamp with smoothTime=.1.
        // One scalar update per rendered frame; no particles or heap work.
        const float dt=float(delta<1000u?delta:1000u)/1000.f,x=20.f*dt;
        const float decay=1.f/(1.f+x+.48f*x*x+.235f*x*x*x),change=v.star_lerp-float(count);
        const float step=(v.star_velocity+20.f*change)*dt;
        v.star_velocity=(v.star_velocity-20.f*step)*decay;const float next=float(count)+(change+step)*decay;
        if((float(count)-v.star_lerp>0)==(next>float(count))){v.star_lerp=float(count);v.star_velocity=0;}
        else v.star_lerp=next;
    }
    return v.stars_visible;
}
static void combat_energy(Canvas&c,bool stars){
    auto&a=c.a;const auto&s=a.game;const int energy=a.combat_energy();const bool empty=energy==0;
    // NCombatUi raises the Regent's energy counter. The star scene stays
    // below-left (and at 0.8 scale), outside the creature's power strip.
    const bool regent=stars&&s.character==Character::Regent;
    const int left=stars?18:6,top=regent?162:174;
    const uint32_t delta=a.map_clock_ms-a.energy_rotation_tick;
    a.energy_rotation_mdeg=(a.energy_rotation_mdeg+(delta%72000u)*(a.energy_rotation_zero?5u:30u))%360000u;
    a.energy_rotation_tick=a.map_clock_ms;a.energy_rotation_zero=empty;
    const int first=U_EnergyLayers+int(s.character)*5;
    if(a.content.ui_image(first)>=0){
        const int layers=s.character==Character::Necrobinder?3:5;
        for(int i=0;i<layers;i++){const bool rotates=i==1||(i==2&&s.character!=Character::Necrobinder);
            combat_energy_layer(c,a.content.ui_image(first+i),rotates?int((a.energy_rotation_mdeg*unsigned(i)/1000)%360):0,empty,left,top);}
    }else c.picture(character_energy_image(a.content,s.character),left,top,42,42);
    const uint16_t outline[]={rgb(77,19,13),rgb(0,79,4),rgb(22,62,100),rgb(120,64,0),rgb(112,45,111)};
    char label[24]{};TextBuffer b{label,sizeof label,0};b.number(energy);b.ch('/');b.number(a.combat_energy(true));
    const int size=c.width(label,12)<=37?12:10;const uint16_t edge=empty?rgb(55,24,26):outline[unsigned(s.character)];
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy)c.centered(label,left+dx,top+13+dy,42,size,edge);
    c.centered(label,left,top+13,42,size,empty?rgb(255,85,85):rgb(255,246,226));
    c.hit(left,top,42,42,HitKind::Battlefield);
    if(stars){
        const int count=int(a.combat_status.star_lerp+.5f),age=int32_t(a.animation-a.combat_status.star_gain_at)*33,star_top=regent?183:187;
        const unsigned light=count&&age>=0&&age<200?255+unsigned(200-age)*255/200:255;
        if(a.content.version()>=FrontendContentVersion){
            for(int layer=0;layer<3;++layer)combat_energy_layer(c,a.content.ui_image(F_StarCounterBase+layer),
                layer?int(a.combat_status.star_rotation_mdeg*unsigned(layer)/1000u%360u):0,count==0,6,star_top,34,true,light);
        }else combat_energy_layer(c,a.content.ui_image(U_StarCounter),0,count==0,6,star_top,34,true,light);
        char number[16]{};TextBuffer b{number,sizeof number,0};b.number(count);
        const int font=c.width(number,12)<=29?12:10;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.centered(number,6+dx,star_top+11+dy,34,font,rgb(0,62,117));
        c.centered(number,6,star_top+11,34,font,count?rgb(255,246,226):rgb(255,85,85));
        c.action(6,star_top,34,32,UiAction::ShowStars);
    }
}
static void combat_resource_tooltip(Canvas&c,bool stars){
    auto&a=c.a;if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.panel||a.paused||a.help||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0)return;
    const bool regent=stars&&a.game.character==Character::Regent;
    const int px=a.pointer.x,py=a.pointer.y,left=stars?18:6,top=regent?162:174,star_top=regent?183:187;
    const bool star=stars&&px>=6&&px<40&&py>=star_top&&py<star_top+32;
    if(!star&&!(px>=left&&px<left+42&&py>=top&&py<top+42))return;
    const char*key=star?"STAR_COUNT":"ENERGY_COUNT";
    char full[64]{},body[1024]{};TextBuffer name{full,sizeof full,0};name.text(key);name.text(".description");
    a.localized_text("static_hover_tips",full,body,sizeof body);
    if(!*body)return;const int lines=mn(7,c.paragraph(body,0,0,148,0,0,10)),height=27+lines*13,x=5,y=mx(25,top-height-5);
    const int skin=a.content.world_image("common","HOVER_TIP");
    if(skin>=0)c.nine_image(skin,x,y,160,height,28,22,46,16);else{c.rect(x,y,160,height,Panel);c.box(x,y,160,height,Gold);}
    c.text(a.content.world_title("static_hover_tips",key),x+6,y+4,10,Gold,148);
    c.paragraph(body,x+6,y+22,148,7,0,10,White);
}
static void combat_power_popups(Canvas&c){
    const auto&a=c.a;const auto&v=a.combat_status;
    for(unsigned i=0;i<v.popup_count;++i){const auto&p=v.popups[i];const int ms=int32_t(a.animation-p.at)*33;
        if(ms<0||ms>=1250||p.actor>unsigned(a.game.enemy_count))continue;
        const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[p.actor]:ui_feedback_identity(a.game,p.actor);
        if(identity!=p.identity)continue;
        const auto pose=enemy_pose(a.game.enemy_count,p.actor?p.actor-1:0);const int x=p.actor?pose.x:60,cy=p.actor?pose.feet-32:91;
        // Original 256px icon grows from 0.4 to 0.8 with Expo.Out; its
        // half-alpha texture and echo peak at 0.25 effective opacity.
        const unsigned growth=TurnBannerPresentation::ease(unsigned(ms),1250);const int size=17+int(growth*17/4096);
        const unsigned icon_alpha=ms<250?unsigned(64*CombatCardMotion::sine(ms*1024/250)/1024):unsigned(64*CombatCardMotion::sine((1250-ms)*1024/1000)/1024);
        c.picture(ui_status_image(a,p.id),x-size/2,cy-size/2,size,size,icon_alpha);
        c.picture(ui_status_image(a,p.id),x-size*11/20,cy-size*11/20,size*11/10,size*11/10,icon_alpha/2);
        if(ms>=1000)continue;
        const unsigned alpha=ms<250?TurnBannerPresentation::ease(unsigned(ms),250):TurnBannerPresentation::ease(unsigned(1000-ms),750);
        const unsigned moved=CombatCardMotion::cubic(unsigned(ms),1250);const int y=cy-33+(p.buff?-1:1)*int(moved*17/1024);
        const char*title=ui_status_title(a,p.id);const int width=mn(130,c.width(title,10)),left=mx(2,mn(318-width,x-width/2));
        Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/4096);
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)text.text(title,left+dx,y+dy,10,Ink,width);
        text.text(title,left,y,10,p.buff?rgb(127,255,0):rgb(255,85,85),width);
    }
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
    const bool stars=combat_stars_visible(a);
    status_strip(c,0,24,149,96);
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
    combat_power_popups(c);
    const auto hand=a.combat_hand();
    int selected=mn(mx(a.cursor,0),mx(0,hand.count-1));
    const bool focused=a.hand_focus&&a.hand_can_focus();
    if(ui_cards_bound(a))a.combat_cards.set_focus(focused&&hand.count?hand.ids[selected]:-1);
    // At rest every card remains on the arc. Focus is earned by hover or keys,
    // not by the cursor index merely existing. The battlefield stays visible.
    for(int i=0;i<hand.count;i++){
        // The focused face has its own topmost region below. Registering its
        // raised pose here as well leaves a duplicate (and sometimes wider)
        // hit column behind the visible face.
        if(focused&&i==selected)continue;
        auto p=a.combat_hand_pose(unsigned(i),focused&&i==selected);
        draw_hand_tile(c,hand_tile(c,hand.ids[i]),p,false);
        auto r=hand_rail(hand.count,i);
        int top=p.cy-HandTileH*p.scale/(2*1024);
        if(a.combat_hand_settled(unsigned(i),focused&&i==selected))
            c.hit(r.x,top,r.w,mx(0,r.y-top),HitKind::Hand,hand.ids[i],!a.combat_actions_locked()&&can_play_card(s,hand.ids[i]));
        else{
            const int sn=CombatCardMotion::magnitude(p.sine),cs=CombatCardMotion::magnitude(p.cosine);
            const int w=(cs*HandTileW+sn*HandTileH)*p.scale/(1024*1024),h=(sn*HandTileW+cs*HandTileH)*p.scale/(1024*1024);
            c.hit(p.cx-w/2,p.cy-h/2,w,h,HitKind::Hand,hand.ids[i],!a.combat_actions_locked()&&can_play_card(s,hand.ids[i]));
        }
    }
    if(hand.count){
        if(focused){
            auto p=a.combat_hand_pose(unsigned(selected),true);
            draw_hand_tile(c,hand_tile(c,hand.ids[selected]),p,true);
            // While X is still approaching the focused position there is no
            // settled bottom rail. Keep its actual lower face hoverable too.
            const int height=a.combat_hand_settled(unsigned(selected),true)?68:HandTileH;
            c.hit(p.cx-HandTileW/2,p.cy-HandTileH/2,HandTileW,height,HitKind::Hand,hand.ids[selected],!a.combat_actions_locked()&&can_play_card(s,hand.ids[selected]));
        }
        // A stable bottom selection rail prevents the raised preview from
        // stealing its neighbours when the pointer crosses from card to card.
        for(int i=0;i<hand.count;i++){
            if(!a.combat_hand_settled(unsigned(i),focused&&i==selected))continue;
            auto r=hand_rail(hand.count,i);
            c.hit(r.x,r.y,r.w,r.h,HitKind::Hand,hand.ids[i],!a.combat_actions_locked()&&can_play_card(s,hand.ids[i]));
        }
    }else c.centered("没有手牌",80,197,160,12,Muted);
    if(ui_cards_bound(a))for(const auto&v:a.combat_cards.sprites){
        if(v.mode==CombatCardMotion::Empty||v.mode==CombatCardMotion::Hand||v.id>=s.card_count)continue;
        draw_hand_tile(c,hand_tile(c,v.id,&v),CombatCardMotion::pose(v),false,a.combat_cards.brightness(v));
    }
    combat_energy(c,stars);
    native_end_turn(c,s.phase==Phase::Combat&&!a.combat_actions_locked());
    const auto piles=a.combat_pile_counts();
    combat_pile(c,A_DrawPile,piles.draw,8,215,1);
    combat_pile(c,A_DiscardPile,piles.discard,291,215,2);
    if(piles.exhaust)combat_pile(c,A_ExhaustPile,piles.exhaust,293,106,3);
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
        c.text("I 详情",left+3,160,10,Muted,44);c.hit(left,157,49,17,HitKind::Command,(int)Key::Detail,hand.count>0);break;
    }
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),150+age*4,163-age*5,26,19,(unsigned)(a.card_fly*18));}
    combat_resource_tooltip(c,stars);
}
