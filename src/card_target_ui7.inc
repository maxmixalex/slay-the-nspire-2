// NSelectionReticle + selection_reticle.tscn. Unlike the map's instances,
// combat uses the shared 50% shadow, and clips only at the viewport edges.
static void target_reticle_corner(Canvas&c,const Picture&p,int x,int y,int size,bool flip_x,bool flip_y,unsigned alpha,bool shadow){
    if(size<=0)return;
    for(int yy=mx(0,y);yy<mn(c.clip_h,y+size);++yy){
        unsigned sy=unsigned((2*(yy-y)+1)*p.h/(2*size));if(flip_y)sy=p.h-1-sy;
        for(int xx=mx(0,x);xx<mn(c.clip_w,x+size);++xx){
            unsigned sx=unsigned((2*(xx-x)+1)*p.w/(2*size));if(flip_x)sx=p.w-1-sx;
            const unsigned at=sy*p.w+sx,source=p.alpha?p.alpha[at]:255;
            unsigned opacity=div255(source*alpha);if(shadow)opacity=div255(opacity*128);
            if(!opacity)continue;const auto*pixel=p.palette?p.palette+p.pixels[at]*2:p.pixels+at*2;
            c.paint(yy*c.pitch+xx,shadow?0:uint16_t(pixel[0]|unsigned(pixel[1])<<8),opacity);
        }
    }
}
static void target_reticle_draw(Canvas&c,TargetReticleMotion&m){
    if(!m.active)return;const unsigned elapsed=c.a.map_clock_ms-m.start;
    int scale=m.scale(c.a.map_clock_ms);unsigned alpha=255;
    if(!m.selected){
        if(elapsed>=200){m.active=false;return;}
        const int progress=CombatCardMotion::sine(int(elapsed)*1024/200);
        scale=m.from_scale+(4301-m.from_scale)*progress/1024;alpha=unsigned(255*(1024-progress)/1024);
    }
    const auto image=c.a.content.image(c.a.content.world_image("map","SELECTION_RETICLE"));
    if(!image.pixels||image.format>3)return;
    const int cx=(m.left+m.right)/2,cy=(m.top+m.bottom)/2,w=(m.right-m.left)*scale/4096,h=(m.bottom-m.top)*scale/4096;
    const int size=mx(1,(7*scale+2048)/4096),inset=mx(1,(2*scale+2048)/4096);
    for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;
        const int x=right?cx+w/2-size+inset:cx-w/2-inset,y=bottom?cy+h/2-size+inset:cy-h/2-inset;
        target_reticle_corner(c,image,x+1,y+1,size,right,bottom,alpha,true);
        target_reticle_corner(c,image,x,y,size,right,bottom,alpha,false);
    }
}
static void card_target_reticles(Canvas&c,bool modal){
    auto&a=c.a;const auto&s=a.game;bool selected[MaxEnemies+2]{};
    if(!modal&&!a.panel&&s.phase==Phase::Combat){
        const int card=a.selected_hand_card();
        if(card>=0&&card<s.card_count&&s.zones[card]==Zone::Hand){
            const auto type=card_target_type(s,s.cards[card],card);
            if(type==CardTargetType::AnyEnemy){
                if(a.keyboard_card>=0&&enemy_allow_target(s,a.target))selected[1+a.target]=true;
                else if(const auto*hit=a.hit_at(a.pointer.x,a.pointer.y))
                    if(hit->enabled&&(hit->kind==HitKind::Enemy||hit->kind==HitKind::EnemyIntent)&&enemy_allow_target(s,hit->value))selected[1+hit->value]=true;
            }else{
                // NMouseCardPlay.PlayZoneThreshold is 75% of viewport height,
                // adjusted by the actual press position (100/50 source px).
                const int threshold=a.pointer.down_y>180?mx(180,a.pointer.down_y-22):mn(180,a.pointer.down_y-11);
                const bool in_play_zone=a.keyboard_card>=0||(a.pointer.visible&&a.pointer.y<threshold);
                if(in_play_zone){
                    if(type==CardTargetType::AllEnemies||type==CardTargetType::RandomEnemy)
                        for(int i=0;i<s.enemy_count;++i)selected[1+i]=enemy_allow_target(s,i);
                    else if(type==CardTargetType::Self||type==CardTargetType::AllAllies)selected[0]=s.hp>0;
                    else if(type==CardTargetType::Osty)selected[MaxEnemies+1]=s.expansion.osty_hp>0;
                }
            }
        }
    }else if(!modal&&a.panel==4&&a.potion_targeting&&s.phase==Phase::Combat){
        if(!a.pointer.visible&&a.target>=0&&a.target<s.enemy_count&&can_use_potion(s,a.scroll,a.target))selected[1+a.target]=true;
        else if(const auto*hit=a.hit_at(a.pointer.x,a.pointer.y))
            if(hit->enabled&&(hit->kind==HitKind::Enemy||hit->kind==HitKind::EnemyIntent)&&can_use_potion(s,a.scroll,hit->value))selected[1+hit->value]=true;
    }
    const bool combat=combat_phase(s.phase)&&!a.title;int scene_x=0,scene_y=0;a.combat_scene_offset(scene_x,scene_y);
    for(int actor=0;actor<MaxEnemies+2;++actor){auto&motion=a.target_reticles[actor];
        if(!combat){motion={};continue;}
        motion.observe(selected[actor],a.map_clock_ms);
        if(selected[actor]){
            if(actor>0&&actor<=MaxEnemies){const auto pose=a.combat_enemy_pose(actor-1);const int h=84*pose.scale/1024;motion.bounds(pose.x-pose.width/2+scene_x,pose.feet-h+scene_y,pose.width,h);}
            else{
                // Authored Bounds from the five creature_visuals scenes;
                // the combat camera uses a 1/6 source-to-screen projection.
                static constexpr int sizes[6][2]={{242,278},{244,244},{250,260},{230,335},{210,270},{232,204}};
                const int index=actor==MaxEnemies+1?5:int(s.character),zoom=s.enemy_world.active?encounter_camera(s.enemy_world.encounter).zoom:1000;
                const int w=sizes[index][0]*zoom/6000,h=sizes[index][1]*zoom/6000;
                const int x=combat_player_x(s)+(actor==MaxEnemies+1?52:0),feet=actor==MaxEnemies+1?123:124;
                motion.bounds(x-w/2+scene_x,feet-h+scene_y,w,h);
            }
        }
        if(!modal)target_reticle_draw(c,motion);
    }
}
