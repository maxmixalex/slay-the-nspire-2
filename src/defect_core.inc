// Full single-player Defect rules for v0.111.0 / 41cef1ea.
static int dpower(const State&s,DPower p){return s.defect.powers[(unsigned)p];}
static bool defect_temporary_focus_power(DPower p){return p>=DPower::FocusedStrikeFocus&&p<=DPower::SynchronizeFocus;}
static void defect_update_temporary_focus(State&s){s.defect.temporary_focus=(int16_t)max(-9999,min(9999,dpower(s,DPower::FocusedStrikeFocus)+dpower(s,DPower::HotfixFocus)-dpower(s,DPower::HyperbeamFocus)+dpower(s,DPower::SynchronizeFocus)));}
static unsigned defect_actor(DPower p){
 return p==DPower::Storm?(unsigned)PostPlayActor::DefectStorm:
        p==DPower::Subroutine?(unsigned)PostPlayActor::DefectSubroutine:
        p==DPower::Smokestack?(unsigned)PostPlayActor::DefectSmokestack:
        p==DPower::TrashToTreasure?(unsigned)PostPlayActor::DefectTrashToTreasure:0;
}
static void defect_gain_power(State&s,DPower p,int amount){
 auto&v=s.defect;unsigned k=(unsigned)p;int old=v.powers[k],value=max(0,min(9999,old+amount));
 if(!old&&value){
  if(v.power_count==DPowerCount){fail(s);return;}v.power_order[v.power_count++]=(uint8_t)k;
  if(p==DPower::Feral)v.feral_used=v.zero_attacks_started;
 }else if(old&&!value){
  for(unsigned i=0;i<v.power_count;i++)if(v.power_order[i]==k){
   for(unsigned j=i+1;j<v.power_count;j++)v.power_order[j-1]=v.power_order[j];v.power_order[--v.power_count]=0;break;
  }
 }
 v.powers[k]=(int16_t)value;unsigned actor=defect_actor(p);
 if(actor){if(!old&&value)after_play_register(s,actor);else if(old&&!value)after_play_unregister(s,actor);}
}
static void defect_card_reset(State&s,int id){
 if(id<0||id>=MaxCards)return;auto&v=s.defect;
 v.value[id]=v.cost_delta[id]=v.turn_origin[id]=v.combat_origin[id]=0;v.deck_origin[id]=0;
}
static void defect_card_created(State&s,int id){defect_card_reset(s,id);}
static void defect_card_copied(State&s,int to,int from){
 if(to<0||from<0||to>=s.card_count||from>=s.card_count)return;auto&v=s.defect;
 v.value[to]=v.value[from];v.cost_delta[to]=v.cost_delta[from];v.turn_origin[to]=v.turn_origin[from];v.combat_origin[to]=v.combat_origin[from];
 v.deck_origin[to]=0; // CreateClone has no permanent deck version.
}
static void defect_deck_card_created(State&s,int id,int deck){
 if(id<0||id>=s.card_count||deck<0||deck>=s.deck_count)return;
 if(s.cards[id].kind==DefectGeneticAlgorithm){s.defect.value[id]=(int16_t)s.defect.genetic_deck[deck];s.defect.deck_origin[id]=(uint16_t)(deck+1);}
}
static void defect_deck_removed(State&s,int at){
 if(at<0||at>=MaxDeck)return;auto&v=s.defect;
 for(int i=at+1;i<MaxDeck;i++)v.genetic_deck[i-1]=v.genetic_deck[i];v.genetic_deck[MaxDeck-1]=0;
 for(auto&origin:v.deck_origin){if(origin==(unsigned)at+1)origin=0;else if(origin>(unsigned)at+1)--origin;}
}
static void defect_deck_replaced(State&s,int at){
 if(at<0||at>=MaxDeck)return;s.defect.genetic_deck[at]=0;
 for(auto&origin:s.defect.deck_origin)if(origin==(unsigned)at+1)origin=0;
}
static void defect_deck_copied(State&s,int to,int from){
 if(to>=0&&from>=0&&to<MaxDeck&&from<MaxDeck)s.defect.genetic_deck[to]=s.defect.genetic_deck[from];
}
static void defect_combat_start(State&s){
 uint16_t permanent[MaxDeck];for(int i=0;i<MaxDeck;i++)permanent[i]=s.defect.genetic_deck[i];
 s.defect=DefectState{};for(int i=0;i<MaxDeck;i++)s.defect.genetic_deck[i]=permanent[i];
 if(s.character==Character::Defect)s.expansion.orb_slots=3;
}
static void defect_temporary_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.turn_cost[id])s.defect.turn_origin[id]=s.defect.cost_delta[id];
}
static void defect_combat_cost_set(State&s,int id){
 if(id>=0&&id<s.card_count&&s.expansion.combat_cost[id])s.defect.combat_origin[id]=s.defect.cost_delta[id];
}
static int defect_cost(const State&s,int id,int current){
 if(id<0||id>=s.card_count||current<0)return current;const auto&v=s.defect;
 int origin=s.expansion.turn_cost[id]?v.turn_origin[id]:s.expansion.combat_cost[id]?v.combat_origin[id]:0;
 current=max(0,min(999,current+v.cost_delta[id]-origin));
 if(dpower(s,DPower::FreePower)&&card_type(s,s.cards[id],id)==CardType::Power&&
    (s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo))return 0;
 return current;
}
static int defect_base_damage(const State&s,Card c,int,int instance,int current){
 if(c.kind==DefectClaw&&instance>=0&&instance<s.card_count)current+=s.defect.value[instance];return min(9999,current);
}
static int defect_base_block(const State&s,Card c,int instance,int current){
 if(c.kind==DefectGeneticAlgorithm&&instance>=0&&instance<s.card_count)current+=s.defect.value[instance];return min(9999,current);
}
static int defect_extra_replays(State&s,Card c){
 int n=s.defect.physical_started<dpower(s,DPower::EchoForm)?1:0;
 if(card_type(s,c)==CardType::Power&&dpower(s,DPower::SignalBoost)){defect_gain_power(s,DPower::SignalBoost,-1);++n;}
 return n;
}
static void defect_play_start(State&s,int id,int resource_energy,bool,int actual_spent){
 auto&v=s.defect;unsigned d=s.expansion.depth;v.physical_started=(uint16_t)min(9999,v.physical_started+1);
 v.spent[d]=(uint16_t)max(0,min(999,resource_energy));v.energy_spent_turn=(uint16_t)min(9999,v.energy_spent_turn+max(0,actual_spent));
 (void)id;
}
static void defect_before_play(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&v=s.defect;unsigned d=s.expansion.depth;
 bool power=card_type(s,s.cards[id],id)==CardType::Power;
 v.storm_before[d]=(int16_t)(power?dpower(s,DPower::Storm):0);v.subroutine_before[d]=(int16_t)(power?dpower(s,DPower::Subroutine):0);
 if(power&&dpower(s,DPower::FreePower))defect_gain_power(s,DPower::FreePower,-1);
 if(card_type(s,s.cards[id],id)==CardType::Attack&&!v.spent[d])v.zero_attacks_started=(uint16_t)min(9999,v.zero_attacks_started+1);
}
static void defect_after_play(State&s,int){s.defect.cards_finished=(uint16_t)min(9999,s.defect.cards_finished+1);}
static void defect_after_play_listener(State&s,unsigned actor){
 unsigned d=s.expansion.depth;auto&v=s.defect;
 if(actor==(unsigned)PostPlayActor::DefectStorm){int n=v.storm_before[d];v.storm_before[d]=0;if(n)queue_extension(s,XOp::DChannel,(int)DOrb::Lightning,n,true);}
 if(actor==(unsigned)PostPlayActor::DefectSubroutine){int n=v.subroutine_before[d];v.subroutine_before[d]=0;if(n)enqueue(s,EffectKind::Energy,n,0,true);}
}
static bool defect_returns_to_hand(State&s,int id){
 if(id<0||id>=s.card_count||card_type(s,s.cards[id],id)!=CardType::Attack||s.defect.spent[s.expansion.depth]||
    s.defect.feral_used>=dpower(s,DPower::Feral))return false;
 ++s.defect.feral_used;return true;
}
static void defect_after_play_group(State&s,int id){
 auto&v=s.defect;unsigned d=s.expansion.depth;
 if(id>=0&&id<s.card_count&&s.cards[id].kind==DefectRocketPunch)v.cost_delta[id]=v.turn_origin[id]=v.combat_origin[id]=0;
 v.spent[d]=0;v.storm_before[d]=v.subroutine_before[d]=0;v.scrape_waiting[d]=v.scrape_count[d]=0;
 for(auto&n:v.scrape_drawn[d])n=0;
}
static void defect_card_drawn(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&v=s.defect;unsigned d=s.expansion.depth;
 if(v.scrape_waiting[d]){v.scrape_waiting[d]=0;if(v.scrape_count[d]<MaxHand)v.scrape_drawn[d][v.scrape_count[d]++]=(uint16_t)(id+1);}
 if(card_type(s,s.cards[id],id)==CardType::Status){
  if(!v.status_drawn&&dpower(s,DPower::Iteration))queue_extension(s,XOp::Draw,dpower(s,DPower::Iteration),0,true);
  v.status_drawn=1;
 }
}
static void defect_card_generated(State&s,int id,bool player_created){
 if(!player_created||id<0||id>=s.card_count||card_type(s,s.cards[id],id)!=CardType::Status)return;
 // Player powers run before card-model listeners. Reverse insertion preserves order.
 queue_extension(s,XOp::DRocketDiscount,0,0,true);
 auto&v=s.defect;for(int i=v.power_count-1;i>=0;i--){DPower p=(DPower)v.power_order[i];
  if(p==DPower::Smokestack||p==DPower::TrashToTreasure)queue_extension(s,XOp::DGeneratedListener,(int)p,dpower(s,p),true);
 }
}
static void defect_attack_killed(State&s,int){
 if(s.playing>=0&&s.playing<s.card_count&&s.cards[s.playing].kind==DefectSunder)enqueue(s,EffectKind::Energy,3,0,true);
}
static int defect_orb_unique(const State&s){
 unsigned seen=0;int n=0;for(unsigned i=0;i<s.expansion.orb_count;i++){unsigned b=1u<<s.defect.orb_kind[i];if(!(seen&b)){seen|=b;++n;}}return n;
}
static bool defect_has_orb(const State&s,DOrb kind){for(unsigned i=0;i<s.expansion.orb_count;i++)if(s.defect.orb_kind[i]==(unsigned)kind)return true;return false;}
static void defect_remove_orb(State&s,unsigned at){
 auto&x=s.expansion;auto&v=s.defect;if(at>=x.orb_count)return;
 for(unsigned i=at+1;i<x.orb_count;i++){x.dark[i-1]=x.dark[i];v.orb_kind[i-1]=v.orb_kind[i];}
 --x.orb_count;x.dark[x.orb_count]=0;v.orb_kind[x.orb_count]=0;
}
static void defect_add_orb_slots(State&s,int n){
 auto&x=s.expansion;x.orb_slots=(uint8_t)max(0,min(MaxOrbs,x.orb_slots+n));
 // RemoveCapacity drops the newest orbs without evoking them.
 while(x.orb_count>x.orb_slots)defect_remove_orb(s,x.orb_count-1);
}
static int defect_random_enemy(State&s){int ids[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)ids[n++]=i;return n?ids[random_below(s.rng_cards,n)]:-1;}
static int defect_enemy_count(const State&s){int n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)++n;return n;}
static void defect_orb_fire(State&s,DOrb kind,int raw,bool evoke,int target=-1){
 int focus=power_value(s,XPower::Focus),n=0;
 if(kind==DOrb::Dark){
  if(!evoke)return;target=-1;
  for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp&&(target<0||s.enemies[i].hp<s.enemies[target].hp))target=i;
  if(target>=0)damage_unpowered(s,target,raw);return;
 }
 if(kind==DOrb::Lightning){
  n=max(0,(evoke?8:3)+focus);if(target<0)target=defect_random_enemy(s);
  if(target>=0&&target<s.enemy_count&&s.enemies[target].hp){
   damage_unpowered(s,target,n);
   if(evoke&&dpower(s,DPower::Thunder)&&s.enemies[target].hp)damage_unpowered(s,target,dpower(s,DPower::Thunder));
  }return;
 }
 if(kind==DOrb::Frost){gain_unpowered_block(s,max(0,(evoke?5:2)+focus));return;}
 if(kind==DOrb::Plasma){s.energy=(int16_t)min(999,s.energy+(evoke?2:1));record(s,LogKind::Energy,evoke?2:1);return;}
 if(kind==DOrb::Glass){n=min(9999,max(0,raw+focus)*(evoke?2:1));for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,n);}
}
static void defect_orb_passive(State&s,unsigned at,int target=-1){
 auto&x=s.expansion;if(at>=x.orb_count)return;DOrb kind=(DOrb)s.defect.orb_kind[at];int raw=x.dark[at];
 if(kind==DOrb::Dark){x.dark[at]=(int16_t)min(9999,raw+max(0,6+power_value(s,XPower::Focus)));return;}
 if(kind==DOrb::Glass){if(raw+power_value(s,XPower::Focus)<=0)return;x.dark[at]=(int16_t)max(0,raw-1);}
 defect_orb_fire(s,kind,raw,false,target);
}
static void defect_channel_dark(State&s,int count){if(count>0)queue_extension(s,XOp::DChannel,(int)DOrb::Dark,count,true);}
static void defect_orbs_end(State&s){
 for(int i=s.expansion.orb_count-1;i>=0;i--)if(s.defect.orb_kind[i]!=(unsigned)DOrb::Plasma)queue_extension(s,XOp::DOrbPassive,i,-1,true);
}
static void defect_turn_start(State&s){
 auto&v=s.defect;v.physical_started=v.cards_finished=v.energy_spent_turn=v.zero_attacks_started=0;v.status_drawn=0;
 if(dpower(s,DPower::EnergyNextTurn)){s.energy=(int16_t)min(999,s.energy+dpower(s,DPower::EnergyNextTurn));defect_gain_power(s,DPower::EnergyNextTurn,-dpower(s,DPower::EnergyNextTurn));}
 for(auto&n:v.turn_origin)n=0;
}
static void defect_after_energy_reset(State&s){
 // Player powers precede orb listeners at this boundary.
 queue_extension(s,XOp::DOrbsStart,0,0,true);
 auto&v=s.defect;for(int i=v.power_count-1;i>=0;i--){DPower p=(DPower)v.power_order[i];
  if(p==DPower::LightningRod||p==DPower::Spinner)queue_extension(s,XOp::DPowerTurn,(int)p,0,true);
 }
}
static void defect_before_hand_draw(State&s){if(dpower(s,DPower::CreativeAi))queue_extension(s,XOp::DPowerTurn,(int)DPower::CreativeAi,0,true);}
static int defect_hand_draw_count(const State&s){return min(MaxCards,dpower(s,DPower::MachineLearning));}
static void defect_turn_ready(State&s){
 // Loop's AfterPlayerTurnStart precedes AfterSideTurnStart powers.
 auto&v=s.defect;for(int i=v.power_count-1;i>=0;i--){DPower p=(DPower)v.power_order[i];
  if(p==DPower::BiasedCognition||p==DPower::Coolant||p==DPower::Feral)queue_extension(s,XOp::DPowerTurn,(int)p,0,true);
 }
 if(dpower(s,DPower::Loop))queue_extension(s,XOp::DPowerTurn,(int)DPower::Loop,0,true);
}
static void defect_turn_end(State&s){if(dpower(s,DPower::Hailstorm)&&defect_has_orb(s,DOrb::Frost))queue_extension(s,XOp::AllDamage,dpower(s,DPower::Hailstorm),0,true);}
static void defect_after_turn_end(State&s){
 auto&v=s.defect;for(int i=v.power_count-1;i>=0;i--){DPower p=(DPower)v.power_order[i];
  if(p==DPower::ConsumingShadow||defect_temporary_focus_power(p))queue_extension(s,XOp::DPowerTurn,(int)p,0,true);
 }
}
static void defect_player_side_end(State&s){
 auto&v=s.defect;
 for(auto&n:v.turn_origin)n=0;
}
static void defect_combat_end(State&s){
 auto&v=s.defect;for(unsigned d=0;d<=MaxAutoFrames;d++){v.storm_before[d]=v.subroutine_before[d]=0;v.spent[d]=v.scrape_waiting[d]=v.scrape_count[d]=0;for(auto&n:v.scrape_drawn[d])n=0;}
}
static Pile defect_status_snapshot(const State&s){
 Pile out{};const Pile* piles[]={&s.hand,&s.draw,&s.discard};
 for(const Pile*p:piles)for(unsigned i=0;i<p->count;i++){int id=p->ids[i];if(card_type(s,s.cards[id],id)==CardType::Status)out.ids[out.count++]=(uint16_t)id;}
 // Original AllPiles ends with PlayPile, ordered outer play before nested play.
 for(unsigned f=0;f<=s.expansion.depth;f++){int id=f<s.expansion.depth?s.expansion.frames[f].playing:s.playing;
  if(id>=0&&id<s.card_count&&s.zones[id]==Zone::Limbo&&card_type(s,s.cards[id],id)==CardType::Status)out.ids[out.count++]=(uint16_t)id;
 }
 return out;
}
static int defect_status_count(const State&s){return defect_status_snapshot(s).count;}
static bool queue_defect(State&s,Card c,int target,int energy){
 if(c.kind<DefectAdaptiveStrike||c.kind>DefectZap)return false;
 const auto&d=definitions[c.kind];int hit=card_base_damage(s,c,target,s.playing),block=defect_base_block(s,c,s.playing,d.block+(c.upgraded?d.block_up:0));
 int p=d.power+(c.upgraded?d.power_up:0),draw=d.draw+(c.upgraded?d.draw_up:0);
 record(s,LogKind::Play,c.kind,c.upgraded);if(d.type==CardType::Attack)relic_attack_begin(s);
 auto op=[&](XOp o,int a=0,int b=0){queue_extension(s,o,a,b);};
 auto pw=[&](DPower power,int n){op(XOp::DPower,(int)power,n);};
 auto channel=[&](DOrb kind,int n=1){op(XOp::DChannel,(int)kind,n);};
 auto attack=[&](int n){enqueue(s,EffectKind::Damage,n,target);};
 auto all=[&](int n){for(int t=0;t<s.enemy_count;t++)enqueue(s,EffectKind::Damage,n,t);};
 auto defend=[&](int n){enqueue_card_block(s,powered_block(s,c,n));};
 auto status=[&](unsigned kind,int n=1){op(XOp::DStatus,n,(int)kind);};
 switch(c.kind){
 case DefectAdaptiveStrike:attack(hit);op(XOp::DAdaptiveClone,s.playing);break;
 case DefectAllForOne:attack(hit);op(XOp::DAllForOne);break;
 case DefectBallLightning:attack(hit);channel(DOrb::Lightning);break;
 case DefectBarrage:op(XOp::DHits,hit,s.expansion.orb_count);break;
 case DefectBeamCell:attack(hit);enqueue(s,EffectKind::Vulnerable,p,target);break;
 case DefectBiasedCognition:op(XOp::DFocus,c.upgraded?6:5);pw(DPower::BiasedCognition,1);break;
 case DefectBoostAway:defend(block);status(Dazed);break;
 case DefectBootSequence:case DefectDefend:case DefectLeap:defend(block);break;
 case DefectBuffer:op(XOp::Power,(int)XPower::Buffer,p);break;
 case DefectBulkUp:op(XOp::DSlots,-1);enqueue(s,EffectKind::Strength,c.upgraded?3:2);enqueue(s,EffectKind::Dexterity,c.upgraded?3:2);break;
 case DefectCapacitor:op(XOp::DSlots,c.upgraded?3:2);break;
 case DefectChaos:op(XOp::DChannel,(int)DOrb::Count,c.upgraded?2:1);break;
 case DefectChargeBattery:defend(block);pw(DPower::EnergyNextTurn,1);break;
 case DefectChill:channel(DOrb::Frost,defect_enemy_count(s));break;
 case DefectClaw:attack(hit);op(XOp::DClaw,c.upgraded?3:2);break;
 case DefectColdSnap:attack(hit);channel(DOrb::Frost);break;
 case DefectCompact:defend(block);op(XOp::DCompact,c.upgraded);break;
 case DefectCompileDriver:attack(hit);op(XOp::DCompileDraw);break;
 case DefectConsumingShadow:channel(DOrb::Dark,c.upgraded?3:2);pw(DPower::ConsumingShadow,1);break;
 case DefectCoolant:pw(DPower::Coolant,p);break;
 case DefectCoolheaded:channel(DOrb::Frost);op(XOp::Draw,draw);break;
 case DefectCreativeAi:pw(DPower::CreativeAi,1);break;
 case DefectDarkness:channel(DOrb::Dark);op(XOp::DDarkness,c.upgraded?2:1);break;
 case DefectDefragment:op(XOp::DFocus,p);break;
 case DefectDoubleEnergy:op(XOp::DDoubleEnergy);break;
 case DefectDualcast:op(XOp::DEvoke,0,2);break;
 case DefectEchoForm:pw(DPower::EchoForm,1);break;
 case DefectFeral:pw(DPower::Feral,1);break;
 case DefectFightThrough:defend(block);status(Wound,2);break;
 case DefectFlakCannon:op(XOp::DFlak,hit);break;
 case DefectFocusedStrike:attack(hit);op(XOp::DFocus,p,1);break;
 case DefectFtl:attack(hit);op(XOp::DFtl,c.upgraded?4:3);break;
 case DefectFusion:channel(DOrb::Plasma);break;
 case DefectGeneticAlgorithm:defend(block);op(XOp::DGenetic,s.playing,c.upgraded?4:3);break;
 case DefectGlacier:defend(block);channel(DOrb::Frost,2);break;
 case DefectGlasswork:defend(block);channel(DOrb::Glass);break;
 case DefectGoForTheEyes:attack(hit);op(XOp::SharedCheckWeakAfterHit,target,p);break;
 case DefectGunkUp:op(XOp::DHits,hit,3);status(Slimed);break;
 case DefectHailstorm:pw(DPower::Hailstorm,p);break;
 case DefectHelixDrill:op(XOp::DHits,hit,max(0,(int)s.defect.energy_spent_turn-cost(s,s.playing)));break;
 case DefectHologram:defend(block);op(XOp::ChooseDiscard,1);break;
 case DefectHotfix:op(XOp::DFocus,2,2);break;
 case DefectHyperbeam:all(hit);op(XOp::DFocus,-3,3);break;
 case DefectIceLance:attack(hit);channel(DOrb::Frost,3);break;
 case DefectIteration:pw(DPower::Iteration,p);break;
 case DefectLightningRod:defend(block);pw(DPower::LightningRod,2);break;
 case DefectLoop:pw(DPower::Loop,c.upgraded?2:1);break;
 case DefectMachineLearning:pw(DPower::MachineLearning,1);break;
 case DefectMeteorStrike:attack(hit);channel(DOrb::Plasma,3);break;
 case DefectModded:op(XOp::DSlots,1);op(XOp::Draw,draw);op(XOp::DModded,s.playing);break;
 case DefectMomentumStrike:attack(hit);op(XOp::DMomentum,s.playing);break;
 case DefectMultiCast:op(XOp::DEvoke,0,energy+(c.upgraded?1:0));break;
 case DefectNull:attack(hit);enqueue(s,EffectKind::Weak,c.upgraded?2:1,target);channel(DOrb::Dark);break;
 case DefectOverclock:op(XOp::Draw,draw);status(Burn);break;
 case DefectQuadcast:op(XOp::DEvoke,0,4);break;
 case DefectRainbow:channel(DOrb::Lightning);channel(DOrb::Frost);channel(DOrb::Dark);break;
 case DefectReboot:op(XOp::DReboot);op(XOp::Draw,draw);break;
 case DefectRefract:attack(hit);attack(hit);channel(DOrb::Glass,2);break;
 case DefectRocketPunch:attack(hit);op(XOp::Draw,draw);break;
 case DefectScavenge:enqueue(s,EffectKind::ChooseExhaust);pw(DPower::EnergyNextTurn,c.upgraded?3:2);break;
 case DefectScrape:attack(hit);op(XOp::DScrape,draw,1);break;
 case DefectShadowShield:defend(block);channel(DOrb::Dark);break;
 case DefectShatter:all(hit);op(XOp::DEvokeAll);break;
 case DefectSignalBoost:pw(DPower::SignalBoost,1);break;
 case DefectSkim:op(XOp::Draw,draw);break;
 case DefectSmokestack:pw(DPower::Smokestack,p);break;
 case DefectSpinner:if(c.upgraded)channel(DOrb::Glass);pw(DPower::Spinner,1);break;
 case DefectStorm:pw(DPower::Storm,p);break;
 case DefectStrike:attack(hit);break;
 case DefectSubroutine:pw(DPower::Subroutine,1);break;
 case DefectSunder:attack(hit);break;
 case DefectSupercritical:enqueue(s,EffectKind::Energy,c.upgraded?6:4);break;
 case DefectSweepingBeam:all(hit);op(XOp::Draw,1);break;
 case DefectSynchronize:op(XOp::DSynchronize,c.upgraded?2:1);break;
 case DefectSynthesis:attack(hit);pw(DPower::FreePower,1);break;
 case DefectTempest:channel(DOrb::Lightning,energy+(c.upgraded?1:0));break;
 case DefectTeslaCoil:attack(hit);op(XOp::DTesla,target,c.upgraded?2:1);break;
 case DefectThunder:pw(DPower::Thunder,p);break;
 case DefectTrashToTreasure:pw(DPower::TrashToTreasure,1);break;
 case DefectTurbo:enqueue(s,EffectKind::Energy,c.upgraded?3:2);status(Void);break;
 case DefectUproar:attack(hit);attack(hit);op(XOp::DUproar);break;
 case DefectVoltaic:op(XOp::DVoltaic);break;
 case DefectWhiteNoise:op(XOp::Generate,3,1);break;
 case DefectZap:channel(DOrb::Lightning);break;
 default:fail(s);return true;
 }
 enqueue(s,EffectKind::Finish);return true;
}
static bool defect_effect(State&s,Effect e){
 auto&v=s.defect;auto&x=s.expansion;unsigned d=x.depth;int a=e.value,b=e.aux;
 switch((XOp)e.target){
 case XOp::DPower:defect_gain_power(s,(DPower)a,b);break;
 case XOp::DFocus:
  gain_power(s,XPower::Focus,a);if(b){defect_gain_power(s,(DPower)((int)DPower::FocusedStrikeFocus+b-1),a<0?-a:a);defect_update_temporary_focus(s);}break;
 case XOp::DSlots:defect_add_orb_slots(s,a);break;
 case XOp::DChannel:{
  if(b<=0||!alive(s))break;
  if(!x.orb_slots&&s.character!=Character::Defect)defect_add_orb_slots(s,1);
  if(!x.orb_slots)break;
  int kind=a;if(kind==(int)DOrb::Count){const DOrb kinds[]={DOrb::Lightning,DOrb::Frost,DOrb::Dark,DOrb::Plasma,DOrb::Glass};kind=(int)kinds[random_below(s.rng_cards,5)];}
  if(b>1)queue_extension(s,XOp::DChannel,a,b-1,true);
  queue_extension(s,XOp::DChannelAppend,kind,0,true);
  if(x.orb_count==x.orb_slots)queue_extension(s,XOp::DEvoke,0,1,true);break;
 }
 case XOp::DChannelAppend:
  if(alive(s)&&x.orb_slots&&x.orb_count<x.orb_slots){
   unsigned i=x.orb_count++;v.orb_kind[i]=(uint8_t)a;x.dark[i]=(int16_t)(a==(int)DOrb::Dark?6:a==(int)DOrb::Glass?4:0);
   if(a==(int)DOrb::Lightning)v.lightning_channeled=(uint16_t)min(9999,v.lightning_channeled+1);
  }break;
 case XOp::DOrbPassive:defect_orb_passive(s,(unsigned)a,b);break;
 case XOp::DOrbsStart:for(int i=x.orb_count-1;i>=0;i--)if(v.orb_kind[i]==(unsigned)DOrb::Plasma)queue_extension(s,XOp::DOrbPassive,i,-1,true);break;
 case XOp::DEvoke:{
  if(b<=0||!x.orb_count||!alive(s))break;unsigned at=a<0?x.orb_count-1:(unsigned)a;if(at>=x.orb_count)break;
  DOrb kind=(DOrb)v.orb_kind[at];int raw=x.dark[at];
  if(b>1)queue_extension(s,XOp::DEvoke,a,b-1,true);else defect_remove_orb(s,at);
  defect_orb_fire(s,kind,raw,true);break;
 }
 case XOp::DEvokeAll:if(x.orb_count){queue_extension(s,XOp::DEvokeAll,0,0,true);queue_extension(s,XOp::DEvoke,0,2,true);}break;
 case XOp::DPowerTurn:{
  DPower p=(DPower)a;int n=b?b:dpower(s,p);if(!n)break;
  if(p==DPower::BiasedCognition)gain_power(s,XPower::Focus,-n);
  else if(defect_temporary_focus_power(p)){gain_power(s,XPower::Focus,p==DPower::HyperbeamFocus?n:-n);defect_gain_power(s,p,-n);defect_update_temporary_focus(s);}
  else if(p==DPower::Coolant)gain_unpowered_block(s,min(9999,n*defect_orb_unique(s)));
  else if(p==DPower::Feral)v.feral_used=0;
  else if(p==DPower::LightningRod){defect_gain_power(s,p,-1);queue_extension(s,XOp::DChannel,(int)DOrb::Lightning,1,true);}
  else if(p==DPower::Spinner)queue_extension(s,XOp::DChannel,(int)DOrb::Glass,n,true);
  else if(p==DPower::CreativeAi){if(n>1)queue_extension(s,XOp::DPowerTurn,a,n-1,true);queue_extension(s,XOp::Generate,3,0,true);}
  else if(p==DPower::Loop){if(n>1)queue_extension(s,XOp::DPowerTurn,a,n-1,true);if(x.orb_count)queue_extension(s,XOp::DOrbPassive,0,-1,true);}
  else if(p==DPower::ConsumingShadow){if(n>1)queue_extension(s,XOp::DPowerTurn,a,n-1,true);queue_extension(s,XOp::DEvoke,-1,1,true);}
  break;
 }
 case XOp::DGeneratedListener:
  if(a==(int)DPower::Smokestack)for(int t=0;t<s.enemy_count;t++)damage_unpowered(s,t,b);
  else if(a==(int)DPower::TrashToTreasure)queue_extension(s,XOp::DChannel,(int)DOrb::Count,b,true);break;
 case XOp::DRocketDiscount:
  for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==DefectRocketPunch)v.cost_delta[id]=(int16_t)max(-9999,v.cost_delta[id]-1);break;
 case XOp::DAdaptiveClone:{
  if(a<0||a>=s.card_count)break;int id=make_card(s,s.cards[a]);if(id<0)break;copy_card_modifiers(s,id,a);
  x.turn_cost[id]=0;x.combat_cost[id]=1;combat_cost_set(s,id);put(s,s.discard,id,Zone::Discard);record(s,LogKind::Create,1,s.cards[id].kind);notify_card_generated(s,id);break;
 }
 case XOp::DAllForOne:
  for(unsigned i=0;i<s.discard.count&&s.hand.count<MaxHand;){int id=s.discard.ids[i];CardType type=card_type(s,s.cards[id],id);
   if(cost(s.cards[id])>=0&&cost(s,id)==0&&(type==CardType::Attack||type==CardType::Skill||type==CardType::Power)){remove(s.discard,i);put(s,s.hand,id,Zone::Hand);}else ++i;
  }break;
 case XOp::DClaw:for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==DefectClaw)v.value[id]=(int16_t)min(9999,v.value[id]+a);break;
 case XOp::DCompact:{
  uint16_t changed[MaxHand];unsigned n=0;
  for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(card_type(s,s.cards[id],id)==CardType::Status){s.cards[id]={Fuel,(uint8_t)a};reset_card_modifiers(s,id);changed[n++]=(uint16_t)id;}}
  // The original successful combat Transform announces each replacement to
  // AfterCardGeneratedForCombat, even when the replacement is a Skill.
  for(int i=(int)n-1;i>=0;i--)notify_card_generated(s,changed[i]);break;
 }
 case XOp::DGenetic:
  if(a>=0&&a<s.card_count){v.value[a]=(int16_t)min(9998,v.value[a]+b);unsigned origin=v.deck_origin[a];if(origin&&origin<=s.deck_count&&s.deck[origin-1].kind==DefectGeneticAlgorithm)v.genetic_deck[origin-1]=(uint16_t)min(9998,v.genetic_deck[origin-1]+b);}break;
 case XOp::DModded:if(a>=0&&a<s.card_count)v.cost_delta[a]=(int16_t)min(9999,v.cost_delta[a]+1);break;
 case XOp::DMomentum:if(a>=0&&a<s.card_count){x.turn_cost[a]=0;x.combat_cost[a]=1;combat_cost_set(s,a);}break;
 case XOp::DScrape:
  if(b){v.scrape_waiting[d]=v.scrape_count[d]=0;for(auto&n:v.scrape_drawn[d])n=0;}
  if(a>0){queue_extension(s,XOp::DScrape,a-1,0,true);v.scrape_waiting[d]=1;queue_extension(s,XOp::Draw,1,0,true);}
  else{v.scrape_waiting[d]=0;queue_extension(s,XOp::DScrapeDiscard,0,0,true);}break;
 case XOp::DScrapeDiscard:{
  uint16_t discarded[MaxHand];unsigned n=0;
  for(unsigned i=0;i<v.scrape_count[d];i++){int id=(int)v.scrape_drawn[d][i]-1;
   if(id>=0&&id<s.card_count&&s.zones[id]==Zone::Hand&&(cost(s.cards[id])<0||cost(s,id)!=0)&&move_instance(s,id,s.hand,s.discard,Zone::Discard)){discarded[n++]=(uint16_t)id;record(s,LogKind::Discard,s.cards[id].kind);}
  }
  v.scrape_waiting[d]=v.scrape_count[d]=0;for(auto&n0:v.scrape_drawn[d])n0=0;
  for(unsigned i=n;i>0;i--)silent_card_discarded(s,discarded[i-1]);break;
 }
 case XOp::DFlak:{
  Pile statuses=defect_status_snapshot(s);
  if(statuses.count)queue_extension(s,XOp::RandomHits,a,statuses.count,true);
  for(unsigned i=statuses.count;i>0;i--)queue_extension(s,XOp::DExhaustStatus,statuses.ids[i-1],0,true);break;
 }
 case XOp::DExhaustStatus:{
  if(a<0||a>=s.card_count||s.zones[a]==Zone::Exhaust)break;
  Pile*from=s.zones[a]==Zone::Hand?&s.hand:s.zones[a]==Zone::Draw?&s.draw:s.zones[a]==Zone::Discard?&s.discard:s.zones[a]==Zone::Power?&s.powers:nullptr;
  if(from)for(unsigned i=0;i<from->count;i++)if(from->ids[i]==a){remove(*from,i);break;}
  exhaust_id(s,a);break;
 }
 case XOp::DHits:
  if(b>0&&alive(s)){if(b>1)queue_extension(s,XOp::DHits,a,b-1,true);enqueue(s,EffectKind::Damage,a,x.target,true);}break;
 case XOp::DUproar:{
  uint16_t eligible[MaxCards];unsigned n=0;
  for(unsigned i=0;i<s.draw.count;i++){int id=s.draw.ids[i];if(card_type(s,s.cards[id],id)==CardType::Attack&&cost(s.cards[id])>=-1)eligible[n++]=(uint16_t)id;}
  if(n)queue_extension(s,XOp::AutoInstance,eligible[random_below(s.rng_cards,n)],0,true);break;
 }
 case XOp::DFtl:if(v.cards_finished<(unsigned)a)queue_extension(s,XOp::Draw,1,0,true);break;
 case XOp::DTesla:
  for(int i=x.orb_count-1;i>=0;i--)if(v.orb_kind[i]==(unsigned)DOrb::Lightning)for(int j=0;j<b;j++)queue_extension(s,XOp::DOrbPassive,i,a,true);break;
 case XOp::DDarkness:
  for(int i=x.orb_count-1;i>=0;i--)if(v.orb_kind[i]==(unsigned)DOrb::Dark)for(int j=0;j<a;j++)queue_extension(s,XOp::DOrbPassive,i,-1,true);break;
 case XOp::DVoltaic:if(v.lightning_channeled)queue_extension(s,XOp::DChannel,(int)DOrb::Lightning,v.lightning_channeled,true);break;
 case XOp::DStatus:
  if(a>0){if(a>1)queue_extension(s,XOp::DStatus,a-1,b,true);int id=make_card(s,{(CardKind)b,0});if(id>=0){put(s,s.discard,id,Zone::Discard);record(s,LogKind::Create,1,b);notify_card_generated(s,id);}}break;
 case XOp::DReboot:
  while(s.hand.count){int id=remove(s.hand,0);put(s,s.draw,id,Zone::Draw);}shuffle(s);break;
 case XOp::DDoubleEnergy:enqueue(s,EffectKind::Energy,s.energy,0,true);break;
 case XOp::DCompileDraw:queue_extension(s,XOp::Draw,defect_orb_unique(s),0,true);break;
 case XOp::DSynchronize:queue_extension(s,XOp::DFocus,defect_orb_unique(s)*a,4,true);break;
 default:return false;
 }
 return true;
}
static bool defect_word(const char*a,const char*b){while(*a&&*b&&*a==*b){++a;++b;}return *a==*b;}
static bool defect_description_value(const State&s,Card c,int target,int instance,const char*name,int&out){
 if(c.kind<DefectAdaptiveStrike||c.kind>DefectZap)return false;
 if(defect_word(name,"Damage")){
  if(c.kind==DefectHailstorm||c.kind==DefectSmokestack||c.kind==DefectThunder){const auto&d=definitions[c.kind];out=d.power+(c.upgraded?d.power_up:0);}
  else out=damage(s,c,target,instance);return true;
 }
 if(defect_word(name,"Block")){
  if(c.kind==DefectCoolant)out=c.upgraded?3:2;else out=card_block(s,c,instance);return true;
 }
 if(defect_word(name,"CalculatedHits")){
  if(c.kind==DefectBarrage)out=s.expansion.orb_count;
  else if(c.kind==DefectFlakCannon)out=defect_status_count(s);
  else if(c.kind==DefectHelixDrill)out=max(0,(int)s.defect.energy_spent_turn-((instance>=0&&instance<s.card_count&&s.zones[instance]==Zone::Limbo)?cost(s,instance):0));
  else return false;return true;
 }
 if(defect_word(name,"CalculatedCards")&&c.kind==DefectCompileDriver){out=defect_orb_unique(s);return true;}
 if(defect_word(name,"CalculatedFocus")&&c.kind==DefectSynchronize){out=defect_orb_unique(s)*(c.upgraded?2:1);return true;}
 if(defect_word(name,"CalculatedChannels")&&c.kind==DefectVoltaic){out=s.defect.lightning_channeled;return true;}
 return false;
}
