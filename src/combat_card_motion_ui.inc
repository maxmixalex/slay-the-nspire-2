static bool ui_cards_bound(const App&a){const auto&m=a.combat_cards;const auto&s=a.game;
 return m.active&&s.enemy_world.active&&m.seed==s.seed&&m.floor==s.floor&&m.combat==s.world.combat_count&&m.encounter==int(s.enemy_world.encounter);
}
HandPose App::combat_hand_pose(unsigned index,bool focused)const{
 const auto hand=combat_hand();
 if(ui_cards_bound(*this))return combat_cards.hand_pose_at(index,focused);
 return index<hand.count&&hand.ids[index]==selected_hand_card()?HandPose{160,198,768,0,1024}:hand_pose(hand.count,int(index),focused);
}
bool App::combat_hand_settled(unsigned index,bool focused)const{
 return !ui_cards_bound(*this)||combat_cards.settled(index,focused);
}
CombatCardMotion::Counts App::combat_pile_counts()const{
 return ui_cards_bound(*this)?combat_cards.visible_counts():CombatCardMotion::capture_counts(game);
}
// Observe actual settled pile membership, including Add commands used by
// return cards and hooks. The byte-per-live-instance history is not a second
// pile or a full State snapshot; it also distinguishes generated cards from
// actual draws instead of inventing a draw-pile origin for every new card.
static bool ui_capture_card_changes(App&a,uint32_t at,bool generated_preview=false){
 auto&m=a.combat_cards;const auto&s=a.game;bool awaited_hand_add=false;
 if(m.manual>=0&&m.manual<int(s.card_count)&&uint8_t(m.zones[m.manual]&127)==unsigned(Zone::Hand)&&s.zones[m.manual]!=Zone::Hand){
  // The observed physical play starts at AddDuringManualCardPlay. Its power
  // tween may be awaited before the first effect arrives; that does not keep
  // the played card in the hand for the intervening 250 ms.
  CombatCardMotion::Cue cue;cue.at=m.manual_started;cue.id=uint16_t(m.manual);cue.from=uint8_t(Zone::Hand);cue.to=uint8_t(Zone::Limbo);
  cue.card=s.cards[m.manual];cue.energy=m.manual_energy;cue.stars=m.manual_stars;cue.fast=a.fast_mode;m.append(cue);
  auto hand=a.combat_feedback.hand.observed;unsigned n=0;for(unsigned i=0;i<hand.count;++i)if(hand.ids[i]!=m.manual)hand.ids[n++]=hand.ids[i];hand.count=uint16_t(n);
  a.combat_feedback.hand.append(hand,m.manual_started);m.zones[m.manual]=uint8_t(unsigned(Zone::Limbo)|CombatCardMotion::Tracked);
 }
 for(unsigned id=0;id<s.card_count;++id){
  const uint8_t before=id<m.card_count?uint8_t(m.zones[id]&127):CombatCardMotion::NewCard;
  const uint8_t to=uint8_t(s.zones[id]);const bool tracked=id<m.card_count&&(m.zones[id]&CombatCardMotion::Tracked);
  if(before!=to){
   if(to==unsigned(Zone::Hand)&&before!=CombatCardMotion::NewCard)awaited_hand_add=true;
   if(tracked||before==unsigned(Zone::Hand)||to==unsigned(Zone::Hand)||(generated_preview&&before==CombatCardMotion::NewCard)){
    CombatCardMotion::Cue cue;cue.at=at;cue.id=uint16_t(id);cue.from=before;cue.to=to;cue.card=s.cards[id];cue.fast=a.fast_mode;cue.preview=generated_preview;
    cue.energy=int16_t(cost(s,id));cue.stars=int16_t(card_star_cost(s,cue.card,id));
    // The manually played card keeps the resource labels seen before
    // payment; an X-cost card must not turn into zero midway through flight.
    if(int(id)==m.manual){cue.energy=m.manual_energy;cue.stars=m.manual_stars;}
    m.append(cue);
   }
  }
  m.zones[id]=uint8_t(to|((tracked||to==unsigned(Zone::Hand)||before==unsigned(Zone::Hand)||(generated_preview&&before==CombatCardMotion::NewCard))?CombatCardMotion::Tracked:0));
 }
 m.card_count=s.card_count;m.append_counts(CombatCardMotion::capture_counts(s),at);return awaited_hand_add;
}
static void ui_card_finish_wait(App&a){
 auto&m=a.combat_cards;auto&f=a.combat_feedback;
 if(m.manual>=0&&m.manual<int(a.game.card_count)){
  const auto before=uint8_t(m.zones[m.manual]&127);const auto to=uint8_t(a.game.zones[m.manual]);
  if((before==unsigned(Zone::Limbo)&&to!=unsigned(Zone::Limbo))||
     (before==unsigned(Zone::Hand)&&to!=unsigned(Zone::Hand)&&to!=unsigned(Zone::Limbo)))
   f.effect_at=ui_feedback_later(f.effect_at,m.manual_min_end);
 }
}
static uint32_t ui_card_add_wait(App&a,uint32_t at){
 auto&m=a.combat_cards;const unsigned wait=a.fast_mode?100u:250u;
 const unsigned elapsed=wait-(at==m.wait_at?m.wait_excess:0u),ticks=(elapsed+32u)/33u;
 m.wait_excess=uint8_t(ticks*33u-elapsed);m.wait_at=at+ticks;return m.wait_at;
}
static void ui_sync_card_queues(App&a){
 auto&m=a.combat_cards;auto&h=a.combat_feedback.hand;
 const bool bound=ui_feedback_bound(a);
 for(;;){
  bool available=false;uint32_t next=0;
  const auto candidate=[&](uint32_t at){if(!available||int32_t(at-next)<0){available=true;next=at;}};
  if(m.head<m.count)candidate(m.cues[m.head].at);
  if(m.count_head<m.count_count)candidate(m.count_cues[m.count_head].at);
  if(bound&&h.head<h.count)candidate(h.cues[h.head].at);
  if(!available||int32_t(a.animation-next)<0)break;
  m.advance(next);
  // Transfers keep the last visible holder pose before the simultaneous
  // hand order is replaced; a batch discard therefore has ten real origins.
  while(m.head<m.count&&m.cues[m.head].at==next)m.apply(m.cues[m.head++]);
  while(m.count_head<m.count_count&&m.count_cues[m.count_head].at==next)m.counts=m.count_cues[m.count_head++].value;
  while(bound&&h.head<h.count&&h.cues[h.head].at==next){h.shown=h.cues[h.head++].cards;m.set_hand(h.shown);}
 }
 if(m.head==m.count)m.head=m.count=0;
 if(m.count_head==m.count_count)m.count_head=m.count_count=0;
 if(bound&&h.head==h.count)h.head=h.count=0;
 m.advance(a.animation);
}
void App::sync_combat_cards(){
 auto&m=combat_cards;
 if(title||!game.enemy_world.active||game.phase==Phase::Error){m.reset();return;}
 if(!ui_cards_bound(*this)){
  if(!combat_phase(game.phase)){m.reset();return;}m.begin(game,animation);
 }
 m.set_selected(selected_hand_card());
 ui_sync_card_queues(*this);
 if(!ui_feedback_bound(*this)){
  // Continue, initial preparation and direct fixture/core entry are already
  // settled. Do not replay old run/room motions when their identity changes.
  const auto hand=CombatHand::capture(game.hand);
  if(!CombatHand::same(hand,m.hand)){m.set_hand(hand);m.counts=m.observed_counts=CombatCardMotion::capture_counts(game);}
 }
}
