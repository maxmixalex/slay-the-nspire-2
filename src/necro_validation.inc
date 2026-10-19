static bool valid_necro_state(const State&s){
 const auto&v=s.necro;
 if(v.power_count>NPowerCount||v.drawn_turn>9999||v.ethereal_played>9999||v.osty_attacks_turn>9999||v.attacks_started_turn>9999||v.creature_deaths>9999||v.player_doom<0||v.player_doom>9999||v.doom_applied_turn>1||v.osty_was_alive>1||v.grimoire_pending>9999)return false;
 if(v.enemy_dead_mask>=(1u<<s.enemy_count))return false;
 bool seen[NPowerCount]{};
 for(unsigned i=0;i<v.power_count;i++){unsigned p=v.power_order[i];if(p>=NPowerCount||seen[p])return false;seen[p]=true;}
 for(unsigned i=v.power_count;i<NPowerCount;i++)if(v.power_order[i])return false;
 for(int p=0;p<NPowerCount;p++)if(v.powers[p]<0||v.powers[p]>9999||seen[p]!=(v.powers[p]>0))return false;
 for(unsigned id=0;id<MaxCards;id++){
  if(v.cost_delta[id]<-9999||v.cost_delta[id]>9999||v.turn_origin[id]<-9999||v.turn_origin[id]>9999||v.combat_origin[id]<-9999||v.combat_origin[id]>9999||v.ethereal[id]>1||v.fetch_played[id]>1||v.flatten_zero[id]>1||v.flatten_origin[id]<-9999||v.flatten_origin[id]>9999||v.scythe_value[id]<0||v.scythe_value[id]>9986||v.deck_origin[id]>s.deck_count)return false;
  if(id>=s.card_count){if(v.cost_delta[id]||v.turn_origin[id]||v.combat_origin[id]||v.ethereal[id]||v.fetch_played[id]||v.flatten_zero[id]||v.flatten_origin[id]||v.scythe_value[id]||v.deck_origin[id])return false;continue;}
  if(v.fetch_played[id]&&s.cards[id].kind!=NecroFetch)return false;
  if((v.flatten_zero[id]||v.flatten_origin[id])&&s.cards[id].kind!=NecroFlatten)return false;
  if(v.scythe_value[id]&&s.cards[id].kind!=NecroTheScythe)return false;
  if(v.deck_origin[id]&&(s.cards[id].kind!=NecroTheScythe||s.deck[v.deck_origin[id]-1].kind!=NecroTheScythe))return false;
  // Transfigure can add combat cost to any non-X playable card, while the two
  // native cost reductions can only belong to their own models.
  if(v.cost_delta[id]<0&&s.cards[id].kind!=NecroBansheesCry&&s.cards[id].kind!=NecroMelancholy)return false;
 }
 for(unsigned i=0;i<MaxDeck;i++)if(v.scythe_deck[i]>9986||(v.scythe_deck[i]&&(i>=s.deck_count||s.deck[i].kind!=NecroTheScythe)))return false;
 for(int t=0;t<MaxEnemies;t++){
  if(v.debilitate[t]<0||v.debilitate[t]>9999||v.hang[t]<0||v.hang[t]>9999||v.oblivion[t]<0||v.oblivion[t]>9999||v.sic_em[t]<0||v.sic_em[t]>9999||v.debuff_count[t]>NDebuffCount)return false;
  if(t>=s.enemy_count&&(v.debilitate[t]||v.hang[t]||v.oblivion[t]||v.sic_em[t]||v.debuff_count[t]))return false;
  int restored=0;for(int p=0;p<NTempStrengthCount;p++){
   if(v.temp_strength[t][p]<0||v.temp_strength[t][p]>9999||(t>=s.enemy_count&&v.temp_strength[t][p]))return false;
   restored+=v.temp_strength[t][p];
  }
  if(restored>s.expansion.restore_strength[t])return false;
  bool order_seen[NDebuffCount]{};
  for(unsigned i=0;i<NDebuffCount;i++){
   unsigned n=v.debuff_order[t][i];if(i>=v.debuff_count[t]){if(n)return false;continue;}
   if(!n||n>NDebuffCount||order_seen[n-1])return false;order_seen[n-1]=true;
  }
 }
 for(unsigned f=0;f<=MaxAutoFrames;f++){
  if(v.attack_total[f]<0||v.attack_total[f]>9999||v.play_index[f]>9999||v.resource_energy[f]>9999||v.first_attack[f]>1)return false;
  bool pending=v.attack_total[f]||v.play_index[f]||v.resource_energy[f]||v.first_attack[f];
  for(int t=0;t<MaxEnemies;t++){if(v.oblivion_before[f][t]<0||v.oblivion_before[f][t]>9999)return false;pending=pending||v.oblivion_before[f][t];}
  if(pending&&(f>s.expansion.depth||!combat_phase(s.phase)))return false;
 }
 if(s.phase==Phase::ExtendedChoice&&s.expansion.choice>=XChoice::NCleanse&&s.expansion.choice<=XChoice::NTransfigure){
  const auto&x=s.expansion;XChoice k=x.choice;
  if(!x.choice_min||x.choice_min!=x.choice_max||x.choice_min>x.choices.count||x.choice_free)return false;
  if(k!=XChoice::NDredge&&x.choice_min!=1)return false;
  if(k==XChoice::NDredge&&(x.choice_min>3||x.choice_min+s.hand.count>MaxHand))return false;
  Zone z=k==XChoice::NCleanse||k==XChoice::NSeance?Zone::Draw:k==XChoice::NDredge?Zone::Discard:Zone::Hand;
  for(unsigned i=0;i<x.choices.count;i++){
   int id=x.choices.ids[i];if(id<0||id>=s.card_count||s.zones[id]!=z)return false;
   if(k==XChoice::NEthereal&&necro_ethereal(s,id))return false;
   if(k==XChoice::NRetain&&(card_retains(s.cards[id])||s.silent.retain[id]))return false;
  }
 }
 return true;
}
static bool valid_necro_effect(const State&s,const Effect&e){
 int a=e.value,b=e.aux;bool instance=a>=0&&a<s.card_count;
 switch((XOp)e.target){
 case XOp::NPower:return a>=0&&a<NPowerCount&&b>0&&b<=9999;
 case XOp::NSummon:return a>=0&&a<=9999&&b>=0&&b<=9999;
 case XOp::NHealOsty:case XOp::NRandomHpLoss:case XOp::NAllHpLoss:case XOp::NCallVoid:case XOp::NSentry:return a>=0&&a<=9999&&b==0;
 case XOp::NKillOsty:case XOp::NCheckDoom:case XOp::NChooseCleanse:case XOp::NChooseEthereal:case XOp::NChooseRetain:case XOp::NChooseSeance:case XOp::NChooseTransfigure:case XOp::NEidolon:return a==0&&b==0;
 case XOp::NFinishOsty:return a==1&&b==0;
 case XOp::NDoom:return a>=-1&&a<s.enemy_count&&b>=0&&b<=9999;
 case XOp::NDoomFromDamage:return a>=0&&a<s.enemy_count&&b==0;
 case XOp::NEnemyPower:return a>=0&&a<5*MaxEnemies&&a%MaxEnemies<s.enemy_count&&b>=0&&b<=9999&&(a/MaxEnemies!=1||b==0);
 case XOp::NApplyCopiedDebuff:return a>=0&&a<NDebuffCount*MaxEnemies&&a%MaxEnemies<s.enemy_count&&b>=-9999&&b<=9999&&b!=0&&(a/MaxEnemies==(int)NDebuff::Strength||b>0);
 case XOp::NTempStrength:return a>=0&&a<NTempStrengthCount*MaxEnemies&&a%MaxEnemies<s.enemy_count&&b>0&&b<=9999;
 case XOp::NEnemyHpLoss:case XOp::NDebuffDamage:return a>=0&&a<=9999&&b>=0&&b<s.enemy_count;
 case XOp::NMakeSoul:return a>=0&&a<=9999&&b>=0&&b<=5;
 case XOp::NChooseDredge:return a>=1&&a<=3&&b==0;
 case XOp::NUpgradeDiscard:return a>=2&&a<=3&&b==0;
 case XOp::NMisery:return a>=0&&a<s.enemy_count&&b==0;
 case XOp::NFetchDraw:return instance&&s.cards[a].kind==NecroFetch&&b==0;
 case XOp::NRepeatedHit:return a>=0&&a<=9999&&b>=0&&b<=9999;
 case XOp::NDeathDoor:return a>=6&&a<=7&&b==0;
 case XOp::NScytheGrow:return instance&&s.cards[a].kind==NecroTheScythe&&(b==5||b==7);
 case XOp::NUndeathClone:return instance&&s.cards[a].kind==NecroUndeath&&b==0;
 case XOp::NPowerTurn:return (a==(int)NPower::Countdown||a==(int)NPower::Neurosurge)&&b==0;
 default:return false;
 }
}
