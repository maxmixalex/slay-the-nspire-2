namespace {
struct TargetArrowMotion {
    const App*owner=nullptr;uint32_t time=0;int x=0,y=0,sine=0,cosine=1024;bool active=false;
};
static TargetArrowMotion target_arrow_motion;
}
static void targeting_arrow(Canvas&c,int from_x,int from_y,int target_x,int target_y,bool controller,bool enemy){
    auto&a=c.a;auto&m=target_arrow_motion;
    if(m.owner!=&a||!m.active){m={};m.owner=&a;m.time=a.map_clock_ms;m.x=from_x*256;m.y=from_y*256;}
    m.active=true;
    if(controller){const unsigned elapsed=mn(72,int(a.map_clock_ms-m.time));
        m.x+=(target_x*256-m.x)*int(elapsed)*14/1000;m.y+=(target_y*256-m.y)*int(elapsed)*14/1000;
        target_x=m.x/256;target_y=m.y/256;
    }else{m.x=target_x*256;m.y=target_y*256;}
    m.time=a.map_clock_ms;
    // NTargetingArrow offsets its head by 88 and final segment by 40 in
    // authored pixels, and bends a quadratic curve away from its origin.
    // Preserve nineteen textured segments and the reference head orientation.
    const int head_x=target_x-m.sine*18/1024,head_y=target_y+m.cosine*18/1024;
    const int end_x=target_x-m.sine*8/1024,end_y=target_y+m.cosine*8/1024;
    const int control_x=from_x-(head_x-from_x)/4;
    const int control_y=from_y>120?head_y+(head_y-from_y)/2:(head_y*3+from_y)/4;
    ui_direction(target_x-control_x,target_y-control_y,m.sine,m.cosine);
    Canvas layer=c;if(enemy){layer.tint_red=230;layer.tint_green=30;layer.tint_blue=27;}
    const auto segment=a.content.image(a.content.ui_image(I6_TargetArrowSegment));
    int previous_x=from_x,previous_y=from_y;
    for(int i=0;i<19;++i){const int inverse=20-i;
        const int x=(from_x*inverse*inverse+2*control_x*i*inverse+end_x*i*i)/400;
        const int y=(from_y*inverse*inverse+2*control_y*i*inverse+end_y*i*i)/400;
        int sn=0,cs=1024;
        if(i)ui_direction(x-previous_x,y-previous_y,sn,cs);
        else ui_direction(2*(control_x-from_x),2*(control_y-from_y),sn,cs);
        const int size=mx(1,29*(1147+1147*i/19)/4096);
        ui_picture_transform(layer,segment,x,y,size,size,sn,cs);previous_x=x;previous_y=y;
    }
    const int scale=enemy?4301:3891;
    ui_picture_transform(layer,a.content.ui_image(I6_TargetArrowHead),head_x,head_y,36*scale/4096,40*scale/4096,m.sine,m.cosine);
}
static void pointer_overlay(Canvas&c){
    auto&a=c.a;
    // The two authored black transition layers composite before the host
    // pointer. Per-row coverage preserves the original moving gradient.
    if(a.map_travel.pending||a.map_travel.fading_in){
        for(int y=0;y<ScreenH;++y){const unsigned alpha=a.map_transition_alpha(y);
            if(alpha==255)c.rect(0,y,ScreenW,1,0);
            else if(alpha)for(int x=0;x<ScreenW;++x)c.paint(y*c.pitch+x,0,alpha);
        }
        if(a.map_transition_blocks_pointer())a.hit_count=0;
    }
    auto&p=a.pointer;bool targeting=false;
    const bool modal=a.title||a.paused||a.settings_open||a.frontend_confirm||a.help||a.library||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||a.map_transition_blocks_pointer();
    int armed=a.keyboard_card>=0?a.keyboard_card:p.armed_card;
    if(armed<0&&p.held&&p.dragging&&p.captured.kind==HitKind::Hand)armed=p.captured.value;
    if(!modal&&armed>=0&&armed<a.game.card_count&&a.game.zones[armed]==Zone::Hand&&a.game.phase==Phase::Combat&&card_targets_enemy(a.game,a.game.cards[armed],armed)){
        const bool controller=a.keyboard_card>=0;
        int x=p.x,y=p.y;bool enemy=false;
        if(controller&&a.target>=0&&a.target<a.game.enemy_count&&a.game.enemies[a.target].hp>0){const auto pose=enemy_pose(a.game,a.target);x=pose.x;y=pose.feet-35;enemy=true;}
        else if(const auto*h=a.hit_at(x,y))enemy=h->enabled&&(h->kind==HitKind::Enemy||h->kind==HitKind::EnemyIntent);
        targeting_arrow(c,160,198,x,y,controller,enemy);targeting=true;
    }else if(!modal&&a.panel==4&&a.potion_targeting){
        const bool controller=!p.visible;int x=p.x,y=p.y;bool enemy=false;
        if(controller&&a.game.phase==Phase::Combat&&a.target>=0&&a.target<a.game.enemy_count){const auto pose=enemy_pose(a.game,a.target);x=pose.x;y=pose.feet-35;enemy=true;}
        else if(const auto*h=a.hit_at(x,y))enemy=h->enabled&&(h->kind==HitKind::Enemy||h->kind==HitKind::EnemyIntent||h->kind==HitKind::PotionMerchant);
        const int origin=128+(a.scroll%3)*(potion_slot_count(a.game)>3?14:17)+7;
        targeting_arrow(c,origin,13,x,y,controller,enemy);targeting=true;
    }
    if(!targeting)target_arrow_motion.active=false;
    // NTargetingArrow hides the normal cursor while following a mouse target.
    if(!p.visible||targeting)return;
    const auto*h=a.hit_at(p.x,p.y);const bool inspect=a.library&&h&&h->kind==HitKind::LibraryItem;
    const int image=a.content.ui_image(inspect?I6_CursorInspect:p.held?I6_CursorTilted:I6_CursorDefault);
    if(image>=0)c.picture(image,p.x-3,p.y-(inspect?3:1),16,16);
}
