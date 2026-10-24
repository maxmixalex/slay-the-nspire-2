// Ordered, resumable original relic listeners. All rules in this module are
// reimplemented against v0.111.0 (41cef1ea); original source/assets stay external.
static bool wr_live(const WorldRelicInstance&r){return !(r.flags&(WorldRelicMelted|WorldRelicDisabled));}
// Card callbacks can still have s.playing set while a relic applies a power.
// Such a power's source is the relic, so it cannot spend Unsettling Lamp.
static unsigned wr_relic_dispatch_depth=0;
struct WRDispatchScope {WRDispatchScope(){++wr_relic_dispatch_depth;}~WRDispatchScope(){--wr_relic_dispatch_depth;}};
static WorldRelicInstance* wr_find(State&s,RelicId id){for(unsigned i=0;i<s.world_relics.count;i++)if(s.world_relics.entries[i].id==id&&wr_live(s.world_relics.entries[i]))return &s.world_relics.entries[i];return nullptr;}
static const WorldRelicInstance* wr_find(const State&s,RelicId id){for(unsigned i=0;i<s.world_relics.count;i++)if(s.world_relics.entries[i].id==id&&wr_live(s.world_relics.entries[i]))return &s.world_relics.entries[i];return nullptr;}
static bool wr_has(const State&s,RelicId id){return s.world_relics.active&&wr_find(s,id);}
static bool wr_card(const State&s,int id){return id>=0&&id<s.card_count;}
static CardType wr_type(const State&s,int id){return wr_card(s,id)?card_type(s,s.cards[id],id):CardType::Status;}
static int wr_cmp(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
static void wr_sort_cards(const State&s,uint16_t*ids,unsigned n){for(unsigned i=1;i<n;i++){unsigned j=i;auto id=ids[i];while(j){auto p=ids[j-1];int order=wr_cmp(definitions[s.cards[p].kind].key,definitions[s.cards[id].kind].key);if(order<0||(order==0&&s.cards[p].upgraded<=s.cards[id].upgraded))break;ids[j]=ids[j-1];--j;}ids[j]=id;}}
static int wr_target(State&s){int targets[MaxEnemies],n=0;for(int i=0;i<s.enemy_count;i++)if(enemy_allow_target(s,i))targets[n++]=i;return n?targets[world_rng(s,WorldRng::CombatTargets).below(n)]:-1;}
static void wr_energy(State&s,int n){s.energy=(int16_t)max(0,min(9999,s.energy+n));record(s,LogKind::Energy,n);}
static void wr_draw(State&s,int n){if(n>0)queue_extension(s,XOp::Draw,n,0,true);}
static void wr_block(State&s,int n){gain_unpowered_block(s,n);}
static void wr_all_damage(State&s,int n){queue_extension(s,XOp::AllDamage,n,0,true);}
static void wr_random_damage(State&s,int n){int t=wr_target(s);if(t>=0)damage_unpowered(s,t,n);}
static bool wr_has_potions(const State&s){for(unsigned i=0;i<potion_slot_count(s);i++)if(potion_at(s,i)!=Potion::Empty)return true;return false;}
static uint32_t wr_play_serial(const State&s){return s.world_relics.play_serial[min(s.expansion.depth,MaxAutoFrames)];}
void world_relic_gain_strength(State&s,int n){if(n>0&&s.world_relics.active)for(unsigned i=0;i<s.world_relics.count;i++){auto&r=s.world_relics.entries[i];if(wr_live(r)&&r.id==WorldRelicRuinedHelmet&&!r.values[0]){n=min(9999,n*2);r.values[0]=1;world_relic_flash(s,r.id);}}
 int old=s.strength;s.strength=(int16_t)max(-9999,min(9999,s.strength+n));if(s.strength!=old)record(s,LogKind::Strength,s.strength-old);}
void world_relic_gain_dexterity(State&s,int n){int old=s.dexterity;s.dexterity=(int16_t)max(-9999,min(9999,s.dexterity+n));if(s.dexterity!=old)record(s,LogKind::Dexterity,s.dexterity-old);}
static int wr_max_energy(const State&s,int value){if(!s.world_relics.active)return value;for(unsigned i=0;i<s.world_relics.count;i++){const auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;switch(r.id){
 case WorldRelicEctoplasm:case WorldRelicSozu:case WorldRelicVelvetChoker:case WorldRelicBlessedAntler:case WorldRelicSpikedGauntlets:case WorldRelicPrismaticGem:case WorldRelicWhisperingEarring:case WorldRelicBloodSoakedRose:case WorldRelicPhilosophersStone:++value;break;
 case WorldRelicPumpkinCandle:if(r.values[0]>0)++value;break;case WorldRelicPaelsFlesh:if(s.turn>=3)++value;break;case WorldRelicBread:if(s.turn!=1)++value;break;default:break;}}
 return max(0,min(9999,value));}
static int wr_hand_draw(State&s,int value,bool commit){if(!s.world_relics.active)return value;for(unsigned i=0;i<s.world_relics.count;i++){auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;int before=value;switch(r.id){
 case WorldRelicBagOfPreparation:case WorldRelicRingOfTheSnake:if(s.turn<=1)value+=2;break;
 case WorldRelicRingOfTheDrake:if(s.turn<=3)value+=2;break;case WorldRelicFiddle:case WorldRelicSneckoEye:value+=2;break;case WorldRelicPaelsBlood:++value;break;
 case WorldRelicBigMushroom:if(s.turn==1)value-=2;break;case WorldRelicBoomingConch:if(s.turn<=1&&world_room_type(s)==2)value+=2;break;
 case WorldRelicPocketwatch:if(s.turn>1&&r.values[1]<=3)value+=3;break;case WorldRelicPollinousCore:if(r.values[0]>=4){value+=2;if(commit)r.values[0]=0;}break;
 case WorldRelicPendulum:if(!r.values[0])++value;break;default:break;}if(commit&&value!=before)world_relic_flash(s,r.id);}
 return max(0,min(MaxCards,value));}
static int wr_energy_cost(const State&s,Card c,int id,int value,bool late){if(!s.world_relics.active||value<0)return value;for(unsigned i=0;i<s.world_relics.count;i++){const auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;
 if(!late&&r.id==WorldRelicSpikedGauntlets&&card_type(s,c,id)==CardType::Power)++value;
 if(late&&r.id==WorldRelicBrilliantScarf&&r.values[0]==4&&wr_card(s,id)&&(s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo))value=0;}return min(9999,value);}
static int wr_star_cost(const State&s,Card,int id,int value){const auto*r=wr_find(s,WorldRelicBrilliantScarf);return s.world_relics.active&&value>=0&&r&&r->values[0]==4&&wr_card(s,id)&&(s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo)?0:value;}
static bool wr_minion(Card c){return c.kind==MinionStrike||c.kind==MinionDiveBomb||c.kind==MinionSacrifice;}
static int wr_damage_add(const State&s,Card c,int id,int value,bool powered){if(!s.world_relics.active||!powered)return value;for(unsigned i=0;i<s.world_relics.count;i++){const auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;switch(r.id){
 case WorldRelicStrikeDummy:if(card_strike_tag(c.kind))value+=3;break;case WorldRelicFakeStrikeDummy:if(card_strike_tag(c.kind))++value;break;case WorldRelicMiniatureCannon:if(c.upgraded)value+=3;break;case WorldRelicMysticLighter:if(wr_card(s,id)&&s.event_cards.cards[id].enchantment!=EventEnchantment::None)value+=9;break;default:break;}}return value;}
static int wr_damage_percent(const State&s,Card c,int id,int,bool powered){if(!s.world_relics.active)return 100;int factor=100;for(unsigned i=0;i<s.world_relics.count;i++){const auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;
 if(r.id==WorldRelicVitruvianMinion&&wr_minion(c))factor*=2;
 if(powered&&r.id==WorldRelicPenNib&&((r.values[1]&&r.values[1]==id+1)||(!r.values[1]&&r.values[0]==9&&(!wr_card(s,id)||s.zones[id]!=Zone::Limbo))))factor*=2;}return factor;}
static int wr_incoming_damage_percent(const State&s,int dealer,bool powered){return powered&&dealer>=0&&dealer<s.enemy_count&&s.enemies[dealer].hp<=s.expansion.doom[dealer]&&wr_has(s,WorldRelicUndyingSigil)?50:100;}
static int wr_enemy_weak_percent(const State&s,int value){return value-(wr_has(s,WorldRelicPaperKrane)?15:0);}
static int wr_player_vulnerable_percent(const State&s,int value,bool pet){return value+(!pet&&wr_has(s,WorldRelicPaperPhrog)?25:0);}
static int wr_block_percent(const State&s,Card c,int id,bool powered){if(!s.world_relics.active)return 100;int factor=100;for(unsigned i=0;i<s.world_relics.count;i++){const auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;
 if(r.id==WorldRelicVitruvianMinion&&wr_minion(c))factor*=2;
 if(powered&&wr_card(s,id)&&r.id==WorldRelicPaelsLegion&&r.values[0]<=0)factor*=2;
 if(powered&&wr_card(s,id)&&r.id==WorldRelicVambrace&&!r.values[0]&&(!r.values[1]||r.values[1]==id+1))factor*=2;}return factor;}
static int wr_player_hp_loss(State&s,int amount){if(!s.world_relics.active)return amount;for(unsigned i=0;i<s.world_relics.count;i++){auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;int before=amount;
 if(r.id==WorldRelicTungstenRod)amount=max(0,amount-1);if(r.id==WorldRelicBeatingRemnant&&combat_phase(s.phase))amount=min(amount,max(0,20-int(r.values[0])));if(before!=amount)world_relic_flash(s,r.id);}return amount;}
static int wr_enemy_hp_loss(State&s,int amount,bool powered){if(powered&&amount>0&&amount<5&&wr_has(s,WorldRelicTheBoot)){world_relic_flash(s,WorldRelicTheBoot);return 5;}return amount;}
static int wr_x_value(State&s,int n){if(wr_has(s,WorldRelicChemicalX)){world_relic_flash(s,WorldRelicChemicalX);return min(9999,n+2);}return n;}
static int wr_play_count(State&s,int,int n,bool commit){if(auto*r=wr_find(s,WorldRelicThrowingAxe);s.world_relics.active&&r&&!r->values[0]){if(commit){r->values[0]=1;world_relic_flash(s,r->id);}return n+1;}return n;}
static int wr_poison_amount(State&s,int target,int n){if(!s.world_relics.active||target<0||n<=0)return n;
 for(unsigned i=0;i<s.world_relics.count;i++){auto&r=s.world_relics.entries[i];if(!wr_live(r))continue;
  if(r.id==WorldRelicSneckoSkull){world_relic_flash(s,r.id);n=min(9999,n+1);}
  else if(r.id==WorldRelicUnsettlingLamp&&!wr_relic_dispatch_depth&&!r.values[0]&&wr_card(s,s.playing)){
   if(!r.values[1])r.values[1]=s.playing+1;if(r.values[1]==s.playing+1)n=min(9999,n*2);
  }
 }return n;}
static int wr_debuff_amount(State&s,int target,EnemyPower power,int n,int source,bool internal){auto*r=wr_find(s,WorldRelicUnsettlingLamp);if(!s.world_relics.active||!r||r->values[0]||source<0||target<0||n==0||internal||wr_relic_dispatch_depth)return n;
 bool debuff=enemy_power_meta[(unsigned)power].debuff||((power==EnemyPower::Strength||power==EnemyPower::Dexterity||power==EnemyPower::TemporaryStrength)&&n<0);if(!debuff)return n;
 if(!r->values[1])r->values[1]=source+1;return r->values[1]==source+1?max(-9999,min(9999,n*2)):n;}
static bool wr_can_play(const State&s,bool){auto*r=wr_find(s,WorldRelicVelvetChoker);return !s.world_relics.active||!r||r->values[0]<6;}
static bool wr_can_draw(State&s,bool from_hand_draw){if(!from_hand_draw&&!s.expansion.enemy_side&&wr_has(s,WorldRelicFiddle)){world_relic_flash(s,WorldRelicFiddle);return false;}return true;}
static bool wr_reset_energy(const State&s){return s.turn<=1||!wr_has(s,WorldRelicIceCream);}
static bool wr_flush(const State&s){return !wr_has(s,WorldRelicRunicPyramid)&&!(wr_has(s,WorldRelicRingingTriangle)&&s.turn==1);}
static bool wr_retain_block(State&s){if(!wr_has(s,WorldRelicSturdyClamp))return false;if(s.block){s.block=(int16_t)min(s.block,10);world_relic_flash(s,WorldRelicSturdyClamp);}return true;}
static bool wr_revive(State&s){auto*r=wr_find(s,WorldRelicLizardTail);if(!s.world_relics.active||!r||r->values[0])return false;r->values[0]=1;world_relic_flash(s,r->id);heal(s,max(1,s.max_hp/2));return true;}
static bool wr_extra_turn(State&s,bool commit){auto*r=wr_find(s,WorldRelicPaelsEye);if(!s.world_relics.active||!r||r->values[0]||!r->values[1]||s.world_relics.manual_plays_turn||(s.turn==1&&wr_has(s,WorldRelicWhisperingEarring)))return false;if(commit){r->values[0]=1;world_relic_flash(s,r->id);}return true;}
static int wr_orb_value(const State&s,DOrb kind,int n){return n+(kind==DOrb::Lightning&&wr_has(s,WorldRelicInfusedCore)?1:0);}
static int wr_orb_passive_count(State&s,int slot,int n,bool commit){if(slot==0&&wr_has(s,WorldRelicGoldPlatedCables)){if(commit)world_relic_flash(s,WorldRelicGoldPlatedCables);return n+1;}return n;}
enum WRAction {WRA_FillPotions=1,WRA_GenerateDraw,WRA_GenerateHand,WRA_GenerateBatch,WRA_Exhaust,WRA_Discard,WRA_Draw,WRA_Strength,WRA_History,WRA_Whisper,WRA_OrbPassives,WRA_Delayed,WRA_TransformEgg,WRA_WhisperStars,WRA_WhisperPlay};
static int wr_frame(State&s){for(unsigned i=0;i<MaxWorldRelicHookFrames;i++)if(!s.world_relics.hook_frames[i].active)return int(i);fail(s);return -1;}
static void wr_action(State&s,unsigned owner,int op,int a=0,int b=0){int at=wr_frame(s);if(at<0)return;auto&f=s.world_relics.hook_frames[at];f=WorldRelicHookFrame{};f.active=1;f.hook=WorldRelicHook::Internal;f.count=1;f.serials[0]=uint16_t(owner);f.args[0]=op;f.args[1]=a;f.args[2]=b;queue_extension(s,XOp::WRelicHook,at,0,true);}
static int wr_generated(State&s,Card card,const EventCardFacts* facts,bool in_draw=false,bool free=false,bool retained=false){int id=make_card(s,card);if(id<0)return -1;if(facts)s.event_cards.cards[id]=*facts;
 if(free)set_free_this_turn(s,id);if(retained)s.silent.retain[id]=1;
 if(in_draw){unsigned at=world_rng(s,WorldRng::Shuffle).below(s.draw.count+1);put(s,s.draw,id,Zone::Draw);for(unsigned i=s.draw.count-1;i>at;i--)s.draw.ids[i]=s.draw.ids[i-1];s.draw.ids[at]=uint16_t(id);}
 else{bool hand=s.hand.count<MaxHand;put(s,hand?s.hand:s.discard,id,hand?Zone::Hand:Zone::Discard);}
 record(s,LogKind::Create,1,card.kind);notify_card_generated(s,id);return id;}
static int wr_clone(State&s,int source,bool ethereal=false){if(!wr_card(s,source))return -1;int id=create_to_hand(s,s.cards[source],false);if(id<0)return id;copy_card_modifiers(s,id,source);if(ethereal)s.necro.ethereal[id]=1;notify_card_generated(s,id);return id;}
static bool wr_ethereal_pool(const State&,CardKind k,const void*){return (definitions[k].keywords&8)!=0;}
static unsigned wr_generate_options(State&s,Card*cards,EventCardFacts*facts,unsigned n,int kind){WorldCardOptions o;o.odds=WorldCardOdds::Uniform;o.flags=WorldNoUpgrades|WorldNoModifyHooks|WorldNoCardPoolModifications;
 if(kind==1)o.pools[0]=WorldCardPool::Colorless;else if(kind==2)o.type_mask=world_type_mask(CardType::Attack);else if(kind==3)o.predicate=wr_ethereal_pool;
 return world_combat_cards(s,world_rng(s,WorldRng::CombatCardGeneration),n,o,true,cards,facts);}
static void wr_batch(State&s,WorldRelicInstance&r,unsigned count,int kind,bool free=false){Card cards[2];EventCardFacts facts[2];count=wr_generate_options(s,cards,facts,min(2,count),kind);for(unsigned i=0;i<count;i++){r.card_tokens[i]=uint16_t(card_token(cards[i]));r.card_facts[i]=facts[i];}if(count)wr_action(s,r.serial,WRA_GenerateBatch,0,int(count)|(free?256:0));}
static void wr_choose_hand(State&s,WorldRelicInstance&r,unsigned low,unsigned high){auto&w=s.world_relics;unsigned n=s.hand.count;if(!n){if(r.id==WorldRelicToastyMittens)world_relic_gain_strength(s,1);return;}
 w.return_phase=uint8_t(s.phase);w.choice=WorldRelicChoice::Hand;w.choice_owner=r.serial;w.choice_min=uint16_t(min(low,n));w.choice_max=uint16_t(min(high,n));w.choice_cancelable=low==0;w.offer_count=uint16_t(n);w.selected_count=0;w.choice_group=w.choice_reward=0;for(unsigned i=0;i<n;i++)w.offers[i]=s.hand.ids[i];s.phase=Phase::WorldRelicChoice;}
static void wr_choose_generated(State&s,WorldRelicInstance&r,unsigned count,int pool,bool optional){Card cards[5];EventCardFacts facts[5];count=wr_generate_options(s,cards,facts,count,pool);if(!count)return;auto&w=s.world_relics;
 w.return_phase=uint8_t(s.phase);w.choice=WorldRelicChoice::CombatOffer;w.choice_owner=r.serial;w.choice_min=optional?0:1;w.choice_max=1;w.choice_cancelable=optional;w.offer_count=uint16_t(count);w.selected_count=0;w.choice_group=w.choice_reward=0;
 for(unsigned i=0;i<count;i++){w.offers[i]=uint16_t(card_token(cards[i]));w.offer_facts[i]=facts[i];}s.phase=Phase::WorldRelicChoice;}
static void wr_buckle(State&s,WorldRelicInstance&r){bool desired=!wr_has_potions(s);if(desired!=bool(r.values[0])){r.values[0]=desired;world_relic_flash(s,r.id);world_relic_gain_dexterity(s,desired?2:-2);}}
static void wr_shuffle_draw_pick(State&s,unsigned maximum,bool zero_cost){uint16_t ids[MaxCards];unsigned n=0;for(unsigned i=0;i<s.draw.count;i++){unsigned id=s.draw.ids[i];if(!zero_cost||local_energy_cost(s,s.cards[id],id)==0)ids[n++]=uint16_t(id);}wr_sort_cards(s,ids,n);reference::shuffle(ids,n,world_rng(s,WorldRng::CombatCardSelection));
 for(unsigned i=0;i<n&&i<maximum;i++){bool hand=s.hand.count<MaxHand;move_instance(s,ids[i],s.draw,hand?s.hand:s.discard,hand?Zone::Hand:Zone::Discard);}}
static void wr_mummified(State&s){uint16_t candidates[MaxHand];unsigned n=0;for(unsigned pass=0;pass<4&&!n;pass++)for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];bool base=cost(s.cards[id])>0||regent_base_star_cost(s.cards[id])>0;bool current=cost(s,id)>0||card_star_cost(s,s.cards[id],id)>0;if((pass==0&&base&&current)||(pass==1&&current)||(pass==2&&base)||pass==3)candidates[n++]=uint16_t(id);}if(n)set_free_this_turn(s,candidates[world_rng(s,WorldRng::CombatCardSelection).below(n)]);}
static void wr_upgrade_deck(State&s,unsigned count){uint16_t ids[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i]))ids[n++]=uint16_t(i);
 for(unsigned i=1;i<n;i++){auto id=ids[i];unsigned j=i;while(j&&wr_cmp(definitions[s.deck[ids[j-1]].kind].key,definitions[s.deck[id].kind].key)>0){ids[j]=ids[j-1];--j;}ids[j]=id;}reference::shuffle(ids,n,world_rng(s,WorldRng::Niche));for(unsigned i=0;i<n&&i<count;i++){s.deck[ids[i]].upgraded=1;record(s,LogKind::Upgrade,s.deck[ids[i]].kind);}}
