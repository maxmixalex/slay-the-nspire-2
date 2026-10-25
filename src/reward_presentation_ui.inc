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
static void reward_pickup_screen(Canvas&c){
 auto&a=c.a;auto&m=a.reward_presentation;
 if(a.title||a.library||a.character_select||a.map_preview.open)return;
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
  int tx=266,ty=11;
  if(f.kind==RewardPickupKind::Potion){
   // The normal belt has three visible slots; an off-page acquisition lands
   // at its page affordance rather than at an unrelated visible bottle.
   tx=f.slot/3==unsigned(a.potion_page)?128+int(f.slot%3)*(potion_slot_count(a.game)>3?14:17)+7:174;
  }else if(f.kind==RewardPickupKind::Relic){tx=f.slot<15?11+int(f.slot)*19:302;ty=32;}
  const auto v=m.sample(f,tx,ty,a.map_clock_ms);if(!v.visible)continue;
  if(f.kind==RewardPickupKind::Card){
   const int deck=f.slot<a.game.deck_count&&card_token(a.game.deck[f.slot])==card_token(f.card)?int(f.slot):-1;
   CombatCardMotion::Sprite sprite;sprite.card=f.card;sprite.energy=int16_t(a.card_offer_cost(f.card,event_card_facts(a.game,f.card,-1,deck)));sprite.stars=int16_t(card_star_cost(a.game,f.card));
   const auto&tile=hand_tile(c,-1,&sprite,deck);
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
   Canvas popup=c;popup.opacity=uint8_t(div255(popup.opacity*alpha));popup.text_fitted(value,99,y+11,37,8,m.gold_to>=m.gold_from?Gold:Red,8);}
 }
}
