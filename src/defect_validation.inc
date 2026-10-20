static bool valid_defect_state(const State&s){
 const auto&v=s.defect;
 if(v.power_count>DPowerCount||v.lightning_channeled>9999||v.physical_started>9999||v.cards_finished>9999||v.energy_spent_turn>9999||v.zero_attacks_started>9999||v.feral_used>9999||v.status_drawn>1||v.temporary_focus< -9999||v.temporary_focus>9999)return false;
 bool seen[DPowerCount]{};
 for(unsigned i=0;i<v.power_count;i++){unsigned p=v.power_order[i];if(p>=DPowerCount||seen[p])return false;seen[p]=true;}
 for(unsigned i=v.power_count;i<DPowerCount;i++)if(v.power_order[i])return false;
 for(int p=0;p<DPowerCount;p++)if(v.powers[p]<0||v.powers[p]>9999||seen[p]!=(v.powers[p]>0))return false;
 if(v.temporary_focus!=max(-9999,min(9999,dpower(s,DPower::FocusedStrikeFocus)+dpower(s,DPower::HotfixFocus)-dpower(s,DPower::HyperbeamFocus)+dpower(s,DPower::SynchronizeFocus))))return false;
 for(int i=0;i<MaxOrbs;i++){
  unsigned kind=v.orb_kind[i];if(kind>=(unsigned)DOrb::Count)return false;
  if(i>=s.expansion.orb_count){if(kind)return false;continue;}
  if(kind!=(unsigned)DOrb::Dark&&kind!=(unsigned)DOrb::Glass&&s.expansion.dark[i])return false;
  if(kind==(unsigned)DOrb::Glass&&s.expansion.dark[i]>4)return false;
 }
 for(unsigned id=0;id<MaxCards;id++){
  if(v.value[id]<0||v.value[id]>9999||v.cost_delta[id]< -9999||v.cost_delta[id]>9999||v.turn_origin[id]< -9999||v.turn_origin[id]>9999||v.combat_origin[id]< -9999||v.combat_origin[id]>9999||v.deck_origin[id]>s.deck_count)return false;
  if(id>=s.card_count){if(v.value[id]||v.cost_delta[id]||v.turn_origin[id]||v.combat_origin[id]||v.deck_origin[id])return false;continue;}
  unsigned kind=s.cards[id].kind;
  if(v.value[id]&&kind!=DefectClaw&&kind!=DefectGeneticAlgorithm)return false;
  if((v.cost_delta[id]||v.turn_origin[id]||v.combat_origin[id])&&kind!=DefectModded&&kind!=DefectRocketPunch)return false;
  if(v.deck_origin[id]&&(kind!=DefectGeneticAlgorithm||s.deck[v.deck_origin[id]-1].kind!=DefectGeneticAlgorithm))return false;
 }
 for(unsigned i=0;i<MaxDeck;i++)if(v.genetic_deck[i]>9998||(v.genetic_deck[i]&&(i>=s.deck_count||s.deck[i].kind!=DefectGeneticAlgorithm)))return false;
 for(unsigned f=0;f<=MaxAutoFrames;f++){
  if(v.storm_before[f]<0||v.storm_before[f]>9999||v.subroutine_before[f]<0||v.subroutine_before[f]>9999||v.spent[f]>999||v.scrape_waiting[f]>1||v.scrape_count[f]>MaxHand)return false;
  bool pending=v.storm_before[f]||v.subroutine_before[f]||v.spent[f]||v.scrape_waiting[f]||v.scrape_count[f];
  for(unsigned i=0;i<MaxHand;i++){
   unsigned n=v.scrape_drawn[f][i];if(i>=v.scrape_count[f]){if(n)return false;continue;}
   if(!n||n>s.card_count)return false; // A reshuffle can draw one instance twice.
  }
  if(pending&&(f>s.expansion.depth||!saved_combat_phase(s)))return false;
 }
 return true;
}
static bool valid_defect_effect(const State&s,const Effect&e){
 const int a=e.value,b=e.aux;bool instance=a>=0&&a<s.card_count;
 switch((XOp)e.target){
 case XOp::DPower:return a>=0&&a<DPowerCount&&b>0&&b<=9999;
 case XOp::DChannel:return a>=0&&a<=(int)DOrb::Count&&b>=0&&b<=9999;
 case XOp::DChannelAppend:return a>=0&&a<(int)DOrb::Count&&b==0;
 case XOp::DOrbPassive:return a>=0&&a<MaxOrbs&&b>=-1&&b<s.enemy_count;
 case XOp::DEvoke:return a>=-1&&a<MaxOrbs&&b>=0&&b<=9999;
 case XOp::DSlots:return a>=-MaxOrbs&&a<=MaxOrbs&&b==0;
 case XOp::DFocus:return a>=-9999&&a<=9999&&b>=0&&b<=4&&(b!=3||a<=0)&&(!b||b==3||a>=0);
 case XOp::DPowerTurn:return a>=0&&a<DPowerCount&&(a==(int)DPower::BiasedCognition||a==(int)DPower::Coolant||a==(int)DPower::Feral||a==(int)DPower::LightningRod||a==(int)DPower::Spinner||a==(int)DPower::CreativeAi||a==(int)DPower::Loop||a==(int)DPower::ConsumingShadow||defect_temporary_focus_power((DPower)a))&&b>=0&&b<=9999;
 case XOp::DGeneratedListener:return (a==(int)DPower::Smokestack||a==(int)DPower::TrashToTreasure)&&b>0&&b<=9999;
 case XOp::DAdaptiveClone:return instance&&s.cards[a].kind==DefectAdaptiveStrike&&b==0;
 case XOp::DClaw:return a>=2&&a<=3&&b==0;
 case XOp::DCompact:return a>=0&&a<=1&&b==0;
 case XOp::DGenetic:return instance&&s.cards[a].kind==DefectGeneticAlgorithm&&b>=3&&b<=4;
 case XOp::DModded:return instance&&s.cards[a].kind==DefectModded&&b==0;
 case XOp::DMomentum:return instance&&s.cards[a].kind==DefectMomentumStrike&&b==0;
 case XOp::DScrape:return a>=0&&a<=5&&b>=0&&b<=1;
 case XOp::DFlak:return a>=0&&a<=9999&&b==0;
 case XOp::DExhaustStatus:return instance&&definitions[s.cards[a].kind].type==CardType::Status&&b==0;
 case XOp::DHits:return a>=0&&a<=9999&&b>=0&&b<=9999;
 case XOp::DFtl:return a>=3&&a<=4&&b==0;
 case XOp::DTesla:return a>=0&&a<s.enemy_count&&b>=1&&b<=2;
 case XOp::DDarkness:case XOp::DSynchronize:return a>=1&&a<=2&&b==0;
 case XOp::DStatus:return a>=0&&a<=MaxCards&&b>=0&&b<CardKinds&&definitions[b].type==CardType::Status;
 case XOp::DEvokeAll:case XOp::DRocketDiscount:case XOp::DAllForOne:case XOp::DScrapeDiscard:case XOp::DUproar:case XOp::DVoltaic:case XOp::DReboot:case XOp::DOrbsStart:case XOp::DDoubleEnergy:case XOp::DCompileDraw:return a==0&&b==0;
 default:return false;
 }
}
