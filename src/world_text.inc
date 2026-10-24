// Read-only localization binding. Never mutate model facts or consume RNG
// while showing an offer, opening a tooltip, or previewing an upgrade.
struct WorldTextNumber {const char*model;const char*name;int base,upgraded;unsigned scale;};
static const WorldTextNumber world_text_numbers[]={
#include "world_text_numbers.inc"
#include "locale_potion_numbers.inc"
};
static const char* const world_enemy_power_keys[]={
#include "world_power_keys.inc"
};
static const char* const world_enchantment_keys[]={"","CORRUPTED","GOOPY","INSTINCT","NIMBLE","PERFECT_FIT","SHARP","SLITHER","SOULS_POWER","SOWN","SPIRAL","STEADY","SWIFT","VIGOROUS","ADROIT","CLONE","GLAM","IMBUED","MOMENTUM","ROYALLY_APPROVED","TEZCATARAS_EMBER"};
static bool wt_equal(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool wt_suffix(const char*a,const char*b){unsigned n=0,m=0;while(a[n])++n;while(b[m])++m;return n>=m&&wt_equal(a+n-m,b);}
static LocalizedValue wt_number(int n,unsigned scale=1){return {true,n,scale,nullptr};}
static LocalizedValue wt_string(const char*s){return {s!=nullptr,0,1,s};}
static EnemyId ui_intent_enemy_kind(const App&a,int slot){
 if(slot<0||slot>=a.game.enemy_count)return EnemyId::Count;
 // The intent and its owner belong to the same displayed actor. A summon
 // may have already reused the canonical slot before its appearance cue.
 if(ui_feedback_bound(a)){
  unsigned kind=a.combat_feedback.identity[slot+1]&0xffffu;
  return kind&&kind<=EnemyKinds?EnemyId(kind-1):EnemyId::Count;
 }
 return a.game.enemy_world.models[slot].kind;
}
struct WorldTextContext {
 const App* app;const char*table;char model_key[192]{};int model=-1,actor=-1,amount=0;
 Card card{};int instance=-1,deck=-1;const EventCardFacts*facts=nullptr;
 mutable char nested[2048]{};
 int option=-1;bool description=true;unsigned depth=0;int relic_position=-1;
 const char*full_key=nullptr;
};
static bool wt_live_card(const WorldTextContext&x){
 const auto&a=*x.app;const auto&s=a.game;
 const bool permanent=!a.library&&x.deck>=0&&x.deck<s.deck_count&&s.deck[x.deck].kind==x.card.kind;
 return !a.library&&!permanent&&!(x.facts&&x.instance<0)&&combat_phase(s.phase);
}
static LocalizedValue wt_resolve(const void*,const char*);
static LocalizedValue wt_catalog_value(const void*opaque,const char*name,LocalizedValue catalog){
 const auto&x=*static_cast<const WorldTextContext*>(opaque);const auto&a=*x.app;const auto&s=a.game;
 if(wt_equal(x.table,"cards")){
  const bool permanent=!a.library&&x.deck>=0&&x.deck<s.deck_count&&s.deck[x.deck].kind==x.card.kind;
  const bool live=wt_live_card(x);
  if(!live){int value=int(catalog.number);
   if(permanent)character_deck_description_value(s,x.card,x.deck,name,value);
   else if(!a.library&&x.facts){
    if(a.detail_has_facts&&a.detail_card==card_token(x.card)&&x.facts==&a.detail_facts){
     if(x.card.kind==DefectGeneticAlgorithm&&(wt_equal(name,"Block")||wt_equal(name,"CalculatedBlock")))value=mn(9999,1+a.detail_genetic);
     if(x.card.kind==NecroTheScythe&&wt_equal(name,"Damage"))value=mn(9999,13+a.detail_scythe);
    }
    event_card_offer_description_value(s,x.card,*x.facts,name,value);
   }
   catalog.number=value;return catalog;
  }
 }
 const auto value=wt_resolve(opaque,name);return value.known?value:catalog;
}
static LocalizedTemplate wt_formatter(char*out,size_t cap,const WorldTextContext*x){
 const auto&c=x->app->content;
 const char*energy=c.world_title("static_hover_tips","ENERGY_COUNT");const char*star=c.world_title("static_hover_tips","STAR_COUNT");bool english=c.english();
 return LocalizedTemplate(out,cap,wt_resolve,x,*energy?energy:english?"Energy":"能量",*star?star:english?"Stars":"辉星",english,wt_catalog_value);
}
static void wt_model_key(WorldTextContext&,const char*);
static LocalizedValue wt_nested(const WorldTextContext&x,const char*table,const char*key,int model=-1){
 if(x.depth>=6)return {};WorldTextContext next=x;next.table=table;next.model=model;next.depth=x.depth+1;next.nested[0]=0;next.model_key[0]=0;wt_model_key(next,key);
 char raw[8192]{};TextBuffer b{raw,sizeof raw,0};b.text(x.app->content.world_string(table,key));
 wt_formatter(x.nested,sizeof x.nested,&next).format(raw);return wt_string(x.nested);
}
static LocalizedValue wt_reference(const WorldTextContext&x,const OriginalTextReference&r){
 switch(r.kind){
 case OriginalTextReferenceKind::Card:{Card card=card_from_token(r.model);const char*title=x.app->content.card_string(card.kind);if(!card.upgraded)return wt_string(title);
  // StringVar receives CardModel.Title, including the upgrade suffix. Dusty
  // Tome deliberately binds the base card and adds its own '+' in the locale.
  TextBuffer b{x.nested,sizeof x.nested,0};b.text(title);b.ch('+');return wt_string(x.nested);}
 case OriginalTextReferenceKind::Relic:return wt_string(x.app->relic_title(RelicId(r.model)));
 case OriginalTextReferenceKind::Potion:{const auto*d=potion_definition(Potion(r.model));if(!d)return {};const char*title=x.app->content.world_title("potions",d->key);return wt_string(*title?title:x.app->content.ui_text(d->name));}
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
 if(wt_equal(x.table,"modifiers")){
  if(wt_equal(name,"Heal"))return wt_number(s.max_hp);
  if(wt_equal(name,"MaxHpLoss"))return wt_number(5);
 }
 if(wt_equal(name,"InCombat"))return wt_number(wt_equal(x.table,"cards")?wt_live_card(x):!a.library&&combat_phase(s.phase));
 if(wt_equal(name,"OnPlayer"))return wt_number(x.actor<=0);
 if(wt_equal(x.table,"monsters")&&wt_equal(x.model_key,"monsters.TEST_SUBJECT")&&wt_equal(name,"Count"))
  return wt_number(a.progress.test_subject_kills>unsigned(INT32_MAX-8)?INT32_MAX:int(a.progress.test_subject_kills)+8);
 if(wt_equal(x.table,"game_over_screen")&&s.world.active){
  if(wt_equal(name,"FloorCount"))return wt_number(s.world.total_floor);
  if(wt_equal(name,"AscensionLevel"))return wt_number(s.world.ascension);
  const auto*r=a.game_over_run();if(r&&ui_run_facts_complete(*r)){
   if(wt_equal(name,"GoldAmount"))return wt_number(r->gold_gained);
   if(wt_equal(name,"EliteCount"))return wt_number(ui_run_elites(*r));
   if(wt_equal(name,"BossCount"))return wt_number(ui_run_bosses(*r));
   if(wt_equal(name,"PlayerDamage"))return wt_number(ui_run_score(*r).total);
   if(wt_equal(name,"PersonalDamage"))return wt_number(a.progress.architect_damage);
  }
 }
 if(wt_equal(name,"singleStarIcon"))return wt_string(InlineStarText);
 if(wt_equal(name,"energyPrefix"))return wt_number(1);
 if(wt_equal(name,"OwnerName")||wt_equal(name,"TargetPlayer")||wt_equal(name,"character")){
  if(x.actor>0&&x.actor<=s.enemy_count){const auto kind=(wt_equal(x.table,"intents")||wt_equal(x.table,"powers"))?ui_intent_enemy_kind(a,x.actor-1):s.enemy_world.models[x.actor-1].kind;if(enemy_definition(kind))return wt_string(a.monster_title(kind));}
  const auto&character=character_definitions[(unsigned)s.character];const char*title=a.content.world_title("characters",character.key);return wt_string(*title?title:a.content.ui_text(character.name));
 }
 if(wt_equal(x.table,"cards")){
  const bool live=wt_live_card(x);
  if(wt_equal(name,"IfUpgraded"))return wt_number(x.card.upgraded!=0);
  if(wt_equal(name,"IsOstyAlive"))return wt_number(live&&s.expansion.osty_hp>0);
  if(wt_equal(name,"TargetType"))return wt_string(live?(card_targets_enemy(s,x.card)?"AnyEnemy":"AllEnemies"):(definitions[x.card.kind].targeted?"AnyEnemy":"AllEnemies"));
  if(wt_equal(name,"IsTargeting"))return wt_number(live&&x.actor>=0&&x.actor<s.enemy_count&&s.enemies[x.actor].hp>0);
  if(x.card.kind==EventCardMadScience){
   unsigned type=x.facts?x.facts->science_type:0,rider=x.facts?x.facts->rider:0;
   const char*types[]={"Attack","Skill","Power"};
   if(wt_equal(name,"CardType"))return wt_string(types[type<3?type:0]);
   if(wt_equal(name,"HasRider"))return wt_number(rider!=0);
   const char*names[]={"None","Sapping","Violence","Choking","Energized","Wisdom","Chaos","Expertise","Curious","Improvement"};
   for(unsigned i=1;i<10;i++)if(wt_equal(name,names[i]))return wt_number(rider==i);
  }
  int value=0;
  if(live&&x.card.kind==SovereignBlade){
   if(wt_equal(name,"GainsBlock"))return wt_number(s.regent.powers[(int)RPower::Parry]>0);
   if(wt_equal(name,"CalculatedBlock")&&character_description_value(s,x.card,x.actor,x.instance,"Block",value))return wt_number(value);
  }
  if(!a.library&&x.deck>=0&&character_deck_description_value(s,x.card,x.deck,name,value))return wt_number(value);
  if(live&&x.instance>=0&&character_description_value(s,x.card,x.actor,x.instance,name,value))return wt_number(value);
  if(live&&x.deck<0&&!x.facts&&character_description_value(s,x.card,x.actor,x.instance,name,value))return wt_number(value);
  if(live&&x.deck<0){
   if(wt_equal(name,"Damage")&&definitions[x.card.kind].type==CardType::Attack)return wt_number(damage(s,x.card,x.actor,x.instance));
   if(wt_equal(name,"Block"))return wt_number(card_block(s,x.card,x.instance));
  }
 }
 if(wt_equal(x.table,"intents")&&x.model>=0&&x.model<s.enemy_count){auto intent=a.combat_intent(x.model);
  if(wt_equal(name,"Damage"))return wt_number(intent.damage);if(wt_equal(name,"Repeat"))return wt_number(intent.hits);
  if(wt_equal(name,"CardCount"))return wt_number(intent.status_cards);
 }
 if(wt_equal(x.table,"events")||wt_equal(x.table,"ancients")){
  if(wt_equal(x.table,"ancients")&&wt_equal(name,"Act1Name")&&s.world.active){
   static constexpr const char*keys[]={"OVERGROWTH","UNDERDOCKS","HIVE","GLORY"};
   const unsigned region=s.world.rooms[0].region;return wt_string(region<4?a.content.world_title("acts",keys[region]):"");
  }
  // The original crystal board binds this screen-local Count to its saved
  // divination charges. Do not make unrelated event Count tokens resolve.
  if(wt_equal(x.table,"events")&&wt_equal(x.full_key,"CRYSTAL_SPHERE.minigame.divinationsRemain")&&
     s.original_events.id==OriginalEventId::CrystalSphere&&wt_equal(name,"Count"))
   return wt_number(s.original_events.crystal_charges);
  int value=0;if(original_event_description_value(s,name,value))return wt_number(value);
  OriginalTextReference ref;if(original_event_description_reference(s,name,x.option,x.description,ref))return wt_reference(x,ref);
  if(wt_equal(name,"event"))return wt_string(a.content.world_title("events",original_event_key(s.original_events.id)));
 }
 if(wt_equal(x.table,"powers")){
  if(wt_equal(name,"Amount"))return wt_number(x.amount);
  if(x.actor<=0){
   if(wt_equal(name,"SelfDamage")){
    if(wt_equal(x.model_key,"powers.CRIMSON_MANTLE_POWER"))return wt_number(a.combat_status_fact(CombatStatus::CrimsonDamage));
    if(wt_equal(x.model_key,"powers.INFERNO_POWER"))return wt_number(a.combat_status_fact(CombatStatus::InfernoDamage));
   }
   // Every instanced strip icon supplies its original slot. The optional
   // model browser also formats each slot individually; an aggregate count
   // is never a model's damage, turn, or progress value.
   if(x.model>=0&&x.model<MaxPowerInstances){
    if(wt_equal(x.model_key,"powers.PANACHE_POWER")&&wt_equal(name,"CardsLeft")&&a.combat_status_fact(CombatStatus::Panache,x.model))
     return wt_number(5-a.combat_status_fact(CombatStatus::PanacheCards,x.model));
    if(wt_equal(x.model_key,"powers.MONOLOGUE_POWER")&&a.combat_status_fact(CombatStatus::Monologue,x.model)){
     if(wt_equal(name,"StrengthPower"))return wt_number(a.combat_status_fact(CombatStatus::Monologue,x.model));
     if(wt_equal(name,"StrengthApplied"))return wt_number(a.combat_status_fact(CombatStatus::MonologueApplied,x.model));
    }
   }
   if(x.model>=0&&x.model<MaxBombs&&wt_equal(x.model_key,"powers.THE_BOMB_POWER")&&
      wt_equal(name,"Damage")&&a.combat_status_fact(CombatStatus::BombTurns,x.model))return wt_number(a.combat_status_fact(CombatStatus::BombDamage,x.model));
  }
  if(wt_equal(name,"Card")&&wt_equal(x.model_key,"powers.NIGHTMARE_POWER")&&x.actor<=0&&x.model>=0&&x.model<MaxNightmares&&a.combat_status_fact(CombatStatus::NightmareActive,x.model)){
   const Card card=card_from_token(a.combat_status_fact(CombatStatus::NightmareCard,x.model));TextBuffer b{x.nested,sizeof x.nested,0};b.text(a.content.card_string(card.kind));if(card.upgraded)b.ch('+');return wt_string(x.nested);
  }
  if(wt_equal(name,"AfflictionTitle")&&x.model==int(EnemyPower::Galvanic))return wt_string(a.content.world_title("afflictions","GALVANIZED"));
  if((wt_equal(name,"DisplayAmount")||wt_equal(name,"SlowAmount"))&&x.model==int(EnemyPower::Slow)){int n=a.combat_status_auxiliary(x.actor>0?x.actor:0,x.model);return wt_number(n*(wt_equal(name,"DisplayAmount")?10:1));}
  if(wt_equal(name,"Applier")||wt_equal(name,"ApplierName")||wt_equal(name,"Applier.StringValue")||wt_equal(name,"ApplierName.StringValue")){
   const auto kind=a.combat_status_source(x.actor>0?x.actor:0,x.model);
   if(enemy_definition(kind))return wt_string(a.monster_title(kind));
   return wt_string(a.content.world_title("characters",character_definitions[(unsigned)s.character].key));
  }
  if(wt_equal(name,"Card")&&x.actor>0&&x.actor<=s.enemy_count){const int token=a.combat_status_fact(CombatStatus::CapturedCard,x.actor-1);if(token>=0)return wt_string(a.content.card_string(card_from_token(token).kind));}
  if(wt_equal(name,"CardsLeft")&&x.model==int(EnemyPower::WitheringPresence))return wt_number(a.combat_status_auxiliary(x.actor>0?x.actor:0,x.model));
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
   for(auto id:ids)if(has_world_relic(s,id)){char key[128]{},formatted[512]{};TextBuffer b{key,sizeof key,0};b.text(world_relic_key(id));b.text(".additionalRestSiteHealText");a.localized_text("relics",key,formatted,sizeof formatted,id);out.ch('\n');out.text(formatted);}
   if(has_run_modifier(s,RunModifier::NightTerrors)){char formatted[512]{};a.localized_text("modifiers","NIGHT_TERRORS.additionalRestSiteHealText",formatted,sizeof formatted);out.ch('\n');out.text(formatted);}
   return wt_string(x.nested);
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
  // These canonical StringVars default to an empty string before SetupForPlayer
  // or acquisition. Their documented generic cond branch is valid in the
  // library too; unrelated missing bindings must remain visibly unresolved.
  if((id==WorldRelicDustyTome&&wt_equal(name,"AncientCard"))||
     (id==WorldRelicArchaicTooth&&(wt_equal(name,"StarterCard")||wt_equal(name,"AncientCard")))||
     (id==WorldRelicTouchOfOrobas&&(wt_equal(name,"StarterRelic")||wt_equal(name,"UpgradedRelic")))||
     (id==WorldRelicSeaGlass&&wt_equal(name,"Character")))return wt_string("");
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
 x.full_key=key;
 TextBuffer b{x.model_key,sizeof x.model_key,0};b.text(x.table);b.ch('.');for(const char*p=key;*p&&*p!='.';p++)b.ch(*p);
}
void App::localized_text(const char*table,const char*key,char*out,size_t cap,int model,int actor,int amount)const{
 if(!cap)return;out[0]=0;WorldTextContext x{this,table,{},model,actor,amount};wt_model_key(x,key);
 const char*raw=content.world_string(table,key);char source[8192]{};TextBuffer b{source,sizeof source,0};b.text(raw);
 wt_formatter(out,cap,&x).format(source);
}
const char* App::monster_title(EnemyId id)const{
 const auto*d=enemy_definition(id);if(!d)return "";
 char key[128]{};TextBuffer b{key,sizeof key,0};b.text(d->key);b.text(".name");
 const char*raw=content.world_string("monsters",key);
 if(id!=EnemyId::TestSubject)return *raw?raw:d->key;
 if(*raw){localized_text("monsters",key,monster_title_buffer,sizeof monster_title_buffer);return monster_title_buffer;}
 TextBuffer fallback{monster_title_buffer,sizeof monster_title_buffer,0};fallback.text(d->key);fallback.ch(' ');
 fallback.number(progress.test_subject_kills>unsigned(INT32_MAX-8)?INT32_MAX:int(progress.test_subject_kills)+8);return monster_title_buffer;
}
const char* App::relic_title(RelicId id,int position)const{
 if(id>=WorldRelicKinds)return "";char key[128]{};world_relic_title_key(game,id,key,sizeof key,position);const char*text=content.world_string("relics",key);
 return *text?text:id<RelicKinds?relic_definitions[id].name:world_relic_key(id);
}
void App::relic_description(RelicId id,char*out,size_t cap,int position)const{
 if(!cap)return;out[0]=0;if(id>=WorldRelicKinds)return;char key[128]{};TextBuffer b{key,sizeof key,0};b.text(world_relic_key(id));b.text(".description");
 WorldTextContext x{this,"relics",{},int(id)};x.relic_position=position;wt_model_key(x,key);char raw[8192]{};TextBuffer source{raw,sizeof raw,0};source.text(content.world_string("relics",key));wt_formatter(out,cap,&x).format(raw);
 if(!*out&&id<RelicKinds){TextBuffer text{out,cap,0};text.text(relic_definitions[id].description);}
}
const char* App::run_modifier_text(RunModifier modifier,bool description)const{
 char key[96]{};TextBuffer b{key,sizeof key,0};b.text(run_modifier_key(modifier));
 b.text(run_modifier_character(modifier)?(description?".cardsModifierDescription":".cardsModifierTitle"):(description?".description":".title"));
 const char*value=content.world_string(run_modifier_character(modifier)?"characters":"modifiers",key);
 return *value?value:description?"":run_modifier_key(modifier);
}
void App::original_event_text(int option,bool description,char*out,size_t cap)const{
 if(!cap)return;out[0]=0;char key[256]{};
 if(option==0&&!game.original_events.finished&&current_neow_modifier(game)!=RunModifier::Count){
  TextBuffer text{out,cap,0};text.text(run_modifier_text(current_neow_modifier(game),description));return;
 }
 if(option<0&&description&&!game.original_events.finished){
  const auto id=game.original_events.id;
  if(id==OriginalEventId::Neow||id==OriginalEventId::Darv||id==OriginalEventId::Nonupeipe||id==OriginalEventId::Tanx||id==OriginalEventId::Orobas||id==OriginalEventId::Pael||id==OriginalEventId::Tezcatara||id==OriginalEventId::Vakuu){
   const auto&dialogue=ancient_dialogue;
   if(dialogue.bound&&dialogue.id==unsigned(id)&&dialogue.seed==game.world.run_seed&&dialogue.count){ancient_dialogue_text(dialogue.current,out,cap);return;}
   // A direct text query before the room is bound uses the canonical source
   // first-visit line. Active conversations use their actual revealed line.
   TextBuffer b{key,sizeof key,0};b.text(original_event_key(id));b.text(".talk.firstVisitEver.0-0.ancient");
   localized_text("ancients",key,out,cap,int(id));return;
  }
 }
 if(original_event_text_key(game,option,description,key,sizeof key)){WorldTextContext x{this,"events",{},int(game.original_events.id)};x.option=option;x.description=description;wt_model_key(x,key);char raw[8192]{};TextBuffer b{raw,sizeof raw,0};b.text(content.world_string("events",key));wt_formatter(out,cap,&x).format(raw);}
}
void App::world_card_description(Card card,int target,char*out,size_t cap,int instance,int deck,const EventCardFacts*facts)const{
 if(!cap)return;out[0]=0;if(card.kind>=CardKinds)return;
 if(card.kind<EventCardBadLuck&&content.version()<LocaleContentVersion){card_description(card,target,out,cap,instance,deck,facts);return;}
 WorldTextContext x{this,"cards",{},int(card.kind),target,0,card,instance,deck,facts};
 if(!x.facts&&instance>=0&&instance<game.card_count)x.facts=&game.event_cards.cards[instance];
 if(!x.facts&&deck>=0&&deck<game.deck_count)x.facts=&game.event_cards.deck[deck];
 wt_model_key(x,definitions[card.kind].key);char source[8192]{};TextBuffer b{source,sizeof source,0};b.text(content.card_string(card.kind,card.upgraded?2:1));
 wt_formatter(out,cap,&x).format(source);
 if(content.version()>=LocaleContentVersion&&wt_live_card(x)){
  size_t n=0;while(n+1<cap&&out[n])++n;TextBuffer suffix{out,cap,n,&content};
  auto keyword=[&](const char*key){suffix.text("\n[gold]");suffix.text(content.world_title("card_keywords",key));suffix.text(content.english()?".[/gold]":"。[/gold]");};
  unsigned replay=card.kind==SovereignBlade?game.regent.powers[(int)RPower::SwordSage]:0;
  if(instance>=0&&instance<game.card_count&&game.cards[instance].kind==card.kind){
   const auto&d=definitions[card.kind];unsigned keywords=card.upgraded?d.upgraded_keywords:d.keywords;
   if(game.silent.sly[instance]&&!(keywords&16))keyword("SLY");
   if(game.silent.retain[instance]&&!card_retains(card))keyword("RETAIN");
   if(game.necro.ethereal[instance]&&!(keywords&8))keyword("ETHEREAL");
   replay+=game.expansion.replay[instance];
  }
  if(replay){suffix.text("\n[gold]");suffix.text(content.world_title("static_hover_tips","REPLAY_DYNAMIC"));suffix.ch(' ');suffix.number(replay);suffix.text("[/gold]");}
 }
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
 // The compendium displays the canonical model, even when an equal combat
 // instance was previously inspected. Local enchantments belong to its copy.
 if(!cap||card.kind>=CardKinds||library)return;
 if(!facts&&instance>=0&&instance<game.card_count)facts=&game.event_cards.cards[instance];
 if(!facts&&deck>=0&&deck<game.deck_count)facts=&game.event_cards.deck[deck];
 size_t n=0;while(n<cap&&out[n])++n;if(n>=cap)return;TextBuffer b{out,cap,n,&content};
 auto append=[&](const char*table,const char*key,int amount){
  char full[160]{},text[2048]{};TextBuffer k{full,sizeof full,0};k.text(key);k.text(".description");WorldTextContext x{this,table,{},-1,-1,amount,card,instance,deck,facts};wt_model_key(x,full);
  char source[4096]{};TextBuffer raw{source,sizeof source,0};raw.text(content.world_string(table,full));wt_formatter(text,sizeof text,&x).format(source);
  b.text("\n[purple]");b.text(content.world_title(table,key));b.text("[/purple]");if(*text){b.text(content.english()?": ":"：");b.text(text);}
 };
 if(facts&&facts->enchantment!=EventEnchantment::None)append("enchantments",event_enchantment_key(facts->enchantment),facts->enchantment_amount);
 if(instance>=0&&instance<game.card_count){unsigned affliction=game.enemy_world.affliction[instance];const char*keys[]={"","BOUND","ENTANGLED","GALVANIZED","HEXED","TAINTED","RINGING","SMOG"};
  if(content.version()>=LocaleContentVersion&&game.silent.inky[instance])append("enchantments","INKY",1);
  if(affliction&&affliction<sizeof keys/sizeof*keys)append("afflictions",keys[affliction],game.enemy_world.affliction_amount[instance]);
 }
}
