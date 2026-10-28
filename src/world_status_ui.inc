static const char* ui_status_key(int id){
 static const char*const legacy_keys[]={
#include "locale_status_index.inc"
 };
 static_assert(sizeof(legacy_keys)/sizeof(legacy_keys[0])==LegacyStatusKinds,"Keep original power bindings aligned");
 if(id>=0&&id<LegacyStatusKinds)return legacy_keys[id];
 static const char*const event_keys[]={"HELLO_WORLD_POWER","CURIOUS_POWER","IMPROVEMENT_POWER","FEEDING_FRENZY_POWER","REBOUND_POWER"};
 if(id>=EventCardStatusStart&&id<StatusKinds)return event_keys[id-EventCardStatusStart];
 if(id>=EnemyStatusStart&&id<EventCardStatusStart)return world_enemy_power_keys[id-EnemyStatusStart];return nullptr;
}
static int ui_status_image(const App&a,int id){const char*key=ui_status_key(id);int image=key?a.content.world_image("powers",key):-1;return image>=0?image:id<LegacyStatusKinds?a.content.ui_image(status_definitions[id].image):-1;}
static const char* ui_status_title(const App&a,int id){
 const char*special=nullptr;
 if(id==44)special=a.content.world_title("static_hover_tips","STAR_COUNT");
 if(id==45)special=a.content.world_string("monsters","OSTY.name");
 if(id==CharacterExtraStatusStart+3)special=a.content.world_string("powers","MOCK_TEMPORARY_STRENGTH_LOSS.title");
 if(special&&*special)return special;
 const char*card=nullptr;
 if(id==CharacterExtraStatusStart+4)card="SETUP_STRIKE.title";
 if(id==DefectStatusStart+(int)DPower::FocusedStrikeFocus)card="FOCUSED_STRIKE.title";
 if(id==DefectStatusStart+(int)DPower::HotfixFocus)card="HOTFIX.title";
 if(id==DefectStatusStart+(int)DPower::HyperbeamFocus)card="HYPERBEAM.title";
 if(id==DefectStatusStart+(int)DPower::SynchronizeFocus)card="SYNCHRONIZE.title";
 if(card){const char*t=a.content.world_string("cards",card);if(*t)return t;}
 const char*key=ui_status_key(id);if(!key)return a.content.ui_text(status_definitions[id].name);
 const char*title=a.content.world_title("powers",key);return *title?title:id<LegacyStatusKinds?a.content.ui_text(status_definitions[id].name):key;
}
static void ui_status_description(const App&a,int actor,int id,char*out,size_t size,int instance=-1){
 if(!size)return;out[0]=0;const char*key=ui_status_key(id);int amount=a.combat_status_amount(actor,id);
 if(instance>=0&&power_instanced(unsigned(id))&&key){
  char full[128]{};TextBuffer name{full,sizeof full,0};name.text(key);name.text(".smartDescription");
  if(!*a.content.world_string("powers",full)){name.n=0;full[0]=0;name.text(key);name.text(".description");}
  if(*a.content.world_string("powers",full)){a.localized_text("powers",full,out,size,instance,actor,a.combat_power_amount(power_token(unsigned(actor),unsigned(id),unsigned(instance))));return;}
 }
 if(id==44&&*a.content.world_string("static_hover_tips","STAR_COUNT.description")){a.localized_text("static_hover_tips","STAR_COUNT.description",out,size,-1,actor,amount);return;}
 if(id==45&&*a.content.world_string("static_hover_tips","SUMMON_STATIC.description")){a.localized_text("static_hover_tips","SUMMON_STATIC.description",out,size,-1,actor,amount);return;}
 if(id==CharacterExtraStatusStart&&actor<=0&&*a.content.world_string("powers","NIGHTMARE_POWER.smartDescription")){
  TextBuffer b{out,size,0};for(int slot=0;slot<MaxNightmares;slot++)if(a.combat_status_fact(CombatStatus::NightmareActive,slot)){char line[1024]{};
   // The optional details browser lists all original independent snapshots.
   // The battle strip itself renders each snapshot and its own tooltip.
   a.localized_text("powers","NIGHTMARE_POWER.smartDescription",line,sizeof line,slot,actor,3);if(b.n)b.ch('\n');b.text(line);
  }return;
 }
 const bool rolling_boulder=id==18+int(XPower::RollingBoulder);
 if(actor<=0&&key&&(id==32||id==43||rolling_boulder||id==RegentStatusStart+int(RPower::Monologue))){
  char full[128]{};TextBuffer name{full,sizeof full,0};name.text(key);name.text(".smartDescription");
  if(*a.content.world_string("powers",full)){
   TextBuffer text{out,size,0};const int slots=id==43?MaxBombs:MaxPowerInstances;
   for(int slot=0;slot<slots;++slot){
    const int value=id==32?a.combat_status_fact(CombatStatus::Panache,slot):id==43?a.combat_status_fact(CombatStatus::BombTurns,slot):
        rolling_boulder?a.combat_status_fact(CombatStatus::Boulder,slot):a.combat_status_fact(CombatStatus::Monologue,slot);
    if(!value)continue;char line[1024]{};a.localized_text("powers",full,line,sizeof line,slot,actor,value);
    if(text.n)text.ch('\n');text.text(line);
   }
   if(text.n)return;
  }
 }
 if(key){char full[128]{};TextBuffer b{full,sizeof full,0};b.text(key);b.text(".smartDescription");
  if(!*a.content.world_string("powers",full)){b.n=0;full[0]=0;b.text(key);b.text(".description");}
  if(*a.content.world_string("powers",full)){a.localized_text("powers",full,out,size,id>=EnemyStatusStart&&id<EventCardStatusStart?id-EnemyStatusStart:-1,actor,amount);return;}
 }
 if(id<0||id>=LegacyStatusKinds)return;
 TextBuffer b{out,size,0};for(const char*p=a.content.ui_text(status_definitions[id].description);*p;){if(p[0]=='{'&&p[1]=='n'&&p[2]=='}'){b.number(amount);p+=3;}else b.ch(*p++);}
}
static const char* ui_enemy_name(const App&a,int enemy){
 return enemy>=0&&enemy<a.game.enemy_count?a.monster_title(ui_intent_enemy_kind(a,enemy)):"";
}
