static void removed_card_preview(Canvas&c,const RewardPresentation::RemovedCard&removed){
 // One bounded visual snapshot is needed after the core compacts the deck.
 // Rasterize once, then scale its whole face (including labels and costs) as
 // the original Card node does. Minimum-font layout must not run at 1px size.
 struct Tile {const Content*owner=nullptr;uint32_t generation=0,start=0;Card card{};int energy=0,stars=0,portrait=-1,frame=-1;CardType type=CardType::Attack;
  char description[1024]{};uint16_t pixels[96*120]{};uint8_t alpha[96*120]{};};
 static Tile tile;
 auto&a=c.a;const int portrait=a.card_face_image(removed.card,-1,-1,&removed.facts),frame=a.card_face_frame(removed.card,-1,-1,&removed.facts);
 const auto type=a.card_face_type(removed.card,-1,-1,&removed.facts);
 if(tile.owner!=&a.content||tile.generation!=a.content.generation()||tile.start!=removed.start||tile.card.kind!=removed.card.kind||tile.card.upgraded!=removed.card.upgraded||
    tile.energy!=removed.energy||tile.stars!=removed.stars||tile.portrait!=portrait||tile.frame!=frame||tile.type!=type||!card_cache_text_equal(tile.description,removed.description)){
  tile.owner=&a.content;tile.generation=a.content.generation();tile.start=removed.start;tile.card=removed.card;tile.energy=removed.energy;tile.stars=removed.stars;tile.portrait=portrait;tile.frame=frame;tile.type=type;
  card_cache_text_copy(tile.description,removed.description);
  for(unsigned i=0;i<96*120;++i){tile.pixels[i]=0;tile.alpha[i]=0;}
  Canvas face{a,tile.pixels,96,96,120,tile.alpha};
  draw_card_face(face,removed.card,12,6,80,112,removed.energy,removed.stars,-1,-1,&removed.facts,removed.description);
 }
 const unsigned scale=removed.scale(a.map_clock_ms);const int w=int(96*scale/4096),h=int(120*scale/4096);
 if(w>0&&h>0)c.picture(Picture{reinterpret_cast<const uint8_t*>(tile.pixels),tile.alpha,nullptr,96,120,1},160-int(52*scale/4096),120-int(62*scale/4096),w,h);
}
static void reward_card_trail(Canvas&c,const RewardPresentation::Flight&flight,int tx,int ty){
 const auto&a=c.a;const uint32_t age=a.map_clock_ms-flight.start;
 static constexpr uint32_t outer[]={0xff2b00,0x00aa1e,0x009ac9,0x9f4600,0xff2049};
 static constexpr uint32_t inner[]={0xffd300,0xffff00,0x00ffe1,0xffff00,0xf5aa9b};
 const unsigned character=unsigned(a.game.character);if(character>=CharacterKinds)return;
 const unsigned fade=age<flight.arrival?255:unsigned(mx(0,255-int((age-flight.arrival)*255/500)));
 if(!fade)return;
 // Eight historical samples reproduce the authored curved ribbon without
 // retaining a frame-dependent trail array. Owner colors match CardTrailVfx.
 for(unsigned i=8;i>0;--i){const uint32_t behind=i*28;
  if(age<behind||age-behind>=flight.arrival)continue;
  const auto v=a.reward_presentation.sample(flight,tx,ty,a.map_clock_ms-behind);
  const int side=3+int((9-i)*11/8);const unsigned alpha=fade*(9-i)/18;
  room_vfx_stamp(c,I7_Glow,v.x-side/2,v.y-side/2,side,side,alpha,outer[character],presentation::Add,false);
  room_vfx_stamp(c,I7_GlowSpeck,v.x-2,v.y-2,4,4,alpha,inner[character],presentation::Add,false);
 }
}
static void reward_inventory_flash(Canvas&c,const RewardPresentation::Flight&flight,int x,int y){
 const uint32_t elapsed=c.a.map_clock_ms-flight.start;if(elapsed<350)return;const uint32_t age=elapsed-350;
 const bool potion=flight.kind==RewardPickupKind::Potion;const unsigned count=potion?1:3;
 if(age>=(potion?1000u:1910u))return;
 const int image=potion?c.a.content.potion_image(Potion(flight.model)):c.a.relic_face_image(RelicId(flight.model),int(c.a.game.character));
 const int width=potion?14:17,height=potion?18:17;
 uint16_t pixels[17*18]{};uint8_t coverage[17*18]{};
 Canvas tile{c.a,pixels,17,width,height,coverage};tile.picture(image,0,0,width,height);
 // Potion power_flash_vfx squares the texture and doubles RGB. Relic flashes
 // use the unchanged icon with the scene's160/255 additive self-modulation.
 if(potion)for(int yy=0;yy<height;++yy)for(int xx=0;xx<width;++xx){const unsigned at=unsigned(yy*17+xx),color=pixels[at];
  const unsigned r=color>>11,g=color>>5&63,b=color&31;
  pixels[at]=uint16_t((mn(31,int(2*r*r/31))<<11)|(mn(63,int(2*g*g/63))<<5)|mn(31,int(2*b*b/31)));
  coverage[at]=uint8_t(div255(unsigned(coverage[at])*coverage[at]));
 }
 // Compact the potion's14 columns so Picture keeps its ordinary dense stride.
 if(width!=17)for(int yy=1;yy<height;++yy)for(int xx=0;xx<width;++xx){pixels[yy*width+xx]=pixels[yy*17+xx];coverage[yy*width+xx]=coverage[yy*17+xx];}
 const Picture picture{reinterpret_cast<const uint8_t*>(pixels),coverage,nullptr,uint16_t(width),uint16_t(height),1};
 for(unsigned i=0;i<count;++i){const uint32_t delay=i*200;if(age<delay)continue;const uint32_t t=age-delay;
  const unsigned life=potion?1000:1510;if(t>=life)continue;
  const unsigned alpha=potion?(t<57?t*255/57:t<533?255-(t-57)*14/476:(1000-t)*241/467):(t<10?160*t/10:160*(1510-t)/1500);
  const unsigned scale=potion?4096+3191*room_curve::sine_in_out(t,1000)/4096:4096+room_curve::cubic_out(t,1000)/4;
  const int w=int(width*scale/4096),h=int(height*scale/4096);c.picture(picture,x-w/2,y-h/2,w,h,alpha,presentation::Add);
 }
}
static void upgraded_card_animation(Canvas&c){
 auto&a=c.a;const auto&m=a.reward_presentation.upgrades;if(!m.alive(a.map_clock_ms))return;
 const unsigned elapsed=a.map_clock_ms-m.start;
 for(unsigned i=0;i<m.count;++i){
  const unsigned delay=i*100u;if(elapsed<delay)continue;const unsigned age=elapsed-delay;if(age>=2250u)continue;
  const auto&card=m.cards[i];const int base_w=m.count>3?63:m.count>1?76:HandFaceW;
  const int center=160+(2*int(i)-int(m.count)+1)*(base_w+8)/2;
  unsigned scale=4096,alpha=255;int x=center,y=113;
  if(age<250u)scale=room_curve::cubic_out(age,250);
  if(age>=1750u){const unsigned t=mn(500u,age-1750u),q=room_curve::multiply(room_curve::multiply(room_curve::linear(t,500),room_curve::linear(t,500)),room_curve::linear(t,500));
   x=center+(266-center)*int(q)/4096;y=113+(11-113)*int(q)/4096;scale=4096-3604u*q/4096;
   if(t>400u)alpha=(500u-t)*255u/100u;}
  Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*alpha));
  // Reuse the existing hand cache: rasterize the committed card once at
  // readable size, then transform its whole face. Running minimum-font
  // layout at each tween size makes text spill out of the growing frame.
  CombatCardMotion::Sprite sprite;sprite.card=card.card;sprite.energy=int16_t(a.card_offer_cost(card.card,&card.facts));sprite.stars=int16_t(card_star_cost(a.game,card.card));
  const auto&tile=hand_tile(c,-1,&sprite,-1,&card.facts,true);
  draw_hand_tile(layer,tile,{x,y,int(unsigned(base_w)*scale*1024u/(unsigned(HandFaceW)*4096u)),0,1024},false);
  // Source golden upgrade sparks use a one-shot radial burst. Twelve
  // integer trajectories keep the real trigger/clock with bounded LCD work.
  if(age<750u){const unsigned spread=8u+age*50u/750u,fade=(750u-age)*255u/750u;
   for(unsigned j=0;j<12;++j){const unsigned angle=j*4096u/12u+i*137u;const int dx=int(CombatCardMotion::sine(angle))*int(spread)/1024;
    const int dy=int(CombatCardMotion::sine(angle+1024u))*int(spread)/1024;
    Canvas spark=layer;spark.opacity=uint8_t(div255(spark.opacity*fade));spark.line(x+dx-1,y+dy,x+dx+1,y+dy,rgb(255,255,138));spark.line(x+dx,y+dy-1,x+dx,y+dy+1,rgb(255,255,138));}
  }
 }
}
#include "card_transform_ui.inc"
static void reward_pickup_screen(Canvas&c){
 auto&a=c.a;auto&m=a.reward_presentation;
 if(a.title||a.library||a.character_select||a.map_preview.open)return;
 upgraded_card_animation(c);
 transformed_card_animation(c);
 const auto&removed=m.removed;
 if(removed.alive(a.map_clock_ms)){
  const uint32_t age=a.map_clock_ms-removed.start;
  // CardPileCmd.RemoveFromDeck: 250ms CUBIC_OUT preview, 250ms hold,
  // then NCardRemoveVfx; the old card node is freed 400ms later. Capture
  // the old description/facts before deck compaction, never the next slot.
  if(age<900)removed_card_preview(c,removed);
  if(age>=500&&a.content.version()>=Ui6ContentVersion){const unsigned frame=(age-500)/50;
   if(frame<I6_CardRemoveCount)c.ui(I6_CardRemoveStart+int(frame),56,30,208,180);
  }
 }
 for(const auto&f:m.flights){
  if(f.kind==RewardPickupKind::None)continue;
  int tx=287,ty=11;
  if(f.kind==RewardPickupKind::Potion){
   // Bottle art, popup and pickup flash share the exact holder anchor.
   tx=hud_potion_layout::landing_x(f.slot,a.potion_page,potion_slot_count(a.game));
  }else if(f.kind==RewardPickupKind::Relic){tx=hud_relic_layout::center_x(f.slot);ty=hud_relic_layout::center_y(f.slot);}
  const auto v=m.sample(f,tx,ty,a.map_clock_ms);if(!v.visible){
   if((f.kind==RewardPickupKind::Potion||f.kind==RewardPickupKind::Relic)&&RewardPresentation::retained(f,a.map_clock_ms))reward_inventory_flash(c,f,tx,ty);
   continue;
  }
  if(f.kind==RewardPickupKind::Card){
   reward_card_trail(c,f,tx,ty);
   const int deck=f.slot<a.game.deck_count&&card_token(a.game.deck[f.slot])==card_token(f.card)?int(f.slot):-1;
   CombatCardMotion::Sprite sprite;sprite.card=f.card;sprite.energy=int16_t(a.card_offer_cost(f.card,f.has_snapshot?&f.snapshot.facts:event_card_facts(a.game,f.card,-1,deck)));sprite.stars=int16_t(card_star_cost(a.game,f.card));
   char description[512]{};
   if(f.has_snapshot)a.world_card_description(f.card,-1,description,sizeof description,-1,-1,&f.snapshot.facts,&f.snapshot);
   const auto&tile=hand_tile(c,-1,&sprite,f.has_snapshot?-1:deck,f.has_snapshot?&f.snapshot.facts:nullptr,false,f.has_snapshot?description:nullptr);
   draw_hand_tile(c,tile,{v.x,v.y,v.scale,CombatCardMotion::sine(v.angle),CombatCardMotion::sine(v.angle+1024)},false,v.shade);
  }else{
   const int image=f.kind==RewardPickupKind::Potion?a.content.potion_image(Potion(f.model)):a.relic_face_image(RelicId(f.model),int(a.game.character));
   const int w=f.kind==RewardPickupKind::Potion?14:17,h=f.kind==RewardPickupKind::Potion?18:17;
   c.picture(image,v.x-w/2,v.y-h/2,w,h,v.alpha);
  }
 }
 if(m.gold_active){
  uint32_t done=0;const int remaining=m.remaining_gold(a.map_clock_ms,&done);const uint32_t age=a.map_clock_ms-m.gold_start;
  unsigned alpha=age<150?age*255/150:255;int y=5+int(age<250?age*6/250:6);
  if(!remaining&&age>=done+250){const unsigned fade=age-done-250;if(fade>=250)return;alpha=fade>=100?0:255*(100-fade)/100;y=11-int(fade*6/250);}
  if(alpha){char value[32]{};TextBuffer text{value,sizeof value,0,&a.content};if(remaining>=0)text.ch('+');text.number(remaining);
   Canvas popup=c;popup.opacity=uint8_t(div255(popup.opacity*alpha));popup.text_fitted(value,88,y+11,30,8,m.gold_to>=m.gold_from?Gold:Red,8);}
 }
}
