// Model-specific continuations. Every random draw is kept in the source stream;
// presentation-only delays and audio are intentionally omitted.
enum ComplexStep {CX_DiscardPotion=100,CX_RandomUpgrade,CX_Tablet,CX_RemoveAt,
 CX_RemoveQuest,CX_RepyDone,CX_Dummy,CX_Bundle,CX_FakeRewards,CX_Future,
 CX_ConveyorNext,CX_Colorful,CX_Tinker,CX_CrystalBegin,CX_CrystalClear,
 CX_CrystalNext,CX_ShopAfter,CX_Wongo,CX_BridgeNext,CX_RelicOffer,CX_ConveyorCard};
unsigned potion_slots(const State&s,uint16_t*out){unsigned n=0;for(unsigned i=0;i<potion_slot_count(s);i++)if(potion_at(s,i)!=Potion::Empty)out[n++]=uint16_t(i);return n;}
unsigned basic_count(const State&s,unsigned kind){unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(card_removable(s,s.deck[i],i)&&(kind==0?basic_strike(s.deck[i].kind):kind==1?basic_defend(s.deck[i].kind):card_rarity(s.deck[i].kind)==1))++n;return n;}
bool book_option(const State&s,unsigned option){const EventEnchantment enchant[]={EventEnchantment::Sharp,EventEnchantment::Nimble,EventEnchantment::Swift};for(unsigned i=0;i<s.deck_count;i++)if(unsigned(card_type(s,s.deck[i],-1,i))==option&&event_card_can_enchant(s,i,enchant[option]))return true;return false;}
void random_one_upgrade(State&s,bool all=false){uint16_t list[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i]))list[n++]=uint16_t(i);
 if(!n)return;if(all)for(unsigned i=0;i<n;i++){s.deck[list[i]].upgraded=1;record(s,LogKind::Upgrade,s.deck[list[i]].kind);notify_history(s,RunHistoryEvent::CardUpgraded,unsigned(card_token(s.deck[list[i]])));notify_deck_upgraded(s,list[i]);}else{unsigned i=unsigned(rand_range(s,0,int(n)));s.deck[list[i]].upgraded=1;record(s,LogKind::Upgrade,s.deck[list[i]].kind);notify_history(s,RunHistoryEvent::CardUpgraded,unsigned(card_token(s.deck[list[i]])));notify_deck_upgraded(s,list[i]);}}
void stable_relics(RelicId*ids,unsigned n){for(unsigned i=1;i<n;i++){RelicId id=ids[i];unsigned j=i;while(j&&compare_text(world_relic_key(ids[j-1]),world_relic_key(id))>0){ids[j]=ids[j-1];--j;}ids[j]=id;}}
void bridge_card(State&s,bool initial){auto&e=s.original_events;uint16_t list[MaxDeck];unsigned n=0;int previous=e.values[1];if(!initial&&previous>=0&&previous<s.deck_count)e.remembered[previous]=1;
 for(unsigned i=0;i<s.deck_count;i++)if(card_removable(s,s.deck[i],i)&&!e.remembered[i]&&(initial?card_rarity(s.deck[i].kind)!=1:previous<0||s.deck[i].kind!=s.deck[previous].kind))list[n++]=uint16_t(i);
 if(!n)for(unsigned i=0;i<s.deck_count;i++)if(card_removable(s,s.deck[i],i))list[n++]=uint16_t(i);
 e.values[1]=n?int16_t(list[rand_range(s,0,int(n))]):-1;e.values[2]=e.values[1]>=0?int16_t(card_token(s.deck[e.values[1]])):-1;
}
const char*dish_key(unsigned dish){const char*keys[]={"CAVIAR","SPICY_SNAPPY","JELLY_LIVER","FRIED_EEL","SUSPICIOUS_CONDIMENT","CLAM_ROLL","GOLDEN_FYSH","SEAPUNK_SALAD"};return dish<8?keys[dish]:"LOCKED";}
void conveyor_roll(State&s){auto&e=s.original_events;int previous=e.values[1];e.values[2]=int16_t(previous);++e.values[0];if(e.values[0]%5==0)e.values[1]=7;else{
 unsigned ids[7],weights[7],n=0,total=0;const unsigned base[]={6,3,3,3,3,6,1};uint16_t slots[MaxPotionSlots];unsigned filled=potion_slots(s,slots);
 for(unsigned i=0;i<7;i++){if(int(i)==previous||(i==4&&filled>=potion_slot_count(s))||(i==5&&s.hp==s.max_hp)||(i==6&&e.values[0]<=1))continue;ids[n]=i;weights[n++]=base[i];total+=base[i];}
 auto rng=event_random(s);auto value=world_float_round(world_random_float(rng)*reference::math::Real::integer(int(total)));keep_random(s,rng);unsigned accumulated=0;e.values[1]=int16_t(ids[n-1]);for(unsigned i=0;i<n;i++){accumulated+=weights[i];if(value<reference::math::Real::integer(int(accumulated))){e.values[1]=int16_t(ids[i]);break;}}
 }options(s);option_key(s,0,"ALL.options.",s.gold>=40?dish_key(e.values[1]):"LOCKED");option_key(s,1,e.page?"GRAB_SOMETHING_OFF_THE_BELT.options.":"INITIAL.options.",e.page?"LEAVE":"OBSERVE_CHEF");}
