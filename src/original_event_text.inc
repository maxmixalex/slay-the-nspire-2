// Locale-independent references for dynamic original event templates. These
// are identifiers, never copied original prose; text stays in the private pack.
namespace {
bool oe_reference(OriginalTextReference&out,OriginalTextReferenceKind kind,unsigned id,const char*table,const char*key,const char*suffix=""){
 out=OriginalTextReference{};out.kind=kind;out.model=uint16_t(id);out.table=table;
 const char*parts[]={key,suffix};unsigned at=0;for(const char*p:parts){if(!p)return false;for(unsigned i=0;p[i];i++){if(at+1>=sizeof out.key)return false;out.key[at++]=p[i];}}out.key[at]=0;return true;
}
bool oe_card(OriginalTextReference&out,Card card){return card.kind<CardKinds&&oe_reference(out,OriginalTextReferenceKind::Card,card_token(card),"cards",definitions[card.kind].key,".title");}
bool oe_relic(OriginalTextReference&out,RelicId id,bool description=false){return id<WorldRelicKinds&&oe_reference(out,description?OriginalTextReferenceKind::RelicDescription:OriginalTextReferenceKind::Relic,id,"relics",world_relic_key(id),description?".description":".title");}
bool oe_potion(OriginalTextReference&out,Potion id){const auto*d=potion_definition(id);return d&&oe_reference(out,OriginalTextReferenceKind::Potion,unsigned(id),"potions",d->key,".title");}
bool oe_enchantment(OriginalTextReference&out,EventEnchantment id,bool description=false){return id!=EventEnchantment::None&&unsigned(id)<EventEnchantmentKinds&&oe_reference(out,description?OriginalTextReferenceKind::EnchantmentDescription:OriginalTextReferenceKind::Enchantment,unsigned(id),"enchantments",event_enchantment_key(id),description?".description":".title");}
bool oe_key(OriginalTextReference&out,const char*table,const char*key){return oe_reference(out,OriginalTextReferenceKind::Localized,0,table,key);}
}
bool original_event_description_reference(const State&s,const char*key,int option,bool description,OriginalTextReference&out){
 out=OriginalTextReference{};const auto&e=s.original_events;if(!e.active||!key||option< -1||option>=int(e.option_count))return false;
 switch(e.id){
 case OriginalEventId::Amalgamator:if(same(key,"Card1"))return oe_card(out,{UltimateStrike,0});if(same(key,"Card2"))return oe_card(out,{UltimateDefend,0});break;
 case OriginalEventId::Bugslayer:if(same(key,"Card1"))return oe_card(out,{EventCardExterminate,0});if(same(key,"Card2"))return oe_card(out,{EventCardSquash,0});break;
 case OriginalEventId::ByrdonisNest:if(same(key,"Card"))return oe_card(out,{EventCardByrdonisEgg,0});break;
 case OriginalEventId::CrystalSphere:if(same(key,"CurseTitle"))return oe_card(out,{EventCardDebt,0});break;
 case OriginalEventId::DrowningBeacon:if(same(key,"Potion"))return oe_potion(out,Potion::GlowwaterPotion);if(same(key,"Relic"))return oe_relic(out,WorldRelicFresnelLens);break;
 case OriginalEventId::FieldOfManSizedHoles:if(same(key,"ResistCurse"))return oe_card(out,{EventCardNormality,0});if(same(key,"Enchantment"))return oe_enchantment(out,EventEnchantment::PerfectFit);break;
 case OriginalEventId::GraveOfTheForgotten:if(same(key,"Relic"))return oe_relic(out,WorldRelicForgottenSoul);if(same(key,"Enchantment"))return oe_enchantment(out,EventEnchantment::SoulsPower);if(same(key,"Curse"))return oe_card(out,{EventCardDecay,0});break;
 case OriginalEventId::LostWisp:if(same(key,"Relic"))return oe_relic(out,WorldRelicLostWisp);if(same(key,"Curse"))return oe_card(out,{EventCardDecay,0});break;
 case OriginalEventId::RoundTeaParty:if(same(key,"Relic"))return oe_relic(out,WorldRelicRoyalPoison);break;
 case OriginalEventId::SapphireSeed:if(same(key,"Enchantment"))return oe_enchantment(out,EventEnchantment::Sown);break;
 case OriginalEventId::SelfHelpBook:if(same(key,"Enchantment1"))return oe_enchantment(out,EventEnchantment::Sharp);if(same(key,"Enchantment2"))return oe_enchantment(out,EventEnchantment::Nimble);if(same(key,"Enchantment3"))return oe_enchantment(out,EventEnchantment::Swift);break;
 case OriginalEventId::SunkenStatue:if(same(key,"Relic"))return oe_relic(out,WorldRelicSwordOfStone);break;
 case OriginalEventId::Symbiote:if(same(key,"Enchantment"))return oe_enchantment(out,EventEnchantment::Corrupted);break;
 case OriginalEventId::ThisOrThat:if(same(key,"Curse"))return oe_card(out,{EventCardClumsy,0});break;
 case OriginalEventId::WoodCarvings:if(same(key,"BirdCard"))return oe_card(out,{EventCardPeck,0});if(same(key,"ToricCard"))return oe_card(out,{EventCardToricToughness,0});if(same(key,"SnakeEnchantment"))return oe_enchantment(out,EventEnchantment::Slither);break;
 case OriginalEventId::TeaMaster:
  if(same(key,"BoneTeaDescription"))return oe_relic(out,WorldRelicBoneTea,true);
  if(same(key,"EmberTeaDescription"))return oe_relic(out,WorldRelicEmberTea,true);
  if(same(key,"TeaOfDiscourtesyDescription"))return oe_relic(out,WorldRelicTeaOfDiscourtesy,true);break;
 case OriginalEventId::StoneOfAllTime:if(same(key,"DrinkRandomPotion"))return oe_potion(out,Potion(e.values[2]));break;
 case OriginalEventId::RanwidTheElder:if(same(key,"Potion"))return oe_potion(out,Potion(e.values[2]));if(same(key,"Relic"))return oe_relic(out,RelicId(e.values[1]));break;
 case OriginalEventId::SlipperyBridge:if(same(key,"RandomCard")&&e.values[2]>=0){bool okay=oe_card(out,card_from_token(unsigned(e.values[2])));if(e.values[1]>=0&&e.values[1]<s.deck_count)out.facts=s.event_cards.deck[e.values[1]];return okay;}break;
 case OriginalEventId::WelcomeToWongos:if(same(key,"RandomRelic"))return oe_relic(out,RelicId(e.values[0]));break;
 case OriginalEventId::DollRoom:if(same(key,"RelicName")&&e.page&&option>=0&&unsigned(option)<e.offer_count)return oe_relic(out,e.offers[option]);break;
 case OriginalEventId::RelicTrader:{const char*keys[]={"TopRelicOwned","MiddleRelicOwned","BottomRelicOwned","TopRelicNew","MiddleRelicNew","BottomRelicNew"};for(unsigned i=0;i<6;i++)if(same(key,keys[i]))return oe_relic(out,RelicId(e.values[i]));break;}
 case OriginalEventId::EndlessConveyor:if(same(key,"CurrentDishTitle")||same(key,"LastDishTitle")){int dish=e.values[same(key,"CurrentDishTitle")?1:2];if(dish<0||dish>=8)return false;char entry[96]{};const char*parts[]={"ENDLESS_CONVEYOR.DISHES.",dish_key(unsigned(dish)),".title"};unsigned at=0;for(const char*p:parts)for(unsigned i=0;p[i];i++)entry[at++]=p[i];return oe_key(out,"events",entry);}break;
 case OriginalEventId::Trial:
  if(same(key,"TrialStory")&&e.page>=2&&e.page<=4){const char*keys[]={"TRIAL.pages.MERCHANT.description","TRIAL.pages.NOBLE.description","TRIAL.pages.NONDESCRIPT.description"};return oe_key(out,"events",keys[e.page-2]);}
  if(same(key,"TrialResult")&&e.finished){char entry[128]{};const char*parts[]={"TRIAL.pages.",e.page_key,".description"};unsigned at=0;for(const char*p:parts)for(unsigned i=0;p[i];i++)entry[at++]=p[i];return oe_key(out,"events",entry);}break;
 case OriginalEventId::AromaOfChaos:case OriginalEventId::SunkenTreasury:
  if((e.id==OriginalEventId::AromaOfChaos&&same(key,"AromaPrinciple"))||(e.id==OriginalEventId::SunkenTreasury&&same(key,"Monologue"))){const char*characters[]={"IRONCLAD","SILENT","DEFECT","REGENT","NECROBINDER"};return oe_reference(out,OriginalTextReferenceKind::Localized,unsigned(s.character),"characters",characters[unsigned(s.character)],e.id==OriginalEventId::AromaOfChaos?".aromaPrinciple":".goldMonologue");}break;
 case OriginalEventId::TheFutureOfPotions:if(option>=0&&option<3){Potion potion=potion_at(s,unsigned(e.values[option]));const auto*d=potion_definition(potion);if(!d)return false;
  if(same(key,"Potion"))return oe_potion(out,potion);
  if(same(key,"Type")){const char*types[]={"CARD_TYPE.ATTACK","CARD_TYPE.SKILL","CARD_TYPE.POWER"};int type=e.values[3+option];return type>=0&&type<3&&oe_key(out,"gameplay_ui",types[type]);}
  if(same(key,"Rarity")){const char*potions[]={"","POTION_RARITY.COMMON","POTION_RARITY.UNCOMMON","POTION_RARITY.RARE","POTION_RARITY.EVENT","POTION_RARITY.TOKEN"};const char*cards[]={"CARD_RARITY.COMMON","CARD_RARITY.UNCOMMON","CARD_RARITY.RARE"};if(d->rarity<1||d->rarity>5)return false;return oe_key(out,"gameplay_ui",description?cards[d->rarity==2?1:d->rarity==3||d->rarity==4?2:0]:potions[d->rarity]);}
 }break;
 default:break;
 }return false;
}
