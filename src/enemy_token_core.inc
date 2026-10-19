static bool queue_enemy_token(State&s,Card c,int,int){
 if(c.kind<EventCardBeckon||c.kind>EventCardWither)return false;
 record(s,LogKind::Play,c.kind,c.upgraded);
 if(c.kind==EventCardFranticEscape)queue_extension(s,XOp::EnemyFranticEscape,s.playing);
 enqueue(s,EffectKind::Finish);return true;
}
static void enemy_token_turn_end(State&s,int id){
 if(id<0||id>=s.card_count)return;
 int amount=0,unblockable=0;switch(s.cards[id].kind){
 case EventCardBeckon:amount=6;unblockable=1;break;
 case EventCardInfection:amount=3;break;
 case EventCardToxic:amount=5;break;
 case EventCardWither:amount=min(9999,3+3*s.enemy_world.card_value[id]);break;
 default:return;
 }
 queue_extension(s,XOp::EnemyCardDamage,amount,unblockable|(id<<1),true);
}
static bool enemy_token_description_value(const State&s,Card c,int,int id,const char*key,int&out){
 if(c.kind==EventCardWither&&defect_word(key,"Damage")){
  out=3;if(id>=0&&id<s.card_count&&s.cards[id].kind==c.kind)out=min(9999,3+3*s.enemy_world.card_value[id]);return true;
 }
 if(c.kind==EventCardDisintegration&&defect_word(key,"DisintegrationPower")){
  out=6;if(id>=0&&id<s.card_count&&s.cards[id].kind==c.kind&&s.enemy_world.card_value[id])out=s.enemy_world.card_value[id];return true;
 }
 return false;
}
