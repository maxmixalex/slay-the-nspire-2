static void reward_pickup_screen(Canvas&c){
 auto&a=c.a;auto&m=a.reward_presentation;
 if(a.title||a.library||a.character_select||a.map_preview.open)return;
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
   const int image=f.kind==RewardPickupKind::Potion?a.content.potion_image(Potion(f.model)):a.relic_face_image(RelicId(f.model));
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
