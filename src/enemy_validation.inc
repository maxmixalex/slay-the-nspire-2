static bool enemy_valid_rng(const reference::Random&r,bool required){
 if(!r.good)return !required;
 return (r.words[0]|r.words[1]|r.words[2]|r.words[3])!=0;
}
static bool enemy_valid_power_order(const uint8_t*order,unsigned count){
 if(count>EnemyPowerCount)return false;
 bool seen[EnemyPowerCount]{};
 for(unsigned i=0;i<count;++i){if(order[i]>=EnemyPowerCount||seen[order[i]])return false;seen[order[i]]=true;}
 return true;
}
static bool valid_enemy_state(const State&s){
 const auto&w=s.enemy_world;
 if(w.active>1||w.ascension>10||w.side>1||w.timed_out>1||w.facing>1||
    unsigned(w.encounter)>=EncounterKinds||w.slot_limit>MaxEnemies||w.turn_count>MaxEnemies||
    (w.actor!=255&&w.actor>=s.enemy_count)||w.choice_stage>16||
    (w.choice_enemy!=255&&w.choice_enemy>=s.enemy_count)||!w.next_creature_id)return false;
 if(w.active&&(!w.slot_limit||w.encounter==EncounterId::None||!s.enemy_count||!enemy_valid_rng(w.composition_rng,true)))return false;
 if(!enemy_valid_power_order(w.player_power_order,w.player_power_count))return false;
 bool named_slots[MaxEnemies]{},turn_slots[MaxEnemies]{};
 for(unsigned i=0;i<w.turn_count;++i){
  unsigned t=w.turn_order[i];if(t>=s.enemy_count||turn_slots[t]||!w.turn_ids[i]||w.turn_ids[i]>=w.next_creature_id)return false;turn_slots[t]=true;
 }
 for(unsigned i=0;i<MaxEnemies;++i){const auto&m=w.models[i];
  if(unsigned(m.kind)>=EnemyKinds||m.node>=MaxEnemyMoves||m.previous>=MaxEnemyMoves||m.resume_node>=MaxEnemyMoves||
     m.history_count>MaxEnemyMoveHistory||m.spawned_this_side>1||m.slot>=MaxEnemies||
     (m.owner!=255&&m.owner>=MaxEnemies)||!enemy_valid_power_order(m.power_order,m.power_count))return false;
  for(unsigned h=0;h<m.history_count;++h)if(m.history[h]>=MaxEnemyMoves)return false;
  if(m.flags&EnemyPresent){
   if(i>=s.enemy_count||!m.combat_id||m.combat_id>=w.next_creature_id||!enemy_valid_rng(m.rng,true))return false;
   for(unsigned j=0;j<i;++j)if((w.models[j].flags&EnemyPresent)&&w.models[j].combat_id==m.combat_id)return false;
   if(!(m.flags&EnemyRemoved)){if(m.slot>=w.slot_limit||named_slots[m.slot])return false;named_slots[m.slot]=true;}
   if((m.flags&EnemyDown)&&s.enemies[i].hp!=0)return false;
  }
  if(m.stolen_gold<0||m.stolen_gold>9999||m.attack_unblocked_hits<0||m.attack_total_damage<0||m.attack_vigor<0)return false;
  if(m.captured_card>s.card_count||m.captured_deck.card.upgraded>1||!event_card_facts_valid(m.captured_deck.card,m.captured_deck.facts))return false;
  if(m.captured_card&&(!(m.flags&EnemyPresent)||(m.flags&EnemyRemoved)||s.zones[m.captured_card-1]!=Zone::Limbo||m.kind!=EnemyId::ThievingHopper))return false;
  for(auto value:m.data)if(value<-9999||value>9999)return false;
  const int visual_max=m.kind==EnemyId::SewerClam?7:3;
  if(m.data[8]<0||m.data[8]>visual_max||m.data[9]<0||m.data[9]>visual_max+1)return false;
  for(unsigned p=0;p<EnemyPowerCount;++p){
   if(m.power[p]<-9999||m.power[p]>9999||m.power_aux[p]<-9999||m.power_aux[p]>9999||m.duration_skip[p]>1||m.source[p]>MaxEnemies)return false;
  }
 }
 for(unsigned p=0;p<EnemyPowerCount;++p)if(w.player_power[p]<-9999||w.player_power[p]>9999||w.player_aux[p]<-9999||w.player_aux[p]>9999||w.player_duration_skip[p]>1||w.player_source[p]>MaxEnemies)return false;
 unsigned models=0;for(auto v:w.spawned_models){if(v>1)return false;models+=v;}
 if(models!=w.spawned_model_count||w.escaped_count>65534||w.bound_played>1||w.used_types>7)return false;
 if(w.returned_gold_count>MaxEnemyExtraRewards)return false;
 for(unsigned i=0;i<MaxEnemyExtraRewards;++i){if(w.returned_reward_kind[i]>2||w.returned_gold[i]>9999)return false;
  const auto&snap=w.returned_cards[i];
  if(!event_card_facts_valid(snap.card,snap.facts)||snap.card.upgraded>1||snap.genetic>9999||snap.scythe>9999)return false;
  if(i<w.returned_gold_count){if(w.returned_reward_kind[i]==0&&!w.returned_gold[i])return false;
   if(w.returned_reward_kind[i]==1&&w.returned_gold[i]!=3)return false;
   if(w.returned_reward_kind[i]==2&&w.returned_gold[i]!=card_token(snap.card))return false;
  }
  else if(w.returned_gold[i]||w.returned_reward_kind[i])return false;
  if(w.returned_reward_kind[i]!=2&&(snap.card.kind||snap.card.upgraded||snap.facts.enchantment!=EventEnchantment::None||snap.facts.science_type||snap.facts.rider||snap.facts.enchantment_amount||snap.facts.energy_offset||snap.facts.progress||snap.genetic||snap.scythe))return false;
 }
 for(unsigned id=0;id<MaxCards;++id){
  if(w.affliction[id]>=unsigned(EnemyAffliction::Count)||w.dampen_restore[id]>1||w.affliction_amount[id]<0||w.affliction_amount[id]>9999||w.card_value[id]<0||w.card_value[id]>9999)return false;
  if((!w.affliction[id])!=(!w.affliction_amount[id]))return false;
  if(id>=s.card_count&&(w.affliction[id]||w.dampen_restore[id]||w.card_value[id]))return false;
 }
 for(unsigned t=0;t<MaxEnemies;++t)if(w.turn_damage[t]<0||w.turn_attacks[t]<0)return false;
 return true;
}
static bool valid_enemy_effect(const State&s,const Effect&e){
 const auto op=(XOp)e.target;const int a=e.value,b=e.aux;
 const auto actor=[&](int t){return t>=0&&t<s.enemy_count;};
 const auto target=[&](int t){return t==-1||actor(t);};
 switch(op){
 case XOp::EnemyMove:case XOp::EnemyMoveFinish:return actor(a)&&b>=0&&b<MaxEnemyMoves;
 case XOp::EnemyAttackStart:return actor(a)&&b>=1&&b<=999;
 case XOp::EnemyHit:return a>=0&&a<=9999&&b>=8&&b<8000&&actor(b%8);
 case XOp::EnemyAttackEnd:return actor(a)&&b==0;
 case XOp::EnemyGainBlock:return a>=0&&a<=9999&&b>=0&&b<16&&actor(b%8);
 case XOp::EnemyApplyPower:{if(a<-9999||a>9999||b<0)return false;int p=b%80,t=(b/80)%9-1,source=b/720-1;return p<EnemyPowerCount&&target(t)&&(source==-1||actor(source));}
 case XOp::EnemyRemovePower:return target(a)&&b>=0&&b<EnemyPowerCount;
 case XOp::EnemyHeal:return a>=0&&a<=9999&&target(b);
 case XOp::EnemyPlayerDamage:case XOp::EnemyLoseMaxHp:return a>=0&&a<=9999&&b>=0&&b<4;
 case XOp::EnemyGenerateCard:return a>=0&&a<CardKinds&&b>=0&&b/1024<=4&&b%1024<=MaxCards;
 case XOp::EnemySpawn:return a>0&&a<EnemyKinds&&b>=0&&b<72;
 case XOp::EnemyEscape:case XOp::EnemyDie:return actor(a)&&b>=0&&b<=3;
 case XOp::EnemyTransform:return actor(a)&&b>0&&b<EnemyKinds;
 case XOp::EnemySideHook:return a>=0&&a<=2&&b>=0&&b<=1&&(a<2||b==0);
 case XOp::EnemyPowerHook:return target(a)&&b>=0&&b<EnemyPowerCount*16;
 case XOp::EnemyCardHook:return a>=0&&a<s.card_count&&b>=0&&b<=16;
 case XOp::EnemyKnowledgeChoice:return actor(a)&&b>=0&&b<=16;
 case XOp::EnemyFranticEscape:case XOp::EnemyWither:return a>=0&&a<s.card_count&&b==0;
 case XOp::EnemyCardDamage:return a>=0&&a<=9999&&b>=0&&(b>>1)<s.card_count;
 case XOp::EnemyAwaken:return actor(a)&&b>=0&&b<MaxEnemyMoves;
 case XOp::EnemyActionSpecial:return actor(a)&&b>=0&&b<=255;
 default:return false;
 }
}
