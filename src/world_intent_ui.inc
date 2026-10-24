struct UiIntentPart {uint16_t flag;const char*image;const char*locale;};
static const UiIntentPart ui_intent_parts[]={
 {EnemyIntentAttack,"ATTACK","ATTACK"},{EnemyIntentBlock,"BLOCK","DEFEND"},
 {EnemyIntentBuff,"BUFF","BUFF"},{EnemyIntentDebuff,"DEBUFF","DEBUFF"},
 {EnemyIntentDebuffStrong,"DEBUFF","DEBUFF"},{EnemyIntentCardDebuff,"CARD_DEBUFF","CARD_DEBUFF"},
 {EnemyIntentStatus,"STATUS","STATUS"},{EnemyIntentHeal,"HEAL","HEAL"},
 {EnemyIntentSummon,"SUMMON","SUMMON"},{EnemyIntentEscape,"ESCAPE","ESCAPE"},
 {EnemyIntentStun,"STUN","STUN"},{EnemyIntentSleep,"SLEEP","SLEEP"},
 {EnemyIntentDeath,"DEATH","DEATH_BLOW"},{EnemyIntentUnknown,"UNKNOWN","UNKNOWN"}
};
static int ui_intent_image(const App&a,const UiIntentPart&part,const EnemyIntent&intent){
 if(part.flag==EnemyIntentAttack){int total=int(intent.damage)*intent.hits;const char*key=total<5?"ATTACK":total<10?"ATTACK_2":total<20?"ATTACK_3":total<40?"ATTACK_4":"ATTACK_5";int image=a.content.world_image("intents",key);if(image>=0)return image;}
 return a.content.world_image("intents",part.image);
}
struct UiIntentValue {char value[16]{},repeat[16]{};int width=0,value_width=0;};
static UiIntentValue ui_intent_value(Canvas&c,const UiIntentPart&part,const EnemyIntent&intent){
 UiIntentValue label;TextBuffer value{label.value,sizeof label.value,0},repeat{label.repeat,sizeof label.repeat,0};
 if(part.flag==EnemyIntentAttack){value.number(intent.damage);if(intent.hits>1){repeat.ch('x');repeat.number(intent.hits);}}
 else if(part.flag==EnemyIntentStatus&&intent.status_cards>0)value.number(intent.status_cards);
 label.value_width=c.width(label.value,10);label.width=label.value_width+c.width(label.repeat,8);return label;
}
static void native_enemy_intent(Canvas&c,int slot,CreaturePose pos){
 const auto intent=c.a.combat_intent(slot);unsigned count=0;for(const auto&part:ui_intent_parts)if(intent.flags&part.flag)++count;
 if(!count)return;const int size=mn(14,mx(9,pos.width/int(count)));int width=0;
 // NIntent moves at PI radians/sec, 10 px amplitude and 8 px offset in
 // the original1680px FourByThree viewport. Its own IntentPos.x may
 // differ from the creature origin (for example Ceremonial Beast).
 pos.intent+=(8+10*CombatCardMotion::sine(int((c.a.animation*33u%2000u)*4096u/2000u))/1024)*4/21;
 Canvas faded=c;faded.opacity=uint8_t(unsigned(c.opacity)*c.a.combat_intent_alpha(slot)/255);
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag)width+=mx(size,ui_intent_value(c,part,intent).width+2);
 const int left=mx(1,mn(c.clip_w-width-1,pos.intent_x-width/2));int next=left;
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag){
  const auto label=ui_intent_value(c,part,intent);const int cell=mx(size,label.width+2),center=next+cell/2;next+=cell;
  faded.picture(ui_intent_image(c.a,part,intent),center-size/2,pos.intent,size,size);
  if(!*label.value)continue;
  // Every original NIntent owns its own Value label, including StatusIntent
  // beside AttackIntent. Reserve the text width so their values stay apart.
  // Preserve cream, outline and the smaller 18/22 repeated-hit suffix.
  const int x=center-label.width/2,y=pos.intent+size-2;
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy){faded.text(label.value,x+dx,y+dy,10,Ink);faded.text(label.repeat,x+label.value_width+dx,y+2+dy,8,Ink);}
  faded.text(label.value,x,y,10,rgb(255,246,226));faded.text(label.repeat,x+label.value_width,y+2,8,rgb(255,246,226));
 }
 const int actor_left=pos.body_left-2,hit_left=mn(actor_left,left),hit_right=mx(actor_left+pos.body_width+4,left+width);
 c.hit(hit_left,pos.intent-2,hit_right-hit_left,size+14,HitKind::EnemyIntent,slot);
}
static void enemy_intent_tooltip(Canvas&c){
 auto&a=c.a;if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.title||a.library||a.paused||a.help||a.panel||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||(!combat_phase(a.game.phase)&&!ui_feedback_bound(a)&&!a.choice_peeking)||!a.game.enemy_world.active)return;
 const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit||(hit->kind!=HitKind::EnemyIntent&&hit->kind!=HitKind::Enemy))return;
 int slot=hit->value;if(slot<0||slot>=a.game.enemy_count||a.combat_health(slot+1).hp<=0)return;auto intent=a.combat_intent(slot);if(!intent.flags)return;
 // The formatter reads the same retained intent as the icon and damage.
 // Keep its name on that actor as well, until any slot replacement cue.
 char body[2048]{};TextBuffer text{body,sizeof body,0};
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag){char key[96]{},line[512]{};TextBuffer b{key,sizeof key,0};b.text(part.locale);b.text(".description");a.localized_text("intents",key,line,sizeof line,slot,slot+1);
  if(text.n)text.ch('\n');text.text(*line?line:a.content.world_title("intents",part.locale));}
 int lines=mn(8,c.paragraph(body,0,0,168,0,0,10)),height=31+lines*13,x=a.pointer.x>=170?8:134,y=48;
 char name_key[128]{};const auto*owner=enemy_definition(ui_intent_enemy_kind(a,slot));
 if(owner){TextBuffer name{name_key,sizeof name_key,0};name.text(owner->key);name.text(".name");}
 const char*name=a.content.world_string("monsters",name_key);
 c.rect(x,y,178,height,Panel);c.box(x,y,178,height,Gold);c.text(*name?name:owner?owner->key:"",x+6,y+5,12,Gold,166);
 c.paragraph(body,x+6,y+25,166,8,0,10,White);
}
