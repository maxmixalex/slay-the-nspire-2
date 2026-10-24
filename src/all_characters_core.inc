// Shared operations deliberately keep the old model/serialized ID prefix.
static void all_characters_attack_damage(State&s,int t,int total,int unblocked,bool pet){
 if(t<0||t>=s.enemy_count)return;
 necro_attack_damage(s,t,total,pet);
 // BeatIntoShape counts blocked powered hits too, but its history predicate
 // requires the player as dealer. Osty remains eligible for ReaperForm.
 if(!pet)regent_after_damage(s,t,true);
 for(int at=MaxPostPlayPowers-1;at>=0;--at){unsigned actor=s.expansion.after_play_order[at];
  if(actor==(unsigned)PostPlayActor::SilentEnvenom&&!pet)silent_attack_damage(s,t,unblocked);
  else if(actor==(unsigned)PostPlayActor::RegentMonarchsGaze&&!pet)
   queue_extension(s,XOp::NTempStrength,(int)NTempStrength::MonarchsGaze*MaxEnemies+t,rpower(s,RPower::MonarchsGaze),true);
  else if(actor==(unsigned)PostPlayActor::NecroReaperForm&&total>0)
   queue_extension(s,XOp::NDoom,t,min(9999,total*npower(s,NPower::ReaperForm)),true);
 }
}
static void notify_card_generated(State&s,int id){
 if(id<0||id>=s.card_count)return;
 if(!combat_phase(s.phase))return;
 world_relic_hook(s,WorldRelicHook::AfterCardGenerated,id);
 s.regent.cards_created=(uint16_t)min(9999,s.regent.cards_created+1);
 bool status=card_type(s,s.cards[id],id)==CardType::Status;
 if(status)queue_extension(s,XOp::DRocketDiscount,0,0,true);enemy_card_generated(s,id);
 // Powers precede the card-model hook, each in the recorded acquisition order.
 for(int at=MaxPostPlayPowers-1;at>=0;--at){unsigned actor=s.expansion.after_play_order[at];
  if(status&&actor==(unsigned)PostPlayActor::DefectSmokestack)queue_extension(s,XOp::DGeneratedListener,(int)DPower::Smokestack,dpower(s,DPower::Smokestack),true);
  else if(status&&actor==(unsigned)PostPlayActor::DefectTrashToTreasure)queue_extension(s,XOp::DGeneratedListener,(int)DPower::TrashToTreasure,dpower(s,DPower::TrashToTreasure),true);
  else if(actor==(unsigned)PostPlayActor::RegentArsenal)enqueue(s,EffectKind::Strength,rpower(s,RPower::Arsenal),0,true);
  else if(actor==(unsigned)PostPlayActor::RegentPillarOfCreation)queue_extension(s,XOp::RUnpoweredBlock,rpower(s,RPower::PillarOfCreation),0,true);
 }
 world_card_entered(s,id);
}
void notify_deck_card_created(State&s,int id,int index){defect_deck_card_created(s,id,index);necro_deck_card_created(s,id,index);event_card_deck_created(s,id,index);}
void notify_deck_removed(State&s,int index){notify_deck_removing(s,index);defect_deck_removed(s,index);necro_deck_removed(s,index);event_card_deck_removed(s,index);}
void notify_deck_replaced(State&s,int index){defect_deck_replaced(s,index);necro_deck_replaced(s,index);event_card_deck_replaced(s,index);}
void notify_deck_copied(State&s,int to,int from){defect_deck_copied(s,to,from);necro_deck_copied(s,to,from);event_card_deck_copied(s,to,from);}
bool character_deck_description_value(const State&s,Card c,int index,const char*key,int&out){
 if(index<0||index>=s.deck_count||s.deck[index].kind!=c.kind)return false;
 if(c.kind==DefectGeneticAlgorithm&&defect_word(key,"Block")){out=min(9999,1+s.defect.genetic_deck[index]);event_card_offer_description_value(s,c,s.event_cards.deck[index],key,out);return true;}
 if(c.kind==NecroTheScythe&&defect_word(key,"Damage")){out=min(9999,13+s.necro.scythe_deck[index]);event_card_offer_description_value(s,c,s.event_cards.deck[index],key,out);return true;}
 if(event_card_deck_description_value(s,c,index,key,out))return true;
 return false;
}
static bool queue_common_token(State&s,Card c,int){
 if(c.kind!=Slimed&&c.kind!=Fuel&&c.kind!=Debris)return false;
 record(s,LogKind::Play,c.kind,c.upgraded);
 if(c.kind==Slimed)queue_extension(s,XOp::Draw,1);
 if(c.kind==Fuel)enqueue(s,EffectKind::Energy,c.upgraded?2:1);
 enqueue(s,EffectKind::Finish);return true;
}
static void all_characters_late(State&s){
 if(s.playing<0)return;
 regent_after_play_late(s,s.cards[s.playing]);necro_after_play_late(s,s.playing);
}
static void burn_hand(State&s){
 for(unsigned i=s.hand.count;i>0;--i)if(s.cards[s.hand.ids[i-1]].kind==Burn)queue_extension(s,XOp::PlayerDamage,2,0,true);
}
static void exhaust_ethereal_hand(State&s){
 // Capture only the hand that exists at hand-flush time. Exhaust-trigger draws
 // complete before ordinary discard but do not join this ethereal snapshot.
 for(unsigned i=s.hand.count;i>0;--i){int id=s.hand.ids[i-1];if(necro_ethereal(s,id)||enemy_card_ethereal(s,id)||(card_keywords(s,s.cards[id],id)&8)){enqueue(s,EffectKind::ExhaustCard,id,0,true);s.effects[0].aux=1;}}
}