static void wr_internal(State&s,WorldRelicInstance&r,int op,int a,int b){switch(op){
 case WRA_FillPotions:{bool open=false;for(unsigned i=0;i<potion_slot_count(s);i++)if(potion_at(s,i)==Potion::Empty)open=true;if(!open)return;Potion p; if(world_random_potions(s,world_rng(s,WorldRng::CombatPotionGeneration),1,&p)!=1)return;
  // Queue the next iteration first so this bottle's acquisition listeners run
  // to completion before another model is rolled.
  if(wr_has(s,WorldRelicSozu)){procure_potion(s,p);break;}wr_action(s,r.serial,WRA_FillPotions);if(!procure_potion(s,p))fail(s);break;}
 case WRA_GenerateDraw:case WRA_GenerateHand:if(b>1)wr_action(s,r.serial,op,a,b-1);wr_generated(s,card_from_token(unsigned(a)),nullptr,op==WRA_GenerateDraw);break;
 case WRA_GenerateBatch:{unsigned total=unsigned(b)&255;if(unsigned(a)>=total||a>=2)return;if(unsigned(a+1)<total)wr_action(s,r.serial,op,a+1,b);wr_generated(s,card_from_token(r.card_tokens[a]),&r.card_facts[a],false,(b&256)!=0);break;}
 case WRA_Exhaust:if(wr_card(s,a)&&s.zones[a]==Zone::Hand){for(unsigned i=0;i<s.hand.count;i++)if(s.hand.ids[i]==a){exhaust(s,int(i));break;}}break;
 case WRA_Discard:if(wr_card(s,a)&&s.zones[a]==Zone::Hand){move_instance(s,a,s.hand,s.discard,Zone::Discard);record(s,LogKind::Discard,s.cards[a].kind);silent_card_discarded(s,a);}break;
 case WRA_Draw:wr_draw(s,a);break;case WRA_Strength:world_relic_gain_strength(s,a);break;
 case WRA_OrbPassives:if(a<s.expansion.orb_count){if(a+1<s.expansion.orb_count)wr_action(s,r.serial,op,a+1);queue_extension(s,XOp::DOrbPassive,a,-1,true);}break;
 case WRA_TransformEgg:{auto&w=s.world_relics;
  if(w.byrdpip_cursor==w.byrdpip_count){for(unsigned i=0;i<w.byrdpip_count;i++)w.byrdpip_cards[i]=0;w.byrdpip_count=w.byrdpip_cursor=0;w.byrdpip_present=1;break;}
  if(w.byrdpip_cursor>w.byrdpip_count||w.byrdpip_count>MaxCards){fail(s);return;}
  const unsigned id=w.byrdpip_cards[w.byrdpip_cursor++];if(!wr_card(s,int(id))||s.cards[id].kind!=EventCardByrdonisEgg){fail(s);return;}
  wr_action(s,r.serial,WRA_TransformEgg);s.cards[id]={EventCardByrdSwoop,0};reset_card_modifiers(s,int(id));
  record(s,LogKind::Transform,EventCardByrdonisEgg,EventCardByrdSwoop);notify_card_generated(s,int(id));break;}
 case WRA_History:if(wr_card(s,a)){int id=make_card(s,s.cards[a]);if(id>=0){copy_card_modifiers(s,id,a);s.world_relics.card_is_dupe[id]=1;s.zones[id]=Zone::Limbo;r.values[2]=id+1;queue_extension(s,XOp::AutoInstance,id,0,true);world_card_entered(s,id);}}break;
 case WRA_Whisper:{if(a>=13||s.turn!=1||!alive(s)){r.values[3]=0;return;}r.values[3]=1;int id=-1;
  for(unsigned i=0;i<s.hand.count;i++){int candidate=s.hand.ids[i];Card card=s.cards[candidate];int price=cost(s,candidate),stars=card_star_cost(s,card,candidate);
   if(cost(card)>=-1&&price<=s.energy&&stars<=s.expansion.stars&&silent_can_play(s,card)&&necro_can_play(s,card)&&enemy_card_can_play(s,candidate,false)&&event_card_play_allowed(s,candidate,false)&&wr_can_play(s,false)){id=candidate;break;}}
  if(id<0){r.values[3]=0;return;}int energy=cost(s,id),stars=card_star_cost(s,s.cards[id],id);energy=energy<0?s.energy:energy;stars=stars<0?s.expansion.stars:stars;
  int target=-1;if(card_targets_enemy(s,s.cards[id],id))for(int t=0;t<s.enemy_count;t++)if(enemy_allow_target(s,t)){target=t;break;}
  // SpendResources is awaited before AutoPlay(skipXCapture). Keep the raw
  // captured X amounts separate from the later automatic ResourceInfo values.
  r.values[0]=id+1;r.values[1]=energy;r.values[2]=stars;
  wr_action(s,r.serial,WRA_WhisperStars,a,target);
  if(!shared_spend_card_energy(s,energy))fail(s);break;}
 case WRA_WhisperStars:
  if(!alive(s)){r.values[3]=0;break;}
  wr_action(s,r.serial,WRA_WhisperPlay,a,b);
  if(!shared_spend_card_stars(s,int(r.values[2])))fail(s);break;
 case WRA_WhisperPlay:{
  if(!alive(s)){r.values[3]=0;break;}
  const int id=int(r.values[0])-1,captured_energy=int(r.values[1]),captured_stars=int(r.values[2]);
  wr_action(s,r.serial,WRA_Whisper,a+1);
  if(!start_instance(s,id,b,true,captured_energy,captured_stars))r.values[3]=0;
  break;}
 default:break;}}
