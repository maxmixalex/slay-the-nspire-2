// Independently expressed bounded state machines, audited against the owner's
// v0.111.0 / 41cef1ea EventModel methods. No original C# is distributed here.
enum EventSpecial { ES_SetSelection=1,ES_GenerateCard,ES_Reflections,ES_DuplicateAll,
 ES_FixedOffer,ES_GridReward,ES_Continue,ES_Custom=100 };
bool push_step(State&s,OriginalEventOp op,int a=0,int b=0,int c=0,int d=0,int f=0){
 auto&e=s.original_events;if(e.step_count>=MaxOriginalEventSteps&&e.step_cursor){unsigned left=e.step_count-e.step_cursor;for(unsigned i=0;i<left;i++)e.steps[i]=e.steps[e.step_cursor+i];for(unsigned i=left;i<MaxOriginalEventSteps;i++)e.steps[i]={};e.step_count=uint8_t(left);e.step_cursor=0;}
 if(e.step_count>=MaxOriginalEventSteps){fail(s);return false;}
 e.steps[e.step_count++]={op,int16_t(a),int16_t(b),int16_t(c),int16_t(d),int16_t(f)};return true;
}
void page(State&s,const char*key){auto&e=s.original_events;if(!copy_text(e.page_key,key))fail(s);e.finished=0;}
void options(State&s,const char*first=nullptr,const char*second=nullptr,const char*third=nullptr,const char*fourth=nullptr){auto&e=s.original_events;
 const char*keys[]={first,second,third,fourth};e.option_count=0;for(auto&key:e.option_keys)copy_text(key,"");
 for(unsigned i=0;i<4;i++)if(keys[i]){if(!copy_text(e.option_keys[e.option_count],keys[i]))fail(s);e.options[e.option_count]=uint8_t(i);++e.option_count;}
}
void finish_page(State&s,const char*key){page(s,key);s.original_events.option_count=0;push_step(s,OriginalEventOp::Finish);}
void option_key(State&s,unsigned index,const char*prefix,const char*key){auto&e=s.original_events;if(index>=MaxOriginalEventOptions){fail(s);return;}
 char text[64]{};unsigned at=0;const char*parts[]={prefix,key};for(const char*part:parts)for(unsigned n=0;part[n];n++){if(at+1>=sizeof text){fail(s);return;}text[at++]=part[n];}
 copy_text(e.option_keys[index],text);e.options[index]=uint8_t(index);if(e.option_count<=index)e.option_count=uint8_t(index+1);
}
unsigned count_removable(const State&s){unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(card_removable(s,s.deck[i],i))++n;return n;}
unsigned count_transformable(const State&s){unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(card_transformable(s,s.deck[i],i))++n;return n;}
unsigned count_enchantable(const State&s,EventEnchantment enchant){unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(event_card_can_enchant(s,i,enchant))++n;return n;}
void select_deck(State&s,OriginalEventSelection kind,unsigned count=1,OriginalEventFilter filter=OriginalEventFilter::Any,EventEnchantment enchant=EventEnchantment::None,int amount=1,int extra=0){
 push_step(s,OriginalEventOp::Special,ES_SetSelection,int(enchant),amount,extra);
 push_step(s,OriginalEventOp::Select,int(kind),int(filter),int(count),int(count));
}
void card(State&s,CardKind kind,bool upgrade=false){push_step(s,definitions[kind].type==CardType::Curse?OriginalEventOp::AddCurse:OriginalEventOp::AddCard,int(kind),1,upgrade);}
void relic(State&s,RelicId id){push_step(s,OriginalEventOp::GainRelic,id);}
void damage(State&s,int value){push_step(s,OriginalEventOp::Damage,value);}
void healing(State&s,int value){push_step(s,OriginalEventOp::Heal,value);}
void gold(State&s,int value){push_step(s,OriginalEventOp::Gold,value);}
void maxhp(State&s,int value){push_step(s,OriginalEventOp::MaxHp,value);}
void queue_random_relic(State&s){push_step(s,OriginalEventOp::RandomRelic,int(WorldRelicPool::Reward));}
void queue_random_potion(State&s,unsigned rarity=0){push_step(s,OriginalEventOp::RandomPotion,int(rarity),1);}
void combat(State&s,const char*key,bool resume=false){auto id=encounter_id(key);if(id==EncounterId::None){fail(s);return;}push_step(s,OriginalEventOp::Combat,int(id),resume);}
void fixed_offer(State&s,CardKind kind){push_step(s,OriginalEventOp::Special,ES_FixedOffer,int(kind));}
int rand_range(State&s,int low,int high){auto rng=event_random(s);int value=rng.range(low,high);keep_random(s,rng);return value;}
bool ancient(OriginalEventId id){switch(id){case OriginalEventId::Neow:case OriginalEventId::Darv:case OriginalEventId::Nonupeipe:case OriginalEventId::Orobas:case OriginalEventId::Pael:case OriginalEventId::Tanx:case OriginalEventId::Tezcatara:case OriginalEventId::Vakuu:return true;default:return false;}}
#include "original_event_modifiers.inc"
#include "original_event_ancients.inc"
struct SimpleEvent {OriginalEventId id;const char*a;const char*b;const char*c;};
const SimpleEvent simple_events[]={
 {OriginalEventId::AromaOfChaos,"LET_GO","MAINTAIN_CONTROL",nullptr},
 {OriginalEventId::DenseVegetation,"TRUDGE_ON","REST",nullptr},
 {OriginalEventId::MorphicGrove,"GROUP","LONER",nullptr},
 {OriginalEventId::DoorsOfLightAndDark,"LIGHT","DARK",nullptr},
 {OriginalEventId::HungryForMushrooms,"BIG_MUSHROOM","FRAGRANT_MUSHROOM",nullptr},
 {OriginalEventId::Bugslayer,"EXTERMINATION","SQUASH",nullptr},
 {OriginalEventId::ByrdonisNest,"EAT","TAKE",nullptr},
 {OriginalEventId::ThisOrThat,"PLAIN","ORNATE",nullptr},
 {OriginalEventId::SunkenStatue,"GRAB_SWORD","DIVE_INTO_WATER",nullptr},
 {OriginalEventId::SunkenTreasury,"FIRST_CHEST","SECOND_CHEST",nullptr},
 {OriginalEventId::DrowningBeacon,"BOTTLE","CLIMB",nullptr},
 {OriginalEventId::LostWisp,"CLAIM","SEARCH",nullptr},
 {OriginalEventId::Wellspring,"BOTTLE","BATHE",nullptr},
 {OriginalEventId::TheLanternKey,"RETURN_THE_KEY","KEEP_THE_KEY",nullptr},
 {OriginalEventId::UnrestSite,"REST","KILL",nullptr},
 {OriginalEventId::PotionCourier,"GRAB_POTIONS","RANSACK",nullptr},
 {OriginalEventId::InfestedAutomaton,"STUDY","TOUCH_CORE",nullptr},
 {OriginalEventId::SpiritGrafter,"LET_IT_IN","REJECTION",nullptr},
 {OriginalEventId::RoomFullOfCheese,"GORGE","SEARCH",nullptr},
 {OriginalEventId::SpiralingWhirlpool,"OBSERVE","DRINK",nullptr},
 {OriginalEventId::SapphireSeed,"EAT","PLANT",nullptr},
 {OriginalEventId::RoundTeaParty,"ENJOY_TEA","PICK_FIGHT",nullptr},
 {OriginalEventId::WhisperingHollow,"GOLD","HUG",nullptr},
 {OriginalEventId::TrashHeap,"DIVE_IN","GRAB",nullptr},
 {OriginalEventId::TheLegendsWereTrue,"NAB_THE_MAP","SLOWLY_FIND_AN_EXIT",nullptr},
 {OriginalEventId::LuminousChoir,"REACH_INTO_THE_FLESH","OFFER_TRIBUTE",nullptr},
 {OriginalEventId::Reflections,"TOUCH_A_MIRROR","SHATTER",nullptr},
 {OriginalEventId::FieldOfManSizedHoles,"RESIST","ENTER_YOUR_HOLE",nullptr},
 {OriginalEventId::BrainLeech,"SHARE_KNOWLEDGE","RIP",nullptr},
 {OriginalEventId::TeaMaster,"BONE_TEA","EMBER_TEA","TEA_OF_DISCOURTESY"},
 {OriginalEventId::GraveOfTheForgotten,"CONFRONT","ACCEPT",nullptr},
 {OriginalEventId::Symbiote,"APPROACH","KILL_WITH_FIRE",nullptr},
 {OriginalEventId::ZenWeaver,"BREATHING_TECHNIQUES","EMOTIONAL_AWARENESS","ARACHNID_ACUPUNCTURE"},
};
bool complex_implemented(OriginalEventId);
bool complex_begin(State&);
bool complex_enabled(const State&,unsigned);
bool complex_choose(State&,unsigned);
bool complex_allowed(const State&,OriginalEventId,unsigned,unsigned);
bool complex_special(State&,int,int,int,int,int);
bool rules_implemented(OriginalEventId id){if(ancient(id)||id==OriginalEventId::AbyssalBaths||id==OriginalEventId::ColorfulPhilosophers)return true;for(auto&entry:simple_events)if(id==entry.id)return true;return complex_implemented(id);}
bool rules_allowed(const State&s,OriginalEventId id,unsigned act,unsigned floor){
 switch(id){
 case OriginalEventId::MorphicGrove:return s.gold>=100&&count_transformable(s)>=2;
 case OriginalEventId::ByrdonisNest:return !world_has_event_pet(s);
 case OriginalEventId::UnrestSite:return s.hp*100<=s.max_hp*70;
 case OriginalEventId::PotionCourier:case OriginalEventId::Symbiote:return act>0;
 case OriginalEventId::RoomFullOfCheese:case OriginalEventId::BrainLeech:return act<2;
 case OriginalEventId::SpiralingWhirlpool:return count_enchantable(s,EventEnchantment::Spiral)>0;
 case OriginalEventId::RoundTeaParty:return s.hp>=12;
 case OriginalEventId::WhisperingHollow:return s.gold>=44;
 case OriginalEventId::TrashHeap:return s.hp>5;
 case OriginalEventId::TheLegendsWereTrue:return act==0&&s.deck_count>0&&s.hp>=10;
 case OriginalEventId::LuminousChoir:{RelicId ids[WorldRelicKinds];return s.gold>=149&&world_relic_candidates(s,ids,WorldRelicKinds,WorldRelicPool::Reward)>0;}
 case OriginalEventId::FieldOfManSizedHoles:return count_enchantable(s,EventEnchantment::PerfectFit)>0;
 case OriginalEventId::TeaMaster:return act<2&&s.gold>=150;
 case OriginalEventId::GraveOfTheForgotten:return count_enchantable(s,EventEnchantment::SoulsPower)>0;
 case OriginalEventId::ZenWeaver:return s.gold>=125;
 default:return complex_implemented(id)?complex_allowed(s,id,act,floor):rules_implemented(id);
 }
}
bool rules_begin(State&s){auto&e=s.original_events;page(s,"INITIAL");
 if(ancient(e.id))return ancient_begin(s);
 if(e.id==OriginalEventId::AbyssalBaths){e.values[0]=3;options(s,"INITIAL.options.IMMERSE","INITIAL.options.ABSTAIN");return true;}
 if(e.id==OriginalEventId::ColorfulPhilosophers){
  const WorldCardPool order[]={WorldCardPool::Necrobinder,WorldCardPool::Ironclad,WorldCardPool::Regent,WorldCardPool::Silent,WorldCardPool::Defect};
  const char*colors[]={"NECROBINDER","IRONCLAD","REGENT","SILENT","DEFECT"};
  unsigned list[5],n=0;for(unsigned i=0;i<5;i++)if(unsigned(order[i])!=unsigned(s.character))list[n++]=i;
  while(n>3){unsigned at=unsigned(rand_range(s,0,int(n)));for(unsigned i=at+1;i<n;i++)list[i-1]=list[i];--n;}
  for(unsigned i=0;i<n;i++){option_key(s,i,"INITIAL.options.",colors[list[i]]);e.values[i]=int16_t(order[list[i]]);}return true;
 }
 bool found=false;for(auto&entry:simple_events)if(entry.id==e.id){option_key(s,0,"INITIAL.options.",entry.a);option_key(s,1,"INITIAL.options.",entry.b);if(entry.c)option_key(s,2,"INITIAL.options.",entry.c);found=true;break;}
 if(!found)return complex_begin(s);
 switch(e.id){
 case OriginalEventId::DenseVegetation:e.values[0]=int16_t(rand_range(s,61,100));e.values[1]=int16_t(rest_heal_amount(s));break;
 case OriginalEventId::ThisOrThat:e.values[0]=int16_t(rand_range(s,41,69));break;
 case OriginalEventId::SunkenStatue:e.values[0]=int16_t(111+rand_range(s,-10,11));break;
 case OriginalEventId::SunkenTreasury:e.values[0]=int16_t(60+rand_range(s,0,16)-8);e.values[1]=int16_t(333+rand_range(s,0,61)-30);break;
 case OriginalEventId::LostWisp:e.values[0]=int16_t(60+rand_range(s,-15,16));break;
 case OriginalEventId::UnrestSite:e.values[0]=int16_t(s.max_hp-s.hp);break;
 case OriginalEventId::SpiralingWhirlpool:e.values[0]=int16_t(s.max_hp*33/100);break;
 case OriginalEventId::WhisperingHollow:e.values[0]=int16_t(35+rand_range(s,-9,10));break;
 case OriginalEventId::LuminousChoir:e.values[0]=int16_t(149-rand_range(s,0,50));break;
 default:break;
 }return true;
}
bool rules_enabled(const State&s,unsigned option){const auto&e=s.original_events;
 if(e.id==OriginalEventId::Neow&&s.run_options.modifiers)return option==0&&current_neow_modifier(s)!=RunModifier::Count;
 if(ancient(e.id))return option<e.offer_count&&e.offers[option]<WorldRelicKinds;
 switch(e.id){
 case OriginalEventId::AbyssalBaths:return option==1||(option==0&&s.max_hp<=997&&e.values[0]<9999);
 case OriginalEventId::MorphicGrove:return option==1||count_transformable(s)>=2;
 case OriginalEventId::TeaMaster:return option==2||s.gold>=(option?150:50);
 case OriginalEventId::ZenWeaver:return s.gold>=(option==0?50:option==1?125:250)&&(option||s.deck_count+2<=MaxDeck);
 case OriginalEventId::WhisperingHollow:case OriginalEventId::LuminousChoir:return (e.id==OriginalEventId::WhisperingHollow?option!=0:option!=1)||s.gold>=e.values[0];
 case OriginalEventId::Symbiote:return option!=0||count_enchantable(s,EventEnchantment::Corrupted)>0;
 case OriginalEventId::GraveOfTheForgotten:return option!=0||count_enchantable(s,EventEnchantment::SoulsPower)>0;
 case OriginalEventId::Reflections:return option==0||s.deck_count*2+1<=MaxDeck;
 default:return complex_implemented(e.id)?complex_enabled(s,option):true;
 }
}
bool rules_choose(State&s,unsigned option){auto&e=s.original_events;
 if(e.id==OriginalEventId::Neow&&s.run_options.modifiers)return option==0&&modifier_neow_choose(s);
 if(ancient(e.id)){relic(s,e.offers[option]);finish_page(s,"DONE");return true;}
 if(e.id==OriginalEventId::ColorfulPhilosophers){push_step(s,OriginalEventOp::Special,ES_Continue,e.values[option]);finish_page(s,"DONE");return true;}
 const char*done=nullptr;for(auto&entry:simple_events)if(entry.id==e.id){done=option==0?entry.a:option==1?entry.b:entry.c;break;}
 switch(e.id){
 case OriginalEventId::AbyssalBaths:
  if(option==1){if(!e.page)healing(s,10);finish_page(s,e.page?"EXIT_BATHS":"ABSTAIN");return true;}
  if(!e.page){e.page=1;page(s,"IMMERSE");}
  else{e.values[1]=int16_t(smaller(9,e.values[1]+1));int resulting_hp=s.hp+2-e.values[0];if(resulting_hp<=e.values[0]-1)page(s,"DEATH_WARNING");else{char key[]="LINGER0";key[6]=char('0'+e.values[1]);page(s,key);}}
  maxhp(s,2);damage(s,e.values[0]);++e.values[0];options(s,"ALL.options.LINGER","ALL.options.EXIT_BATHS");return true;
 case OriginalEventId::AromaOfChaos:select_deck(s,option?OriginalEventSelection::Upgrade:OriginalEventSelection::Transform);break;
 case OriginalEventId::DenseVegetation:
  if(e.page){combat(s,"DenseVegetationEventEncounter");done="REST";break;}
  if(!option){damage(s,8);gold(s,e.values[0]);}else{push_step(s,OriginalEventOp::Rest);e.page=1;page(s,"REST");options(s,"REST.options.FIGHT");return true;}break;
 case OriginalEventId::MorphicGrove:if(!option){gold(s,-s.gold);select_deck(s,OriginalEventSelection::Transform,2);}else maxhp(s,5);break;
 case OriginalEventId::DoorsOfLightAndDark:if(!option)push_step(s,OriginalEventOp::RandomUpgrade,2,1);else select_deck(s,OriginalEventSelection::Remove);break;
 case OriginalEventId::HungryForMushrooms:relic(s,option?WorldRelicFragrantMushroom:WorldRelicBigMushroom);break;
 case OriginalEventId::Bugslayer:card(s,option?EventCardSquash:EventCardExterminate);break;
 case OriginalEventId::ByrdonisNest:if(option)card(s,EventCardByrdonisEgg);else maxhp(s,7);break;
 case OriginalEventId::ThisOrThat:if(!option){damage(s,6);gold(s,e.values[0]);}else{queue_random_relic(s);card(s,EventCardClumsy);}break;
 case OriginalEventId::SunkenStatue:if(!option)relic(s,WorldRelicSwordOfStone);else{gold(s,e.values[0]);damage(s,7);}break;
 case OriginalEventId::SunkenTreasury:gold(s,e.values[option]);if(option)card(s,EventCardGreed);break;
 case OriginalEventId::DrowningBeacon:if(!option)push_step(s,OriginalEventOp::Potion,int(Potion::GlowwaterPotion));else{maxhp(s,-13);relic(s,WorldRelicFresnelLens);}break;
 case OriginalEventId::LostWisp:if(!option){card(s,EventCardDecay);relic(s,WorldRelicLostWisp);}else gold(s,e.values[0]);break;
 case OriginalEventId::Wellspring:if(!option)queue_random_potion(s,WorldPotionUniform);else{select_deck(s,OriginalEventSelection::Remove);card(s,EventCardGuilty);}break;
 case OriginalEventId::TheLanternKey:
  if(e.page){combat(s,"MysteriousKnightEventEncounter");done="KEEP_THE_KEY";break;}
  if(!option){gold(s,100);done="DONE.options.RETURN_THE_KEY";}else{e.page=1;page(s,"KEEP_THE_KEY");options(s,"KEEP_THE_KEY.options.FIGHT");return true;}break;
 case OriginalEventId::UnrestSite:if(!option){healing(s,e.values[0]);card(s,EventCardPoorSleep);}else{maxhp(s,-8);queue_random_relic(s);}break;
 case OriginalEventId::PotionCourier:if(!option)push_step(s,OriginalEventOp::Special,ES_Continue,-1);else queue_random_potion(s,2);break;
 case OriginalEventId::InfestedAutomaton:push_step(s,OriginalEventOp::Special,ES_GenerateCard,int(option));break;
 case OriginalEventId::SpiritGrafter:if(!option){healing(s,25);card(s,EventCardMetamorphosis);}else{select_deck(s,OriginalEventSelection::Upgrade);damage(s,10);}break;
 case OriginalEventId::RoomFullOfCheese:if(!option)push_step(s,OriginalEventOp::Special,ES_GridReward,8,int(WorldCardPool::Own),2,2);else{damage(s,14);relic(s,WorldRelicChosenCheese);}break;
 case OriginalEventId::SpiralingWhirlpool:if(!option)select_deck(s,OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,EventEnchantment::Spiral);else healing(s,e.values[0]);break;
 case OriginalEventId::SapphireSeed:if(!option){healing(s,9);select_deck(s,OriginalEventSelection::Upgrade);}else select_deck(s,OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,EventEnchantment::Sown);break;
 case OriginalEventId::RoundTeaParty:
  if(e.page){damage(s,11);queue_random_relic(s);done="CONTINUE_FIGHT";}
  else if(!option){relic(s,WorldRelicRoyalPoison);healing(s,-1);}else{e.page=1;page(s,"PICK_FIGHT");options(s,"PICK_FIGHT.options.CONTINUE_FIGHT");return true;}break;
 case OriginalEventId::WhisperingHollow:if(!option){gold(s,-e.values[0]);push_step(s,OriginalEventOp::Special,ES_Continue,-2);}else{select_deck(s,OriginalEventSelection::Transform);damage(s,9);}break;
 case OriginalEventId::TrashHeap:if(!option){const RelicId pool[]={WorldRelicDarkstonePeriapt,WorldRelicDreamCatcher,WorldRelicHandDrill,WorldRelicMawBank,WorldRelicTheBoot};damage(s,8);relic(s,pool[rand_range(s,0,5)]);}else{const CardKind pool[]={EventCardCaltrops,EventCardClash,EventCardDistraction,EventCardDualWield,EventCardEntrench,EventCardHelloWorld,EventCardOutmaneuver,EventCardRebound,EventCardRipAndTear,EventCardStack};gold(s,100);card(s,pool[rand_range(s,0,10)]);}break;
 case OriginalEventId::TheLegendsWereTrue:if(!option)card(s,EventCardSpoilsMap);else{damage(s,8);queue_random_potion(s,WorldPotionUniform);}break;
 case OriginalEventId::LuminousChoir:if(!option){select_deck(s,OriginalEventSelection::Remove,2);card(s,EventCardSporeMind);}else{gold(s,-e.values[0]);queue_random_relic(s);}break;
 case OriginalEventId::Reflections:if(!option)push_step(s,OriginalEventOp::Special,ES_Reflections);else{push_step(s,OriginalEventOp::Special,ES_DuplicateAll);card(s,EventCardBadLuck);}break;
 case OriginalEventId::FieldOfManSizedHoles:if(!option){select_deck(s,OriginalEventSelection::Remove,2);card(s,EventCardNormality);}else select_deck(s,OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,EventEnchantment::PerfectFit);break;
 case OriginalEventId::BrainLeech:if(!option)push_step(s,OriginalEventOp::Special,ES_GridReward,5,int(WorldCardPool::Own),-1,1);else{damage(s,5);push_step(s,OriginalEventOp::Special,ES_GridReward,3,int(WorldCardPool::Colorless),-1,0);}break;
 case OriginalEventId::TeaMaster:if(option<2)gold(s,option?-150:-50);relic(s,option==0?WorldRelicBoneTea:option==1?WorldRelicEmberTea:WorldRelicTeaOfDiscourtesy);done=option==2?"TEA_OF_DISCOURTESY":"DONE";break;
 case OriginalEventId::GraveOfTheForgotten:if(!option){card(s,EventCardDecay);select_deck(s,OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,EventEnchantment::SoulsPower);}else relic(s,WorldRelicForgottenSoul);break;
 case OriginalEventId::Symbiote:select_deck(s,option?OriginalEventSelection::Transform:OriginalEventSelection::Enchant,1,OriginalEventFilter::Any,option?EventEnchantment::None:EventEnchantment::Corrupted);break;
 case OriginalEventId::ZenWeaver:if(!option){gold(s,-50);card(s,EventCardEnlightenment);card(s,EventCardEnlightenment);}else{select_deck(s,OriginalEventSelection::Remove,option);gold(s,option==1?-125:-250);}break;
 default:return complex_choose(s,option);
 }
 if(!done)return false;finish_page(s,done);return true;
}
bool event_special(State&s,int op,int a,int b,int c,int d){auto&e=s.original_events;
 switch(op){
 case ES_SetSelection:e.select_enchantment=uint16_t(a);e.select_amount=int16_t(b);e.select_extra=int16_t(c);return true;
 case ES_GenerateCard:{WorldCardOptions o;if(a){o.zero_energy=1;o.flags|=WorldNoCardPoolModifications;}else o.type_mask=world_type_mask(CardType::Power);Card result;EventCardFacts facts;
  if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),1,o,&result,&facts))return false;return event_add_deck_card(s,result,&facts)>=0;}
 case ES_GridReward:{WorldCardOptions o;o.pools[0]=WorldCardPool(b);o.odds=c<0?WorldCardOdds::RegularEncounter:WorldCardOdds::Uniform;if(c>0)o.rarity_mask=world_rarity_mask(c);
  if(c>=0)o.flags|=WorldNoRarityModification;if(!d){o.flags|=WorldIsCardReward|WorldNoRarityModification|WorldNoCardPoolModifications;if(e.id==OriginalEventId::ColorfulPhilosophers)o.flags&=~WorldNoUpgradeRoll;}
  if(!d){reward_reset(s);return reward_cards(s,unsigned(a),o)&&reward_offer(s);}
  Card cards[MaxOriginalEventOffers];EventCardFacts facts[MaxOriginalEventOffers];unsigned count=0;
  if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),unsigned(a),o,cards,facts,MaxOriginalEventOffers,&count))return false;
  e.offer_count=uint16_t(count);for(unsigned i=0;i<count;i++){e.offers[i]=uint16_t(card_token(cards[i]));e.offer_facts[i]=facts[i];}
  return pause_selection(s,OriginalEventSelection::OfferCard,OriginalEventFilter::Any,unsigned(d),d?unsigned(d):1,d==0);}
 case ES_FixedOffer:e.offer_count=1;e.offers[0]=uint16_t(card_token({CardKind(a),0}));e.offer_facts[0]={};return pause_selection(s,OriginalEventSelection::OfferCard,OriginalEventFilter::Any,0,1,true);
 case ES_Reflections:{auto rng=event_random(s);for(unsigned phase=0;phase<2;phase++){uint16_t list[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(phase?card_upgradeable(s.deck[i]):s.deck[i].upgraded!=0)list[n++]=uint16_t(i);
   for(unsigned i=0;i<(phase?4U:2U)&&n;i++){unsigned at=rng.below(n);s.deck[list[at]].upgraded=uint8_t(phase);if(phase)record(s,LogKind::Upgrade,s.deck[list[at]].kind);for(unsigned j=at+1;j<n;j++)list[j-1]=list[j];--n;}}
  keep_random(s,rng);return true;}
 case ES_DuplicateAll:{unsigned count=s.deck_count;if(count*2+1>MaxDeck)return false;e.selection=OriginalEventSelection::Duplicate;e.filter=OriginalEventFilter::Any;e.select_min=e.select_max=uint8_t(count);e.selected_count=uint16_t(count);e.selected_cursor=0;e.selection_committed=1;
  for(unsigned i=0;i<count;i++){e.selected[i]=uint16_t(i);e.selected_cards[i]={s.deck[i],s.event_cards.deck[i],s.defect.genetic_deck[i],s.necro.scythe_deck[i]};}apply_selection(s);return s.phase!=Phase::Error;}
 case ES_Continue:{reward_reset(s);if(a== -1){for(unsigned i=0;i<3;i++)if(!reward_potion(s,Potion::FoulPotion))return false;}
  else if(a== -2){if(!reward_random_potion(s)||!reward_random_potion(s))return false;}
  else for(unsigned rarity=2;rarity<=4;rarity++){WorldCardOptions o;o.pools[0]=WorldCardPool(a);o.odds=WorldCardOdds::Uniform;o.rarity_mask=world_rarity_mask(rarity);o.flags=WorldIsCardReward|WorldNoRarityModification|WorldNoCardPoolModifications;if(!reward_cards(s,3,o))return false;}return reward_offer(s);}
 default:return complex_special(s,op,a,b,c,d);
 }
}
#include "original_event_complex.inc"
