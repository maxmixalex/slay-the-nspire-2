// Original dynamic description bindings. No localization prose is embedded.
namespace {
const WorldRelicInstance* wrt_instance(const State&s,RelicId id,int position){
 if(position>=0)return unsigned(position)<s.world_relics.count&&s.world_relics.entries[position].id==id?&s.world_relics.entries[position]:nullptr;
 int at=w_first(s,id);return at>=0?&s.world_relics.entries[at]:nullptr;
}
bool wrt_reference(OriginalTextReference&out,OriginalTextReferenceKind kind,unsigned model,const char*table,const char*key,const char*suffix){
 out=OriginalTextReference{};out.kind=kind;out.model=uint16_t(model);out.table=table;const char*parts[]={key,suffix};unsigned at=0;
 for(const char*p:parts){if(!p)return false;for(unsigned i=0;p[i];i++){if(at+1>=sizeof out.key)return false;out.key[at++]=p[i];}}out.key[at]=0;return true;
}
bool wrt_card(OriginalTextReference&out,Card card,const EventCardFacts*facts=nullptr){if(card.kind>=CardKinds)return false;bool okay=wrt_reference(out,OriginalTextReferenceKind::Card,card_token(card),"cards",definitions[card.kind].key,".title");if(facts)out.facts=*facts;return okay;}
EventEnchantment wrt_enchantment(RelicId id,int&amount){amount=1;switch(id){
 case WorldRelicBeautifulBracelet:amount=2;return EventEnchantment::Swift;
 case WorldRelicElectricShrymp:return EventEnchantment::Imbued;
 case WorldRelicGnarledHammer:amount=3;return EventEnchantment::Sharp;
 case WorldRelicKifuda:amount=3;return EventEnchantment::Adroit;
 case WorldRelicNutritiousSoup:return EventEnchantment::TezcatarasEmber;
 case WorldRelicPaelsClaw:return EventEnchantment::Goopy;
 case WorldRelicPaelsGrowth:amount=4;return EventEnchantment::Clone;
 case WorldRelicPunchDagger:amount=5;return EventEnchantment::Momentum;
 case WorldRelicRoyalStamp:return EventEnchantment::RoyallyApproved;
 case WorldRelicTriBoomerang:return EventEnchantment::Instinct;
 default:return EventEnchantment::None;
}}
}
bool world_relic_description_value(const State&s,RelicId id,const char*key,int&value,int position){
 if(id>=WorldRelicKinds||!key)return false;const auto*r=wrt_instance(s,id,position);
 if(w_equal(key,"Combats")&&(id==WorldRelicBoneTea||id==WorldRelicEmberTea||id==WorldRelicTeaOfDiscourtesy)){value=r?w_max(0,int(r->values[0])):id==WorldRelicEmberTea?5:1;return true;}
 if(w_equal(key,"Rooms")&&id==WorldRelicWingedBoots){value=r?w_max(0,3-int(r->values[0])):3;return true;}
 if(w_equal(key,"RemainingCombats")&&id==WorldRelicWongosMysteryTicket){value=r?w_max(0,5-int(r->values[0])):5;return true;}
 return false;
}
bool world_relic_description_reference(const State&s,RelicId id,const char*key,OriginalTextReference&out,int position,unsigned item){
 out=OriginalTextReference{};if(id>=WorldRelicKinds||!key)return false;const auto*r=wrt_instance(s,id,position);
 // Prepared ancient offers take precedence over an earlier owned copy unless
 // the UI explicitly addresses an owned position.
 int prepared=position<0?world_relic_prepared_value(s,id):-1;
 if(id==WorldRelicTouchOfOrobas&&(w_equal(key,"StarterRelic")||w_equal(key,"UpgradedRelic"))){unsigned which=w_equal(key,"UpgradedRelic")?1:0;int model=prepared>=0?world_relic_prepared_value(s,id,which):r&&r->values[7]?int(r->values[which]):-1;
  return model>=0&&model<int(WorldRelicKinds)&&wrt_reference(out,OriginalTextReferenceKind::Relic,unsigned(model),"relics",world_relic_key(RelicId(model)),".title");}
 if(id==WorldRelicArchaicTooth&&(w_equal(key,"StarterCard")||w_equal(key,"AncientCard"))){unsigned which=w_equal(key,"AncientCard")?1:0;
  if(prepared>=0&&prepared<s.deck_count){Card card=s.deck[prepared];if(which)card.kind=CardKind(world_relic_prepared_value(s,id,1));return wrt_card(out,card,&s.event_cards.deck[prepared]);}
  return r&&r->values[7]&&wrt_card(out,card_from_token(r->card_tokens[which]),&r->card_facts[which]);}
 if(id==WorldRelicDustyTome&&w_equal(key,"AncientCard")){int model=prepared>=0?prepared:r&&r->values[7]?int(r->values[0]):-1;return model>=0&&model<CardKinds&&wrt_card(out,{CardKind(model),0});}
 if(id==WorldRelicSeaGlass&&w_equal(key,"Character")){int model=prepared>=0?prepared:r&&r->values[7]?int(r->values[0]):-1;const char*characters[]={"IRONCLAD","SILENT","DEFECT","REGENT","NECROBINDER"};return model>=0&&model<5&&wrt_reference(out,OriginalTextReferenceKind::Localized,unsigned(model),"characters",characters[model],".title");}
 if(id==WorldRelicPaelsTooth&&w_equal(key,"CardTitles")){if(item>=s.world_relics.tooth_count)return false;const auto&card=s.world_relics.tooth_cards[item];return wrt_card(out,card_from_token(card.token),&card.facts);}
 if(id==WorldRelicFurCoat&&w_equal(key,"Title"))return wrt_reference(out,OriginalTextReferenceKind::Relic,id,"relics",world_relic_key(id),".title");
 if(w_equal(key,"EnchantmentName")||w_equal(key,"Enchantment")||w_equal(key,"EnchantmentDescription")){int amount;auto enchant=wrt_enchantment(id,amount);if(enchant==EventEnchantment::None)return false;bool description=w_equal(key,"EnchantmentDescription");
  bool okay=wrt_reference(out,description?OriginalTextReferenceKind::EnchantmentDescription:OriginalTextReferenceKind::Enchantment,unsigned(enchant),"enchantments",event_enchantment_key(enchant),description?".description":".title");out.facts.enchantment=enchant;out.facts.enchantment_amount=int16_t(amount);return okay;}
 return false;
}
unsigned world_relic_description_reference_count(const State&s,RelicId id,const char*key,int position){if(id==WorldRelicPaelsTooth&&w_equal(key,"CardTitles"))return s.world_relics.tooth_count;OriginalTextReference out;return world_relic_description_reference(s,id,key,out,position)?1:0;}