static void wr_listener(State&s,WorldRelicInstance&r,WorldRelicHook hook,int a,int b,int c){auto&v=r.values;bool first=s.turn<=1;bool card=wr_card(s,a);CardType type=wr_type(s,a);auto flash=[&](){world_relic_flash(s,r.id);};
 switch(hook){
 case WorldRelicHook::BeforeCombatStart:
  switch(r.id){
  case WorldRelicAnchor:flash();wr_block(s,10);break;case WorldRelicFakeAnchor:flash();wr_block(s,4);break;
  case WorldRelicDelicateFrond:flash();wr_action(s,r.serial,WRA_FillPotions);break;
  case WorldRelicTeaOfDiscourtesy:if(v[0]>0){--v[0];flash();wr_action(s,r.serial,WRA_GenerateDraw,card_token({Dazed,0}),2);}break;
  case WorldRelicPhylacteryUnbound:necro_summon(s,5);break;case WorldRelicBoundPhylactery:necro_summon(s,1);break;
  case WorldRelicFurCoat:if(world_relic_fur_room(s)){flash();for(unsigned i=0;i<s.enemy_count;i++)if(enemy_allow_target(s,int(i)))s.enemies[i].hp=1;}break;
  case WorldRelicSneckoEye:case WorldRelicFakeSneckoEye:v[0]=1;break;
  case WorldRelicByrdpip:s.world_relics.byrdpip_present=1;break;case WorldRelicPaelsLegion:s.world_relics.legion_present=1;v[0]=v[1]=v[2]=0;break;
  case WorldRelicPantograph:if(world_room_type(s)==3){flash();heal(s,25);}break;
  case WorldRelicBeltBuckle:v[0]=0;wr_buckle(s,r);break;
  case WorldRelicUnsettlingLamp:case WorldRelicVambrace:case WorldRelicKusarigama:case WorldRelicLetterOpener:case WorldRelicHistoryCourse:case WorldRelicWhisperingEarring:for(auto&n:v)n=0;break;
  default:break;}break;
 case WorldRelicHook::BeforeCombatStartLate:if(r.id==WorldRelicPetrifiedToad){flash();procure_potion(s,Potion::PotionShapedRock);}break;
 case WorldRelicHook::BeforeSideTurnStart:
  if(a)break;
  switch(r.id){
  case WorldRelicRegalite:case WorldRelicBrilliantScarf:case WorldRelicVelvetChoker:case WorldRelicKunai:case WorldRelicShuriken:case WorldRelicOrnamentalFan:case WorldRelicOrichalcum:case WorldRelicFakeOrichalcum:case WorldRelicBeatingRemnant:case WorldRelicDemonTongue:case WorldRelicMiniRegent:case WorldRelicRippleBasin:v[0]=0;break;
  case WorldRelicMusicBox:v[0]=v[1]=0;break;case WorldRelicRainbowRing:v[0]=v[1]=v[2]=v[3]=0;break;
  case WorldRelicPocketwatch:v[1]=v[0];v[0]=0;break;case WorldRelicEmotionChip:v[1]=v[0];v[0]=0;break;
  case WorldRelicHistoryCourse:v[1]=v[0];v[0]=0;break;
  case WorldRelicPowerCell:if(first){flash();wr_shuffle_draw_pick(s,2,true);}break;
  case WorldRelicCrackedCore:if(first){flash();queue_extension(s,XOp::DChannel,int(DOrb::Lightning),1,true);}break;
  case WorldRelicRedMask:case WorldRelicBagOfMarbles:case WorldRelicTwistedFunnel:if(first){flash();for(int t=0;t<s.enemy_count;t++)if(enemy_allow_target(s,t)){if(r.id==WorldRelicRedMask)apply_weak(s,t,1);else if(r.id==WorldRelicBagOfMarbles)apply_vulnerable(s,t,1);else apply_poison(s,t,4);}}break;
  default:break;}break;
 case WorldRelicHook::AfterSideTurnStart:
  if(a)break;
  switch(r.id){
  case WorldRelicAkabeko:if(first){flash();s.vigor=(int16_t)min(9999,s.vigor+8);}break;
  case WorldRelicDivineDestiny:if(first){flash();regent_gain_stars(s,7);}break;
  case WorldRelicSealOfGold:if(s.gold>=3){flash();wr_energy(s,1);gain_gold(s,-3);}break;
  case WorldRelicPhylacteryUnbound:necro_summon(s,2);break;
  case WorldRelicFencingManual:if(first){flash();forge_blade(s,10);}break;
  case WorldRelicHappyFlower:case WorldRelicFakeHappyFlower:v[0]=(v[0]+1)%(r.id==WorldRelicHappyFlower?3:5);if(!v[0]){flash();wr_energy(s,1);}break;
  case WorldRelicBigHat:if(first){flash();wr_batch(s,r,2,3);}break;case WorldRelicOrangeDough:if(first){flash();wr_batch(s,r,2,1);}break;
  case WorldRelicVeryHotCocoa:if(first){flash();wr_energy(s,4);}break;
  case WorldRelicBrimstone:flash();world_relic_gain_strength(s,2);for(int t=0;t<s.enemy_count;t++)if(enemy_allow_target(s,t))enemy_apply_power(s,t,EnemyPower::Strength,1,-1);break;
  case WorldRelicDiamondDiadem:if(first){flash();wr_block(s,20);silent_gain_power(s,SPower::Blur,1);}break;
  case WorldRelicCrossbow:flash();wr_batch(s,r,1,2,true);break;
  case WorldRelicPaelsLegion:v[0]=max(-9999,int(v[0])-1);v[1]=0;break;
  case WorldRelicBoomingConch:if(first&&world_room_type(s)==2){flash();wr_energy(s,1);}break;
  case WorldRelicCandelabra:if(s.turn==2){flash();wr_energy(s,2);}break;case WorldRelicChandelier:if(s.turn==3){flash();wr_energy(s,3);}break;
  case WorldRelicSymbioticVirus:if(first)queue_extension(s,XOp::DChannel,int(DOrb::Dark),1,true);break;
  case WorldRelicPaelsTears:if(v[0]){flash();wr_energy(s,2);}break;case WorldRelicBread:if(first)wr_energy(s,-2);break;
  case WorldRelicLetterOpener:v[0]=0;break;
  case WorldRelicInfusedCore:if(first)queue_extension(s,XOp::DChannel,int(DOrb::Lightning),3,true);break;
  case WorldRelicPaelsEye:if(!v[0])v[1]=1;break;
  case WorldRelicSai:flash();wr_block(s,7);break;case WorldRelicLantern:if(first){flash();wr_energy(s,1);}break;
  case WorldRelicRunicCapacitor:if(first){flash();defect_add_orb_slots(s,3);}break;
  default:break;}break;
 case WorldRelicHook::BeforeHandDraw:
  switch(r.id){
  case WorldRelicPollinousCore:++v[0];break;case WorldRelicPendulum:v[0]=(v[0]+1)%3;if(!v[0])flash();break;
  case WorldRelicNinjaScroll:if(first){flash();wr_action(s,r.serial,WRA_GenerateHand,card_token({Shiv,0}),3);}break;
  case WorldRelicFuneraryMask:if(first){flash();wr_action(s,r.serial,WRA_GenerateDraw,card_token({Soul,0}),3);}break;
  case WorldRelicBlessedAntler:if(first){flash();wr_action(s,r.serial,WRA_GenerateDraw,card_token({Dazed,0}),3);}break;
  case WorldRelicRadiantPearl:if(first)wr_action(s,r.serial,WRA_GenerateHand,card_token({EventCardLuminesce,0}),1);break;
  case WorldRelicJeweledMask:if(first){uint16_t ids[MaxCards];unsigned n=0;for(unsigned pass=0;pass<2&&!n;pass++)for(unsigned i=0;i<s.draw.count;i++){unsigned id=s.draw.ids[i];if(wr_type(s,int(id))==CardType::Power&&(pass||!card_innate(s,s.cards[id],int(id))))ids[n++]=uint16_t(id);}if(n){int id=ids[world_rng(s,WorldRng::CombatCardSelection).below(n)];flash();set_free_this_turn(s,id);bool hand=s.hand.count<MaxHand;move_instance(s,id,s.draw,hand?s.hand:s.discard,hand?Zone::Hand:Zone::Discard);}}break;
  case WorldRelicToolbox:if(first){flash();wr_choose_generated(s,r,3,1,true);}break;
  default:break;}break;
 case WorldRelicHook::AfterHandDraw:
  switch(r.id){
  case WorldRelicMercuryHourglass:flash();wr_all_damage(s,3);break;case WorldRelicMrStruggles:flash();wr_all_damage(s,s.turn);break;
  case WorldRelicEmotionChip:if(v[1]){flash();wr_action(s,r.serial,WRA_OrbPassives,0);}break;
  case WorldRelicBellows:case WorldRelicBoneTea:if(first&&(r.id==WorldRelicBellows||v[0]>0)){if(r.id==WorldRelicBoneTea)--v[0];flash();for(unsigned i=0;i<s.hand.count;i++)if(card_upgradeable(s.cards[s.hand.ids[i]]))s.cards[s.hand.ids[i]].upgraded=1;}break;
  case WorldRelicFestivePopper:if(first){flash();wr_all_damage(s,9);}break;
  case WorldRelicVexingPuzzlebox:if(first){flash();wr_batch(s,r,1,0,true);}break;
  case WorldRelicChoicesParadox:if(first){flash();wr_choose_generated(s,r,5,0,false);}break;
  case WorldRelicRoyalPoison:if(first){flash();player_hp_loss(s,4,false,false,-1);}break;
  case WorldRelicGamblingChip:if(first)wr_choose_hand(s,r,0,MaxHand);break;
  case WorldRelicToastyMittens:flash();wr_choose_hand(s,r,1,1);break;
  default:break;}break;
 case WorldRelicHook::AfterHandDrawLate:if(first&&(r.id==WorldRelicBloodVial||r.id==WorldRelicFakeBloodVial)){flash();heal(s,r.id==WorldRelicBloodVial?2:1);}break;
 case WorldRelicHook::AfterEnergyReset:
  if(r.id==WorldRelicVenerableTeaSet||r.id==WorldRelicFakeVenerableTeaSet){if(v[0]){v[0]=0;flash();wr_energy(s,r.id==WorldRelicVenerableTeaSet?2:1);}}
  else if(r.id==WorldRelicArtOfWar){if(s.turn>1){if(!v[1]){flash();wr_energy(s,1);}v[0]=v[1]=0;}}break;
 case WorldRelicHook::AfterEnergyResetLate:if(r.id==WorldRelicBoundPhylactery&&s.turn!=1)necro_summon(s,1);break;
 case WorldRelicHook::BeforeSideTurnEndVeryEarly:if(!a&&(r.id==WorldRelicOrichalcum||r.id==WorldRelicFakeOrichalcum)&&!s.block)v[0]=1;break;
 case WorldRelicHook::BeforeSideTurnEndEarly:if(!a&&r.id==WorldRelicPaelsEye&&wr_extra_turn(s,false))for(int i=s.hand.count-1;i>=0;i--)wr_action(s,r.serial,WRA_Exhaust,s.hand.ids[i]);break;
 case WorldRelicHook::BeforeSideTurnEnd:
  if(a)break;switch(r.id){
  case WorldRelicCloakClasp:if(s.hand.count){flash();wr_block(s,s.hand.count);}break;case WorldRelicPaelsTears:v[0]=s.energy>0;break;
  case WorldRelicScreamingFlagon:if(!s.hand.count){flash();wr_all_damage(s,20);}break;
  case WorldRelicStoneCalendar:if(s.turn==7){flash();wr_all_damage(s,52);}break;
  case WorldRelicOrichalcum:case WorldRelicFakeOrichalcum:if(v[0]){v[0]=0;flash();wr_block(s,r.id==WorldRelicOrichalcum?6:3);}break;
  case WorldRelicRippleBasin:if(!v[0]){flash();wr_block(s,4);}break;default:break;}break;
 case WorldRelicHook::AfterSideTurnEnd:
  if(a)break;switch(r.id){
  case WorldRelicArtOfWar:v[1]=v[0];v[0]=0;break;case WorldRelicKusarigama:v[0]=0;break;
  case WorldRelicJossPaper:v[0]+=v[1];v[1]=0;if(v[0]>=5){flash();wr_draw(s,int(v[0]/5));v[0]%=5;}break;
  case WorldRelicParryingShield:if(s.block>=10){flash();wr_random_damage(s,6);}break;case WorldRelicLunarPastry:regent_gain_stars(s,1);break;default:break;}break;
 case WorldRelicHook::BeforeCardPlayed:
  if(!card)break;switch(r.id){
  case WorldRelicChemicalX:if(cost(s.cards[a])==-1)flash();break;
  case WorldRelicIntimidatingHelmet:if(b>=2){flash();wr_block(s,4);}break;
  case WorldRelicPenNib:if(type==CardType::Attack){v[0]=(v[0]+1)%10;if(!v[0]){v[1]=a+1;flash();}}break;
  case WorldRelicMusicBox:if(type==CardType::Attack&&!v[0]&&!v[1])v[1]=a+1;break;default:break;}break;
 case WorldRelicHook::AfterCardPlayed:
  if(!card)break;switch(r.id){
  case WorldRelicNunchaku:if(type==CardType::Attack){v[0]=(v[0]+1)%10;if(!v[0]){flash();wr_energy(s,1);}}break;
  case WorldRelicPenNib:if(v[1]==a+1)v[1]=0;break;
  case WorldRelicBrilliantScarf:if(!(c&0x10000))++v[0];break;
  case WorldRelicDaughterOfTheWind:if(type==CardType::Attack){flash();wr_block(s,1);}break;
  case WorldRelicPaelsLegion:if(v[2]&&uint32_t(v[2])==wr_play_serial(s)){flash();v[2]=0;v[0]=2;v[1]=1;}break;
  case WorldRelicIvoryTile:if(b>=3){flash();wr_energy(s,1);}break;
  case WorldRelicIronClub:v[0]=(v[0]+1)%4;if(!v[0]){flash();wr_draw(s,1);}break;
  case WorldRelicVelvetChoker:case WorldRelicPocketwatch:++v[0];break;
  case WorldRelicMusicBox:if(v[1]==a+1){v[0]=1;v[1]=0;flash();wr_clone(s,a,true);}break;
  case WorldRelicRainbowRing:if(!v[3]){if(type==CardType::Attack)v[0]=1;if(type==CardType::Skill)v[1]=1;if(type==CardType::Power)v[2]=1;if(v[0]&&v[1]&&v[2]){flash();world_relic_gain_strength(s,1);world_relic_gain_dexterity(s,1);v[3]=1;}}break;
  case WorldRelicUnsettlingLamp:if(!v[0]&&v[1]==a+1){v[0]=1;flash();}break;
  case WorldRelicLostWisp:if(type==CardType::Power){flash();wr_all_damage(s,8);}break;
  case WorldRelicGamePiece:if(type==CardType::Power){flash();wr_draw(s,1);}break;
  case WorldRelicArtOfWar:if(type==CardType::Attack&&!v[1])v[0]=1;break;
  case WorldRelicMummifiedHand:if(type==CardType::Power)wr_mummified(s);break;
  case WorldRelicKunai:case WorldRelicShuriken:case WorldRelicOrnamentalFan:case WorldRelicKusarigama:if(type==CardType::Attack){v[0]=(v[0]+1)%3;if(!v[0]){flash();if(r.id==WorldRelicKunai)world_relic_gain_dexterity(s,1);else if(r.id==WorldRelicShuriken)world_relic_gain_strength(s,1);else if(r.id==WorldRelicOrnamentalFan)wr_block(s,4);else wr_random_damage(s,6);}}break;
  case WorldRelicTuningFork:if(type==CardType::Skill){++v[0];if(v[0]>=10){flash();v[0]-=10;wr_block(s,7);}}break;
  case WorldRelicPermafrost:if(type==CardType::Power&&!v[0]){v[0]=1;flash();wr_block(s,7);}break;
  case WorldRelicLetterOpener:if(type==CardType::Skill){v[0]=(v[0]+1)%3;if(!v[0]){flash();wr_all_damage(s,5);}}break;
  case WorldRelicRippleBasin:if(type==CardType::Attack)v[0]=1;break;
  case WorldRelicRazorTooth:if((type==CardType::Attack||type==CardType::Skill)&&card_upgradeable(s.cards[a]))s.cards[a].upgraded=1;break;
  case WorldRelicVambrace:if(v[1]==a+1&&!v[0])v[0]=1;break;
  case WorldRelicHelicalDart:if(s.cards[a].kind==Shiv){flash();s.world_relics.helical_dart=(int16_t)min(9999,s.world_relics.helical_dart+1);world_relic_gain_dexterity(s,1);}break;
  case WorldRelicHistoryCourse:if(type==CardType::Attack&&!s.world_relics.card_is_dupe[a])v[0]=a+1;break;default:break;}break;
 case WorldRelicHook::AfterCardExhausted:
  if(!card)break;switch(r.id){
  case WorldRelicForgottenSoul:flash();wr_random_damage(s,1);break;
  case WorldRelicBurningSticks:if(!v[0]&&type==CardType::Skill){v[0]=1;flash();wr_clone(s,a);}break;
  case WorldRelicJossPaper:if(b)++v[1];else{++v[0];if(v[0]>=5){flash();wr_draw(s,int(v[0]/5));v[0]%=5;}}break;
  case WorldRelicCharonsAshes:flash();wr_all_damage(s,3);break;default:break;}break;
 case WorldRelicHook::AfterCardDiscarded:if(!s.expansion.enemy_side){if(r.id==WorldRelicTingsha){flash();wr_random_damage(s,3);}else if(r.id==WorldRelicToughBandages){flash();wr_block(s,3);}}break;
 case WorldRelicHook::AfterCardGenerated:if(r.id==WorldRelicRegalite&&!v[0]){v[0]=1;flash();wr_block(s,4);}break;
 case WorldRelicHook::AfterHandEmptied:if(r.id==WorldRelicUnceasingTop&&!s.expansion.enemy_side&&!s.hand.count){flash();wr_draw(s,1);}break;
 case WorldRelicHook::AfterShuffle:if(r.id==WorldRelicTheAbacus){flash();wr_block(s,6);}else if(r.id==WorldRelicBiiigHug){flash();wr_action(s,r.serial,WRA_GenerateDraw,card_token({EventCardSoot,0}),1);}break;
 case WorldRelicHook::AfterAttack:if(r.id==WorldRelicBoneFlute&&a==-2){flash();wr_block(s,2);}break;
 case WorldRelicHook::AfterBlock:
  if(a<=0||b!=-1||!wr_card(s,c))break;
  if(r.id==WorldRelicVambrace&&!v[0]&&(!v[1]||v[1]==c+1)){flash();v[1]=c+1;}
  if(r.id==WorldRelicPaelsLegion&&v[0]<=0&&(!v[2]||uint32_t(v[2])==wr_play_serial(s)))v[2]=int32_t(wr_play_serial(s));break;
 case WorldRelicHook::AfterBlockCleared:
  if(a!=-1)break;switch(r.id){case WorldRelicHornCleat:if(s.turn==2){flash();wr_block(s,14);}break;case WorldRelicCaptainsWheel:if(s.turn==3){flash();wr_block(s,18);}break;case WorldRelicSparklingRouge:if(s.turn==3){flash();world_relic_gain_strength(s,1);world_relic_gain_dexterity(s,1);}break;default:break;}break;
 case WorldRelicHook::AfterBlockBroken:if(a>=0&&b<0&&r.id==WorldRelicHandDrill){flash();apply_vulnerable(s,a,2);}break;
 case WorldRelicHook::AfterHpLost:
  if(a<=0||!(b&4))break;switch(r.id){
  case WorldRelicEmotionChip:v[0]=1;flash();break;
  case WorldRelicCentennialPuzzle:if(!v[0]){v[0]=1;flash();wr_draw(s,3);}break;
  case WorldRelicLavaLamp:if(b&2)v[0]=1;break;
  case WorldRelicSelfFormingClay:s.world_relics.self_forming_clay=(int16_t)min(9999,s.world_relics.self_forming_clay+3);break;
  case WorldRelicBeatingRemnant:v[0]=min(9999,int(v[0])+a);break;
  case WorldRelicDemonTongue:if(!s.expansion.enemy_side&&!v[0]){v[0]=1;flash();heal(s,a);}break;default:break;}break;
 case WorldRelicHook::AfterEnemyDeath:if(r.id==WorldRelicGremlinHorn){flash();wr_energy(s,1);wr_draw(s,1);}break;
 case WorldRelicHook::AfterEnemyAdded:
  if(a<0||a>=s.enemy_count)break;if(r.id==WorldRelicFurCoat&&world_relic_fur_room(s)){flash();s.enemies[a].hp=1;}else if(r.id==WorldRelicPhilosophersStone){flash();enemy_apply_power(s,a,EnemyPower::Strength,1,-1);}break;
 case WorldRelicHook::AfterOrbChanneled:if(r.id==WorldRelicMetronome&&++v[0]==7){flash();wr_all_damage(s,30);}break;
 case WorldRelicHook::AfterStarsSpent:
  if(r.id==WorldRelicMiniRegent&&!v[0]){v[0]=1;flash();world_relic_gain_strength(s,1);}
  if(r.id==WorldRelicGalacticDust){v[0]+=max(0,a);if(v[0]>=10){flash();wr_block(s,int(v[0]/10)*10);v[0]%=10;}}break;
 case WorldRelicHook::AfterPotionUsed:case WorldRelicHook::AfterPotionProcured:case WorldRelicHook::AfterPotionDiscarded:
  if(!combat_phase(s.phase))break;if(r.id==WorldRelicBeltBuckle)wr_buckle(s,r);
  else if(r.id==WorldRelicReptileTrinket&&hook==WorldRelicHook::AfterPotionUsed){flash();s.world_relics.reptile_trinket=(int16_t)min(9999,s.world_relics.reptile_trinket+3);world_relic_gain_strength(s,3);}break;
 case WorldRelicHook::AfterFlush:
  if(r.id==WorldRelicBookmark){uint16_t ids[MaxHand];unsigned n=0;for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(cost(s.cards[id])>=0&&local_energy_cost(s,s.cards[id],id)>0)ids[n++]=uint16_t(id);}if(n){flash();int id=ids[world_rng(s,WorldRng::CombatCardSelection).below(n)];s.silent.discount[id]=(uint16_t)min(9999,s.silent.discount[id]+1);}}break;
 case WorldRelicHook::AfterDiedToDoom:if(r.id==WorldRelicBookRepairKnife&&a>0){flash();heal(s,a*3);}break;
 case WorldRelicHook::AfterAutoPrePlay:
  if(r.id==WorldRelicHistoryCourse&&s.turn>1&&v[1]>0&&wr_card(s,int(v[1])-1)){flash();wr_action(s,r.serial,WRA_History,int(v[1])-1);}break;
 case WorldRelicHook::AfterAutoPrePlayLate:if(r.id==WorldRelicWhisperingEarring&&first){flash();wr_action(s,r.serial,WRA_Whisper,0);}break;

 case WorldRelicHook::AfterCombatVictoryEarly:if(r.id==WorldRelicMeatOnTheBone&&s.hp>0&&s.hp<=s.max_hp/2){flash();heal(s,12);}break;
 case WorldRelicHook::AfterCombatVictory:
  switch(r.id){
  case WorldRelicWarHammer:if(world_room_type(s)==2){flash();wr_upgrade_deck(s,4);}break;
  case WorldRelicSwordOfStone:if(world_room_type(s)==2){++v[0];flash();if(v[0]>=5)world_relic_replace(s,r.serial,WorldRelicSwordOfJade);}break;
  case WorldRelicBurningBlood:flash();heal(s,6);break;case WorldRelicBlackBlood:flash();heal(s,12);break;case WorldRelicBeltBuckle:v[0]=0;break;default:break;}break;
 case WorldRelicHook::AfterCombatEnd:
  switch(r.id){
  case WorldRelicRegalite:case WorldRelicCentennialPuzzle:case WorldRelicBrilliantScarf:case WorldRelicBurningSticks:case WorldRelicRuinedHelmet:
  case WorldRelicPaelsLegion:case WorldRelicVelvetChoker:case WorldRelicMusicBox:case WorldRelicRainbowRing:case WorldRelicPaelsTears:case WorldRelicThrowingAxe:case WorldRelicUnsettlingLamp:case WorldRelicPocketwatch:case WorldRelicArtOfWar:case WorldRelicMetronome:case WorldRelicKunai:case WorldRelicShuriken:case WorldRelicPaelsEye:case WorldRelicRippleBasin:case WorldRelicOrnamentalFan:case WorldRelicKusarigama:case WorldRelicVambrace:case WorldRelicMiniRegent:case WorldRelicLetterOpener:case WorldRelicHistoryCourse:case WorldRelicWhisperingEarring:case WorldRelicEmotionChip:for(auto&n:v)n=0;break;
  case WorldRelicChosenCheese:flash();increase_max_hp(s,1);break;
  case WorldRelicFishingRod:if(world_room_type(s)==1&&++v[0]%3==0){flash();uint16_t ids[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i]))ids[n++]=uint16_t(i);if(n)s.deck[ids[world_rng(s,WorldRng::Niche).below(n)]].upgraded=1;}break;
  case WorldRelicPaelsTooth:if(s.hp>0&&s.world_relics.tooth_count){auto&w=s.world_relics;unsigned chosen=world_rng(s,WorldRng::Rewards).below(w.tooth_count);auto snap=w.tooth_cards[chosen];for(unsigned i=chosen+1;i<w.tooth_count;i++)w.tooth_cards[i-1]=w.tooth_cards[i];w.tooth_cards[--w.tooth_count]=WorldRelicCardSnapshot{};
   EventDeckSnapshot card{};card.card=card_from_token(snap.token);card.facts=snap.facts;card.genetic=snap.genetic;card.scythe=snap.scythe;if(card_upgradeable(card.card))card.card.upgraded=1;flash();int added=event_restore_deck_card(s,card);if(added>=0)world_relic_hook(s,WorldRelicHook::AfterDeckAdded,added);}break;
  case WorldRelicToyBox:if(v[0]<15&&++v[0]%3==0){flash();for(unsigned i=0;i<s.world_relics.count;i++){auto&wax=s.world_relics.entries[i];if((wax.flags&WorldRelicWax)&&!(wax.flags&WorldRelicMelted)){wax.flags|=WorldRelicMelted;break;}}}break;
  case WorldRelicJossPaper:v[1]=0;break;case WorldRelicPenNib:v[1]=0;break;default:break;}break;
 default:break;}
}
static void wr_dispatch(State&s,unsigned serial,WorldRelicHook hook,int a,int b,int c){int at=world_relic_instance_index(s,serial);if(at<0||!wr_live(s.world_relics.entries[at]))return;
 WRDispatchScope scope;
 if(hook==WorldRelicHook::Internal){wr_internal(s,s.world_relics.entries[at],a,b,c);return;}
 world_relic_run_hook(s,serial,hook,a,b,c);at=world_relic_instance_index(s,serial);if(at>=0&&wr_live(s.world_relics.entries[at]))wr_listener(s,s.world_relics.entries[at],hook,a,b,c);}