void architect_page(State&s){auto&e=s.original_events;const char*chars[]={"IRONCLAD","SILENT","DEFECT","REGENT","NECROBINDER"};
 char key[64]{};unsigned at=0;const char*prefix="talk.";for(unsigned i=0;prefix[i];i++)key[at++]=prefix[i];for(unsigned i=0;chars[unsigned(s.character)][i];i++)key[at++]=chars[unsigned(s.character)][i];
 key[at++]='.';key[at++]=char('0'+e.values[0]);key[at++]='-';key[at++]=char('0'+e.values[1]);
 if(!(e.values[0]==0&&(s.character==Character::Regent||s.character==Character::Necrobinder)))key[at++]='r';
 const bool char_speaker=s.character==Character::Regent||s.character==Character::Necrobinder?(e.values[1]%2==0):(e.values[1]%2!=0);const char*suffix=char_speaker?".char":".ancient";for(unsigned i=0;suffix[i];i++)key[at++]=suffix[i];
 page(s,key);options(s,"PROCEED");e.finished=uint8_t(e.values[1]+1>=e.values[2]);
}
bool crystal_begin(State&,unsigned);
bool crystal_clear(State&,unsigned,unsigned);
bool crystal_next(State&);
const SimpleEvent complex_events[]={
 {OriginalEventId::Amalgamator,"COMBINE_STRIKES","COMBINE_DEFENDS",nullptr},
 {OriginalEventId::JungleMazeAdventure,"SOLO_QUEST","JOIN_FORCES",nullptr},
 {OriginalEventId::ColossalFlower,"EXTRACT_CURRENT_PRIZE_1","REACH_DEEPER_1",nullptr},
 {OriginalEventId::WoodCarvings,"BIRD","SNAKE","TORUS"},
 {OriginalEventId::TabletOfTruth,"DECIPHER_1","SMASH",nullptr},
 {OriginalEventId::WarHistorianRepy,"UNLOCK_CAGE","UNLOCK_CHEST",nullptr},
 {OriginalEventId::WaterloggedScriptorium,"BLOODY_INK","TENTACLE_QUILL","PRICKLY_SPONGE"},
 {OriginalEventId::StoneOfAllTime,"LIFT","PUSH",nullptr},
 {OriginalEventId::RanwidTheElder,"POTION","GOLD","RELIC"},
 {OriginalEventId::SlipperyBridge,"OVERCOME","HOLD_ON_0",nullptr},
 {OriginalEventId::RelicTrader,"TOP","MIDDLE","BOTTOM"},
 {OriginalEventId::BattlewornDummy,"SETTING_1","SETTING_2","SETTING_3"},
 {OriginalEventId::DollRoom,"RANDOM","TAKE_SOME_TIME","EXAMINE"},
 {OriginalEventId::SelfHelpBook,"READ_THE_BACK","READ_PASSAGE","READ_ENTIRE_BOOK"},
 {OriginalEventId::PunchOff,"NAB","I_CAN_TAKE_THEM",nullptr},
 {OriginalEventId::TheFutureOfPotions,"POTION","POTION","POTION"},
 {OriginalEventId::Trial,"ACCEPT","REJECT",nullptr},
 {OriginalEventId::TinkerTime,"CHOOSE_CARD_TYPE",nullptr,nullptr},
 {OriginalEventId::WelcomeToWongos,"BARGAIN_BIN","FEATURED_ITEM","MYSTERY_BOX"},
 {OriginalEventId::CrystalSphere,"UNCOVER_FUTURE","PAYMENT_PLAN",nullptr},
};
bool complex_implemented(OriginalEventId id){for(auto&entry:complex_events)if(entry.id==id)return true;return id==OriginalEventId::TheArchitect||id==OriginalEventId::EndlessConveyor||id==OriginalEventId::FakeMerchant;}
bool complex_allowed(const State&s,OriginalEventId id,unsigned act,unsigned floor){uint16_t slots[MaxPotionSlots];RelicId relics[WorldRelicKinds];
 switch(id){
 case OriginalEventId::Amalgamator:return basic_count(s,0)>=2&&basic_count(s,1)>=2;
 case OriginalEventId::ColossalFlower:return s.hp>=19;
 case OriginalEventId::WoodCarvings:return basic_count(s,2)>0;
 case OriginalEventId::WarHistorianRepy:return false;
 case OriginalEventId::WaterloggedScriptorium:return s.gold>=55;
 case OriginalEventId::StoneOfAllTime:return act==1&&potion_slots(s,slots)>0;
 case OriginalEventId::RanwidTheElder:return act>0&&s.gold>=100&&potion_slots(s,slots)>0&&world_relic_list(s,relics,WorldRelicKinds,true)>0;
 case OriginalEventId::SlipperyBridge:return floor>6&&count_removable(s)>0;
 case OriginalEventId::RelicTrader:return act>0&&world_relic_list(s,relics,WorldRelicKinds,true)>=5;
 case OriginalEventId::DollRoom:return act==1;
 case OriginalEventId::PunchOff:return floor>=6;
 case OriginalEventId::TheFutureOfPotions:return potion_slots(s,slots)>=2;
 case OriginalEventId::EndlessConveyor:return s.gold>=120;
 case OriginalEventId::WelcomeToWongos:return act==1&&s.gold>=100;
 case OriginalEventId::CrystalSphere:return act>0&&s.gold>=100;
 case OriginalEventId::FakeMerchant:{bool foul=false;for(unsigned i=0;i<potion_slot_count(s);i++)if(potion_at(s,i)==Potion::FoulPotion)foul=true;return act>0&&(s.gold>=100||foul);}
 default:return true;
 }
}
bool complex_begin(State&s){auto&e=s.original_events;
 for(auto&entry:complex_events)if(entry.id==e.id){option_key(s,0,"INITIAL.options.",entry.a);if(entry.b)option_key(s,1,"INITIAL.options.",entry.b);if(entry.c)option_key(s,2,"INITIAL.options.",entry.c);break;}
 switch(e.id){
 case OriginalEventId::JungleMazeAdventure:{auto rng=event_random(s);e.values[0]=int16_t(world_float_truncate(reference::math::Real::integer(150)+world_random_float(rng,-15,15)));e.values[1]=int16_t(world_float_truncate(reference::math::Real::integer(50)+world_random_float(rng,-15,15)));keep_random(s,rng);break;}
 case OriginalEventId::TabletOfTruth:e.values[0]=3;break;
 case OriginalEventId::StoneOfAllTime:case OriginalEventId::RanwidTheElder:{uint16_t slots[MaxPotionSlots];unsigned n=potion_slots(s,slots);e.values[0]=n?int16_t(slots[rand_range(s,0,int(n))]):-1;e.values[2]=e.values[0]>=0?int16_t(potion_at(s,unsigned(e.values[0]))):0;
  if(e.id==OriginalEventId::RanwidTheElder){RelicId ids[WorldRelicKinds];unsigned nr=world_relic_list(s,ids,WorldRelicKinds,true);e.values[1]=nr?int16_t(ids[rand_range(s,0,int(nr))]):-1;}break;}
 case OriginalEventId::SlipperyBridge:e.values[1]=-1;bridge_card(s,true);break;
 case OriginalEventId::RelicTrader:{RelicId ids[WorldRelicKinds];unsigned n=world_relic_list(s,ids,WorldRelicKinds,true);stable_relics(ids,n);auto rng=event_random(s);reference::shuffle(ids,n,rng);keep_random(s,rng);
  e.option_count=uint8_t(smaller(3,int(n)));for(unsigned i=0;i<e.option_count;i++)e.values[i]=int16_t(ids[i]);if(n)for(unsigned i=0;i<3;i++)e.values[3+i]=int16_t(random_world_relic(s));if(!n)finish_page(s,"DONE");break;}
 case OriginalEventId::SelfHelpBook:if(!book_option(s,0)&&!book_option(s,1)&&!book_option(s,2)){e.page=1;options(s,"INITIAL.options.NO_OPTIONS");}break;
 case OriginalEventId::PunchOff:e.values[0]=int16_t(rand_range(s,91,99));break;
 case OriginalEventId::TheFutureOfPotions:{uint16_t slots[MaxPotionSlots];unsigned n=potion_slots(s,slots);auto rng=event_random(s);for(unsigned i=0;i<n;i++){const auto*def=potion_definition(potion_at(s,slots[i]));unsigned type=rng.below(def&&(def->rarity==1||def->rarity==5)?2:3);if(i<3){e.values[i]=int16_t(slots[i]);e.values[3+i]=int16_t(type);}}keep_random(s,rng);e.option_count=uint8_t(smaller(int(n),3));break;}
 case OriginalEventId::WelcomeToWongos:e.values[0]=int16_t(world_relic_pull(s,WorldRelicPool::Rare,false,true));option_key(s,3,"INITIAL.options.","LEAVE");break;
 case OriginalEventId::CrystalSphere:e.values[0]=int16_t(50+rand_range(s,1,50));break;
 case OriginalEventId::EndlessConveyor:e.values[1]=-1;conveyor_roll(s);break;
 case OriginalEventId::FakeMerchant:{RelicId pool[]={WorldRelicFakeAnchor,WorldRelicFakeBloodVial,WorldRelicFakeHappyFlower,WorldRelicFakeLeesWaffle,WorldRelicFakeMango,WorldRelicFakeOrichalcum,WorldRelicFakeSneckoEye,WorldRelicFakeStrikeDummy,WorldRelicFakeVenerableTeaSet};auto rng=event_random(s);reference::shuffle(pool,9,rng);keep_random(s,rng);e.offer_count=6;options(s);
  using reference::math::Real;for(unsigned i=0;i<6;i++){e.offers[i]=pool[i];option_key(s,i,"INITIAL.options.",world_relic_key(pool[i]));Real lo=world_float(0x3f59999aU),range=world_float_round(world_float(0x3f933333U)-lo);auto random=world_float_round(Real::fraction53(world_rng(s,WorldRng::Shops).next()>>11)*range+lo);int32_t value=0;if(!reference::math::round_even_int(world_float_round(random*Real::integer(50)),value))return false;e.choice_prices[i]=int16_t(value);}option_key(s,6,"","LEAVE");break;}
 case OriginalEventId::TheArchitect:{unsigned visits=s.event_run.character_wins[unsigned(s.character)],max_dialogue=s.character==Character::Silent||s.character==Character::Necrobinder?4:3;
  if(visits<max_dialogue){e.values[0]=int16_t(visits);rand_range(s,0,1);}else{unsigned first=s.character==Character::Regent||s.character==Character::Necrobinder?1:0;e.values[0]=int16_t(rand_range(s,int(first),int(max_dialogue)));}
  e.values[1]=0;e.values[2]=s.character==Character::Silent?1:s.character==Character::Ironclad&&e.values[0]==0?2:s.character==Character::Necrobinder&&e.values[0]<3?2:3;architect_page(s);break;}
 case OriginalEventId::Trial:e.values[0]=101+int16_t(s.revision%898);break; // Chaotic display-only entrant number; never consumes the event RNG.
 default:break;
 }return true;
}
bool complex_enabled(const State&s,unsigned option){const auto&e=s.original_events;
 switch(e.id){
 case OriginalEventId::Amalgamator:return basic_count(s,option)>=2;
 case OriginalEventId::WoodCarvings:return option!=1||count_enchantable(s,EventEnchantment::Slither)>0;
 case OriginalEventId::WaterloggedScriptorium:return option==0||s.gold>=(option==1?55:99);
 case OriginalEventId::StoneOfAllTime:return option?count_enchantable(s,EventEnchantment::Vigorous)>0:e.values[0]>=0&&potion_at(s,unsigned(e.values[0]))!=Potion::Empty;
 case OriginalEventId::RanwidTheElder:return option==0?e.values[0]>=0:option==1?s.gold>=100:e.values[1]>=0&&has_world_relic(s,RelicId(e.values[1]));
 case OriginalEventId::SlipperyBridge:return option==1?e.values[0]<9996:e.values[1]>=0&&e.values[1]<s.deck_count;
 case OriginalEventId::SelfHelpBook:return e.page||book_option(s,option);
 case OriginalEventId::WelcomeToWongos:return option==3||s.gold>=100*int(option+1);
 case OriginalEventId::CrystalSphere:return option==1||s.gold>=e.values[0];
 case OriginalEventId::EndlessConveyor:return option==1||s.gold>=40;
 case OriginalEventId::FakeMerchant:return option==6||(option<6&&e.offers[option]<WorldRelicKinds&&s.gold>=e.choice_prices[option]);
 default:return true;
 }
}
bool complex_choose(State&s,unsigned option){auto&e=s.original_events;const char*done=nullptr;for(auto&entry:complex_events)if(entry.id==e.id){done=option==0?entry.a:option==1?entry.b:entry.c;break;}
 switch(e.id){
 case OriginalEventId::Amalgamator:select_deck(s,OriginalEventSelection::Remove,2,option?OriginalEventFilter::BasicDefend:OriginalEventFilter::BasicStrike);card(s,option?UltimateDefend:UltimateStrike);break;
 case OriginalEventId::JungleMazeAdventure:if(!option){rand_range(s,0,3);rand_range(s,0,2);damage(s,18);}gold(s,e.values[option]);break;
 case OriginalEventId::ColossalFlower:
  if(!option){const int amounts[]={35,75,135};gold(s,amounts[e.page]);done=e.page<2?"EXTRACT_CURRENT_PRIZE":"EXTRACT_INSTEAD";}
  else if(e.page==2){damage(s,7);relic(s,WorldRelicPollinousCore);done="POLLINOUS_CORE";}
  else{damage(s,5+e.page);++e.page;if(e.page==1){page(s,"REACH_DEEPER_1");options(s,"REACH_DEEPER_1.options.EXTRACT_CURRENT_PRIZE_2","REACH_DEEPER_1.options.REACH_DEEPER_2");}else{page(s,"REACH_DEEPER_2");options(s,"REACH_DEEPER_2.options.EXTRACT_INSTEAD","REACH_DEEPER_2.options.POLLINOUS_CORE");}return true;}break;
 case OriginalEventId::WoodCarvings:if(option==1)select_deck(s,OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,EventEnchantment::Slither);else select_deck(s,OriginalEventSelection::Transform,1,OriginalEventFilter::Basic,EventEnchantment::None,0,(option==0?EventCardPeck:EventCardToricToughness)+1);break;
 case OriginalEventId::TabletOfTruth:if(option==1){if(!e.page)healing(s,20);done=e.page?"GIVE_UP":"SMASH";}else{push_step(s,OriginalEventOp::Special,CX_Tablet);return true;}break;
 case OriginalEventId::WarHistorianRepy:{unsigned choice=e.page?unsigned(e.values[0]):option;push_step(s,OriginalEventOp::Special,CX_RemoveQuest,e.page?MaxDeck:1);
  if(choice==0){e.values[14]=1;relic(s,WorldRelicHistoryCourse);}else push_step(s,OriginalEventOp::Special,CX_Bundle,2,2);
  push_step(s,OriginalEventOp::Special,CX_RepyDone,int(choice),e.page);return true;}
 case OriginalEventId::WaterloggedScriptorium:if(!option)maxhp(s,6);else{gold(s,option==1?-55:-99);select_deck(s,OriginalEventSelection::Enchant,option,OriginalEventFilter::Any,EventEnchantment::Steady);}break;
 case OriginalEventId::StoneOfAllTime:if(!option){push_step(s,OriginalEventOp::Special,CX_DiscardPotion,e.values[0]);maxhp(s,10);}else{damage(s,6);select_deck(s,OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,EventEnchantment::Vigorous,8);}push_step(s,OriginalEventOp::Special,CX_RandomUpgrade,0,1);break;
 case OriginalEventId::RanwidTheElder:if(option==0)push_step(s,OriginalEventOp::Special,CX_DiscardPotion,e.values[0]);else if(option==1)gold(s,-100);else push_step(s,OriginalEventOp::LoseRelic,e.values[1]);queue_random_relic(s);if(option==2)queue_random_relic(s);break;
 case OriginalEventId::SlipperyBridge:if(!option){push_step(s,OriginalEventOp::Special,CX_RemoveAt,e.values[1]);done="OVERCOME";}else{damage(s,3+e.values[0]);push_step(s,OriginalEventOp::Special,CX_BridgeNext);return true;}break;
 case OriginalEventId::RelicTrader:push_step(s,OriginalEventOp::LoseRelic,e.values[option]);relic(s,RelicId(e.values[option+3]));done="DONE";break;
 case OriginalEventId::BattlewornDummy:{const char*keys[]={"BattlewornDummyEventV1Encounter","BattlewornDummyEventV2Encounter","BattlewornDummyEventV3Encounter"};e.values[0]=int16_t(option);combat(s,keys[option],true);push_step(s,OriginalEventOp::Special,CX_Dummy);return true;}
 case OriginalEventId::DollRoom:{const RelicId dolls[]={WorldRelicDaughterOfTheWind,WorldRelicMrStruggles,WorldRelicBingBong};unsigned chosen=0;
  if(e.page)chosen=unsigned(e.values[option]);else if(!option)chosen=unsigned(rand_range(s,0,3));else{damage(s,option==1?5:15);unsigned ids[]={2,0,1};auto rng=event_random(s);reference::shuffle(ids,3,rng);keep_random(s,rng);e.page=uint8_t(option);page(s,option==1?"TAKE_SOME_TIME":"EXAMINE");options(s);for(unsigned i=0;i<(option==1?2U:3U);i++){e.values[i]=int16_t(ids[i]);option_key(s,i,"TAKE.options.","TAKE");e.offers[i]=dolls[ids[i]];}e.offer_count=option==1?2:3;return true;}
  relic(s,dolls[chosen]);done=chosen==0?"DAUGHTER_OF_WIND":chosen==1?"MR_STRUGGLES":"FABLE";break;}
 case OriginalEventId::SelfHelpBook:if(e.page)done="NO_OPTIONS";else{const EventEnchantment enchant[]={EventEnchantment::Sharp,EventEnchantment::Nimble,EventEnchantment::Swift};const OriginalEventFilter filter[]={OriginalEventFilter::Attack,OriginalEventFilter::Skill,OriginalEventFilter::Power};select_deck(s,OriginalEventSelection::Enchant,1,filter[option],enchant[option],2);}break;
 case OriginalEventId::PunchOff:if(e.page){combat(s,"PunchOffEventEncounter");done="I_CAN_TAKE_THEM";}else if(!option){card(s,EventCardInjury);push_step(s,OriginalEventOp::Special,CX_Bundle,0,1);}else{e.page=1;page(s,"I_CAN_TAKE_THEM");options(s,"I_CAN_TAKE_THEM.options.FIGHT");return true;}break;
 case OriginalEventId::TheFutureOfPotions:{const auto*def=potion_definition(potion_at(s,unsigned(e.values[option])));if(!def)return false;
  int rarity=def->rarity==3||def->rarity==4?4:def->rarity==2?3:2;push_step(s,OriginalEventOp::Special,CX_DiscardPotion,e.values[option]);
  push_step(s,OriginalEventOp::Special,CX_Future,rarity,e.values[3+option]);done="DONE";break;}
 case OriginalEventId::Trial:
  if(e.page==5){if(!option){event_dead(s);return true;}e.page=1;page(s,"REJECT");options(s,"REJECT.options.ACCEPT","REJECT.options.DOUBLE_DOWN");return true;}
  if(e.page<=1){if(option){if(e.page){e.page=5;options(s,"CONFIRM_ABANDON","CANCEL");}else{e.page=1;page(s,"REJECT");options(s,"REJECT.options.ACCEPT","REJECT.options.DOUBLE_DOWN");}return true;}
   e.page=uint8_t(2+rand_range(s,0,3));const char*pages[]={"MERCHANT","NOBLE","NONDESCRIPT"};const char*prefix[]={"MERCHANT.options.","NOBLE.options.","NONDESCRIPT.options."};page(s,pages[e.page-2]);options(s);option_key(s,0,prefix[e.page-2],"GUILTY");option_key(s,1,prefix[e.page-2],"INNOCENT");return true;}
  if(e.page==2){card(s,option?EventCardShame:EventCardRegret);if(option)select_deck(s,OriginalEventSelection::Upgrade,2);else{queue_random_relic(s);queue_random_relic(s);}done=option?"MERCHANT_INNOCENT":"MERCHANT_GUILTY";}
  else if(e.page==3){if(!option)healing(s,10);else{card(s,EventCardRegret);gold(s,300);}done=option?"NOBLE_INNOCENT":"NOBLE_GUILTY";}
  else{card(s,EventCardDoubt);if(option)select_deck(s,OriginalEventSelection::Transform,2);else push_step(s,OriginalEventOp::Special,CX_Bundle,0,0,2);done=option?"NONDESCRIPT_INNOCENT":"NONDESCRIPT_GUILTY";}break;
 case OriginalEventId::TinkerTime:
  if(e.page<2){auto rng=event_random(s);unsigned list[]={0,1,2};reference::shuffle(list,3,rng);keep_random(s,rng);options(s);
   if(!e.page){e.page=1;page(s,"CHOOSE_CARD_TYPE");const char*types[]={"ATTACK","SKILL","POWER"};for(unsigned i=0;i<2;i++){e.values[i]=int16_t(list[i]);option_key(s,i,"CHOOSE_CARD_TYPE.options.",types[list[i]]);}}
   else{e.values[2]=e.values[option];e.page=2;page(s,"CHOOSE_RIDER");const char*riders[]={"SAPPING","VIOLENCE","CHOKING","ENERGIZED","WISDOM","CHAOS","EXPERTISE","CURIOUS","IMPROVEMENT"};for(unsigned i=0;i<2;i++){e.values[i]=int16_t(e.values[2]*3+list[i]+1);option_key(s,i,"CHOOSE_RIDER.options.",riders[e.values[i]-1]);}}return true;}
  push_step(s,OriginalEventOp::Special,CX_Tinker,e.values[2],e.values[option]);done="DONE";break;
 case OriginalEventId::WelcomeToWongos:
  if(option==3){uint16_t ids[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].upgraded)ids[n++]=uint16_t(i);if(n){const unsigned at=ids[rand_range(s,0,int(n))];s.deck[at].upgraded=0;notify_history(s,RunHistoryEvent::CardDowngraded,unsigned(card_token(s.deck[at])));}done="LEAVE";}
  else{gold(s,-100*int(option+1));if(option==0)push_step(s,OriginalEventOp::Special,CX_Wongo,0);else relic(s,option==1?RelicId(e.values[0]):WorldRelicWongosMysteryTicket);push_step(s,OriginalEventOp::Special,CX_Wongo,int(option+1));return true;}break;
 case OriginalEventId::CrystalSphere:if(!option)gold(s,-e.values[0]);else card(s,EventCardDebt);push_step(s,OriginalEventOp::Special,CX_CrystalBegin,option?6:3);return true;
 case OriginalEventId::EndlessConveyor:
  if(option){if(!e.page){push_step(s,OriginalEventOp::Special,CX_RandomUpgrade);done="OBSERVE_CHEF";}else done="LEAVE";break;}
  if(e.values[1]!=6)gold(s,-40);switch(e.values[1]){case 0:maxhp(s,4);break;case 1:push_step(s,OriginalEventOp::Special,CX_RandomUpgrade);break;case 2:select_deck(s,OriginalEventSelection::Transform);break;case 3:push_step(s,OriginalEventOp::Special,CX_ConveyorCard);break;case 4:queue_random_potion(s,WorldPotionUniform);break;case 5:healing(s,10);break;case 6:gold(s,75);break;case 7:card(s,EventCardFeedingFrenzy);break;default:return false;}push_step(s,OriginalEventOp::Special,CX_ConveyorNext);return true;
 case OriginalEventId::FakeMerchant:if(option==6){for(unsigned i=0;i<6;++i)if(e.offers[i]<WorldRelicKinds)notify_history(s,RunHistoryEvent::RelicSkipped,e.offers[i]);finish_page(s,"INITIAL");return true;}gold(s,-e.choice_prices[option]);relic(s,e.offers[option]);push_step(s,OriginalEventOp::Special,CX_ShopAfter,int(option),e.choice_prices[option]);return true;
 case OriginalEventId::TheArchitect:++e.values[1];architect_page(s);return true;
 default:return false;
 }
 if(!done)return false;finish_page(s,done);return true;
}
bool complex_special(State&s,int op,int a,int b,int c,int){auto&e=s.original_events;
 switch(op){
 case CX_DiscardPotion:{if(a<0||a>=int(potion_slot_count(s)))return false;Potion old=potion_at(s,unsigned(a));if(old==Potion::Empty)return false;if(!potion_set(s,unsigned(a),Potion::Empty))return false;notify_history(s,RunHistoryEvent::PotionDiscarded,unsigned(old));world_relic_hook(s,WorldRelicHook::AfterPotionDiscarded,int(old),a);return true;}
 case CX_RandomUpgrade:if(b)rand_range(s,0,100);else random_one_upgrade(s,a!=0);return true;
 case CX_Tablet:{if(!a){const int loss=e.values[0];const bool lethal=loss>=s.max_hp;
   maxhp(s,lethal?1-s.max_hp:-loss);if(lethal)push_step(s,OriginalEventOp::KillPlayer);
   push_step(s,OriginalEventOp::Special,CX_Tablet,1,lethal?0:1);return true;
  }if(b)random_one_upgrade(s,e.page==4);++e.page;
  const char*pages[]={"","DECIPHER_1","DECIPHER_2","DECIPHER_3","DECIPHER_4","DECIPHER_5"};if(e.page==5){finish_page(s,pages[e.page]);return true;}
  e.values[0]=e.page==1?6:e.page==2?12:e.page==3?24:int16_t(s.max_hp-1);page(s,pages[e.page]);options(s);const char*prefix[]={"","DECIPHER_1.options.","DECIPHER_2.options.","DECIPHER_3.options.","DECIPHER_4.options."};option_key(s,0,prefix[e.page],"DECIPHER");option_key(s,1,"DECIPHER.options.","GIVE_UP");return true;}
 case CX_RemoveAt:return event_remove_deck_card(s,a);
 case CX_RemoveQuest:{unsigned left=unsigned(a);for(unsigned i=0;i<s.deck_count&&left;)if(s.deck[i].kind==EventCardLanternKey){notify_history(s,RunHistoryEvent::QuestCompleted,EventCardLanternKey);EventDeckSnapshot snap;if(!event_take_deck_card(s,int(i),snap))return false;notify_history(s,RunHistoryEvent::CardRemoved,unsigned(card_token(snap.card)));world_relic_hook(s,WorldRelicHook::AfterDeckRemoved,EventCardLanternKey,int(i));--left;}else ++i;return true;}
 case CX_RepyDone:{if(e.values[14])s.event_run.freed_repy=1;bool key=false;for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind==EventCardLanternKey)key=true;
  if(!b&&key){e.page=1;e.values[0]=int16_t(1-a);page(s,a?"UNLOCK_CHEST":"UNLOCK_CAGE");options(s,a?"INITIAL.options.UNLOCK_CAGE":"INITIAL.options.UNLOCK_CHEST");}
  else finish_page(s,b?(a?"EXTRA_UNLOCK_CHEST":"EXTRA_UNLOCK_CAGE"):(a?"UNLOCK_CHEST":"UNLOCK_CAGE"));return true;}
 case CX_Dummy:if(enemy_encounter_timed_out(s)){finish_page(s,"DEFEAT");return true;}
  if(e.values[0]==1)push_step(s,OriginalEventOp::RandomUpgrade,2,1);else{reward_reset(s);if(e.values[0]==0){if(!reward_random_potion(s,WorldPotionUniform))return false;}else if(!reward_random_relic(s))return false;if(!reward_offer(s))return false;}finish_page(s,"VICTORY");return true;
 case CX_Bundle:{reward_reset(s);if(a<0){for(int i=0;i<b;i++)if(!reward_random_relic(s))return false;b=0;a=-a;}for(int i=0;i<a;i++)if(!reward_random_potion(s))return false;for(int i=0;i<b;i++)if(!reward_random_relic(s))return false;
  for(int i=0;i<c;i++){WorldCardOptions o;o.flags|=WorldIsCardReward;if(!reward_cards(s,3,o))return false;}return reward_offer(s);}
 case CX_RelicOffer:reward_reset(s);if(!reward_relic(s,RelicId(a)))return false;return reward_offer(s);
 case CX_FakeRewards:{reward_reset(s);if(!reward_relic(s,WorldRelicFakeMerchantsRug))return false;for(unsigned i=0;i<6;i++)if(e.values[2+i]>=0&&!reward_relic(s,RelicId(e.values[2+i])))return false;return reward_offer(s);}
 case CX_Future:{WorldCardOptions o;o.odds=WorldCardOdds::Uniform;o.rarity_mask=world_rarity_mask(unsigned(a));o.type_mask=world_type_mask(CardType(b));o.flags|=WorldIsCardReward|WorldNoRarityModification|WorldNoCardPoolModifications;
  reward_reset(s);if(!reward_cards(s,3,o,false,true))return false;return reward_offer(s);}
 case CX_ConveyorNext:e.page=1;page(s,"GRAB_SOMETHING_OFF_THE_BELT");conveyor_roll(s);return true;
 case CX_Colorful:{reward_reset(s);for(unsigned rarity=2;rarity<=4;rarity++){WorldCardOptions o;o.pools[0]=WorldCardPool(a);o.odds=WorldCardOdds::Uniform;o.rarity_mask=world_rarity_mask(rarity);o.flags=WorldIsCardReward|WorldNoRarityModification|WorldNoCardPoolModifications;if(!reward_cards(s,3,o))return false;}return reward_offer(s);}
 case CX_Tinker:{EventCardFacts facts{};facts.science_type=uint8_t(a);facts.rider=uint8_t(b);return event_add_deck_card(s,{EventCardMadScience,0},&facts)>=0;}
 case CX_CrystalBegin:return crystal_begin(s,unsigned(a));
 case CX_CrystalClear:return crystal_clear(s,unsigned(a),unsigned(b));
 case CX_CrystalNext:return crystal_next(s);
 case CX_ShopAfter:if(a<0||a>=6)return false;notify_history(s,RunHistoryEvent::RelicBought,e.offers[a],unsigned(b));e.offers[a]=NoWorldRelic;world_relic_hook(s,WorldRelicHook::AfterItemPurchased,ShopRelicStart+a,b);return true;
 case CX_Wongo:
  if(!a)return gain_world_relic(s,world_relic_pull(s,WorldRelicPool::Common,false,true));
  {unsigned points=a==1?32:a==2?16:8;s.event_run.wongo_earned=points;unsigned now=s.event_run.wongo_total%2000+points;if(now>=2000){relic(s,WorldRelicWongoCustomerAppreciationBadge);finish_page(s,"AFTER_BUY_RECEIVE_BADGE");}else finish_page(s,(s.event_run.wongo_total+points)/2000?"AFTER_BUY_BADGE_COUNTER":"AFTER_BUY");return true;}
 case CX_BridgeNext:{++e.values[0];bridge_card(s,false);const char*pages[]={"HOLD_ON_0","HOLD_ON_1","HOLD_ON_2","HOLD_ON_3","HOLD_ON_4","HOLD_ON_5","HOLD_ON_6","HOLD_ON_LOOP"};unsigned before=unsigned(smaller(e.values[0]-1,7)),after=unsigned(smaller(e.values[0],7));page(s,pages[before]);options(s,"INITIAL.options.OVERCOME");char prefix[64]{};unsigned at=0;for(unsigned i=0;pages[before][i];i++)prefix[at++]=pages[before][i];const char*middle=".options.";for(unsigned i=0;middle[i];i++)prefix[at++]=middle[i];option_key(s,1,prefix,pages[after]);return true;}
 case CX_ConveyorCard:{WorldCardOptions o;o.pools[0]=WorldCardPool::Colorless;Card result;EventCardFacts facts;if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),1,o,&result,&facts))return false;return event_add_deck_card(s,result,&facts)>=0;}
 default:return false;
 }
}
#include "original_event_crystal.inc"
