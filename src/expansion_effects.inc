static void death_cleanup(State&s){
 for(unsigned id=0;id<s.card_count;id++)if(s.zones[id]==Zone::Limbo)put(s,s.discard,id,Zone::Discard);
 s.playing=-1;s.effect_count=0;s.expansion.depth=0;s.expansion.potion_active=0;s.expansion.enemy_side=0;
 s.discard_required=s.exhaust_limit=0;clear_extended_choice(s);relic_attack_end(s);s.expansion.triple=0;s.phase=Phase::Dead;
}
static bool revive(State&s){
 if(s.hp>0)return true;
 if(!combat_phase(s.phase))return false;
 for(auto&p:s.potions)if(p==Potion::FairyInABottle){p=Potion::Empty;s.hp=(int16_t)max(1,s.max_hp*3/10);record(s,LogKind::Potion,(int)Potion::FairyInABottle);record(s,LogKind::Heal,s.hp);return true;}
 return false;
}
void player_hp_loss(State&s,int amount,bool attack,bool blockable){
 if(s.hp<=0)return;auto&x=s.expansion;amount=max(0,amount);
 if(combat_phase(s.phase)&&power_value(s,XPower::Intangible))amount=min(1,amount);
 int absorbed=blockable?min(s.block,amount):0;s.block-=(int16_t)absorbed;int unblocked=amount-absorbed,loss=unblocked;
 if(attack&&x.osty_hp){int pet=min(loss,x.osty_hp);x.osty_hp-=(int16_t)pet;loss-=pet;}
 if(loss&&combat_phase(s.phase)&&power_value(s,XPower::Buffer)){gain_power(s,XPower::Buffer,-1);loss=0;}
 int old=s.hp;s.hp=(int16_t)max(0,s.hp-loss);record(s,LogKind::LoseHp,old-s.hp);
 if(!s.hp&&!revive(s)){death_cleanup(s);return;}
 if(combat_phase(s.phase))relic_after_hp_loss(s,min(old,loss));
 if(attack&&loss>0&&power_value(s,XPower::Gambit)){
  x.powers[(int)XPower::Gambit]=0;s.hp=0;if(!revive(s))death_cleanup(s);
 }
}
int enemy_damage(const State&s,int i){
 if(i<0||i>=s.enemy_count)return 0;const auto&e=s.enemies[i];if(e.move==2)return 0;
 int n=max(0,(e.move==0?12:6)+e.strength),mul=(e.weak?75:100)*(s.expansion.beetle[i]?70:100);
 return n*mul/10000;
}
static void autoplay_from(State&s,Pile&p,int filter,bool top){
 if(!alive(s))return;
 // Top-of-draw auto play refills an empty draw pile. Random draw selection does
 // not fabricate a shuffle when its sampled source contains no cards.
 if(top&&!p.count&&s.discard.count){queue_extension(s,XOp::AutoTop,0,0,true);shuffle(s);return;}
 uint16_t ids[MaxCards];unsigned n=0;
 for(unsigned i=0;i<p.count;i++)if(!filter||(int)definitions[s.cards[p.ids[i]].kind].type==filter-1)ids[n++]=p.ids[i];
 if(n)start_instance(s,top?ids[n-1]:ids[random_below(s.rng_cards,n)],-1,true);
}
static void next_enemy(State&s,int i){
 if(!alive(s)){victory(s);return;}
 if(i>=s.enemy_count){queue_extension(s,XOp::EnemyDone,0,0,true);return;}
 queue_extension(s,XOp::EnemyAct,i+1,0,true);
 auto&e=s.enemies[i];if(!e.hp)return;
 e.block=0;int hit=enemy_damage(s,i);bool attack=e.move!=2;record(s,LogKind::Enemy,i,e.move);
 if(attack&&s.thorns)damage_unpowered(s,i,s.thorns);
 player_hp_loss(s,hit,attack,true);if(s.phase==Phase::Dead)return;
 if(attack&&s.flame_barrier)damage_unpowered(s,i,s.flame_barrier);
 if(e.hp){
  if(e.move==1)e.block=5;if(e.move==2)e.strength=(int16_t)min(9999,e.strength+2);
  if(s.expansion.demise[i]){int n=min(e.hp,s.expansion.demise[i]);e.hp-=(int16_t)n;record(s,LogKind::LoseHp,n,i+1);}
  if(s.expansion.doom[i]>=e.hp&&s.expansion.doom[i])e.hp=0;
 }
 if(e.weak)--e.weak;if(e.vulnerable)--e.vulnerable;if(s.expansion.beetle[i])--s.expansion.beetle[i];
 e.move=(uint8_t)((e.move+1)%3);
}
static void channel_dark(State&s){
 auto&x=s.expansion;if(!x.orb_slots)return;
 if(x.orb_count>=x.orb_slots){
  int target=-1;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp&&(target<0||s.enemies[i].hp<s.enemies[target].hp))target=i;
  if(target>=0)damage_unpowered(s,target,x.dark[0]);
  for(int i=1;i<x.orb_count;i++)x.dark[i-1]=x.dark[i];x.dark[--x.orb_count]=0;
 }
 x.dark[x.orb_count++]=6;
}
static void forge_blade(State&s,int n){
 bool found=false;auto&x=s.expansion;
 for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==SovereignBlade&&s.zones[id]!=Zone::Exhaust){
  x.blade_damage[id]=(int16_t)min(9999,max(10,x.blade_damage[id])+n);found=true;
 }
 if(!found){int id=create_to_hand(s,{SovereignBlade,0});if(id>=0)x.blade_damage[id]=(int16_t)min(9999,10+n);}
}
static void extension_effect(State&s,Effect e){
 auto&x=s.expansion;auto op=(XOp)e.target;int a=e.value,b=e.aux;
 switch(op){
 case XOp::Noop:break;
 case XOp::Power:{
  if(a<0||a>=XPowerCount){fail(s);break;}
  int16_t*instances=a==(int)XPower::Automation?x.automation:a==(int)XPower::Panache?x.panache:a==(int)XPower::RollingBoulder?x.boulder:nullptr;
  if(instances){int i=0;while(i<MaxPowerInstances&&instances[i])++i;if(i==MaxPowerInstances){fail(s);break;}instances[i]=(int16_t)b;if(a==(int)XPower::Panache)x.panache_skip[i]=1;}
  if(a==(int)XPower::Gambit)x.powers[a]=1;else gain_power(s,(XPower)a,b);break;}
 case XOp::AllDamage:for(int i=0;i<s.enemy_count;i++)damage_unpowered(s,i,a);break;
 case XOp::StrengthLoss:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp){int n=min(b,min(9999+s.enemies[a].strength,9999-x.restore_strength[a]));s.enemies[a].strength-=(int16_t)n;x.restore_strength[a]+=(int16_t)n;}break;
 case XOp::AllRare:
  for(int i=(int)s.draw.count-1;i>=0;i--){int id=s.draw.ids[i];if(card_rarity(s.cards[id].kind)==4){
   if(s.hand.count<MaxHand)move_instance(s,id,s.draw,s.hand,Zone::Hand);else move_instance(s,id,s.draw,s.discard,Zone::Discard);
  }}break;
 case XOp::Offer:offer_cards(s,a,b);break;
 case XOp::Generate:generate_card(s,a,b);break;
 case XOp::ChooseDraw:choose_draw(s,a,b>0?b:0,b<0?-b:1);break;
 case XOp::ChooseDiscard:if(s.hand.count<MaxHand)set_extended_choice(s,XChoice::Discard,s.discard,1,1,b);break;
 case XOp::ChooseTop:set_extended_choice(s,XChoice::Top,s.hand,1,1);break;
 case XOp::DiscardRedraw:set_extended_choice(s,XChoice::DiscardRedraw,s.hand,0,s.hand.count);break;
 case XOp::ExhaustAny:set_extended_choice(s,XChoice::Exhaust,s.hand,0,s.hand.count);break;
 case XOp::Free:set_extended_choice(s,XChoice::Free,s.hand,1,1);break;
 case XOp::Transform:set_extended_choice(s,XChoice::Transform,s.hand,a,a);break;
 case XOp::AutoDraw:autoplay_from(s,s.draw,a,false);break;
 case XOp::AutoDiscard:autoplay_from(s,s.discard,a,false);break;
 case XOp::AutoTop:autoplay_from(s,s.draw,0,true);break;
 case XOp::AutoInstance:start_instance(s,a,-1,true);break;
 case XOp::Draw:queue_extension(s,XOp::DrawOne,a,0,true);break;
 case XOp::DrawOne:draw_one(s,a);break;
 case XOp::EscapeBlock:
  if(x.last_drawn>=0&&x.last_drawn<s.card_count&&definitions[s.cards[x.last_drawn].kind].type==CardType::Skill)gain_unpowered_block(s,a);break;
 case XOp::EnchantReplay:{
  uint16_t ids[MaxCards];unsigned n=0;for(unsigned i=0;i<s.draw.count;i++)if(!x.replay[s.draw.ids[i]])ids[n++]=s.draw.ids[i];
  if(n)x.replay[ids[random_below(s.rng_cards,n)]]=(uint8_t)min(255,a);break;}
 case XOp::Create:for(int i=0;i<a;i++){Card c{(uint8_t)(b&255),(uint8_t)((b>>8)&1)};if(create_to_hand(s,c)<0)break;}break;
 case XOp::CopyDamage:{int amount=x.last_damage;for(int i=0;i<s.enemy_count;i++)if(i!=a)damage_unpowered(s,i,amount);break;}
 case XOp::DamageBlock:if(s.playing>=0)gain_unpowered_block(s,powered_block(s,s.cards[s.playing],x.last_damage));break;
 case XOp::ReturnNext:if(a>=0&&a<s.card_count)x.return_turn[a]=(uint16_t)(s.turn+1);break;
 case XOp::Prolong:s.block_next_turn=(int16_t)min(9999,s.block_next_turn+s.block);break;
 case XOp::Restless:
  if(!s.hand.count){enqueue(s,EffectKind::Energy,b,0,true);queue_extension(s,XOp::Draw,a,0,true);}break;
 case XOp::Bomb:{int i=0;while(i<MaxBombs&&x.bomb_turns[i])++i;if(i==MaxBombs){fail(s);break;}x.bomb_damage[i]=(int16_t)a;x.bomb_turns[i]=(uint8_t)b;break;}
 case XOp::ShuffleAll:
  while(s.hand.count){int id=remove(s.hand,s.hand.count-1);put(s,s.discard,id,Zone::Discard);}shuffle(s);break;
 case XOp::RandomCosts:for(unsigned i=0;i<s.hand.count;i++)x.turn_cost[s.hand.ids[i]]=(uint8_t)(1+random_below(s.rng_cards,4));break;
 case XOp::StrikeReplay:for(unsigned id=0;id<s.card_count;id++)if(card_strike_tag(s.cards[id].kind)&&x.replay[id]<255)++x.replay[id];break;
 case XOp::PotionDone:x.potion_active=0;break;
 case XOp::Demise:if(a>=0&&a<s.enemy_count&&s.enemies[a].hp)x.demise[a]=(int16_t)min(9999,x.demise[a]+b);break;
 case XOp::ExhaustHand:for(unsigned i=s.hand.count;i>0;i--)enqueue(s,EffectKind::ExhaustCard,s.hand.ids[i-1],0,true);break;
 case XOp::Strength:s.strength=(int16_t)min(9999,s.strength+a);record(s,LogKind::Strength,a);break;
 case XOp::Dexterity:s.dexterity=(int16_t)min(9999,s.dexterity+a);record(s,LogKind::Dexterity,a);break;
 case XOp::AfterPlay:after_play(s,a);break;
 case XOp::RandomHits:{
  if(b<=0||!alive(s))break;if(b>1)queue_extension(s,XOp::RandomHits,a,b-1,true);
  int ids[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp)ids[n++]=i;
  if(n)enqueue(s,EffectKind::Damage,a,ids[random_below(s.rng_cards,n)],true);break;}
 case XOp::Potion:procure_potion(s,random_potion(s));break;
 case XOp::PlayerDamage:player_hp_loss(s,a,false,true);break;
 case XOp::Summon:
  if(!x.osty_hp)x.osty_max_hp=0;
  {int n=min(a,9999-x.osty_max_hp);x.osty_max_hp+=(int16_t)n;x.osty_hp+=(int16_t)n;}break;
 case XOp::Forge:forge_blade(s,a);break;
 case XOp::ChannelDark:for(int i=0;i<a;i++)channel_dark(s);break;
 case XOp::OrbsEnd:for(int i=0;i<x.orb_count;i++)x.dark[i]=(int16_t)min(9999,x.dark[i]+max(0,6+power_value(s,XPower::Focus)));break;
 case XOp::TurnEnd:
  queue_extension(s,XOp::EndDiscard,0,0,true);
  relic_hook(s,RelicHook::TurnEnd);
  if(power_value(s,XPower::Plating))gain_unpowered_block(s,power_value(s,XPower::Plating));
  if(power_value(s,XPower::Regen)){heal(s,power_value(s,XPower::Regen));gain_power(s,XPower::Regen,-1);}
  if(power_value(s,XPower::Ritual))s.strength=(int16_t)min(9999,s.strength+power_value(s,XPower::Ritual));
  for(int i=MaxBombs-1;i>=0;i--)if(x.bomb_turns[i]&&!--x.bomb_turns[i]){queue_extension(s,XOp::AllDamage,x.bomb_damage[i],0,true);x.bomb_damage[i]=0;}
  queue_extension(s,XOp::OrbsEnd,0,0,true);break;
 case XOp::EndDiscard:
  if(!alive(s)){victory(s);break;}
  for(unsigned i=0;i<s.hand.count;){int id=s.hand.ids[i];if(card_retains(s.cards[id])||power_value(s,XPower::Retain)){++i;continue;}remove(s.hand,i);put(s,s.discard,id,Zone::Discard);}
  if(power_value(s,XPower::Retain))gain_power(s,XPower::Retain,-1);
  queue_extension(s,XOp::EnemyStart,0,0,true);break;
 case XOp::EnemyStart:
  s.strength=(int16_t)max(0,s.strength-s.temporary_strength);s.temporary_strength=0;
  s.dexterity=(int16_t)max(0,s.dexterity-s.temporary_dexterity);s.temporary_dexterity=0;
  s.no_draw=0;x.powers[(int)XPower::Duplicator]=0;x.enemy_side=1;
  if(x.skip_enemy){--x.skip_enemy;queue_extension(s,XOp::TurnStart,0,0,true);break;}
  for(int i=0;i<s.enemy_count;i++){auto&e=s.enemies[i];if(e.hp&&e.poison){int n=e.poison;e.hp=(int16_t)max(0,e.hp-n);if(e.hp)--e.poison;record(s,LogKind::PoisonDamage,n,i);}}
  if(!alive(s)){victory(s);break;}queue_extension(s,XOp::EnemyAct,0,0,true);break;
 case XOp::EnemyAct:next_enemy(s,a);break;
 case XOp::EnemyDone:
  s.flame_barrier=0;if(s.no_block)--s.no_block;
  if(power_value(s,XPower::Intangible))gain_power(s,XPower::Intangible,-1);
  for(int i=0;i<s.enemy_count;i++){s.enemies[i].strength=(int16_t)min(9999,s.enemies[i].strength+x.restore_strength[i]);x.restore_strength[i]=0;}
  if(!alive(s)){victory(s);break;}queue_extension(s,XOp::TurnStart,0,0,true);break;
 case XOp::TurnStart:
  if(s.turn>=20000){fail(s);break;}
  ++s.turn;x.enemy_side=0;s.block=(int16_t)min(9999,s.block_next_turn+power_value(s,XPower::Plating));s.block_next_turn=0;s.energy=3;
  x.nonpower_started=0;x.returning=0;for(auto&v:x.turn_cost)v=0;for(auto&v:x.panache_count)v=0;
  relic_reset_turn_counters(s);if(s.block)record(s,LogKind::Block,s.block);
  if(power_value(s,XPower::Plating))gain_power(s,XPower::Plating,-1);
  if(s.demon_form){s.strength=(int16_t)min(9999,s.strength+s.demon_form);record(s,LogKind::Strength,s.demon_form);}
  s.vigor=(int16_t)min(9999,s.vigor+power_value(s,XPower::PrepTime));
  for(int i=0;i<s.enemy_count;i++)if(s.noxious_fumes)apply_poison(s,i,s.noxious_fumes);
  for(unsigned id=0;id<s.card_count;id++)if(x.return_turn[id]&&x.return_turn[id]<=s.turn){
   x.return_turn[id]=0;
   if(s.zones[id]==Zone::Draw){if(s.hand.count<MaxHand)move_instance(s,id,s.draw,s.hand,Zone::Hand);else move_instance(s,id,s.draw,s.discard,Zone::Discard);}
   else if(s.zones[id]==Zone::Discard&&s.hand.count<MaxHand)move_instance(s,id,s.discard,s.hand,Zone::Hand);
  }
  queue_extension(s,XOp::TurnDraw,0,0,true);
  for(int i=0;i<power_value(s,XPower::Mayhem);i++)queue_extension(s,XOp::AutoTop,0,0,true);
  if(s.infinite_blades)queue_extension(s,XOp::Create,s.infinite_blades,Shiv,true);
  break;
 case XOp::TurnDraw:{
  queue_extension(s,XOp::TurnReady,0,0,true);int n=5+s.draw_next_turn;s.draw_next_turn=0;
  if(power_value(s,XPower::Clarity)){++n;gain_power(s,XPower::Clarity,-1);}
  if(power_value(s,XPower::Radiance)){s.energy=(int16_t)min(999,s.energy+1);gain_power(s,XPower::Radiance,-1);}
  queue_extension(s,XOp::Draw,n,0,true);break;}
 case XOp::TurnReady:{
  relic_hook(s,RelicHook::TurnStart);record(s,LogKind::Turn,s.turn);
  if(power_value(s,XPower::Entropy))queue_extension(s,XOp::Transform,power_value(s,XPower::Entropy),0,true);
  int total=0;for(int i=MaxPowerInstances-1;i>=0;i--)if(x.boulder[i]){queue_extension(s,XOp::AllDamage,x.boulder[i],0,true);x.boulder[i]=(int16_t)min(9999,x.boulder[i]+5);total+=x.boulder[i];}
  x.powers[(int)XPower::RollingBoulder]=(int16_t)min(9999,total);break;}
 default:fail(s);break;
 }
}
