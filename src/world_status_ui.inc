static const char* ui_status_key(int id){
 static const char*const event_keys[]={"HELLO_WORLD_POWER","CURIOUS_POWER","IMPROVEMENT_POWER","FEEDING_FRENZY_POWER","REBOUND_POWER"};
 if(id>=EventCardStatusStart&&id<StatusKinds)return event_keys[id-EventCardStatusStart];
 if(id>=EnemyStatusStart&&id<EventCardStatusStart)return world_enemy_power_keys[id-EnemyStatusStart];return nullptr;
}
static int ui_status_image(const App&a,int id){const char*key=ui_status_key(id);return key?a.content.world_image("powers",key):a.content.ui_image(status_definitions[id].image);}
static int ui_status_amount(const State&s,int actor,int id){
 if(id==EnemyStatusStart+int(EnemyPower::Slow)||id==EnemyStatusStart+int(EnemyPower::WitheringPresence)){
  unsigned power=unsigned(id-EnemyStatusStart);int aux=actor>0?s.enemy_world.models[actor-1].power_aux[power]:s.enemy_world.player_aux[power];
  return id==EnemyStatusStart+int(EnemyPower::Slow)?aux*10:aux;
 }return status_amount(s,actor,id);
}
static const char* ui_status_title(const App&a,int id){const char*key=ui_status_key(id);if(!key)return status_definitions[id].name;
 const char*title=a.content.world_title("powers",key);return *title?title:key;
}
static void ui_status_description(const App&a,int actor,int id,char*out,size_t size){
 if(!size)return;out[0]=0;const char*key=ui_status_key(id);int amount=status_amount(a.game,actor,id);
 if(key){char full[128]{};TextBuffer b{full,sizeof full,0};b.text(key);b.text(".smartDescription");
  if(!*a.content.world_string("powers",full)){b.n=0;full[0]=0;b.text(key);b.text(".description");}
  a.localized_text("powers",full,out,size,id>=EnemyStatusStart&&id<EventCardStatusStart?id-EnemyStatusStart:-1,actor,amount);return;
 }
 TextBuffer b{out,size,0};for(const char*p=status_definitions[id].description;*p;){if(p[0]=='{'&&p[1]=='n'&&p[2]=='}'){b.number(amount);p+=3;}else b.ch(*p++);}
}
static const char* ui_enemy_name(const App&a,int enemy){
 if(enemy<0||enemy>=a.game.enemy_count)return "";const auto*d=enemy_definition(a.game.enemy_world.models[enemy].kind);if(!d)return "";
 char key[128]{};TextBuffer b{key,sizeof key,0};b.text(d->key);b.text(".name");const char*name=a.content.world_string("monsters",key);return *name?name:d->key;
}