void world_relic_schedule_acquisition(State&s){auto&w=s.world_relics;int index=-1;
 for(unsigned i=0;i<MaxWorldRelicHookFrames;i++)if(w.hook_frames[i].active&&w.hook_frames[i].hook==WorldRelicHook::Acquisition){index=int(i);break;}
 if(index<0){index=wr_frame(s);if(index<0)return;auto&f=w.hook_frames[index];f=WorldRelicHookFrame{};f.active=1;f.hook=WorldRelicHook::Acquisition;}
 // Nested acquisition must precede the suspended parent's next listener.
 // Move its existing marker instead of creating two independent pumps.
 for(unsigned i=0;i<s.effect_count;i++)if(s.effects[i].kind==EffectKind::Extended&&s.effects[i].target==int(XOp::WRelicHook)&&s.effects[i].value==index){for(unsigned j=i+1;j<s.effect_count;j++)s.effects[j-1]=s.effects[j];s.effects[--s.effect_count]={};break;}
 queue_extension(s,XOp::WRelicHook,index,0,true);
}
void world_relic_effect(State&s,int index){if(index<0||index>=int(MaxWorldRelicHookFrames))return;auto&f=s.world_relics.hook_frames[index];if(!f.active)return;
 if(f.hook==WorldRelicHook::Acquisition){f=WorldRelicHookFrame{};world_relic_acquisition_resume(s);return;}
 while(f.active&&s.world_relics.choice==WorldRelicChoice::None&&s.phase!=Phase::Dead&&s.phase!=Phase::Error){
  if(f.cursor==f.count){f=WorldRelicHookFrame{};return;}
  unsigned serial=f.serials[f.cursor++];auto hook=f.hook;int a=f.args[0],b=f.args[1],c=f.args[2];bool combat=combat_phase(s.phase);
  if(combat&&f.cursor<f.count)queue_extension(s,XOp::WRelicHook,index,0,true);
  if(f.cursor==f.count)f=WorldRelicHookFrame{};
  wr_dispatch(s,serial,hook,a,b,c);
  if(combat)return;
 }
}
void world_relic_hook(State&s,WorldRelicHook hook,int a,int b,int c){if(!s.world_relics.active||s.phase==Phase::Error)return;auto&w=s.world_relics;
 // These listeners only act in combat. A reward menu may stay open while a
 // bottle is used or discarded; do not accumulate empty suspended callbacks.
 const bool suspended_combat=s.phase==Phase::WorldRelicChoice&&combat_phase(Phase(w.return_phase));
 if(!combat_phase(s.phase)&&!suspended_combat&&(hook==WorldRelicHook::AfterPotionUsed||hook==WorldRelicHook::AfterPotionProcured||hook==WorldRelicHook::AfterPotionDiscarded||hook==WorldRelicHook::AfterHpChanged))return;
 // All four original pile listeners require the permanent Deck. This engine
 // represents it with AfterDeckAdded/Removed, never with a combat Zone.
 if(hook==WorldRelicHook::AfterCardChangedPiles)return;
 if(hook==WorldRelicHook::BeforeCombatStart){w.self_forming_clay=w.reptile_trinket=w.helical_dart=0;w.byrdpip_present=w.legion_present=0;w.manual_plays_turn=0;for(auto&v:w.card_is_dupe)v=0;}
 if(hook==WorldRelicHook::BeforeSideTurnStart&&!a)w.manual_plays_turn=0;
 if(hook==WorldRelicHook::AfterCardPlayed&&!(c&0x10000))w.manual_plays_turn=(uint16_t)min(9999,w.manual_plays_turn+1);
 // Confused is a single power even if both the real and fake eye are owned.
 if(hook==WorldRelicHook::AfterCardDrawn&&wr_card(s,a)&&cost(s.cards[a])>=0&&(wr_has(s,WorldRelicSneckoEye)||wr_has(s,WorldRelicFakeSneckoEye))){s.expansion.combat_cost[a]=uint8_t(world_rng(s,WorldRng::CombatEnergyCosts).below(4)+1);s.expansion.turn_cost[a]=0;combat_cost_set(s,a);}
 if(hook==WorldRelicHook::AfterCardDrawn)return;
 int at=wr_frame(s);if(at<0)return;auto&f=w.hook_frames[at];f=WorldRelicHookFrame{};f.active=1;f.hook=hook;f.args[0]=a;f.args[1]=b;f.args[2]=c;
 for(unsigned i=0;i<w.count;i++)if(wr_live(w.entries[i]))f.serials[f.count++]=w.entries[i].serial;
 if(!f.count){f=WorldRelicHookFrame{};return;}
 if(combat_phase(s.phase))queue_extension(s,XOp::WRelicHook,at,0,true);else world_relic_effect(s,at);
}
void world_relic_resume(State&s){if(s.world_relics.choice!=WorldRelicChoice::None||s.phase==Phase::Error||s.phase==Phase::Dead)return;
 for(int i=int(MaxWorldRelicHookFrames)-1;i>=0;i--)if(s.world_relics.hook_frames[i].active){bool queued=false;for(unsigned j=0;j<s.effect_count;j++)if(s.effects[j].kind==EffectKind::Extended&&s.effects[j].target==int(XOp::WRelicHook)&&s.effects[j].value==i)queued=true;
  if(!queued){if(combat_phase(s.phase))queue_extension(s,XOp::WRelicHook,i,0,true);else world_relic_effect(s,i);}if(s.world_relics.choice!=WorldRelicChoice::None)return;}
}
void world_relic_combat_acquired(State&s,unsigned serial){if(!combat_phase(s.phase))return;int at=world_relic_instance_index(s,serial);if(at<0)return;auto&r=s.world_relics.entries[at];switch(r.id){
 case WorldRelicSneckoEye:case WorldRelicFakeSneckoEye:r.values[0]=1;break;
 case WorldRelicBeltBuckle:wr_buckle(s,r);break;case WorldRelicPaelsLegion:s.world_relics.legion_present=1;break;
 case WorldRelicByrdpip:{auto&w=s.world_relics;if(w.byrdpip_count){fail(s);return;}
  auto capture=[&](unsigned id){if(id<s.card_count&&s.cards[id].kind==EventCardByrdonisEgg){if(w.byrdpip_count>=MaxCards){fail(s);return;}w.byrdpip_cards[w.byrdpip_count++]=uint16_t(id);}};
  for(unsigned i=0;i<s.hand.count;i++)capture(s.hand.ids[i]);
  for(unsigned i=s.draw.count;i>0;i--)capture(s.draw.ids[i-1]);
  for(unsigned i=0;i<s.discard.count;i++)capture(s.discard.ids[i]);
  for(unsigned i=0;i<s.exhaust.count;i++)capture(s.exhaust.ids[i]);
  for(unsigned id=0;id<s.card_count;id++)if(s.zones[id]==Zone::Limbo)capture(id);
  if(w.byrdpip_count)wr_action(s,r.serial,WRA_TransformEgg);else w.byrdpip_present=1;break;}
 default:break;}}
