// Batch transformations remove every original before any replacement enters
// the permanent deck. Store the pending replacements across nested callbacks.
static bool w_transform_card(State& s,unsigned idx,reference::Random& rng,
 CardKind fixed,bool upgrade,unsigned carry,WorldRelicCardSnapshot& out){
 if(idx>=s.deck_count||!card_transformable(s,s.deck[idx],int(idx)))return false;
 Card card{};EventCardFacts facts{};
 if(!world_transform_card(s,rng,s.deck[idx],false,card,facts,fixed))return false;
 if(carry){
  if(s.deck[idx].upgraded&&card_upgradeable(card))card.upgraded=1;
  const auto& old=s.event_cards.deck[idx];
  if(old.enchantment!=EventEnchantment::None&&
     (carry==1||event_card_can_enchant(s,card,facts,old.enchantment)))
   if(!event_card_apply_enchantment(card,facts,old.enchantment,old.enchantment_amount))return false;
 }
 if(upgrade&&card_upgradeable(card))card.upgraded=1;
 out={uint16_t(card_token(card)),0,0,facts};return true;
}
static bool w_transform_batch(State& s,unsigned owner,const uint16_t* ids,unsigned count,unsigned mode){
 auto& w=s.world_relics;if(!count)return true;
 if(count>MaxDeck||w.transform_count){w_error(s);return false;}
 auto& rng=world_rng(s,mode==1?WorldRng::Niche:WorldRng::Transformations);
 // Pandora and Claws materialize the replacements before entering Transform.
 // Leafy Poultice asks Transform to generate them during its removal pass.
 for(unsigned i=0;i<count;i++){
  unsigned at=ids[i];if(at>=s.deck_count||!card_transformable(s,s.deck[at],int(at))){w_error(s);return false;}
  for(unsigned j=0;j<i;j++)if(ids[j]==at){w_error(s);return false;}
  w.transforms[i].original=uint16_t(s.deck[at].kind);
  if(mode&& !w_transform_card(s,at,rng,mode==2?CardKind(EventCardMaul):CardKind(CardKinds),false,mode==2?2:0,w.transforms[i].card)){w_error(s);return false;}
 }
 for(unsigned i=0;i<count;i++){
  unsigned at=ids[i];for(unsigned j=0;j<i;j++)if(ids[j]<ids[i])--at;
  auto& entry=w.transforms[i];entry.index=uint16_t(at);
  if(!mode&&!w_transform_card(s,at,rng,CardKinds,false,0,entry.card)){w_error(s);return false;}
  // The saved transform queue retains only the original model kind. Keep
  // its complete upgrade token in the observer's pending journal entry,
  // then finalize it after the normal deck modifiers have run on append.
  notify_history(s,RunHistoryEvent::CardTransformed,unsigned(card_token(s.deck[at])),entry.card.token,1);
  EventDeckSnapshot removed;if(!event_take_deck_card(s,int(at),removed)){w_error(s);return false;}
 }
 reference::comparison_sort(w.transforms,count,[](const WorldRelicTransformEntry& entry){return entry.index;});w.transform_count=uint16_t(count);w.transform_cursor=0;
 return w_enqueue(s,WorldRelicOp::TransformAppend,owner);
}
static void w_transform_append(State& s,unsigned owner){
 auto& w=s.world_relics;
 if(w.transform_cursor==w.transform_count){for(unsigned i=0;i<w.transform_count;i++)w.transforms[i]=WorldRelicTransformEntry{};w.transform_count=w.transform_cursor=0;return;}
 if(w.transform_cursor>w.transform_count||w.transform_count>MaxDeck){w_error(s);return;}
 const auto entry=w.transforms[w.transform_cursor++];
 if(!w_enqueue(s,WorldRelicOp::TransformAppend,owner))return;
 int at=w_restore_card(s,entry.card,false,false,false);if(at<0){w_error(s);return;}
 record(s,LogKind::Transform,entry.original,s.deck[at].kind);
 notify_history(s,RunHistoryEvent::CardTransformed,entry.original,unsigned(card_token(s.deck[at])),2);
}
