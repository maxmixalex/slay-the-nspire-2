// Read-only localization binding. Never mutate model facts or consume RNG
// while showing an offer, opening a tooltip, or previewing an upgrade.
struct WorldTextNumber {const char*model;const char*name;int base,upgraded;unsigned scale;};
static const WorldTextNumber world_text_numbers[]={
#include "world_text_numbers.inc"
};
static const char* const world_enemy_power_keys[]={
#include "world_power_keys.inc"
};
static const char* const world_enchantment_keys[]={"","CORRUPTED","GOOPY","INSTINCT","NIMBLE","PERFECT_FIT","SHARP","SLITHER","SOULS_POWER","SOWN","SPIRAL","STEADY","SWIFT","VIGOROUS","ADROIT","CLONE","GLAM","IMBUED","MOMENTUM","ROYALLY_APPROVED","TEZCATARAS_EMBER"};
static bool wt_equal(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool wt_suffix(const char*a,const char*b){unsigned n=0,m=0;while(a[n])++n;while(b[m])++m;return n>=m&&wt_equal(a+n-m,b);}
static LocalizedValue wt_number(int n,unsigned scale=1){return {true,n,scale,nullptr};}
static LocalizedValue wt_string(const char*s){return {s!=nullptr,0,1,s};}
struct WorldTextContext {
 const App* app;const char*table;char model_key[192]{};int model=-1,actor=-1,amount=0;
 Card card{};int instance=-1,deck=-1;const EventCardFacts*facts=nullptr;
 mutable char nested[2048]{};
 int option=-1;bool description=true;unsigned depth=0;int relic_position=-1;
};
static LocalizedValue wt_resolve(const void*,const char*);
static void wt_model_key(WorldTextContext&,const char*);
static LocalizedValue wt_nested(const WorldTextContext&x,const char*table,const char*key,int model=-1){
 if(x.depth>=6)return {};WorldTextContext next=x;next.table=table;next.model=model;next.depth=x.depth+1;next.nested[0]=0;next.model_key[0]=0;wt_model_key(next,key);
 char raw[8192]{};TextBuffer b{raw,sizeof raw,0};b.text(x.app->content.world_string(table,key));
 LocalizedTemplate(x.nested,sizeof x.nested,wt_resolve,&next).format(raw);return wt_string(x.nested);
}
static LocalizedValue wt_reference(const WorldTextContext&x,const OriginalTextReference&r){
 switch(r.kind){
 case OriginalTextReferenceKind::Card:return wt_string(x.app->content.card_string(card_from_token(r.model).kind));
 case OriginalTextReferenceKind::Relic:return wt_string(x.app->relic_title(RelicId(r.model)));
 case OriginalTextReferenceKind::Potion:{const auto*d=potion_definition(Potion(r.model));return d?wt_string(d->name):LocalizedValue{};}
 case OriginalTextReferenceKind::Enchantment:return wt_string(x.app->content.world_title("enchantments",event_enchantment_key(EventEnchantment(r.model))));
 case OriginalTextReferenceKind::EnchantmentDescription:{WorldTextContext nested=x;nested.amount=r.facts.enchantment_amount;auto v=wt_nested(nested,r.table,r.key,r.model);TextBuffer b{x.nested,sizeof x.nested,0};b.text(v.text);return wt_string(x.nested);}
 case OriginalTextReferenceKind::RelicDescription:case OriginalTextReferenceKind::Localized:return wt_nested(x,r.table,r.key,r.model);
 default:return {};
 }
}
static LocalizedValue wt_resolve(const void*opaque,const char*name){
 const auto&x=*static_cast<const WorldTextContext*>(opaque);const auto&a=*x.app;const auto&s=a.game;
 if(wt_suffix(name,".StringValue")){char base[96]{};unsigned n=0;while(name[n])++n;if(n>=sizeof base)return {};for(unsigned i=0;i+12<n;i++)base[i]=name[i];return wt_resolve(opaque,base);}
 if(wt_equal(name,"IsMultiplayer"))return wt_number(0);
 if(wt_equal(name,"InCombat"))return wt_number(!a.library&&combat_phase(s.phase));
 if(wt_equal(name,"OnPlayer"))return wt_number(x.actor<=0);
 if(wt_equal(name,"singleStarIcon"))return wt_string("[gold]星星[/gold]");
 if(wt_equal(name,"energyPrefix"))return wt_number(1);
 if(wt_equal(name,"OwnerName")||wt_equal(name,"TargetPlayer")||wt_equal(name,"character")){
  if(x.actor>0&&x.actor<=s.enemy_count){const auto*d=enemy_definition(s.enemy_world.models[x.actor-1].kind);if(d){char key[128]{};TextBuffer b{key,sizeof key,0};b.text(d->key);b.text(".name");return wt_string(a.content.world_string("monsters",key));}}
  return wt_string(character_definitions[(unsigned)s.character].name);
 }
 if(wt_equal(x.table,"cards")){
  if(wt_equal(name,"IfUpgraded"))return wt_number(x.card.upgraded!=0);
  if(wt_equal(name,"IsTargeting"))return wt_number(x.actor>=0&&x.actor<s.enemy_count&&s.enemies[x.actor].hp>0);
  if(x.card.kind==EventCardMadScience){
   unsigned type=x.facts?x.facts->science_type:0,rider=x.facts?x.facts->rider:0;
   const char*types[]={"Attack","Skill","Power"};
   if(wt_equal(name,"CardType"))return wt_string(types[type<3?type:0]);
   if(wt_equal(name,"HasRider"))return wt_number(rider!=0);
   const char*names[]={"None","Sapping","Violence","Choking","Energized","Wisdom","Chaos","Expertise","Curious","Improvement"};
   for(unsigned i=1;i<10;i++)if(wt_equal(name,names[i]))return wt_number(rider==i);
  }
  int value=0;
  if(!a.library&&x.deck>=0&&character_deck_description_value(s,x.card,x.deck,name,value))return wt_number(value);
  if(!a.library&&x.instance>=0&&character_description_value(s,x.card,x.actor,x.instance,name,value))return wt_number(value);
  if(!a.library&&combat_phase(s.phase)&&x.deck<0&&!x.facts&&character_description_value(s,x.card,x.actor,x.instance,name,value))return wt_number(value);
 }
 if(wt_equal(x.table,"intents")&&x.model>=0&&x.model<s.enemy_count){auto intent=enemy_intent(s,x.model);
  if(wt_equal(name,"Damage"))return wt_number(intent.damage);if(wt_equal(name,"Repeat"))return wt_number(intent.hits);
  if(wt_equal(name,"CardCount"))return wt_number(intent.status_cards);
 }
 if(wt_equal(x.table,"events")||wt_equal(x.table,"ancients")){
  int value=0;if(original_event_description_value(s,name,value))return wt_number(value);
  OriginalTextReference ref;if(original_event_description_reference(s,name,x.option,x.description,ref))return wt_reference(x,ref);
  if(wt_equal(name,"event"))return wt_string(a.content.world_title("events",original_event_key(s.original_events.id)));
 }
 if(wt_equal(x.table,"powers")){
  if(wt_equal(name,"Amount"))return wt_number(x.amount);
  if(wt_equal(name,"AfflictionTitle")&&x.model==int(EnemyPower::Galvanic))return wt_string(a.content.world_title("afflictions","GALVANIZED"));
  if((wt_equal(name,"DisplayAmount")||wt_equal(name,"SlowAmount"))&&x.model==int(EnemyPower::Slow)){int n=x.actor>0?s.enemy_world.models[x.actor-1].power_aux[x.model]:s.enemy_world.player_aux[x.model];return wt_number(n*(wt_equal(name,"DisplayAmount")?10:1));}
  if(wt_equal(name,"Applier")||wt_equal(name,"ApplierName")||wt_equal(name,"Applier.StringValue")||wt_equal(name,"ApplierName.StringValue")){
   unsigned source=255;if(x.model>=0&&x.model<EnemyPowerCount){source=x.actor>0?s.enemy_world.models[x.actor-1].source[x.model]:s.enemy_world.player_source[x.model];}
   if(source<s.enemy_count){const auto*d=enemy_definition(s.enemy_world.models[source].kind);if(d){char key[128]{};TextBuffer b{key,sizeof key,0};b.text(d->key);b.text(".name");return wt_string(a.content.world_string("monsters",key));}}
   return wt_string(character_definitions[(unsigned)s.character].name);
  }
  if(wt_equal(name,"Card")&&x.actor>0&&x.actor<=s.enemy_count){const auto&m=s.enemy_world.models[x.actor-1];if(m.captured_card&&m.captured_card<=s.card_count)return wt_string(a.content.card_string(s.cards[m.captured_card-1].kind));}
  if(wt_equal(name,"CardsLeft")&&x.model==int(EnemyPower::WitheringPresence))return wt_number(x.actor>0?s.enemy_world.models[x.actor-1].power_aux[x.model]:s.enemy_world.player_aux[x.model]);
 }
 if(wt_equal(x.table,"enchantments")||wt_equal(x.table,"afflictions")){
  if(wt_equal(name,"Amount")||wt_equal(name,"Block")||wt_equal(name,"Damage")||wt_equal(name,"WeakPower"))return wt_number(x.amount);
  if(wt_equal(name,"TargetType"))return wt_string(x.card.kind<CardKinds&&definitions[x.card.kind].targeted?"AnyEnemy":"AllEnemies");
 }
 if(wt_equal(x.table,"rest_site_ui")){
  if(wt_equal(name,"Heal"))return wt_number(rest_heal_amount(s));
  if(wt_equal(name,"Count"))return wt_number(1);
  if(wt_equal(name,"Cards"))return wt_number(2);
  if(wt_equal(name,"MaxHp")||wt_equal(name,"RekindleAmount"))return wt_number(5);
  if(wt_equal(name,"LiftsLeft"))return wt_number(mx(0,3-world_relic_counter(s,WorldRelicGirya)));
  if(wt_equal(name,"RelicName"))return wt_string(a.relic_title(WorldRelicPumpkinCandle));
  if(wt_equal(name,"EnchantmentName"))return wt_string(a.content.world_title("enchantments","CLONE"));
  if(wt_equal(name,"ExtraText")){TextBuffer out{x.nested,sizeof x.nested,0};x.nested[0]=0;
   const RelicId ids[]={WorldRelicRegalPillow,WorldRelicDreamCatcher,WorldRelicStoneHumidifier,WorldRelicTinyMailbox};
   for(auto id:ids)if(has_world_relic(s,id)){char key[128]{},formatted[512]{};TextBuffer b{key,sizeof key,0};b.text(world_relic_key(id));b.text(".additionalRestSiteHealText");a.localized_text("relics",key,formatted,sizeof formatted,id);out.ch('\n');out.text(formatted);}return wt_string(x.nested);
  }
 }
 if(wt_equal(x.table,"relics")){
  RelicId id=RelicId(x.model);if(wt_equal(name,"Title"))return wt_string(a.relic_title(id,x.relic_position));
  int value=0;if(world_relic_description_value(s,id,name,value,x.relic_position))return wt_number(value);
  OriginalTextReference ref;
  if(wt_equal(name,"CardTitles")){char list[2048]{};TextBuffer b{list,sizeof list,0};unsigned count=world_relic_description_reference_count(s,id,name,x.relic_position);
   for(unsigned i=0;i<count;i++)if(world_relic_description_reference(s,id,name,ref,x.relic_position,i)){auto v=wt_reference(x,ref);if(i)b.ch('\n');b.text("- ");b.text(v.text);}
   TextBuffer out{x.nested,sizeof x.nested,0};out.text(list);return wt_string(x.nested);
  }
  if(world_relic_description_reference(s,id,name,ref,x.relic_position))return wt_reference(x,ref);
 }
 for(const auto&v:world_text_numbers)if(wt_equal(x.model_key,v.model)&&wt_equal(name,v.name)){int value=wt_equal(x.table,"cards")&&x.card.upgraded?v.upgraded:v.base;if(wt_equal(x.table,"cards")&&x.facts&&x.instance<0&&x.deck<0&&v.scale==1)event_card_offer_description_value(s,x.card,*x.facts,name,value);return wt_number(value,v.scale);}
 // Typed PowerVar names have a Power suffix while the independent mechanics
 // metadata uses the corresponding power's short name in a few old models.
 if(wt_suffix(name,"Power")){char short_name[96]{};unsigned n=0;while(name[n])++n;for(unsigned i=0;i+5<n&&i+1<sizeof short_name;i++)short_name[i]=name[i];
  for(const auto&v:world_text_numbers)if(wt_equal(x.model_key,v.model)&&wt_equal(short_name,v.name))return wt_number(v.base,v.scale);
 }
 return {};
}
static void wt_model_key(WorldTextContext&x,const char*key){
 TextBuffer b{x.model_key,sizeof x.model_key,0};b.text(x.table);b.ch('.');for(const char*p=key;*p&&*p!='.';p++)b.ch(*p);
}
void App::localized_text(const char*table,const char*key,char*out,size_t cap,int model,int actor,int amount)const{
 if(!cap)return;out[0]=0;WorldTextContext x{this,table,{},model,actor,amount};wt_model_key(x,key);
 const char*raw=content.world_string(table,key);char source[8192]{};TextBuffer b{source,sizeof source,0};b.text(raw);
 LocalizedTemplate(out,cap,wt_resolve,&x).format(source);
}
const char* App::relic_title(RelicId id,int position)const{
 if(id>=WorldRelicKinds)return "";char key[128]{};world_relic_title_key(game,id,key,sizeof key,position);const char*text=content.world_string("relics",key);
 return *text?text:id<RelicKinds?relic_definitions[id].name:world_relic_key(id);
}
void App::relic_description(RelicId id,char*out,size_t cap,int position)const{
 if(!cap)return;out[0]=0;if(id>=WorldRelicKinds)return;char key[128]{};TextBuffer b{key,sizeof key,0};b.text(world_relic_key(id));b.text(".description");
 WorldTextContext x{this,"relics",{},int(id)};x.relic_position=position;wt_model_key(x,key);char raw[8192]{};TextBuffer source{raw,sizeof raw,0};source.text(content.world_string("relics",key));LocalizedTemplate(out,cap,wt_resolve,&x).format(raw);
 if(!*out&&id<RelicKinds){TextBuffer text{out,cap,0};text.text(relic_definitions[id].description);}
}
void App::original_event_text(int option,bool description,char*out,size_t cap)const{
 if(!cap)return;out[0]=0;char key[256]{};
 if(option<0&&description&&!game.original_events.finished){
  const auto id=game.original_events.id;
  if(id==OriginalEventId::Neow||id==OriginalEventId::Darv||id==OriginalEventId::Nonupeipe||id==OriginalEventId::Tanx||id==OriginalEventId::Orobas||id==OriginalEventId::Pael||id==OriginalEventId::Tezcatara||id==OriginalEventId::Vakuu){
   // No cross-run AncientStats profile exists yet. The exact first-visit line
   // is the explicit historyless fallback; it does not invent a visit count.
   TextBuffer b{key,sizeof key,0};b.text(original_event_key(id));b.text(".talk.firstVisitEver.0-0.ancient");
   localized_text("ancients",key,out,cap,int(id));return;
  }
 }
 if(original_event_text_key(game,option,description,key,sizeof key)){WorldTextContext x{this,"events",{},int(game.original_events.id)};x.option=option;x.description=description;wt_model_key(x,key);char raw[8192]{};TextBuffer b{raw,sizeof raw,0};b.text(content.world_string("events",key));LocalizedTemplate(out,cap,wt_resolve,&x).format(raw);}
}
void App::world_card_description(Card card,int target,char*out,size_t cap,int instance,int deck,const EventCardFacts*facts)const{
 if(!cap)return;out[0]=0;if(card.kind>=CardKinds)return;
 if(card.kind<EventCardBadLuck){card_description(card,target,out,cap,instance,deck,facts);return;}
 WorldTextContext x{this,"cards",{},int(card.kind),target,0,card,instance,deck,facts};
 if(!x.facts&&instance>=0&&instance<game.card_count)x.facts=&game.event_cards.cards[instance];
 if(!x.facts&&deck>=0&&deck<game.deck_count)x.facts=&game.event_cards.deck[deck];
 wt_model_key(x,definitions[card.kind].key);char source[8192]{};TextBuffer b{source,sizeof source,0};b.text(content.card_string(card.kind,card.upgraded?2:1));
 LocalizedTemplate(out,cap,wt_resolve,&x).format(source);
 card_modifier_text(card,out,cap,instance,deck,x.facts);
}
int App::card_face_image(Card card,int instance,int deck,const EventCardFacts*facts)const{
 if(!facts&&instance>=0&&instance<game.card_count)facts=&game.event_cards.cards[instance];
 if(!facts&&deck>=0&&deck<game.deck_count)facts=&game.event_cards.deck[deck];
 if(card.kind==EventCardMadScience){const char*keys[]={"MAD_SCIENCE_ATTACK","MAD_SCIENCE_SKILL","MAD_SCIENCE_POWER"};unsigned type=facts?facts->science_type:0;int image=content.world_image("cards",keys[type<3?type:0]);if(image>=0)return image;}
 if(card.kind==EventCardWither){unsigned level=instance>=0&&instance<game.card_count?game.enemy_world.card_value[instance]:0;int image=content.world_image("cards",level==0?"WITHER1":level==1?"WITHER2":"WITHER3");if(image>=0)return image;}
 return content.card_image(card.kind);
}
CardType App::card_face_type(Card card,int instance,int deck,const EventCardFacts*facts)const{
 if(card.kind>=CardKinds)return CardType::Skill;
 if(card.kind==EventCardMadScience&&facts)return CardType(facts->science_type<3?facts->science_type:0);
 return card_type(game,card,instance,deck);
}
int App::card_face_frame(Card card,int instance,int deck,const EventCardFacts*facts)const{
 if(card.kind==EventCardMadScience)return content.ui_image(U_ColorlessAttack+int(card_face_type(card,instance,deck,facts)));
 return content.frame_image(card);
}
int App::card_offer_cost(Card card,const EventCardFacts*facts)const{
 int n=cost(card);if(facts)event_card_offer_description_value(game,card,*facts,"EnergyCost",n);return n;
}
int App::relic_face_image(RelicId id)const{
 if(id==WorldRelicYummyCookie){const char*keys[]={"YUMMY_COOKIE_IRONCLAD","YUMMY_COOKIE_SILENT","YUMMY_COOKIE_DEFECT","YUMMY_COOKIE_REGENT","YUMMY_COOKIE_NECRO"};int image=content.world_image("relics",keys[unsigned(game.character)<5?unsigned(game.character):0]);if(image>=0)return image;}
 return content.relic_image(id);
}
void App::card_modifier_text(Card card,char*out,size_t cap,int instance,int deck,const EventCardFacts*facts)const{
 if(!cap||card.kind>=CardKinds)return;
 if(!facts&&instance>=0&&instance<game.card_count)facts=&game.event_cards.cards[instance];
 if(!facts&&deck>=0&&deck<game.deck_count)facts=&game.event_cards.deck[deck];
 size_t n=0;while(n<cap&&out[n])++n;if(n>=cap)return;TextBuffer b{out,cap,n};
 auto append=[&](const char*table,const char*key,int amount){
  char full[160]{},text[2048]{};TextBuffer k{full,sizeof full,0};k.text(key);k.text(".description");WorldTextContext x{this,table,{},-1,-1,amount,card,instance,deck,facts};wt_model_key(x,full);
  char source[4096]{};TextBuffer raw{source,sizeof source,0};raw.text(content.world_string(table,full));LocalizedTemplate(text,sizeof text,wt_resolve,&x).format(source);
  b.text("\n[purple]");b.text(content.world_title(table,key));b.text("[/purple]");if(*text){b.text("：");b.text(text);}
 };
 if(facts&&facts->enchantment!=EventEnchantment::None)append("enchantments",event_enchantment_key(facts->enchantment),facts->enchantment_amount);
 if(instance>=0&&instance<game.card_count){unsigned affliction=game.enemy_world.affliction[instance];const char*keys[]={"","BOUND","ENTANGLED","GALVANIZED","HEXED","TAINTED","RINGING","SMOG"};
  if(affliction&&affliction<sizeof keys/sizeof*keys)append("afflictions",keys[affliction],game.enemy_world.affliction_amount[instance]);
 }
}
