// Anchoring follows the owner's pinned combat_ui/player_hand/pile scenes.
// The integer fan, tile dimensions and preview are small-screen adaptations.
namespace {
struct HandTile {
    bool valid=false,playable=false,resource_cross=false,description_truncated=false;
    uint8_t resource_styles=0;
    const Content* owner=nullptr;
    Card card{};
    uint32_t used=0;
    uint32_t sampled_frame=0,sampled_revision=0;
    int sampled_instance=-1,sampled_deck=-1,sampled_target=-1;
    const CombatCardMotion::Sprite*sampled_visual=nullptr;
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
static const HandTile& hand_tile(Canvas&c,int instance,const CombatCardMotion::Sprite* visual=nullptr,int deck=-1){
    auto&a=c.a;Card card=visual?visual->card:a.game.cards[instance];
    ++hand_tile_clock;
    // A selected face is requested again by its automatic full-description
    // tip in the same frame. Reuse the already resolved instance sample;
    // target/revision/card identity guard input arriving between frames.
    for(auto&t:hand_tiles)if(t.valid&&t.owner==&a.content&&t.sampled_frame==small_card_frame&&
        t.sampled_revision==a.game.revision&&t.sampled_instance==instance&&t.sampled_deck==deck&&
        t.sampled_target==a.target&&t.sampled_visual==visual&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded){t.used=hand_tile_clock;return t;}
    auto stamp=[&](HandTile&t){t.sampled_frame=small_card_frame;t.sampled_revision=a.game.revision;t.sampled_instance=instance;t.sampled_deck=deck;t.sampled_target=a.target;t.sampled_visual=visual;};
    int energy=visual?visual->energy:cost(a.game,instance),stars=visual?visual->stars:card_star_cost(a.game,card,instance);
    const unsigned resource_styles=card_face_resource_styles(a,card,energy,stars,instance,event_card_facts(a.game,card,instance,deck));
    const bool resource_cross=card_face_resource_cross(a,card,energy,stars,instance);
    bool playable=visual||can_play_card(a.game,instance);
    char description[512]{};a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,description,sizeof description,instance,deck);
    int portrait=a.card_face_image(card,instance,deck),frame=a.card_face_frame(card,instance,deck);
    HandTile*slot=nullptr;
    // A holder index changes when a neighbour leaves the hand. Reuse the
    // visual key across hand, play and flight instead of repainting every
    // shifted slot, while keeping the same ten 80 x 112 cache allocations.
    for(auto&t:hand_tiles){
        if(t.valid&&t.owner==&a.content&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded&&t.energy==energy&&t.stars==stars&&
           t.playable==playable&&t.resource_cross==resource_cross&&t.resource_styles==resource_styles&&t.portrait==portrait&&t.frame==frame&&same_text(t.description,description)){t.used=hand_tile_clock;stamp(t);return t;}
        if(!slot||(!t.valid&&slot->valid)||(t.valid==slot->valid&&int32_t(t.used-slot->used)<0))slot=&t;
    }
    auto&t=*slot;t.used=hand_tile_clock;stamp(t);
    t.owner=&a.content;t.card=card;t.energy=energy;t.stars=stars;t.playable=playable;t.portrait=portrait;t.frame=frame;
    t.resource_styles=uint8_t(resource_styles);
    t.resource_cross=resource_cross;
    for(unsigned i=0;i<sizeof description;i++)t.description[i]=description[i];
    for(int i=0;i<HandTileW*HandTileH;i++){t.pixels[i]=0;t.alpha[i]=0;}
    Canvas tile{a,t.pixels,HandTileW,HandTileW,HandTileH,t.alpha};
    // Left inset contains the original star cost below the energy icon;
    // neither resource adds a second title row or moves the card portrait.
    draw_card_face(tile,card,HandFaceX,HandFaceY,HandFaceW,HandFaceH,energy,stars,instance,deck,nullptr,description,&t.description_truncated);
    // NHandCardHolder hides its playable highlight; it does not gray the
    // entire portrait, description and resource label when a card cannot play.
    t.valid=true;return t;
}
static void draw_hand_tile(Canvas&c,const HandTile&t,HandPose p,bool focused,unsigned brightness=255){
    if(p.scale<=0)return;
    int sn=p.sine<0?-p.sine:p.sine,cs=p.cosine<0?-p.cosine:p.cosine;
    constexpr int radius_x=(HandPivotX+1023)/1024,radius_y=(HandTileH*1024-HandPivotY+1023)/1024;
    int half_w=((cs*radius_x+sn*radius_y)*p.scale)/(1024*1024)+2;
    int half_h=((sn*radius_x+cs*radius_y)*p.scale)/(1024*1024)+2;
    int left=mx(0,p.cx-half_w),right=mn(320,p.cx+half_w);
    int top=mx(0,p.cy-half_h),bottom=mn(240,p.cy+half_h);
    // Inverse affine sampling: only integer additions in the inner loop.
    int du=p.cosine*1024/p.scale,dv=-p.sine*1024/p.scale;
    if(focused){
        // The outline belongs to the card face, not to its transparent cache
        // allocation. Transform the same 69 x 97 face and its 2px inset with
        // the hand pose so focus remains aligned after scale/rotation changes.
        const auto bounds=card_face::focus_bounds(HandFaceX,HandFaceY,HandFaceW,HandFaceH);
        const int xs[]={bounds.x,bounds.x+bounds.w-1,bounds.x+bounds.w-1,bounds.x};
        const int ys[]={bounds.y,bounds.y,bounds.y+bounds.h-1,bounds.y+bounds.h-1};
        int px[4],py[4];
        for(int i=0;i<4;++i){const int dx=xs[i]*1024-HandPivotX,dy=ys[i]*1024-HandPivotY;
            px[i]=p.cx+int((int64_t(dx)*p.cosine-int64_t(dy)*p.sine)*p.scale/(1024LL*1024*1024));
            py[i]=p.cy+int((int64_t(dx)*p.sine+int64_t(dy)*p.cosine)*p.scale/(1024LL*1024*1024));}
        for(int i=0;i<4;++i)c.line(px[i],py[i],px[(i+1)%4],py[(i+1)%4],t.playable?Gold:Muted);
    }
    for(int y=top;y<bottom;y++){
        int u=HandPivotX+((left-p.cx)*p.cosine+(y-p.cy)*p.sine)*1024/p.scale;
        int v=HandPivotY+(-(left-p.cx)*p.sine+(y-p.cy)*p.cosine)*1024/p.scale;
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
    // All three authored controls are80px square in the1680x1260 viewport.
    // Draw/discard counts sit on opposite lower corners of those controls.
    c.picture(image,x,y,15,15);
    char label[16]{};TextBuffer b{label,sizeof label,0};b.number(count);
    const int count_x=x+(tab==1?7:tab==2?-5:-1),count_y=y+(tab==3?3:7);
    health_label(c,label,count_x,count_y,17,8,Gold,Ink);
    c.hit(x,y,15,15,HitKind::OpenPile,tab);
}
static void combat_hand_index(Canvas&c,unsigned index,HandPose p){
 if(!c.a.show_card_indices||p.scale<=0)return;
 // NHandCardHolder.HandIndex is centered256 source px above a300x422
 // face. The port face is69x97px; keep the holder's rotation and scale.
 uint16_t pixels[16*12]{};uint8_t alpha[16*12]{};
 Canvas tile{c.a,pixels,16,16,12,alpha};char label[8]{};TextBuffer b{label,sizeof label,0};b.number(int(index+1));
 health_label(tile,label,0,2,16,8,rgb(255,246,226),Ink);
 const int cx=p.cx+59*p.sine*p.scale/(1024*1024),cy=p.cy-59*p.cosine*p.scale/(1024*1024);
 const Picture image{reinterpret_cast<const uint8_t*>(pixels),alpha,nullptr,16,12,1};
 ui_picture_transform(c,image,cx,cy,mx(1,16*p.scale/1024),mx(1,12*p.scale/1024),p.sine,p.cosine);
}
static void combat_actors(Canvas&c){
    auto&a=c.a;const auto&s=a.game;const auto player=a.combat_player_pose();c.actor(0,player.sprite_x,player.sprite_y,player.scale);
    const bool pet=s.character==Character::Necrobinder||s.expansion.osty_max_hp>0;
    if(pet){const auto p=a.combat_player_pose(true);const auto&m=a.osty_motion;const auto clip=a.content.clip(m.clip);const unsigned elapsed=(a.animation-m.start)*33;
        if(m.clip>=0&&elapsed<(unsigned)clip.count*clip.frame_ms)c.scaled_clip(m.clip,p.sprite_x,p.sprite_y,elapsed,p.scale,false);
        else if(a.combat_health(MaxEnemies+1).hp>0)c.scaled_clip(AnimOstyIdle,p.sprite_x,p.sprite_y,a.animation*33,p.scale);
        else{const auto dead=a.content.clip(AnimOstyDie);c.scaled_clip(AnimOstyDie,p.sprite_x,p.sprite_y,dead.count?uint32_t(dead.count-1)*dead.frame_ms:0u,p.scale,false);}
    }
    int shared=-1;bool shared_action=false;
    for(int i=0;i<s.enemy_count;i++){
        if(s.enemy_world.active&&!a.enemy_opacity(i))continue;
        const auto*v=s.enemy_world.active?world_enemy_visual(a.enemy_presentation[i].model):nullptr;
        if(v&&v->shared_group){const auto&m=a.enemy_presentation[i];bool action=!m.loop&&int32_t(a.animation-m.start)>=0&&int32_t(m.until-a.animation)>0;
            if(shared<0||(action&&(!shared_action||int32_t(m.start-a.enemy_presentation[shared].start)>=0))||(!shared_action&&!action&&s.enemies[i].hp>0)){shared=i;shared_action=action;}continue;}
        if(s.enemies[i].hp>0||s.enemy_world.active){auto p=a.combat_enemy_pose(i);Canvas actor=c;
            if(s.enemy_world.active)actor.opacity=uint8_t(a.enemy_opacity(i));actor.actor(i+1,p.sprite_x,p.sprite_y,p.scale);}
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
static void combat_energy_layer(Canvas&c,int image,int angle,bool empty,int left=19,int top=192,int size=24,bool star=false,unsigned light=255){
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
    if(count<v.star_observed){v.star_count.decrease(count);v.star_gain_at=a.animation-7;}
    const uint32_t delta=a.map_clock_ms-v.star_tick;
    v.star_rotation_mdeg=(v.star_rotation_mdeg+(delta%72000u)*(v.star_zero?5u:30u))%360000u;
    v.star_tick=a.map_clock_ms;v.star_zero=count==0;v.star_observed=count;
    v.star_count.advance(count,delta);
    return v.stars_visible;
}
static void combat_energy(Canvas&c,bool stars){
    auto&a=c.a;const auto&s=a.game;const int energy=a.combat_energy();const bool empty=energy==0;
    // NCombatUi raises the Regent's energy counter. The star scene stays
    // below-left (and at 0.8 scale), outside the creature's power strip.
    const bool regent=stars&&s.character==Character::Regent;
    const int left=19,top=regent?154:192;
    const uint32_t delta=a.map_clock_ms-a.energy_rotation_tick;
    a.energy_rotation_mdeg=(a.energy_rotation_mdeg+(delta%72000u)*(a.energy_rotation_zero?5u:30u))%360000u;
    a.energy_rotation_tick=a.map_clock_ms;a.energy_rotation_zero=empty;
    const int first=U_EnergyLayers+int(s.character)*5;
    if(a.content.ui_image(first)>=0){
        const int layers=s.character==Character::Necrobinder?3:5;
        for(int i=0;i<layers;i++){const bool rotates=i==1||(i==2&&s.character!=Character::Necrobinder);
            combat_energy_layer(c,a.content.ui_image(first+i),rotates?int((a.energy_rotation_mdeg*unsigned(i)/1000)%360):0,empty,left,top);}
    }else c.picture(character_energy_image(a.content,s.character),left,top,24,24);
    const uint16_t outline[]={rgb(77,19,13),rgb(0,79,4),rgb(22,62,100),rgb(120,64,0),rgb(112,45,111)};
    char label[24]{};TextBuffer b{label,sizeof label,0};b.number(energy);b.ch('/');b.number(a.combat_energy(true));
    const int size=8;const uint16_t edge=empty?rgb(55,24,26):outline[unsigned(s.character)];
    health_label(c,label,left-2,top+8,28,size,empty?rgb(255,85,85):rgb(255,246,226),edge);
    c.hit(left,top,24,24,HitKind::Battlefield);
    if(stars){
        const int count=a.combat_status.star_count.displayed(),age=int32_t(a.animation-a.combat_status.star_gain_at)*33,star_top=200;
        const unsigned light=count&&age>=0&&age<200?255+unsigned(200-age)*255/200:255;
        if(a.content.version()>=FrontendContentVersion){
            for(int layer=0;layer<3;++layer)combat_energy_layer(c,a.content.ui_image(F_StarCounterBase+layer),
                layer?int(a.combat_status.star_rotation_mdeg*unsigned(layer)/1000u%360u):0,count==0,12,star_top,20,true,light);
        }else combat_energy_layer(c,a.content.ui_image(U_StarCounter),0,count==0,12,star_top,20,true,light);
        char number[16]{};TextBuffer b{number,sizeof number,0};b.number(count);
        health_label(c,number,12,star_top+7,20,8,count?rgb(255,246,226):rgb(255,85,85),rgb(0,62,117));
        c.action(12,star_top,20,20,UiAction::ShowStars);
    }
}
static void combat_resource_tooltip(Canvas&c,bool stars){
    auto&a=c.a;if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.panel||a.paused||a.help||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0)return;
    const bool regent=stars&&a.game.character==Character::Regent;
    const int px=a.pointer.x,py=a.pointer.y,left=19,top=regent?154:192,star_top=200;
    const bool star=stars&&px>=12&&px<32&&py>=star_top&&py<star_top+20;
    if(!star&&!(px>=left&&px<left+24&&py>=top&&py<top+24))return;
    const auto*hit=a.hit_at(px,py);if(hit&&hit->kind==HitKind::Hand)return;
    const char*key=star?"STAR_COUNT":"ENERGY_COUNT";
    char full[64]{},body[1024]{};TextBuffer name{full,sizeof full,0};name.text(key);name.text(".description");
    a.localized_text("static_hover_tips",full,body,sizeof body);
    if(!*body)return;
    // NEnergyCounter (-70,-200) / NStarCounter (-34,-300), projected
    // from the source 1680px viewport, relative to the actual counter.
    source_hover_box(c,a.content.world_title("static_hover_tips",key),body,star?12-6:left-13,star?star_top-57:top-38);
}
#include "combat_sparks_ui7.inc"
#include "combat_vfx_ui6.inc"
static void combat_scene_shake(Canvas&c,int first_hit){
 int dx=0,dy=0;c.a.combat_scene_offset(dx,dy);if(!dx&&!dy)return;
 // NCombatRoom shakes CombatSceneContainer (background and creatures),
 // while CombatUi and the foreground VFX container are siblings. Move only
 // the already-painted scene, before cards/topbar/popups are composed.
 const int first_y=dy>0?239:22,last_y=dy>0?21:240,step_y=dy>0?-1:1;
 const int first_x=dx>0?319:0,last_x=dx>0?-1:320,step_x=dx>0?-1:1;
 for(int y=first_y;y!=last_y;y+=step_y){const int sy=health_bar::bounded(y-dy,22,239);
  for(int x=first_x;x!=last_x;x+=step_x){const int sx=health_bar::bounded(x-dx,0,319);c.pixels[y*c.pitch+x]=c.pixels[sy*c.pitch+sx];}}
 for(int i=first_hit;i<c.a.hit_count;++i){c.a.hits[i].x+=dx;c.a.hits[i].y+=dy;}
}
#include "combat_orb_ui.inc"
static void combat_reward_room(Canvas&c){
    // NCombatUi.AnimOut withdraws the hand, piles, energy and end-turn
    // controls. The room's creatures and player health remain underneath
    // the reward stack; NCreature.OnCombatEnded clears intents and orbs.
    auto&a=c.a;const int hits=a.hit_count;c.picture(a.combat_background(),0,22,320,218);combat_actors(c);
    const auto player=a.combat_player_pose();
    c.hp(a.combat_health(0),a.health_bars[0],creature_health_left(player),player.hp,player.width);
    if(a.combat_powers_frozen())status_strip(c,0,player.body_left,player.status,mx(player.body_width+5,48));
    if(a.game.character==Character::Necrobinder||a.game.expansion.osty_max_hp>0){
        const auto pet=a.combat_player_pose(true);c.hp(a.combat_health(MaxEnemies+1),a.health_bars[MaxEnemies+1],creature_health_left(pet),pet.hp,pet.width);
    }
    a.hit_count=hits; // The shared reward backstop owns pointer input.
}
static void combat_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(a.combat_background(),0,22,320,218);
    c.hit(0,44,320,140,HitKind::Battlefield);
    const int scene_hits=a.hit_count;
    const auto player=a.combat_player_pose();const int player_x=player.x;combat_actors(c);c.hp(a.combat_health(0),a.health_bars[0],creature_health_left(player),player.hp,player.width);
    combat_orbs(c);
    const bool pet=s.character==Character::Necrobinder||s.expansion.osty_max_hp>0;
    if(pet){
        const auto p=a.combat_player_pose(true);c.hp(a.combat_health(MaxEnemies+1),a.health_bars[MaxEnemies+1],creature_health_left(p),p.hp,p.width);
        c.action(p.body_left,p.body_top,p.body_width,p.hp+health_bar::Height-p.body_top,UiAction::ShowOsty);
    }
    const bool stars=combat_stars_visible(a);
    status_strip(c,0,player.body_left,player.status,mx(player.body_width+5,48));
    for(int i=0;i<s.enemy_count;i++){
        const auto&e=s.enemies[i];const auto pos=a.combat_enemy_pose(i);int x=pos.x;
        if(a.combat_health(i+1).hp<=0){health_block_break(c,a.health_bars[i+1],creature_health_left(pos),pos.hp);if(!s.enemy_world.active)c.centered("已击败",x-32,105,64,10,Muted);continue;}
        c.hit(pos.body_left-2,mn(pos.body_top,pos.intent),pos.body_width+4,pos.status-mn(pos.body_top,pos.intent)+15,HitKind::Enemy,i);
        if(s.enemy_world.active){native_enemy_intent(c,i,pos);if(a.target==i)c.text("›",pos.body_left-7,pos.intent+3,10,Gold);}
        else{
            const bool attacking=e.move!=2;c.picture(attacking?A_AttackIntent:A_BuffIntent,x-pos.width/2,pos.intent,13,13);
            char label[32]{};TextBuffer tb{label,sizeof label,0};if(attacking)tb.number(enemy_damage(s,i));else tb.text("强化");
            c.text(label,x-pos.width/2+14,pos.intent,10,attacking?Gold:Green,pos.width-11);
            if(a.target==i)c.text("▼",x-5,pos.intent+14,8,Gold);
        }
        c.hp(a.combat_health(i+1),a.health_bars[i+1],creature_health_left(pos),pos.hp,pos.width);status_strip(c,i+1,pos.body_left,pos.status,mx(pos.body_width+5,22));

    }
    combat_scene_shake(c,scene_hits);
    combat_hit_sparks(c);
    if(s.enemy_world.active)combat_damage_popups(c);
    else for(int i=0;i<=s.enemy_count;i++){
        const int age=int(a.animation)-a.hit_start[i];
        if(a.last_hit[i]&&age>=0&&age<24){const auto p=a.combat_enemy_pose(i?i-1:0);c.num(a.last_hit[i],i?p.x-12:48,(i?p.feet-30:91)-age/2,16,Red);}
    }
    combat_power_flashes(c);combat_power_popups(c);
    // combat_ui.tscn draws counters, end-turn and pile controls before
    // Preview/Hand/PlayContainer. A raised outer card owns its overlapping
    // pixels and mouse area; the old reversed order cut holes in that face.
    combat_energy(c,stars);
    native_end_turn(c,s.phase==Phase::Combat&&!a.combat_actions_locked());
    const auto piles=a.combat_pile_counts();
    combat_pile(c,A_DrawPile,piles.draw,hand_project(15),hand_project(1165),1);
    combat_pile(c,A_DiscardPile,piles.discard,hand_project(1586),hand_project(1165),2);
    if(piles.exhaust)combat_pile(c,A_ExhaustPile,piles.exhaust,hand_project(1590),hand_project(970),3);
    if(ui_cards_bound(a))for(const auto&v:a.combat_cards.sprites)
        if(v.mode==CombatCardMotion::Preview&&v.id<s.card_count)
            draw_hand_tile(c,hand_tile(c,v.id,&v),CombatCardMotion::pose(v),false,a.combat_cards.brightness(v));
    const auto hand=a.combat_hand();
    int selected=mn(mx(a.cursor,0),mx(0,hand.count-1));
    const bool focused=a.hand_focus&&a.hand_can_focus();
    const int held_card=a.selected_hand_card();
    if(ui_cards_bound(a))a.combat_cards.set_focus(focused&&hand.count?hand.ids[selected]:-1);
    // Hitboxes use the same live transformed frame as painting, including
    // hover-neighbour motion and interpolation after a card leaves the hand.
    // Original holders ignore the mouse while a dragged card owns input.
    const bool dragging=a.pointer.held&&a.pointer.dragging&&held_card>=0;
    const auto hand_hit=[&](HandPose pose,int id,bool enabled){if(dragging)return;
        const auto r=hand_bounds(pose);c.hit(r.x,r.y,r.w,r.h,HitKind::Hand,id,enabled);};
    for(int i=0;i<hand.count;i++){
        if((focused&&i==selected)||hand.ids[i]==held_card)continue;
        const auto pose=a.combat_hand_pose(unsigned(i),false);
        draw_hand_tile(c,hand_tile(c,hand.ids[i]),pose,false);
        combat_hand_index(c,unsigned(i),pose);
        hand_hit(pose,hand.ids[i],!a.combat_actions_locked()&&can_play_card(s,hand.ids[i]));
    }
    if(hand.count){
        if(focused&&hand.ids[selected]!=held_card){
            const auto pose=a.combat_hand_pose(unsigned(selected),true);
            draw_hand_tile(c,hand_tile(c,hand.ids[selected]),pose,true);
            combat_hand_index(c,unsigned(selected),pose);
            hand_hit(pose,hand.ids[selected],!a.combat_actions_locked()&&can_play_card(s,hand.ids[selected]));
        }
    }else c.centered("没有手牌",80,197,160,12,Muted);
    if(held_card>=0)for(unsigned i=0;i<hand.count;++i)if(hand.ids[i]==held_card){
        const auto pose=a.combat_hand_pose(i,false);draw_hand_tile(c,hand_tile(c,held_card),pose,false);
        hand_hit(pose,held_card,!a.combat_actions_locked());break;
    }
    if(ui_cards_bound(a))for(const auto&v:a.combat_cards.sprites){
        if(v.mode==CombatCardMotion::Empty||v.mode==CombatCardMotion::Hand||v.mode==CombatCardMotion::Preview||v.id>=s.card_count)continue;
        auto pose=CombatCardMotion::pose(v);
        if(v.mode==CombatCardMotion::Play){const unsigned peek=a.combat_peek_amount();
            pose.cx=(pose.cx*int(1024-peek)+31*int(peek))/1024;pose.cy=(pose.cy*int(1024-peek)+88*int(peek))/1024;
            pose.scale=pose.scale*int(2048-peek)/2048;}
        draw_hand_tile(c,hand_tile(c,v.id,&v),pose,false,a.combat_cards.brightness(v));
    }
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),150+age*4,163-age*5,26,19,(unsigned)(a.card_fly*18));}
    combat_resource_tooltip(c,stars);
}
