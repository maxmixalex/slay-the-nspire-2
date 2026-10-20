// Independently implemented event/ancient card rules for v0.111.0 / 41cef1ea.
static bool ec_word(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int ec_power(const State&s,EventCardPower p){return s.event_cards.powers[unsigned(p)];}
static bool ec_new_card(unsigned k){return k>=EventCardBadLuck&&k<CardKinds&&!(k>=EventCardBeckon&&k<=EventCardWither);}
static EventCardFacts ec_default(Card c){EventCardFacts f{};if(c.kind==EventCardSpoilsMap)f.progress=1;return f;}
const EventCardFacts* event_card_facts(const State&s,Card c,int id,int deck){
 if(id>=0&&id<s.card_count&&s.cards[id].kind==c.kind)return &s.event_cards.cards[id];
 if(deck>=0&&deck<s.deck_count&&s.deck[deck].kind==c.kind)return &s.event_cards.deck[deck];
 return nullptr;
}
static EventEnchantment ec_enchantment(const State&s,Card c,int id=-1,int deck=-1){const auto*f=event_card_facts(s,c,id,deck);return f?f->enchantment:EventEnchantment::None;}
CardColor event_card_color(unsigned k){
 switch(k){
 case EventCardClash:case EventCardDualWield:case EventCardEntrench:return CardColor::Ironclad;
 case EventCardCaltrops:case EventCardDistraction:case EventCardOutmaneuver:return CardColor::Silent;
 case EventCardHelloWorld:case EventCardRebound:case EventCardRipAndTear:case EventCardStack:return CardColor::Defect;
 default:return definitions[k].type==CardType::Curse||definitions[k].type==CardType::Status?CardColor::Token:CardColor::Colorless;
 }
}
bool event_card_type(const State&s,Card c,int id,int deck,CardType&out){
 if(c.kind!=EventCardMadScience)return false;const auto*f=event_card_facts(s,c,id,deck);out=f?CardType(f->science_type):CardType::Attack;return true;
}
bool event_card_targeted(const State&s,Card c,int id,int deck,bool&out){CardType t;if(!event_card_type(s,c,id,deck,t))return false;out=t==CardType::Attack;return true;}
const char* event_card_portrait(const State&s,Card c,int id,int deck){
 if(c.kind!=EventCardMadScience)return nullptr;CardType t=CardType::Attack;event_card_type(s,c,id,deck,t);
 return t==CardType::Skill?"event/mad_science_skill":t==CardType::Power?"event/mad_science_power":"event/mad_science_attack";
}
uint8_t event_card_keywords(const State&s,Card c,int id,int deck,uint8_t bits){
 switch(ec_enchantment(s,c,id,deck)){
 case EventEnchantment::Goopy:bits|=1;break;
 case EventEnchantment::SoulsPower:bits&=uint8_t(~1U);break;
 case EventEnchantment::Steady:bits|=4;break;
 case EventEnchantment::RoyallyApproved:bits|=6;break;
 case EventEnchantment::TezcatarasEmber:bits|=64;break;
 default:break;
 }return bits;
}
bool event_card_gains_block(const State&s,Card c,int id,int deck){
 if(c.kind==EventCardMadScience){CardType t;event_card_type(s,c,id,deck,t);return t==CardType::Skill;}
 return world_card_gains_block(c.kind);
}
const char* event_enchantment_key(EventEnchantment e){
 static const char* const keys[]={"","CORRUPTED","GOOPY","INSTINCT","NIMBLE","PERFECT_FIT","SHARP","SLITHER","SOULS_POWER","SOWN","SPIRAL","STEADY","SWIFT","VIGOROUS","ADROIT","CLONE","GLAM","IMBUED","MOMENTUM","ROYALLY_APPROVED","TEZCATARAS_EMBER"};
 return unsigned(e)<EventEnchantmentKinds?keys[unsigned(e)]:"";
}
static bool ec_defend_tag(Card c){return c.kind==Defend||c.kind==DefendSilent||c.kind==DefectDefend||c.kind==RegentDefend||c.kind==NecroDefendNecrobinder||c.kind==UltimateDefend;}
bool event_card_can_enchant(const State&s,int deck,EventEnchantment e){
 if(deck<0||deck>=s.deck_count||e==EventEnchantment::None||unsigned(e)>=EventEnchantmentKinds)return false;
 if(card_keywords(s,s.deck[deck],-1,deck)&128)return false;
 return event_card_can_enchant(s,s.deck[deck],s.event_cards.deck[deck],e);
}
bool event_card_can_enchant(const State&,Card c,const EventCardFacts&f,EventEnchantment e){
 if(c.kind>=CardKinds||e==EventEnchantment::None||unsigned(e)>=EventEnchantmentKinds)return false;
 CardType t=c.kind==EventCardMadScience?CardType(f.science_type):definitions[c.kind].type;
 uint8_t words=c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords;if(definitions[c.kind].exhausts)words|=1;
 if(f.enchantment!=EventEnchantment::None||t==CardType::Curse||t==CardType::Status||t==CardType::Quest)return false;
 switch(e){
 case EventEnchantment::Corrupted:case EventEnchantment::Instinct:case EventEnchantment::Sharp:case EventEnchantment::Vigorous:case EventEnchantment::Momentum:return t==CardType::Attack;
 case EventEnchantment::Goopy:return ec_defend_tag(c);
 case EventEnchantment::Nimble:return c.kind==EventCardMadScience?t==CardType::Skill:world_card_gains_block(c.kind);
 case EventEnchantment::Slither:return cost(c)!=-1;
 case EventEnchantment::SoulsPower:return (words&1)!=0;
 case EventEnchantment::Spiral:return card_rarity(c.kind)==1&&(card_strike_tag(c.kind)||ec_defend_tag(c));
 case EventEnchantment::Imbued:return t==CardType::Skill;
 case EventEnchantment::RoyallyApproved:return t==CardType::Attack||t==CardType::Skill;
 default:return true;
 }
}
bool event_card_apply_enchantment(Card&c,EventCardFacts&f,EventEnchantment e,int amount){
 if(c.kind>=CardKinds||f.enchantment!=EventEnchantment::None||e==EventEnchantment::None||unsigned(e)>=EventEnchantmentKinds||amount<0||amount>9999)return false;
 f.enchantment=e;f.enchantment_amount=(int16_t)amount;if(e==EventEnchantment::TezcatarasEmber)f.energy_offset=(int16_t)-max(0,cost(c));return true;
}
bool event_card_enchant(State&s,int deck,EventEnchantment e,int amount){
 if(!event_card_can_enchant(s,deck,e)||amount<0||amount>9999)return false;
 if(!event_card_apply_enchantment(s.deck[deck],s.event_cards.deck[deck],e,amount))return false;
 ++s.revision;return true;
}
int event_add_deck_card(State&s,Card c,const EventCardFacts*facts){
 if(c.kind>=CardKinds||c.upgraded>1||s.deck_count>=MaxDeck)return -1;
 int index=s.deck_count++;s.deck[index]=c;notify_deck_replaced(s,index);s.event_cards.deck[index]=facts?*facts:ec_default(c);
 record(s,LogKind::Reward,c.kind,c.upgraded);world_relic_hook(s,WorldRelicHook::AfterDeckAdded,index);return index;
}
static bool ec_remove_deck(State&s,int index,bool hooks){
 if(index<0||index>=s.deck_count)return false;Card removed=s.deck[index];
 notify_deck_removed(s,index);for(int i=index+1;i<s.deck_count;++i)s.deck[i-1]=s.deck[i];s.deck[--s.deck_count]={};
 record(s,LogKind::Remove,removed.kind);if(hooks)world_relic_hook(s,WorldRelicHook::AfterDeckRemoved,removed.kind,index);return true;
}
bool event_remove_deck_card(State&s,int index){return index>=0&&index<s.deck_count&&card_removable(s,s.deck[index],index)&&ec_remove_deck(s,index,true);}
bool event_replace_deck_card(State&s,int index,Card c,const EventCardFacts*facts){
 if(index<0||index>=s.deck_count||c.kind>=CardKinds||!card_transformable(s,s.deck[index],index))return false;
 Card old=s.deck[index];notify_deck_replaced(s,index);s.deck[index]=c;s.event_cards.deck[index]=facts?*facts:ec_default(c);record(s,LogKind::Transform,old.kind,c.kind);return true;
}
bool event_take_deck_card(State&s,int index,EventDeckSnapshot&snap){
 if(index<0||index>=s.deck_count)return false;snap={s.deck[index],s.event_cards.deck[index],s.defect.genetic_deck[index],s.necro.scythe_deck[index]};return ec_remove_deck(s,index,false);
}
int event_restore_deck_card(State&s,const EventDeckSnapshot&snap){
 if(s.deck_count>=MaxDeck||snap.card.kind>=CardKinds)return -1;int index=s.deck_count++;s.deck[index]=snap.card;notify_deck_replaced(s,index);s.event_cards.deck[index]=snap.facts;s.defect.genetic_deck[index]=snap.genetic;s.necro.scythe_deck[index]=snap.scythe;return index;
}
bool event_card_hatch(State&s,int deck){return deck>=0&&deck<s.deck_count&&s.deck[deck].kind==EventCardByrdonisEgg&&gain_world_relic(s,world_relic_id("BYRDPIP"));}
bool event_card_spoils_complete(State&s,int deck){
 if(deck<0||deck>=s.deck_count||s.deck[deck].kind!=EventCardSpoilsMap)return false;
 gain_gold(s,600);return event_remove_deck_card(s,deck);
}
int event_card_spoils_act(const State&s){for(unsigned i=0;i<s.deck_count;++i)if(s.deck[i].kind==EventCardSpoilsMap)return s.event_cards.deck[i].progress;return -1;}
bool event_card_forces_historian(const State&s,unsigned act){if(act!=2)return false;for(unsigned i=0;i<s.deck_count;++i)if(s.deck[i].kind==EventCardLanternKey)return true;return false;}
void event_card_before_room(State&s,bool unknown,bool first){
 if(!unknown||!first)return;for(unsigned i=0;i<s.deck_count;++i)if(s.deck[i].kind==EventCardDowsing&&++s.event_cards.deck[i].progress>=5)event_replace_deck_card(s,i,{EventCardAbundance,0});
}
static void event_card_reset(State&s,int id){if(id<0||id>=MaxCards)return;auto&v=s.event_cards;v.cards[id]=ec_default(id<s.card_count?s.cards[id]:Card{});v.deck_origin[id]=0;v.enchantment_disabled[id]=0;v.momentum[id]=v.maul_damage[id]=0;}
static void event_card_created(State&s,int id){event_card_reset(s,id);}
static void event_card_copied(State&s,int to,int from){
 if(to<0||to>=s.card_count||from<0||from>=s.card_count)return;auto&v=s.event_cards;v.cards[to]=v.cards[from];v.enchantment_disabled[to]=v.enchantment_disabled[from];v.momentum[to]=v.momentum[from];v.maul_damage[to]=v.maul_damage[from];v.deck_origin[to]=0;
}
static void event_card_deck_created(State&s,int id,int deck){if(id<0||id>=s.card_count||deck<0||deck>=s.deck_count)return;s.event_cards.cards[id]=s.event_cards.deck[deck];s.event_cards.deck_origin[id]=(int16_t)(deck+1);}
static void event_card_deck_removed(State&s,int index){
 auto&v=s.event_cards;for(int i=index+1;i<s.deck_count;++i)v.deck[i-1]=v.deck[i];v.deck[s.deck_count-1]={};
 for(auto&n:v.deck_origin){if(n==index+1)n=0;else if(n>index+1)--n;}
}
static void event_card_deck_replaced(State&s,int index){if(index<0||index>=MaxDeck)return;s.event_cards.deck[index]=ec_default(s.deck[index]);for(auto&n:s.event_cards.deck_origin)if(n==index+1)n=0;}
static void event_card_deck_copied(State&s,int to,int from){if(to>=0&&to<MaxDeck&&from>=0&&from<s.deck_count)s.event_cards.deck[to]=s.event_cards.deck[from];}
static int event_card_base_cost(const State&s,Card c,int id,int base){const auto*f=event_card_facts(s,c,id);return base>=0&&f?max(0,base+f->energy_offset):base;}
static int event_card_cost(const State&s,Card c,int id,int base){return base>0&&card_type(s,c,id)==CardType::Power?max(0,base-ec_power(s,EventCardPower::Curious)):base;}
static int event_card_base_damage(const State&s,Card c,int id,int base){return c.kind==EventCardMaul&&id>=0&&id<s.card_count?min(9999,base+s.event_cards.maul_damage[id]):base;}
static int event_card_base_block(const State&s,Card c,int id,int base){
 if(c.kind==EventCardStack)return min(9999,int(s.discard.count)+(c.upgraded?3:0));
 if(c.kind==EventCardMadScience)return card_type(s,c,id)==CardType::Skill?8:0;return base;
}
static int event_enchantment_damage_add(const State&s,Card c,int id){
 const auto*f=event_card_facts(s,c,id);if(!f)return 0;
 switch(f->enchantment){case EventEnchantment::Sharp:return f->enchantment_amount;case EventEnchantment::TezcatarasEmber:return 3;
 case EventEnchantment::Vigorous:return id>=0&&!s.event_cards.enchantment_disabled[id]?f->enchantment_amount:0;
 case EventEnchantment::Momentum:return id>=0?s.event_cards.momentum[id]:0;default:return 0;}
}
static int event_enchantment_damage_halves(const State&s,Card c,int id){auto e=ec_enchantment(s,c,id);return e==EventEnchantment::Corrupted?3:e==EventEnchantment::Instinct?4:2;}
static int event_enchantment_block_add(const State&s,Card c,int id){const auto*f=event_card_facts(s,c,id);return !f?0:f->enchantment==EventEnchantment::Nimble?f->enchantment_amount:f->enchantment==EventEnchantment::Goopy?f->enchantment_amount-1:0;}
static int event_card_replays(const State&s,int id){if(id<0||id>=s.card_count)return 0;auto e=s.event_cards.cards[id].enchantment;return e==EventEnchantment::Spiral||(e==EventEnchantment::Glam&&!s.event_cards.enchantment_disabled[id]);}
static bool event_card_play_allowed(const State&s,int id,bool automatic){
 if(id<0||id>=s.card_count)return false;
 for(unsigned i=0;i<s.hand.count;++i){Card c=s.cards[s.hand.ids[i]];
  if(c.kind==EventCardNormality&&s.event_cards.plays_started>=3)return false;
  if(c.kind==EventCardEnthralled&&!automatic&&s.cards[id].kind!=EventCardEnthralled)return false;
  if(s.cards[id].kind==EventCardClash&&card_type(s,c,s.hand.ids[i])!=CardType::Attack)return false;
 }return true;
}
static void event_card_begin_play(State&s,int){
 auto&v=s.event_cards;v.plays_started=(uint16_t)min(9999,v.plays_started+1);
}
static void event_card_prepare_play(State&s,int,bool discard){auto&v=s.event_cards;v.rebound_return[s.expansion.depth]=discard&&ec_power(s,EventCardPower::Rebound)>0;if(v.rebound_return[s.expansion.depth])--v.powers[unsigned(EventCardPower::Rebound)];}
static bool event_card_returns_to_draw(const State&s,int){return s.event_cards.rebound_return[s.expansion.depth]!=0;}
static void event_card_after_return_to_draw(State&s,int){s.event_cards.rebound_return[s.expansion.depth]=0;}
enum class ECSpecial:int16_t {Enlightenment,Maul,Whistle,FeedingFrenzy,Entrench,UpgradeAll,MaxHpLoss,MadScience,Rebound,Power,Thorns};
static void ec_gain_power(State&s,EventCardPower p,int amount){
 auto&v=s.event_cards;unsigned k=unsigned(p);int before=v.powers[k];v.powers[k]=(int16_t)max(0,min(9999,before+amount));
 if(!before&&v.powers[k]){if(v.power_count>=EventCardPowerKinds+MaxPowerInstances){fail(s);return;}v.power_order[v.power_count++]=(uint8_t)k;}
 if(before&&!v.powers[k])for(unsigned i=0;i<v.power_count;++i)if(v.power_order[i]==k){for(unsigned j=i+1;j<v.power_count;++j)v.power_order[j-1]=v.power_order[j];v.power_order[--v.power_count]=0;break;}
}
static void ec_generate(State&s,int mode,int count){
 WorldCardOptions o;if(mode==1)o.type_mask=world_type_mask(CardType::Skill);else if(mode==2)o.type_mask=world_type_mask(CardType::Attack);else if(mode==3)o.rarity_mask=world_rarity_mask(2);
 Card cards[MaxCards];EventCardFacts facts[MaxCards];unsigned made=world_combat_cards(s,world_rng(s,WorldRng::CombatCardGeneration),min(MaxCards,count),o,mode!=2,cards,facts);
 for(unsigned i=made;i>0;--i)queue_extension(s,XOp::EventCardMaterialize,card_token(cards[i-1]),mode==2?2:mode==3?0:1,true);
}
static bool queue_event_card(State&s,Card c,int target,int){
 if(!ec_new_card(c.kind))return false;
 const auto&d=definitions[c.kind];int hit=card_base_damage(s,c,target,s.playing),p=d.power+(c.upgraded?d.power_up:0),block=event_card_base_block(s,c,s.playing,d.block+(c.upgraded?d.block_up:0));
 record(s,LogKind::Play,c.kind,c.upgraded);if(card_type(s,c,s.playing)==CardType::Attack)relic_attack_begin(s);
 auto op=[&](XOp x,int a=0,int b=0){queue_extension(s,x,a,b);};auto special=[&](ECSpecial x,int a=0){op(XOp::EventCardSpecial,int(x),a);};
 auto attack=[&](){enqueue(s,EffectKind::Damage,hit,target);};auto defend=[&](){enqueue_card_block(s,powered_block(s,c,block,s.playing));};
 switch(c.kind){
 case EventCardCaltrops:special(ECSpecial::Thorns,p);break;
 case EventCardClash:case EventCardByrdSwoop:attack();break;
 case EventCardDistraction:op(XOp::EventCardGenerate,1,1);break;
 case EventCardDualWield:op(XOp::EventCardOffer,int(XChoice::EventDualWield),p);break;
 case EventCardEnlightenment:special(ECSpecial::Enlightenment,c.upgraded);break;
 case EventCardEntrench:special(ECSpecial::Entrench);break;
 case EventCardExterminate:for(int n=0;n<4;++n)for(int t=0;t<s.enemy_count;++t)enqueue(s,EffectKind::Damage,hit,t);break;
 case EventCardFeedingFrenzy:special(ECSpecial::FeedingFrenzy,p);break;
 case EventCardHelloWorld:special(ECSpecial::Power,int(EventCardPower::HelloWorld));break;
 case EventCardMadScience:{const auto*f=event_card_facts(s,c,s.playing);CardType t=card_type(s,c,s.playing);unsigned rider=f?f->rider:0;
  if(t==CardType::Attack){unsigned hits=rider==unsigned(TinkerRider::Violence)?3:1;while(hits--)attack();}
  else if(t==CardType::Skill)defend();special(ECSpecial::MadScience,rider);break;}
 case EventCardMetamorphosis:op(XOp::EventCardGenerate,2,p);break;
 case EventCardOutmaneuver:op(XOp::DPower,int(DPower::EnergyNextTurn),p);break;
 case EventCardPeck:for(int i=0;i<p;++i)attack();break;
 case EventCardRebound:attack();special(ECSpecial::Rebound);break;
 case EventCardRipAndTear:for(int i=0;i<2;++i)enqueue(s,EffectKind::RandomDamage,hit);break;
 case EventCardSquash:attack();enqueue(s,EffectKind::Vulnerable,p,target);break;
 case EventCardStack:defend();break;
 case EventCardToricToughness:op(XOp::EventCardToric,block,s.playing);break;
 case EventCardApotheosis:special(ECSpecial::UpgradeAll);break;
 case EventCardApparition:op(XOp::Power,int(XPower::Intangible),1);break;
 case EventCardBrightestFlame:enqueue(s,EffectKind::Energy,c.upgraded?3:2);op(XOp::Draw,c.upgraded?3:2);special(ECSpecial::MaxHpLoss,2);break;
 case EventCardLuminesce:enqueue(s,EffectKind::Energy,p);break;
 case EventCardMaul:attack();attack();special(ECSpecial::Maul,p);break;
 case EventCardNeowsFury:attack();op(XOp::EventCardOffer,int(XChoice::EventNeowsFury),p);break;
 case EventCardRelax:defend();enqueue(s,EffectKind::DrawNextTurn,c.upgraded?3:2);op(XOp::DPower,int(DPower::EnergyNextTurn),p);break;
 case EventCardWhistle:attack();special(ECSpecial::Whistle,target);break;
 case EventCardWish:op(XOp::EventCardOffer,int(XChoice::EventWish),1);break;
 case EventCardAbundance:op(XOp::EventCardOffer,int(XChoice::EventAbundance),1);break;
 default:break; // Playable curses intentionally have no OnPlay body.
 }
 enqueue(s,EffectKind::Finish);return true;
}
static void event_card_after_body(State&s,int id){if(id>=0&&id<s.card_count)queue_extension(s,XOp::EventEnchantAfterBody,id,0,true);}
static void event_card_after_play(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&v=s.event_cards;auto&f=v.cards[id];
 if(f.enchantment==EventEnchantment::Goopy){f.enchantment_amount=(int16_t)min(9999,f.enchantment_amount+1);int origin=v.deck_origin[id];if(origin>0&&origin<=s.deck_count)v.deck[origin-1].enchantment_amount=(int16_t)min(9999,v.deck[origin-1].enchantment_amount+1);}
 if(f.enchantment==EventEnchantment::Vigorous||f.enchantment==EventEnchantment::Glam)v.enchantment_disabled[id]=1;
}
static void event_card_after_draw(State&s,int id){if(id>=0&&id<s.card_count&&s.zones[id]==Zone::Hand&&s.event_cards.cards[id].enchantment==EventEnchantment::Slither){combat_cost_set(s,id);s.expansion.combat_cost[id]=(int16_t)(world_rng(s,WorldRng::CombatEnergyCosts).below(4)+1);}}
static void event_card_after_shuffle(State&s,bool initial){
 if(initial)return;for(unsigned id=0;id<s.card_count;++id)if(s.zones[id]==Zone::Draw&&s.event_cards.cards[id].enchantment==EventEnchantment::PerfectFit){
  for(unsigned i=0;i<s.draw.count;++i)if(s.draw.ids[i]==id){remove(s.draw,i);put(s,s.draw,id,Zone::Draw);break;}}
}
static void event_card_auto_preplay(State&s){if(s.turn!=1)return;for(int id=s.card_count-1;id>=0;--id)if(s.zones[id]==Zone::Draw&&s.event_cards.cards[id].enchantment==EventEnchantment::Imbued)queue_extension(s,XOp::AutoInstance,id,0,true);}
static void event_card_before_hand_draw(State&s){if(ec_power(s,EventCardPower::HelloWorld))queue_extension(s,XOp::EventCardGenerate,3,ec_power(s,EventCardPower::HelloWorld),true);}
static void event_card_after_block_cleared(State&s){
 auto&v=s.event_cards;for(unsigned i=0;i<MaxPowerInstances;++i)if(v.toric_turns[i]){gain_unpowered_block(s,v.toric_block[i]);if(!--v.toric_turns[i])v.toric_block[i]=0;}
}
static void event_card_before_hand_end(State&s){s.event_cards.end_hand_count=(uint8_t)s.hand.count;}
static void event_card_hand_end(State&s){for(int i=s.hand.count-1;i>=0;--i)if(definitions[s.cards[s.hand.ids[i]].kind].type==CardType::Curse)queue_extension(s,XOp::EventCardHandEnd,s.hand.ids[i],0,true);}
static void event_card_player_side_end(State&s){
 auto&v=s.event_cards;if(ec_power(s,EventCardPower::FeedingFrenzy)){s.strength=(int16_t)max(-9999,s.strength-ec_power(s,EventCardPower::FeedingFrenzy));ec_gain_power(s,EventCardPower::FeedingFrenzy,-ec_power(s,EventCardPower::FeedingFrenzy));}
 ec_gain_power(s,EventCardPower::Rebound,-ec_power(s,EventCardPower::Rebound));v.plays_started=0;for(auto&n:v.rebound_return)n=0;
}
static void event_card_combat_start(State&s){
 auto&v=s.event_cards;for(auto&n:v.powers)n=0;for(auto&n:v.power_order)n=0;v.power_count=0;v.plays_started=0;v.end_hand_count=0;
 for(auto&n:v.toric_block)n=0;for(auto&n:v.toric_turns)n=0;for(auto&n:v.rebound_return)n=0;
 for(unsigned id=0;id<s.card_count;++id){v.enchantment_disabled[id]=0;v.momentum[id]=v.maul_damage[id]=0;}
 // Initial draw bottom is index zero in this engine, opposite original list.
 for(unsigned id=0;id<s.card_count;++id)if(s.zones[id]==Zone::Draw&&v.cards[id].enchantment==EventEnchantment::Imbued){for(unsigned at=0;at<s.draw.count;++at)if(s.draw.ids[at]==id){remove(s.draw,at);put(s,s.draw,id,Zone::Draw);for(unsigned j=s.draw.count-1;j>0;--j)s.draw.ids[j]=s.draw.ids[j-1];s.draw.ids[0]=(uint16_t)id;break;}}
}
static void event_card_combat_end(State&s,bool won){
 if(!won)return;
 for(int i=s.deck_count-1;i>=0;--i)if(s.deck[i].kind==EventCardGuilty&&++s.event_cards.deck[i].progress>=5)event_remove_deck_card(s,i);
 uint16_t eligible[MaxDeck];unsigned count=0;for(unsigned i=0;i<s.deck_count;++i)if(card_upgradeable(s.deck[i]))eligible[count++]=(uint16_t)i;
 for(int n=ec_power(s,EventCardPower::Improvement);n>0&&count;--n){unsigned at=world_rng(s,WorldRng::CombatCardSelection).below(count);s.deck[eligible[at]].upgraded=1;for(unsigned j=at+1;j<count;++j)eligible[j-1]=eligible[j];--count;}
}
static bool event_card_choice(XChoice c){return c==XChoice::EventDualWield||c==XChoice::EventNeowsFury||c==XChoice::EventWish||c==XChoice::EventAbundance;}
static bool choose_event_card(State&s,const uint16_t*ids,unsigned count){
 auto&x=s.expansion;if(s.phase!=Phase::ExtendedChoice||!event_card_choice(x.choice)||count<x.choice_min||count>x.choice_max||(count&&!ids))return false;
 for(unsigned i=0;i<count;++i){bool found=false;for(unsigned j=0;j<x.choices.count;++j)if(x.choices.ids[j]==ids[i])found=true;if(!found)return false;for(unsigned j=0;j<i;++j)if(ids[j]==ids[i])return false;}
 XChoice which=x.choice;int copies=s.event_cards.choice_copies;s.event_cards.choice_copies=0;clear_extended_choice(s);s.phase=Phase::Combat;
 if(which==XChoice::EventDualWield&&count){for(int n=0;n<copies;++n){int id=create_to_hand(s,s.cards[ids[0]],false);if(id<0)break;copy_card_modifiers(s,id,ids[0]);notify_card_generated(s,id);}}
 else if(which==XChoice::EventAbundance&&count){int id=create_to_hand(s,x.offers[ids[0]-MaxCards]);if(id>=0)set_free_this_turn(s,id);}
 else for(unsigned i=0;i<count;++i){int id=ids[i];if(s.hand.count>=MaxHand)break;move_instance(s,id,which==XChoice::EventWish?s.draw:s.discard,s.hand,Zone::Hand);}
 resolve_pending(s);++s.revision;return true;
}
static bool event_card_effect(State&s,Effect e){
 auto&v=s.event_cards;auto&x=s.expansion;int a=e.value,b=e.aux;Card c=s.playing>=0?s.cards[s.playing]:Card{};
 switch(XOp(e.target)){
 case XOp::EventCardSpecial:switch(ECSpecial(a)){
  case ECSpecial::Enlightenment:for(unsigned i=0;i<s.hand.count;++i){int id=s.hand.ids[i];if(card_preview_cost(s,s.cards[id],id)>1){if(b){combat_cost_set(s,id);x.combat_cost[id]=2;}else{temporary_cost_set(s,id);x.turn_cost[id]=2;}}}break;
  case ECSpecial::Maul:for(unsigned id=0;id<s.card_count;++id)if(s.cards[id].kind==EventCardMaul)v.maul_damage[id]=(int16_t)min(9999,v.maul_damage[id]+b);break;
  case ECSpecial::Whistle:enemy_stun(s,b);break;
  case ECSpecial::FeedingFrenzy:world_relic_gain_strength(s,b);ec_gain_power(s,EventCardPower::FeedingFrenzy,b);break;
  case ECSpecial::Entrench:gain_unpowered_block(s,s.block);break;
  case ECSpecial::UpgradeAll:for(unsigned id=0;id<s.card_count;++id)if(int(id)!=s.playing&&card_upgradeable(s.cards[id]))s.cards[id].upgraded=1;break;
  case ECSpecial::MaxHpLoss:s.max_hp=(int16_t)max(1,s.max_hp-b);s.hp=(int16_t)min(s.hp,s.max_hp);break;
  case ECSpecial::Rebound:ec_gain_power(s,EventCardPower::Rebound,1);break;
  case ECSpecial::Power:ec_gain_power(s,EventCardPower(b),1);break;
  case ECSpecial::Thorns:s.thorns=(int16_t)min(9999,s.thorns+b);break;
  case ECSpecial::MadScience:switch(TinkerRider(b)){
   case TinkerRider::Sapping:enqueue(s,EffectKind::Vulnerable,2,x.target,true);enqueue(s,EffectKind::Weak,2,x.target,true);break;
   case TinkerRider::Choking:queue_extension(s,XOp::SPower,SPowerCount+x.target,6,true);break;
   case TinkerRider::Energized:enqueue(s,EffectKind::Energy,2,0,true);break;
   case TinkerRider::Wisdom:queue_extension(s,XOp::Draw,3,0,true);break;
   case TinkerRider::Chaos:queue_extension(s,XOp::EventCardGenerate,0,1,true);break;
   case TinkerRider::Expertise:enqueue(s,EffectKind::Dexterity,2,0,true);enqueue(s,EffectKind::Strength,2,0,true);break;
   case TinkerRider::Curious:ec_gain_power(s,EventCardPower::Curious,1);break;
   case TinkerRider::Improvement:ec_gain_power(s,EventCardPower::Improvement,1);break;
   default:break;
  }break;
 }break;
 case XOp::EventEnchantAfterBody:{
  if(a<0||a>=s.card_count)break;auto&f=v.cards[a];int amount=f.enchantment_amount;
  switch(f.enchantment){
   case EventEnchantment::Corrupted:player_hp_loss(s,2,false,false,a);break;
   case EventEnchantment::Sown:if(!v.enchantment_disabled[a]){v.enchantment_disabled[a]=1;enqueue(s,EffectKind::Energy,amount,0,true);}break;
   case EventEnchantment::Swift:if(!v.enchantment_disabled[a]){v.enchantment_disabled[a]=1;queue_extension(s,XOp::Draw,amount,0,true);}break;
   case EventEnchantment::Adroit:enqueue_card_block(s,powered_block(s,s.cards[a],amount,a),true);break;
   case EventEnchantment::Momentum:v.momentum[a]=(int16_t)min(9999,v.momentum[a]+amount);break;
   default:break;
  }break;
 }
 case XOp::EventCardToric:{
  unsigned slot=0;while(slot<MaxPowerInstances&&v.toric_turns[slot])++slot;if(slot==MaxPowerInstances){fail(s);break;}
  int before=s.block;gain_card_block(s,powered_block(s,c,a,b));v.toric_block[slot]=(int16_t)max(0,s.block-before);v.toric_turns[slot]=2;break;
 }
 case XOp::EventCardGenerate:ec_generate(s,a,b);break;
 case XOp::EventCardMaterialize:{
  int id=make_card(s,card_from_token(a));if(id<0)break;
  if(b==2){set_free_this_combat(s,id);unsigned at=s.draw.count-world_rng(s,WorldRng::Shuffle).below(s.draw.count+1);put(s,s.draw,id,Zone::Draw);for(unsigned j=s.draw.count-1;j>at;--j)s.draw.ids[j]=s.draw.ids[j-1];s.draw.ids[at]=(uint16_t)id;}
  else{bool room=s.hand.count<MaxHand;put(s,room?s.hand:s.discard,id,room?Zone::Hand:Zone::Discard);if(b==1)set_free_this_turn(s,id);}
  notify_card_generated(s,id);break;
 }
 case XOp::EventCardOffer:{
  Pile pool{};XChoice which=XChoice(a);
  if(which==XChoice::EventDualWield){for(unsigned i=0;i<s.hand.count;++i){int id=s.hand.ids[i];CardType t=card_type(s,s.cards[id],id);if(t==CardType::Attack||t==CardType::Power)pool.ids[pool.count++]=(uint16_t)id;}}
  else if(which==XChoice::EventAbundance){WorldCardOptions o;o.type_mask=world_type_mask(CardType::Power);Card cards[3];EventCardFacts facts[3];unsigned n=world_combat_cards(s,world_rng(s,WorldRng::CombatCardGeneration),3,o,true,cards,facts);for(unsigned i=0;i<n;++i){cards[i].upgraded=card_upgradeable(cards[i])?1:0;x.offers[i]=cards[i];pool.ids[pool.count++]=(uint16_t)(MaxCards+i);}}
  else pool=which==XChoice::EventWish?s.draw:s.discard;
  int maximum=which==XChoice::EventNeowsFury?min(b,MaxHand-int(s.hand.count)):1;
  set_extended_choice(s,which,pool,which==XChoice::EventNeowsFury?0:1,maximum);
  if(which==XChoice::EventDualWield&&s.phase==Phase::ExtendedChoice)v.choice_copies=(uint8_t)b;break;
 }
 case XOp::EventCardHandEnd:if(a>=0&&a<s.card_count&&s.zones[a]==Zone::Hand)switch(s.cards[a].kind){
  case EventCardBadLuck:player_hp_loss(s,13,false,false,a);break;
  case EventCardDebt:s.gold=(int16_t)max(0,s.gold-10);break;
  case EventCardDecay:player_hp_loss(s,2,false,true,a);break;
  case EventCardDoubt:enemy_apply_power(s,-1,EnemyPower::Weak,1,-1);break;
  case EventCardShame:enemy_apply_power(s,-1,EnemyPower::Frail,1,-1);break;
  case EventCardRegret:player_hp_loss(s,v.end_hand_count,false,false,a);break;
  default:break;
 }break;
 default:return false;
 }return true;
}
struct EventCardVariable {CardKind kind;const char*key;int16_t base,upgrade;};
static const EventCardVariable ec_variables[]={
 {EventCardBadLuck,"HpLoss",13,0},{EventCardCaltrops,"ThornsPower",3,2},{EventCardDebt,"Gold",10,0},
 {EventCardDoubt,"WeakPower",1,0},{EventCardDualWield,"Cards",1,1},{EventCardExterminate,"Repeat",4,0},
 {EventCardFeedingFrenzy,"StrengthPower",5,2},{EventCardGuilty,"Combats",5,0},
 {EventCardMadScience,"SappingWeak",2,0},{EventCardMadScience,"SappingVulnerable",2,0},
 {EventCardMadScience,"ViolenceHits",3,0},{EventCardMadScience,"ChokingDamage",6,0},
 {EventCardMadScience,"EnergizedEnergy",2,0},{EventCardMadScience,"WisdomCards",3,0},
 {EventCardMadScience,"ExpertiseStrength",2,0},{EventCardMadScience,"ExpertiseDexterity",2,0},
 {EventCardMadScience,"CuriousReduction",1,0},{EventCardMetamorphosis,"Cards",3,2},
 {EventCardNormality,"CalculationBase",3,0},{EventCardNormality,"CalculationExtra",-1,0},
 {EventCardOutmaneuver,"Energy",2,1},{EventCardPeck,"Repeat",3,1},
 {EventCardShame,"Frail",1,0},{EventCardSpoilsMap,"Gold",600,0},{EventCardSquash,"VulnerablePower",2,1},
 {EventCardStack,"CalculationBase",0,3},{EventCardStack,"CalculationExtra",1,0},
 {EventCardToricToughness,"Turns",2,0},{EventCardApparition,"IntangiblePower",1,0},
 {EventCardBrightestFlame,"MaxHp",2,0},{EventCardBrightestFlame,"Energy",2,1},{EventCardBrightestFlame,"Cards",2,1},
 {EventCardDowsing,"Rooms",5,0},{EventCardLuminesce,"Energy",2,1},{EventCardMaul,"Increase",2,1},
 {EventCardNeowsFury,"Cards",2,1},{EventCardRelax,"Cards",2,1},{EventCardRelax,"Energy",2,1}
};
bool event_card_description_value(const State&s,Card c,int target,int instance,const char*key,int&out){
 if(!ec_new_card(c.kind))return false;
 for(const auto&v:ec_variables)if(c.kind==v.kind&&ec_word(key,v.key)){out=v.base+(c.upgraded?v.upgrade:0);return true;}
 if(ec_word(key,"Damage")&&definitions[c.kind].damage){out=c.kind==EventCardDecay?2:damage(s,c,target,instance);return true;}
 if(ec_word(key,"Block")||ec_word(key,"CalculatedBlock")){out=card_block(s,c,instance);return true;}
 if(c.kind==EventCardNormality&&ec_word(key,"CalculatedCards")){out=max(0,3-int(s.event_cards.plays_started));return true;}
 if(c.kind==EventCardRegret&&ec_word(key,"HpLoss")){out=s.hand.count;return true;}
 if(c.kind==EventCardMadScience){const auto*f=event_card_facts(s,c,instance);if(ec_word(key,"TinkerTimeType")){out=f?f->science_type:0;return true;}if(ec_word(key,"TinkerTimeRider")){out=f?f->rider:0;return true;}}
 return false;
}
bool event_card_deck_description_value(const State&s,Card c,int deck,const char*key,int&out){
 if(deck<0||deck>=s.deck_count||s.deck[deck].kind!=c.kind)return false;
 const auto&f=s.event_cards.deck[deck];
 if((c.kind==EventCardGuilty&&ec_word(key,"Combats"))||(c.kind==EventCardDowsing&&ec_word(key,"Rooms"))){out=max(0,5-f.progress);return true;}
 if(c.kind==EventCardMadScience&&ec_word(key,"Block")){out=f.science_type==1?8:0;return true;}
 if(c.kind==EventCardMadScience&&ec_word(key,"TinkerTimeType")){out=f.science_type;return true;}
 if(c.kind==EventCardMadScience&&ec_word(key,"TinkerTimeRider")){out=f.rider;return true;}
 if(ec_word(key,"Damage")&&definitions[c.kind].damage){out=definitions[c.kind].damage+(c.upgraded?definitions[c.kind].damage_up:0);
  if(f.enchantment==EventEnchantment::Sharp||f.enchantment==EventEnchantment::Vigorous)out+=f.enchantment_amount;
  if(f.enchantment==EventEnchantment::TezcatarasEmber)out+=3;
  if(f.enchantment==EventEnchantment::Corrupted)out=out*3/2;if(f.enchantment==EventEnchantment::Instinct)out*=2;return true;}
 if(ec_word(key,"Block")&&(definitions[c.kind].block||c.kind==EventCardStack)){out=definitions[c.kind].block+(c.upgraded?definitions[c.kind].block_up:0);if(f.enchantment==EventEnchantment::Nimble)out+=f.enchantment_amount;if(f.enchantment==EventEnchantment::Goopy)out+=f.enchantment_amount-1;return true;}
 return false;
}
