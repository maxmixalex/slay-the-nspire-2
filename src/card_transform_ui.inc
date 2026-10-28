static void card_transform_specks(Canvas&c,int x,int y,int width,uint32_t age,uint32_t seed,bool end){
 const unsigned count=end?3:12;
 for(unsigned i=0;i<count;++i){
  const uint32_t hash=room_vfx_hash(seed+i*0x9e3779b9u),life=(end?400u:200u)+hash%(end?1601u:801u);
  if(age>=life)continue;
  const int source_x=end?25+int(hash%151)-75:int(hash%423)-211;
  const int source_y=end?25+int((hash>>9)%151)-75:int((hash>>9)%301)-150;
  const int speed=(end?300:100)+int((hash>>17)%(end?901:2401));
  const int angle=(end?2560:2350)+int(hash%229)-114;
  // Particle coordinates and velocities use the original300px card
  // domain. Cosmetic hashes replace Godot's independent particle RNG.
  const int dx=CombatCardMotion::sine(angle+1024),dy=CombatCardMotion::sine(angle);
  const int travel=int(int64_t(speed)*age*(2000-int(age)/2)/2000000);
  const int radial=end?0:150+int(hash%101),distance=mx(1,mx(source_x<0?-source_x:source_x,source_y<0?-source_y:source_y));
  const int radial_x=int(int64_t(source_x)*radial*age*(2000-age)/(distance*2000000ll));
  const int radial_y=int(int64_t(source_y)*radial*age*(2000-age)/(distance*2000000ll));
  const int turbulence=CombatCardMotion::sine(int(age*3+(hash>>12)))*int(age)/64000;
  const int px=x+(source_x+dx*travel/1024+radial_x+turbulence)*width/300;
  const int py=y+(source_y+dy*travel/1024+radial_y-turbulence/2)*width/300;
  const unsigned q=age*4096/life;
  const int scale=mx(0,CardTransformPresentation::hermite(4096,0,0,-10624,4096,q));
  const int size=mx(1,(13+int(hash%14))*width*scale/(300*4096));
  const int elongation=4096+mn(4096*2,speed*4096/300);
  Canvas spark=c;
  ui_picture_transform(spark,c.a.content.ui_image(I13_CardTransformSpeck),px,py,size,mx(1,size*elongation/4096),
      CombatCardMotion::sine(angle-1024),CombatCardMotion::sine(angle));
 }
}
static void card_transform_particles(Canvas&c,int x,int y,int width,uint32_t age,uint32_t seed){
 if(age>=CardTransformPresentation::Swap)
  card_transform_specks(c,x,y,width,age-CardTransformPresentation::Swap,seed,false);
 if(age<CardTransformPresentation::EndParticles)return;
 const uint32_t elapsed=age-CardTransformPresentation::EndParticles;
 const int ring_x=x+150*width/300,ring_y=y-211*width/300;
 if(elapsed<500){
  const unsigned q=elapsed*4096/500,split=614;
  const int scale=q<=split?CardTransformPresentation::hermite(1004,3483,38317,754,split,q*4096/split):
      CardTransformPresentation::hermite(3483,4096,754,659,4096-split,(q-split)*4096/(4096-split));
  const int size=mx(1,256*width*4*scale/(300*5*4096));
  ui_picture_transform(c,c.a.content.ui_image(I13_CardTransformRing+elapsed*I13_CardTransformRingCount/500),ring_x,ring_y,size,size,-724,-724);
 }
 if(elapsed<150){
  const unsigned q=elapsed*4096/150;
  const int scale=mx(0,CardTransformPresentation::hermite(4096,0,-8966,0,4096,q));
  const int size=mx(1,256*width*3*scale/(300*4*4096));
  const int angle=int(room_vfx_hash(seed)%4096)+int(elapsed)*(int(room_vfx_hash(seed+1)%8193)-4096)/1000;
  ui_picture_transform(c,c.a.content.ui_image(I13_CardTransformFlare),x+146*width/300,y-207*width/300,size,size,
      CombatCardMotion::sine(angle),CombatCardMotion::sine(angle+1024));
 }
 card_transform_specks(c,x+125*width/300,y-184*width/300,width,elapsed,seed^0x527a109bu,true);
}
static const char* card_transform_description(App&a,const CardTransformPresentation::Pair&p,bool after,unsigned slot){
 struct Text {const Content*owner=nullptr;uint32_t generation=0,start=0,key=0;char text[512]{};};
 static Text descriptions[CardTransformPresentation::Columns];auto&cache=descriptions[slot%CardTransformPresentation::Columns];
 const auto&f=after?p.after:p.before;const auto&model=f.model;
 uint32_t key=unsigned(card_token(model.card))|unsigned(after)<<16;
 const unsigned facts[]={unsigned(model.facts.enchantment),unsigned(uint16_t(model.facts.enchantment_amount)),
     unsigned(model.facts.science_type),unsigned(model.facts.rider),unsigned(uint16_t(model.facts.energy_offset)),
     unsigned(uint16_t(model.facts.progress)),unsigned(model.genetic),unsigned(model.scythe)};
 for(unsigned value:facts)key=(key^value)*16777619u;
 if(cache.owner!=&a.content||cache.generation!=a.content.generation()||cache.start!=p.start||cache.key!=key){
  cache.owner=&a.content;cache.generation=a.content.generation();cache.start=p.start;cache.key=key;
  a.world_card_description(model.card,-1,cache.text,sizeof cache.text,-1,-1,&model.facts,&model);
 }return cache.text;
}
static void transformed_card_animation(Canvas&c){
 auto&a=c.a;auto&t=a.reward_presentation.transforms;
 for(unsigned i=0;i<t.count;++i){auto&p=t.pairs[i];
  if(!CardTransformPresentation::active(p,a.map_clock_ms)||int32_t(a.map_clock_ms-p.start)<0)continue;
  const uint32_t age=a.map_clock_ms-p.start;
  const int width=t.group_size(i)>1?76:HandFaceW,x=t.center(i),y=120;
  if(age>=CardTransformPresentation::Fly){
   if(!p.launched){a.reward_presentation.append(RewardPickupKind::Card,p.after.model.card.kind,unsigned(p.deck),x,y,
       p.start+CardTransformPresentation::Fly,p.after.model.card,&p.after.model);p.launched=true;}
  }else{
   const bool after=age>=CardTransformPresentation::Swap;const auto&face=after?p.after:p.before;
   CombatCardMotion::Sprite sprite;sprite.card=face.model.card;sprite.energy=face.energy;sprite.stars=face.stars;
   const auto&tile=hand_tile(c,-1,&sprite,-1,&face.model.facts,false,card_transform_description(a,p,after,i));
   const int sx=CardTransformPresentation::scale(age,false)*width/HandFaceW,sy=CardTransformPresentation::scale(age,true)*width/HandFaceW;
   const int tile_w=HandTileW*sx/4096,tile_h=HandTileH*sy/4096;
   if(tile_w&&tile_h)c.picture(Picture{reinterpret_cast<const uint8_t*>(tile.pixels),tile.alpha,nullptr,HandTileW,HandTileH,1},
       x-int(int64_t(HandPivotX)*sx/(1024*4096)),y-int(int64_t(HandPivotY)*sy/(1024*4096)),tile_w,tile_h);
   const int card_w=HandFaceW*sx/4096,card_h=HandFaceH*sy/4096;
   if(card_w&&card_h){
    const unsigned overlay=CardTransformPresentation::overlay(age),glow=CardTransformPresentation::glow(age);
    if(overlay)c.ui(I13_CardTransformMask,x-card_w/2,y-card_h/2,card_w,card_h,overlay);
    if(glow){const unsigned scale=CardTransformPresentation::glow_scale(age);
     const int w=card_w*scale/4096,h=card_h*scale/4096;c.ui(I13_CardTransformGlow,x-w/2,y-h/2,w,h,glow);}
    if(age>=CardTransformPresentation::ShineParticles&&age<CardTransformPresentation::ShineParticles+350)
     c.ui(I13_CardTransformShine+CardTransformPresentation::shine_frame(age),x-card_w/2,y-card_h/2,card_w,card_h);
   }
  }
  card_transform_particles(c,x,y,width,age,p.start^unsigned(card_token(p.before.model.card)));
 }
}