void world_relic_combat_select(State&s,unsigned serial,const uint16_t*ids,unsigned count){int at=world_relic_instance_index(s,serial);if(at<0)return;auto&r=s.world_relics.entries[at];
 if(r.id==WorldRelicToolbox||r.id==WorldRelicChoicesParadox){if(count){unsigned index=ids[0];if(index<s.world_relics.offer_count){Card card=card_from_token(s.world_relics.offers[index]);EventCardFacts facts=s.world_relics.offer_facts[index];wr_generated(s,card,&facts,false,false,r.id==WorldRelicChoicesParadox);}}return;}
 if(r.id==WorldRelicGamblingChip){if(count){wr_action(s,serial,WRA_Draw,int(count));for(int i=int(count)-1;i>=0;i--)wr_action(s,serial,WRA_Discard,ids[i]);}}
 else if(r.id==WorldRelicToastyMittens){wr_action(s,serial,WRA_Strength,1);if(count)wr_action(s,serial,WRA_Exhaust,ids[0]);}
}
static void wr_power_after_block_clear(State&s){if(!s.world_relics.active)return;int n=s.world_relics.self_forming_clay;s.world_relics.self_forming_clay=0;if(n)wr_block(s,n);}
static void wr_power_side_end(State&s,int side){if(!s.world_relics.active||side)return;auto&w=s.world_relics;int strength=w.reptile_trinket,dexterity=w.helical_dart;w.reptile_trinket=w.helical_dart=0;if(strength)world_relic_gain_strength(s,-strength);if(dexterity)world_relic_gain_dexterity(s,-dexterity);}
static bool wr_automatic_choice(State&s){const auto*r=wr_find(s,WorldRelicWhisperingEarring);if(!s.world_relics.active||!r||!r->values[3]||!choice_phase(s.phase))return false;
 uint16_t ids[MaxWorldRelicOffers];unsigned n=0;
 if(s.phase==Phase::WorldRelicChoice){if(s.world_relics.choice==WorldRelicChoice::Potion)return false;unsigned available=world_relic_eligible(s,ids,MaxWorldRelicOffers);n=min(available,s.world_relics.choice_max);return world_relic_select(s,ids,n);}
 if(s.phase==Phase::ExhaustChoice)return choose_exhaust(s,0);
 if(s.phase==Phase::ReturnChoice)return s.discard.count&&choose_return_to_draw(s,s.discard.ids[0]);
 if(s.phase==Phase::ExtendedChoice){const auto&x=s.expansion;if(x.choice==XChoice::Offer){ids[0]=MaxCards;n=1;}else{n=min(x.choice_max,x.choices.count);for(unsigned i=0;i<n;i++)ids[i]=x.choices.ids[i];}return choose_extended(s,ids,n);}
 n=min(choice_max(s),s.hand.count);for(unsigned i=0;i<n;i++)ids[i]=s.hand.ids[i];if(s.phase==Phase::DiscardChoice)return choose_discard(s,ids,n);if(s.phase==Phase::ExhaustManyChoice)return choose_exhaust_many(s,ids,n);return false;}
