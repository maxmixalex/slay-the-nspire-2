// Anchoring follows the owner's pinned combat_ui/player_hand/pile scenes.
// The integer fan, tile dimensions and preview are small-screen adaptations.
namespace {
struct HandTile {
    bool valid=false,playable=false,resource_cross=false,description_truncated=false,sampled_locked=false;
    uint8_t resource_styles=0;
    uint8_t glow=0,overlay=0;
    const Content* owner=nullptr;
    Card card{};
    uint32_t used=0,enchantment=0;
    uint32_t built=0;
    uint32_t sampled_frame=0,sampled_revision=0,sampled_generation=0;
    int sampled_instance=-1,sampled_deck=-1,sampled_target=-1;
    const CombatCardMotion::Sprite*sampled_visual=nullptr;
    int energy=0,stars=0,portrait=-1,frame=-1;
    char description[512]{};
    uint16_t pixels[HandTileW*HandTileH]{};
    uint8_t alpha[HandTileW*HandTileH]{};
};
static HandTile hand_tiles[MaxHand];
struct SelectedHandReader {
    HandTile tile{};
    const HandTile*source=nullptr;
    uint32_t built=0,started=0;
    int instance=-1,skip=-1,maximum=0;
    uint16_t background[72*48]{};
    uint8_t coverage[72*48]{};
};
static SelectedHandReader selected_hand_reader;
static_assert(sizeof(SelectedHandReader)<56u*1024u,"Only one selected description may animate");
struct HandGlowSlot {HandCardHighlight motion{};const Content*owner=nullptr;int instance=-1;uint32_t used=0;};
static HandGlowSlot hand_glows[MaxHand];
static uint32_t hand_tile_clock=0,hand_build_clock=0;
static bool same_text(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
}
static void invalidate_hand_tiles(){for(auto&t:hand_tiles)t.valid=false;for(auto&g:hand_glows)g={};selected_hand_reader.instance=-1;invalidate_card_face_cache();}
static void card_title(Canvas&c,const char*name,int x,int y,int width,uint16_t ink){
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(name,x+dx,y+dy,width,10,Ink);
    c.centered(name,x,y,width,10,ink);
}
static const HandTile& hand_tile(Canvas&c,int instance,const CombatCardMotion::Sprite* visual=nullptr,int deck=-1,const EventCardFacts*frozen_facts=nullptr,bool upgrade_preview=false,const char*description_override=nullptr){
    auto&a=c.a;Card card=visual?visual->card:a.game.cards[instance];
    ++hand_tile_clock;const int preview_target=visual?-1:a.card_preview_target(instance);
    // A selected face is requested again by its automatic full-description
    // tip in the same frame. Reuse the already resolved instance sample;
    // target/revision/card identity guard input arriving between frames.
    const bool locked=a.combat_actions_locked();
    for(auto&t:hand_tiles)if(!frozen_facts&&!upgrade_preview&&!description_override&&t.valid&&t.owner==&a.content&&t.sampled_generation==a.content.generation()&&t.sampled_locked==locked&&
        t.sampled_revision==a.game.revision&&t.sampled_instance==instance&&t.sampled_deck==deck&&
        t.sampled_target==preview_target&&t.sampled_visual==visual&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded&&
        (!visual||(t.energy==visual->energy&&t.stars==visual->stars))){t.used=hand_tile_clock;return t;}
    auto stamp=[&](HandTile&t){t.sampled_frame=small_card_frame;t.sampled_revision=a.game.revision;t.sampled_generation=a.content.generation();t.sampled_locked=locked;t.sampled_instance=frozen_facts||description_override||upgrade_preview?-2:instance;t.sampled_deck=deck;t.sampled_target=preview_target;t.sampled_visual=visual;};
    int energy=visual?visual->energy:cost(a.game,instance),stars=visual?visual->stars:card_star_cost(a.game,card,instance);
    const auto*facts=frozen_facts?frozen_facts:event_card_facts(a.game,card,instance,deck);
    const uint32_t enchantment=card_face_enchantment_key(a,facts,instance);
    const unsigned overlay=card_face_overlay(a,card,instance);
    const unsigned resource_styles=card_face_resource_styles(a,card,energy,stars,instance,facts,upgrade_preview);
    const bool resource_cross=card_face_resource_cross(a,card,energy,stars,instance);
    bool playable=visual||can_play_card(a.game,instance);
    const unsigned glow=!visual&&!a.combat_actions_locked()&&a.game.phase==Phase::Combat?hand_card_glow(a.game,card,instance,playable):0;
    char description[512]{};
    if(description_override){TextBuffer text{description,sizeof description,0};text.text(description_override);}
    else a.card_description(card,preview_target,description,sizeof description,instance,deck,frozen_facts);
    int portrait=a.card_face_image(card,instance,deck,facts),frame=a.card_face_frame(card,instance,deck,facts);
    HandTile*slot=nullptr;
    // A holder index changes when a neighbour leaves the hand. Reuse the
    // visual key across hand, play and flight instead of repainting every
    // shifted slot, while keeping ten bounded104x136 cache allocations.
    for(auto&t:hand_tiles){
        if(t.valid&&t.owner==&a.content&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded&&t.energy==energy&&t.stars==stars&&
           t.playable==playable&&t.glow==glow&&t.overlay==overlay&&t.resource_cross==resource_cross&&t.resource_styles==resource_styles&&t.enchantment==enchantment&&t.portrait==portrait&&t.frame==frame&&same_text(t.description,description)){t.used=hand_tile_clock;stamp(t);return t;}
        if(!slot||(!t.valid&&slot->valid)||(t.valid==slot->valid&&int32_t(t.used-slot->used)<0))slot=&t;
    }
    auto&t=*slot;t.used=hand_tile_clock;stamp(t);
    t.owner=&a.content;t.card=card;t.energy=energy;t.stars=stars;t.playable=playable;t.portrait=portrait;t.frame=frame;
    t.glow=uint8_t(glow);t.overlay=uint8_t(overlay);t.enchantment=enchantment;
    t.resource_styles=uint8_t(resource_styles);
    t.resource_cross=resource_cross;
    for(unsigned i=0;i<sizeof description;i++)t.description[i]=description[i];
    for(int i=0;i<HandTileW*HandTileH;i++){t.pixels[i]=0;t.alpha[i]=0;}
    Canvas tile{a,t.pixels,HandTileW,HandTileW,HandTileH,t.alpha};
    // Left inset contains the original star cost below the energy icon;
    // neither resource adds a second title row or moves the card portrait.
    draw_card_face(tile,card,HandFaceX,HandFaceY,HandFaceW,HandFaceH,energy,stars,instance,deck,facts,description,&t.description_truncated,true,upgrade_preview,false);
    // NHandCardHolder hides its playable highlight; it does not gray the
    // entire portrait, description and resource label when a card cannot play.
    t.built=++hand_build_clock;t.valid=true;return t;
}
static const HandTile& selected_hand_tile(Canvas&c,int instance){
    const auto&source=hand_tile(c,instance);auto&r=selected_hand_reader;
    if(!source.description_truncated){r.instance=-1;return source;}
    const auto layout=card_face::layout(HandFaceX,HandFaceY,HandFaceW,HandFaceH);auto rect=layout.description;
    const auto fonts=card_face_fonts(c,HandFaceW);const int size=c.a.content.font_size(fonts.body);
    if(!fonts.compact){const int bottom=rect.y+rect.h;rect.y=mx(rect.y,layout.type.y+mx(11,layout.type.h));rect.h=mx(0,bottom-rect.y);}
    if(r.instance!=instance||r.source!=&source||r.built!=source.built){
        r.instance=instance;r.source=&source;r.built=source.built;r.started=c.a.map_clock_ms;r.skip=-1;
        const auto metric=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
        r.maximum=mx(0,fitted_text::line_count(source.description,rect.w,size,metric)*(size+1)-rect.h);
        if(rect.w>72||rect.h>48){r.maximum=0;return source;}
        r.tile=source;auto&t=r.tile;
        for(int i=0;i<HandTileW*HandTileH;++i){t.pixels[i]=0;t.alpha[i]=0;}
        Canvas tile{c.a,t.pixels,HandTileW,HandTileW,HandTileH,t.alpha};
        const auto*facts=event_card_facts(c.a.game,source.card,instance,-1);
        render_card_face(tile,source.card,HandFaceX,HandFaceY,HandFaceW,HandFaceH,source.energy,source.stars,instance,-1,facts,"");
        for(int y=0;y<rect.h;++y)for(int x=0;x<rect.w;++x){const int at=(rect.y+y)*HandTileW+rect.x+x;
            r.background[y*rect.w+x]=t.pixels[at];r.coverage[y*rect.w+x]=t.alpha[at];}
    }
    if(!r.maximum)return source;
    // Eight pixels per second with a pause at each end. Restore only the
    // description background; repainting portrait/frame/cost every pixel used
    // to turn readable overflow into another combat rendering stall.
    const unsigned travel=unsigned(r.maximum)*125u,cycle=2800u+travel*2u,age=(c.a.map_clock_ms-r.started)%cycle;
    const int skip=age<1400?0:age<1400+travel?int((age-1400)/125u):age<2800+travel?r.maximum:mx(0,r.maximum-int((age-2800-travel)/125u));
    if(r.skip==skip)return r.tile;
    r.skip=skip;auto&t=r.tile;
    for(int y=0;y<rect.h;++y)for(int x=0;x<rect.w;++x){const int at=(rect.y+y)*HandTileW+rect.x+x;
        t.pixels[at]=r.background[y*rect.w+x];t.alpha[at]=r.coverage[y*rect.w+x];}
    Canvas tile{c.a,t.pixels,HandTileW,HandTileW,HandTileH,t.alpha};
    card_description_window(tile,source.description,rect,size,skip);
    return t;
}
static void hand_card_animated_layers(Canvas&c,const HandTile&t,HandPose p,unsigned brightness,unsigned erosion){
    Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(layer.brightness)*brightness));
    if(erosion)layer.opacity=uint8_t(unsigned(layer.opacity)*(1024-mn(1024u,erosion))/1024);
    if(card_rarity(t.card.kind)==5){
        const int image=c.a.content.ui_image(I14_AncientFlame+(c.a.map_clock_ms/100)%I14_AncientFlameCount);
        const auto art=c.a.content.image(image);if(!art.pixels)return;
        const int dx=HandFaceW*1024/300,dy=-217*HandFaceH*1024/422;
        const int cx=p.cx+int((int64_t(dx)*p.cosine-int64_t(dy)*p.sine)*p.scale/(1024ll*1024*1024));
        const int cy=p.cy+int((int64_t(dx)*p.sine+int64_t(dy)*p.cosine)*p.scale/(1024ll*1024*1024));
        ui_picture_transform(layer,art,cx,cy,int(art.w)*HandFaceW*p.scale/(96*1024),int(art.h)*HandFaceW*p.scale/(96*1024),p.sine,p.cosine);
    }else if(const int image=card_overlay_image(c.a,t.overlay);image>=0)
        ui_picture_transform(layer,image,p.cx,p.cy,324*HandFaceW*p.scale/(300*1024),454*HandFaceH*p.scale/(422*1024),p.sine,p.cosine);
}
static void draw_hand_tile(Canvas&c,const HandTile&t,HandPose p,bool focused,unsigned brightness=255,unsigned erosion=0,bool highlight=false){
    if(p.scale<=0)return;
    int sn=p.sine<0?-p.sine:p.sine,cs=p.cosine<0?-p.cosine:p.cosine;
    constexpr int radius_x=(HandPivotX+1023)/1024,radius_y=(HandTileH*1024-HandPivotY+1023)/1024;
    int half_w=((cs*radius_x+sn*radius_y)*p.scale)/(1024*1024)+2;
    int half_h=((sn*radius_x+cs*radius_y)*p.scale)/(1024*1024)+2;
    int left=mx(0,p.cx-half_w),right=mn(320,p.cx+half_w);
    int top=mx(0,p.cy-half_h),bottom=mn(240,p.cy+half_h);
    // Inverse affine sampling: only integer additions in the inner loop.
    int du=p.cosine*1024/p.scale,dv=-p.sine*1024/p.scale;
    unsigned glow_width=0,glow_color=1;Picture sdf{};
    if(highlight&&c.a.content.has_selection_assets()){
        HandGlowSlot*slot=nullptr;for(auto&g:hand_glows){if(g.owner==&c.a.content&&g.instance==t.sampled_instance){slot=&g;break;}if(!slot||int32_t(g.used-slot->used)<0)slot=&g;}
        if(slot->owner!=&c.a.content||slot->instance!=t.sampled_instance){*slot={};slot->owner=&c.a.content;slot->instance=t.sampled_instance;}
        slot->used=hand_tile_clock;glow_width=slot->motion.observe(t.glow,c.a.selected_hand_card()==t.sampled_instance,c.a.map_clock_ms);glow_color=slot->motion.color;
        if(glow_width)sdf=c.a.content.image(c.a.content.ui_image(I9_CardHighlightSdf));
    }
    // Original .075/.15 distance bands become disproportionately broad
    // after the LCD hand enlargement. Preserve the ripple at half width.
    glow_width=(glow_width+1u)/2u;
    const bool glow=glow_width&&sdf.pixels&&sdf.format==0&&sdf.w==290&&sdf.h==181;
    const card_highlight::Context shader(glow_width,c.a.map_clock_ms,true);const unsigned transparent=shader.transparent_through();
    // Highlight's 759x951 TextureRect uses KEEP_ASPECT_CENTERED on a square
    // source texture: the actual drawing is 759x759, centered 96px lower.
    // Stretching it to the container height makes the top protrude by 27px.
    constexpr int glow_x=HandPivotX-381*HandFaceW*1024/300,glow_y=HandPivotY-379*HandFaceH*1024/422;
    constexpr int glow_w=759*HandFaceW*1024/300,glow_h=759*HandFaceH*1024/422;
    if(focused){
        // The outline belongs to the card face, not to its transparent cache
        // allocation. Transform the same86x121 face and its2px inset with
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
    // Generated-card previews and settled unhighlighted holders use the
    // dense picture path. It skips transparent spans and avoids an affine
    // division/sampler for an exactly axis-aligned, native-size face.
    if(!glow&&!erosion&&p.scale==1024&&p.sine==0&&p.cosine==1024){
        Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(layer.brightness)*brightness));
        layer.picture(Picture{reinterpret_cast<const uint8_t*>(t.pixels),t.alpha,nullptr,HandTileW,HandTileH,1},
            p.cx-HandPivotX/1024,p.cy-HandPivotY/1024,HandTileW,HandTileH);
        hand_card_animated_layers(c,t,p,brightness,erosion);return;
    }
    for(int y=top;y<bottom;y++){
        int u=HandPivotX+((left-p.cx)*p.cosine+(y-p.cy)*p.sine)*1024/p.scale;
        int v=HandPivotY+(-(left-p.cx)*p.sine+(y-p.cy)*p.cosine)*1024/p.scale;
        for(int x=left;x<right;x++,u+=du,v+=dv){
            if(u<0||v<0||u>=HandTileW*1024||v>=HandTileH*1024)continue;
            // The original distance field/ripple follows the same affine
            // holder transform. Opaque interiors need no shader evaluation.
            if(glow&&!(u>(HandFaceX+5)*1024&&u<(HandFaceX+HandFaceW-5)*1024&&v>(HandFaceY+5)*1024&&v<(HandFaceY+HandFaceH-5)*1024)){
                const int gx=(u-glow_x)*145/glow_w,gy=(v-glow_y)*181/glow_h;
                if(gx>=0&&gx<145&&gy>=0&&gy<181){const uint8_t*source=sdf.pixels+gy*580+gx*4;const unsigned scalar=source[0]|unsigned(source[1])<<8;
                    if(scalar>transparent){const unsigned alpha=shader.alpha(uint16_t(scalar));if(alpha){const uint16_t original=uint16_t(source[2]|unsigned(source[3])<<8);
                        const uint16_t color=glow_color==3?presentation::tint_rgb(original,212,0,84):glow_color==2?presentation::tint_rgb(original,255,200,0):presentation::tint_rgb(original,0,244,252);
                        c.paint(y*c.pitch+x,color,alpha,presentation::Add);
                    }}
                }
            }
            int at=(v/1024)*HandTileW+u/1024;
            if(!t.alpha[at])continue;
            uint16_t color=t.pixels[at];
            if(erosion){const int sx=u/1024,sy=v/1024,noise=int((unsigned(sx*73+sy*151)^unsigned(sx*sy*19))&63u)-32;
                const int edge=(sy-HandFaceY)*1024/HandFaceH+noise-int(erosion);if(edge<0)continue;if(edge<52)color=rgb(255,115+edge*2,45+edge);}
            if(brightness<255)color=CombatCardMotion::modulate(color,brightness);
            c.paint(y*c.pitch+x,color,t.alpha[at]);
        }
    }
    hand_card_animated_layers(c,t,p,brightness,erosion);
}
#include "combat_sovereign_ui.inc"
static void combat_pile(Canvas&c,int image,int count,int x,int y,int tab){
    // NCombatCardPile scales Icon, not its sibling CountContainer. Focus is
    // a 50ms linear tween to1.25; unfocus returns over500ms with QuintOut.
    struct Motion {const Content*owner=nullptr;uint32_t generation=0,start=0,last=0;int from=4096,value=4096;unsigned state=0;};
    static Motion motions[3];auto&m=motions[tab-1];auto&a=c.a;const auto now=a.map_clock_ms;
    if(m.owner!=&a.content||m.generation!=a.content.generation()||int32_t(now-m.last)<0){m={};m.owner=&a.content;m.generation=a.content.generation();m.start=now;}
    m.last=now;constexpr int size=combat_controls::PileSize;
    const int hit_left=x-(tab==2?5:0),hit_right=x+size+(tab==1?5:0);
    const bool hover=a.pointer.visible&&!a.panel&&!a.paused&&!a.help&&!a.settings_open&&!a.frontend_confirm&&a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&
        a.pointer.x>=hit_left&&a.pointer.x<hit_right&&a.pointer.y>=y&&a.pointer.y<y+26;
    const unsigned state=hover?(a.pointer.held?2u:1u):0u;
    if(state!=m.state){m.from=m.value;m.state=state;m.start=now;}
    const unsigned duration=state==1?50u:state==2?250u:500u,age=now-m.start;
    int q=age>=duration?4096:int(age*4096u/duration);
    if(state!=1){const int inv=4096-q,q2=inv*inv/4096;q=4096-(state==2?q2*inv/4096:q2*q2/4096*inv/4096);}
    m.value=m.from+((state==1?5120:4096)-m.from)*q/4096;
    const int draw_size=(size*m.value+2048)/4096;Canvas icon=c;if(state==2)icon.brightness=169;
    icon.picture(image,x+(size-draw_size)/2,y+(size-draw_size)/2,draw_size,draw_size);
    char label[16]{};TextBuffer b{label,sizeof label,0};b.number(count);
    Canvas text=c;text.font_face=uint8_t(source_label_face(c.a.content.original_label_fonts(),5));
    if(tab==3){
        // Exhaust's pink count belongs in the center of its own embers.
        // Only Draw/Discard have the separate red count-badge texture.
        health_label(text,label,x,y+7,size,10,rgb(255,181,226),rgb(52,31,47),true);
        c.hit(x,y,size,size,HitKind::OpenPile,tab);return;
    }
    // The authored badge is 48px on an 80px pile, not nearly as large as
    // the pile itself. Its 15px LCD size leaves the complete lower edge.
    constexpr int badge_size=15;const int badge_x=x+(tab==1?14:-5),badge_y=y+11;
    if(c.a.content.version()>=Ui12ContentVersion)c.ui(I12_PileCount,badge_x,badge_y,badge_size,badge_size);
    health_label(text,label,badge_x,badge_y+3,badge_size,8,rgb(255,246,226),rgb(136,31,32),true);
    c.hit(mn(x,badge_x),y,mx(x+size,badge_x+badge_size)-mn(x,badge_x),26,HitKind::OpenPile,tab);
}
static void combat_hand_index(Canvas&c,unsigned index,HandPose p){
 if(!c.a.show_card_indices||p.scale<=0)return;
 // Keep the label above the enlarged face, rather than at the old69px
 // holder's baseline where it landed inside the card title banner.
 uint16_t pixels[20*14]{};uint8_t alpha[20*14]{};
 Canvas tile{c.a,pixels,20,20,14,alpha};char label[8]{};TextBuffer b{label,sizeof label,0};b.number(int(index+1));
 health_label(tile,label,0,1,20,10,rgb(255,246,226),Ink,true);
 constexpr int rise=HandFaceH/2+12;
 const int cx=p.cx+rise*p.sine*p.scale/(1024*1024),cy=p.cy-rise*p.cosine*p.scale/(1024*1024);
 const Picture image{reinterpret_cast<const uint8_t*>(pixels),alpha,nullptr,20,14,1};
 ui_picture_transform(c,image,cx,cy,mx(1,20*p.scale/1024),mx(1,14*p.scale/1024),p.sine,p.cosine);
}
static void combat_player_speech(Canvas&c){
    auto&a=c.a;const unsigned age=a.map_clock_ms-a.combat_speech_at;
    if(!a.combat_speech_key||age>=1800u)return;
    const auto p=a.combat_player_pose();
    // The source speaker position is on the player's right. Anchor the last
    // smoke puff to that side instead of leaving the tail left of the actor.
    const int x=mx(5,mn(199,p.center_x+p.body_width/4+3)),y=mx(33,mn(102,p.body_top-43));
    const unsigned alpha=age<140?age*255/140:age<1450?255:(1800-age)*255/350;
    Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*alpha));
    if(a.content.version()>=Ui12ContentVersion){
        Canvas outside=layer;outside.tint_red=42;outside.tint_green=63;outside.tint_blue=64;
        outside.picture(a.content.ui_image(I12_PlayerSpeech),x-5,y-6,123,65,192);
        Canvas smoke=layer;smoke.tint_red=82;smoke.tint_green=smoke.tint_blue=102;
        for(int i=0;i<3;++i){const int drift=int((age/90+unsigned(i)*2)%5)-2;
            smoke.picture(a.content.ui_image(I12_PlayerSpeech),x+i*29-8,y-3+drift,62,54,224);}
        smoke.picture(a.content.ui_image(I12_PlayerSpeech),x-3,y+39,19,17,180);
        smoke.picture(a.content.ui_image(I12_PlayerSpeech),x-8,y+51,11,10,140);
    }else layer.nine_ui(U_HoverTip,x,y,112,51,5,5,5,5);
    const char*label=a.content.world_string("combat_messages",a.combat_speech_key);
    layer.text_visible_characters=age<400?unsigned(age*80/400):UINT32_MAX;
    layer.fitted(label,x+7,y+8,98,37,10,10,rgb(255,246,226),true,false,false,true);
}
#include "phobia_swarm_ui.inc"
static void combat_actors(Canvas&c){
    auto&a=c.a;const auto&s=a.game;const auto player=a.combat_player_pose();combat_sovereign_blades(c,true);c.actor(0,player.sprite_x,player.sprite_y,player.scale);combat_sovereign_blades(c,false);
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
        if(combat_doom_hides(a,unsigned(i+1)))continue;
        if(s.enemies[i].hp>0||s.enemy_world.active){auto p=a.combat_enemy_pose(i);Canvas actor=c;
            if(s.enemy_world.active&&a.enemy_presentation[i].instance_hue)actor.color_matrix=presentation::CreatureHue[a.enemy_presentation[i].instance_hue];
            if(s.enemy_world.active)actor.opacity=uint8_t(a.enemy_opacity(i));actor.actor(i+1,p.sprite_x,p.sprite_y,p.scale);
            creature_sleep_particles(actor,i);
            if(s.enemy_world.active)phobia_swarm_body(actor,a.enemy_presentation[i].model,p.sprite_x,p.sprite_y,p.scale,a.animation*33,s.seed^a.enemy_presentation[i].identity);}
    }
    if(shared>=0){int clip;uint32_t elapsed;bool loop;
        // The original Kaiser actor shares one background skeleton. Its baked
        // 320 x 218 frame is already in scene coordinates; draw it only once.
        if(a.enemy_frame(shared,clip,elapsed,loop)){Canvas actor=c;actor.opacity=uint8_t(a.enemy_opacity(shared));if(!combat_doom_shared_frame(actor,clip,elapsed,loop))actor.clip(clip,0,22-combat_controls::SceneLift,elapsed,false,loop);}
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
static void combat_energy(Canvas&c,bool stars,bool include_stars=true){
    auto&a=c.a;const auto&s=a.game;const int energy=a.combat_energy();const bool empty=energy==0;
    // Readable LCD resource rail stays below the raised creature bars.
    // Regent's smaller Stars counter sits tightly below-left of the energy.
    const int left=combat_controls::EnergyX,top=combat_controls::EnergyY;
    const uint32_t delta=a.map_clock_ms-a.energy_rotation_tick;
    a.energy_rotation_mdeg=(a.energy_rotation_mdeg+(delta%72000u)*(a.energy_rotation_zero?5u:30u))%360000u;
    a.energy_rotation_tick=a.map_clock_ms;a.energy_rotation_zero=empty;
    const int first=U_EnergyLayers+int(s.character)*5;
    if(a.content.ui_image(first)>=0){
        const int layers=s.character==Character::Necrobinder?3:5;
        for(int i=0;i<layers;i++){const bool rotates=i==1||(i==2&&s.character!=Character::Necrobinder);
            combat_energy_layer(c,a.content.ui_image(first+i),rotates?int((a.energy_rotation_mdeg*unsigned(i)/1000)%360):0,empty,left,top,34);}
    }else c.picture(character_energy_image(a.content,s.character),left,top,34,34);
    const uint16_t outline[]={rgb(77,19,13),rgb(0,79,4),rgb(22,62,100),rgb(120,64,0),rgb(112,45,111)};
    char label[24]{};TextBuffer b{label,sizeof label,0};b.number(energy);b.ch('/');b.number(a.combat_energy(true));
    const int size=12;const uint16_t edge=empty?rgb(55,24,26):outline[unsigned(s.character)];
    Canvas number_label=c;number_label.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),5));
    health_label(number_label,label,left-2,top+10,38,size,empty?rgb(255,85,85):rgb(255,246,226),edge,true);
    c.hit(left,top,34,34,HitKind::Battlefield);
    if(stars&&include_stars){
        const int count=a.combat_status.star_count.displayed(),age=int32_t(a.animation-a.combat_status.star_gain_at)*33,star_top=combat_controls::StarY;
        const unsigned light=count&&age>=0&&age<200?255+unsigned(200-age)*255/200:255;
        if(a.content.version()>=FrontendContentVersion){
            for(int layer=0;layer<3;++layer)combat_energy_layer(c,a.content.ui_image(F_StarCounterBase+layer),
                layer?int(a.combat_status.star_rotation_mdeg*unsigned(layer)/1000u%360u):0,count==0,combat_controls::StarX,star_top,combat_controls::StarSize,true,light);
        }else combat_energy_layer(c,a.content.ui_image(U_StarCounter),0,count==0,combat_controls::StarX,star_top,combat_controls::StarSize,true,light);
        char number[16]{};TextBuffer b{number,sizeof number,0};b.number(count);
        health_label(number_label,number,combat_controls::StarX,star_top+7,combat_controls::StarSize,12,count?rgb(255,246,226):rgb(255,85,85),rgb(0,62,117),true);
        c.action(combat_controls::StarX,star_top,combat_controls::StarSize,combat_controls::StarSize,UiAction::ShowStars);
    }
}
static void combat_resource_tooltip(Canvas&c,bool stars){
    auto&a=c.a;if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.panel||a.paused||a.help||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0)return;
    const int px=a.pointer.x,py=a.pointer.y,left=combat_controls::EnergyX,top=combat_controls::EnergyY,star_top=combat_controls::StarY;
    const bool star=stars&&px>=combat_controls::StarX&&px<combat_controls::StarX+combat_controls::StarSize&&py>=star_top&&py<star_top+combat_controls::StarSize;
    if(!star&&!(px>=left&&px<left+34&&py>=top&&py<top+34))return;
    const auto*hit=a.hit_at(px,py);if(hit&&hit->kind==HitKind::Hand)return;
    const char*key=star?"STAR_COUNT":"ENERGY_COUNT";
    char full[64]{},body[1024]{};TextBuffer name{full,sizeof full,0};name.text(key);name.text(".description");
    a.localized_text("static_hover_tips",full,body,sizeof body);
    if(!*body)return;
    // NEnergyCounter (-70,-200) / NStarCounter (-34,-300), projected
    // from the source 1680px viewport, relative to the actual counter.
    source_hover_box(c,a.content.world_title("static_hover_tips",key),body,star?combat_controls::StarX-6:left-13,star?star_top-57:top-38);
}
#include "combat_sparks_ui7.inc"
#include "combat_secondary_ui11.inc"
#include "combat_vfx_ui6.inc"
static void combat_scene_shake(Canvas&c,int first_hit){
 int dx=0,dy=0;c.a.combat_scene_offset(dx,dy);if(!dx&&!dy)return;
 // NCombatRoom shakes CombatSceneContainer (background and creatures),
 // while CombatUi and the foreground VFX container are siblings. Move only
 // the already-painted scene, before cards/topbar/popups are composed.
 screen_translate(c,dx,dy,first_hit,22);
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
    combat_scene_shake(c,hits);
    combat_doom_layers(c,false);combat_doom_bodies(c);combat_doom_layers(c,true);combat_doom_overlay(c);
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
        if(combat_doom_hides(a,unsigned(i+1)))continue;
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
    // NDoomVfx is in the foreground CombatVfxContainer: its detached
    // body and particles retain their registration while the scene shakes.
    combat_doom_layers(c,false);combat_doom_bodies(c);combat_doom_layers(c,true);combat_doom_overlay(c);
    combat_hit_sparks(c);
    combat_secondary_sparks(c);
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
    combat_pile(c,A_DrawPile,piles.draw,combat_controls::DrawX,combat_controls::DrawY,1);
    combat_pile(c,A_DiscardPile,piles.discard,combat_controls::DiscardX,combat_controls::DiscardY,2);
    if(piles.exhaust)combat_pile(c,A_ExhaustPile,piles.exhaust,combat_controls::ExhaustX,combat_controls::ExhaustY,3);
    combat_card_secondary(c);
    if(ui_cards_bound(a))for(const auto&v:a.combat_cards.sprites)
        if(v.mode==CombatCardMotion::Preview&&v.id<s.card_count)
            draw_hand_tile(c,hand_tile(c,v.id,&v),CombatCardMotion::pose(v),false,a.combat_cards.brightness(v));
    const bool choosing_hand=combat_selection_hand(a.combat_selection_kind())&&a.combat_choice_ready();
    if(!choosing_hand||a.choice_peeking||a.panel){
    auto hand=a.combat_hand();
    if(choosing_hand){unsigned remaining=0;
        for(unsigned i=0;i<hand.count;++i){bool selected=false;for(unsigned j=0;j<a.choice_selected_count;++j)if(a.choice_selected[j]==hand.ids[i])selected=true;
            if(!selected)hand.ids[remaining++]=hand.ids[i];}hand.count=uint16_t(remaining);}
    int selected=mn(mx(a.cursor,0),mx(0,hand.count-1));
    const bool focused=a.hand_focus&&a.hand_can_focus();
    const int held_card=a.selected_hand_card();
    if(held_card<0&&(!focused||!hand.count))selected_hand_reader.instance=-1;
    if(ui_cards_bound(a))a.combat_cards.set_focus(focused&&hand.count?hand.ids[selected]:-1);
    // Hitboxes use the same live transformed frame as painting, including
    // hover-neighbour motion and interpolation after a card leaves the hand.
    // Original holders ignore the mouse while a dragged card owns input.
    const bool dragging=a.pointer.held&&a.pointer.dragging&&held_card>=0;
    const auto hand_hit=[&](HandPose pose,int id,bool enabled){if(dragging)return;
        const auto r=hand_bounds(pose);c.hit(r.x,r.y,r.w,r.h,choosing_hand?HitKind::InstanceCard:HitKind::Hand,id,enabled);};
    const auto visible_hand_pose=[&](unsigned i,bool focus){return choosing_hand?hand_pose(int(hand.count),int(i),false):a.combat_hand_pose(i,focus);};
    for(int i=0;i<hand.count;i++){
        if((focused&&i==selected)||hand.ids[i]==held_card)continue;
        const auto pose=visible_hand_pose(unsigned(i),false);
        draw_hand_tile(c,hand_tile(c,hand.ids[i]),pose,false,255,0,true);
        combat_hand_index(c,unsigned(i),pose);
        hand_hit(pose,hand.ids[i],!a.combat_actions_locked());
    }
    if(hand.count){
        if(focused&&hand.ids[selected]!=held_card){
            const auto pose=visible_hand_pose(unsigned(selected),true);
            draw_hand_tile(c,held_card<0?selected_hand_tile(c,hand.ids[selected]):hand_tile(c,hand.ids[selected]),pose,true,255,0,true);
            combat_hand_index(c,unsigned(selected),pose);
            hand_hit(pose,hand.ids[selected],!a.combat_actions_locked());
        }
    }
    if(held_card>=0)for(unsigned i=0;i<hand.count;++i)if(hand.ids[i]==held_card){
        const auto pose=visible_hand_pose(i,false);draw_hand_tile(c,selected_hand_tile(c,held_card),pose,false,255,0,true);
        hand_hit(pose,held_card,!a.combat_actions_locked());break;
    }
    }
    if(ui_cards_bound(a))for(const auto&v:a.combat_cards.sprites){
        if(v.mode==CombatCardMotion::Shuffle){const auto p=CombatCardMotion::pose(v);const int size=mx(7,p.scale*14/HandFocusScale);c.ui(I8_TinyCardBack,p.cx-size/2,p.cy-size/2,size,size);continue;}
        if(v.mode==CombatCardMotion::Empty||v.mode==CombatCardMotion::Hand||v.mode==CombatCardMotion::Preview||v.mode==CombatCardMotion::Tail||v.id>=s.card_count)continue;
        const unsigned exhaust_age=(a.animation-v.at)*33u;if(v.mode==CombatCardMotion::Exhaust&&exhaust_age>=400u)continue;
        auto pose=CombatCardMotion::pose(v);
        if(v.mode==CombatCardMotion::Play){const unsigned peek=a.combat_peek_amount();
            pose.cx=(pose.cx*int(1024-peek)+31*int(peek))/1024;pose.cy=(pose.cy*int(1024-peek)+88*int(peek))/1024;
            pose.scale=pose.scale*int(2048-peek)/2048;}
        draw_hand_tile(c,hand_tile(c,v.id,&v),pose,false,a.combat_cards.brightness(v),v.mode==CombatCardMotion::Exhaust?combat_exhaust_progress(exhaust_age):0u);
    }
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),150+age*4,163-age*5,26,19,(unsigned)(a.card_fly*18));}
    combat_forge_card_previews(c);
    combat_player_speech(c);
    combat_resource_tooltip(c,stars);
}
