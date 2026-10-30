static void card_effect_texture(Canvas&c,int image,int x,int y,int w,int h,int sine=0,int cosine=1024,unsigned opacity=255,unsigned mode=presentation::Normal){
 if(!w||!h||!opacity)return;
 if(mode==presentation::Normal){ui_picture_transform(c,c.a.content.image(image),x,y,w,h,sine,cosine,false,opacity);return;}
 const auto p=c.a.content.image(image);if(!p.pixels||p.format>3)return;
 const int sn=CombatCardMotion::magnitude(sine),cs=CombatCardMotion::magnitude(cosine);
 const int hw=(cs*w+sn*h)/2048+2,hh=(sn*w+cs*h)/2048+2;
 const int left=mx(0,x-hw),right=mn(c.clip_w,x+hw),top=mx(0,y-hh),bottom=mn(c.clip_h,y+hh);
 const int du=cosine*int(p.w)/w,dv=-sine*int(p.h)/h;
 for(int yy=top;yy<bottom;++yy){int u=int(p.w)*512+((left-x)*cosine+(yy-y)*sine)*int(p.w)/w;
  int v=int(p.h)*512+(-(left-x)*sine+(yy-y)*cosine)*int(p.h)/h;
  for(int xx=left;xx<right;++xx,u+=du,v+=dv){if(u<0||v<0||u>=int(p.w)*1024||v>=int(p.h)*1024)continue;
   const unsigned at=unsigned(v/1024)*p.w+unsigned(u/1024),alpha=div255((p.alpha?p.alpha[at]:255u)*opacity);if(!alpha)continue;
   const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;c.paint(yy*c.pitch+xx,uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha,mode);
  }
 }
}
static void card_smith_particles(Canvas&,HandPose,unsigned,uint32_t);
static void combat_smith_sparks(Canvas&c,HandPose pose,uint32_t elapsed,uint32_t seed){card_smith_particles(c,pose,elapsed,seed);}
static void combat_sovereign_blades(Canvas&c,bool behind){
 auto&a=c.a;if(!combat_phase(a.game.phase)||a.combat_health(0).hp<=0)return;
 const auto&m=a.combat_cards.blades;const uint32_t now=a.animation*33u;
 const auto camera=encounter_camera(a.game.enemy_world.encounter);
 const auto layout=a.creature_layout.right?a.creature_layout:combat_layout_for(a.game);
 const auto player=a.combat_player_pose();
 for(unsigned i=0;i<SovereignBladePresentation::Capacity;++i){const auto&blade=m.blades[i];if(!blade.active)continue;
  const auto path=m.orbit(i,now);const bool back=path.phase>16384&&path.phase<51118&&blade.damage<120&&!blade.attacking;
  if(back!=behind)continue;
  const unsigned size=SovereignBladePresentation::size(blade,now);if(!size)continue;
  const unsigned strength=mn(200u,unsigned(blade.damage));
  const int source_x=path.x+(3200-path.x)*int(strength)/250;
  const int source_y=path.y-(int(size)-4096)*1600/4096;
  int x=player.x+combat_project_extent(camera,layout,source_x,1,16),y=player.feet+combat_project_extent(camera,layout,source_y,1,16);
  int angle=1024;
  if(blade.attacking){const unsigned age=now-blade.attack;angle=CombatCardMotion::direction(blade.ty-y,blade.tx-x)+1024;
   const int pull=combat_project_extent(camera,layout,50);
   if(age<80)x-=pull*int(SovereignBladePresentation::cubic(age,80))/4096;
   else if(age<130){const unsigned t=(age-80)*4096/50,q=t*t/4096*t/4096*t/4096*t/4096;x=x-pull+(blade.tx-x+pull)*int(q)/4096;y+=(blade.ty-y)*int(q)/4096;}
   else if(age<680){x=blade.tx;y=blade.ty;}
   else {const unsigned q=mn(4096u,(age-680)*4096/120);x=blade.tx+(x-blade.tx)*int(q)/4096;y=blade.ty+(y-blade.ty)*int(q)/4096;}
  }
  const int sine=CombatCardMotion::sine(angle),cosine=CombatCardMotion::sine(angle+1024);
  // The source ScaleContainer is.2; our full500x1500 texture keeps its
  // authored hilt pivot at(250,1135),385 source pixels below its center.
  const int offset=combat_project_extent(camera,layout,77,int(size),4096);
  x+=sine*offset/1024;y-=cosine*offset/1024;
  const int w=mx(1,combat_project_extent(camera,layout,100,int(size),4096));
  const int h=mx(1,combat_project_extent(camera,layout,300,int(size),4096));
  card_effect_texture(c,a.content.ui_image(I14_SovereignBlade+(blade.damage>=60)),x,y,w,h,sine,cosine);
  // NSovereignBladeVfx owns a hoverable hitbox attached to the actual sword.
  // Ignore attack/removal and targeting just as the source does; inspecting
  // the orbiting model must never play or select the matching hand card.
  if(!blade.attacking&&!blade.removing&&blade.id<a.game.card_count&&a.game.phase==Phase::Combat&&
     a.selected_hand_card()<0&&!a.pointer.held&&!a.panel){
   const int half_w=mx(5,(CombatCardMotion::magnitude(cosine)*w+CombatCardMotion::magnitude(sine)*h+2047)/2048);
   const int half_h=mx(5,(CombatCardMotion::magnitude(sine)*w+CombatCardMotion::magnitude(cosine)*h+2047)/2048);
   c.hit(x-half_w,y-half_h,2*half_w,2*half_h,HitKind::SovereignBlade,blade.id);
  }
  if(blade.forging){const unsigned age=now-blade.forged;if(age<550){const unsigned t=age>50?(age-50)*4096/500:0;
    const unsigned alpha=age<50?age*255/50:255-(t*t/4096*t/4096)*255/4096;
    Canvas glow=c;glow.tint_green=115;glow.tint_blue=0;
    card_effect_texture(glow,a.content.ui_image(I14_SovereignBlade+2),x,y,w,h,sine,cosine,alpha,presentation::Add);}}
 }
}
static void combat_forge_card_previews(Canvas&c){
 auto&a=c.a;const auto&m=a.combat_cards.blades;const uint32_t now=a.animation*33u;
 unsigned offhand=0;for(const auto&b:m.blades)if(b.active&&b.forging&&b.id<a.game.card_count&&a.game.zones[b.id]!=Zone::Hand)++offhand;
 unsigned slot=0;
 for(const auto&b:m.blades){if(!b.active||!b.forging||b.id>=a.game.card_count)continue;
  const unsigned age=now-b.forged;if(age>=900)continue;
  HandPose pose{};bool visible=false;const auto hand=a.combat_hand();
  for(unsigned j=0;j<hand.count;++j)if(hand.ids[j]==b.id){pose=a.combat_hand_pose(j,a.hand_focus&&a.cursor==int(j));visible=true;break;}
  if(!visible){const int width=offhand>1?64:HandFaceW;
   pose={160+(2*int(slot++)-int(mn(3u,offhand))+1)*(width+6)/2,112,int(SovereignBladePresentation::cubic(age,250))*width*1024/(4096*HandFaceW),0,1024};
   if(slot>3)continue;
   CombatCardMotion::Sprite visual;visual.card=a.game.cards[b.id];visual.energy=int16_t(cost(a.game,b.id));visual.stars=int16_t(card_star_cost(a.game,visual.card,b.id));
   draw_hand_tile(c,hand_tile(c,b.id,&visual),pose,false);
  }
  combat_smith_sparks(c,pose,age,b.forged^unsigned(b.id));
 }
}
