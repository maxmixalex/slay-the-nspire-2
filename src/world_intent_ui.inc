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
static void native_enemy_intent(Canvas&c,int slot,CreaturePose pos){
 auto intent=enemy_intent(c.a.game,slot);unsigned count=0;for(const auto&part:ui_intent_parts)if(intent.flags&part.flag)++count;
 if(!count)return;int size=mn(13,mx(7,pos.width/int(count))),left=pos.x-int(count)*size/2;
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag){c.picture(ui_intent_image(c.a,part,intent),left,pos.intent,size,size);left+=size;}
 if(intent.flags&EnemyIntentAttack){char label[32]{};TextBuffer b{label,sizeof label,0};b.number(intent.damage);if(intent.hits>1){b.text("×");b.number(intent.hits);}c.centered(label,pos.x-pos.width/2,pos.intent+size-1,pos.width,10,Gold);}
 else if((intent.flags&EnemyIntentStatus)&&intent.status_cards>0)c.num(intent.status_cards,pos.x-3,pos.intent+size-1,10,White);
 c.hit(pos.x-pos.width/2-2,pos.intent-2,pos.width+4,size+14,HitKind::EnemyIntent,slot);
}
static void enemy_intent_tooltip(Canvas&c){
 auto&a=c.a;if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.title||a.library||a.paused||a.help||a.panel||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||!combat_phase(a.game.phase)||!a.game.enemy_world.active)return;
 const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit||(hit->kind!=HitKind::EnemyIntent&&hit->kind!=HitKind::Enemy))return;
 int slot=hit->value;if(slot<0||slot>=a.game.enemy_count||a.game.enemies[slot].hp<=0)return;auto intent=enemy_intent(a.game,slot);
 char body[2048]{};TextBuffer text{body,sizeof body,0};
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag){char key[96]{},line[512]{};TextBuffer b{key,sizeof key,0};b.text(part.locale);b.text(".description");a.localized_text("intents",key,line,sizeof line,slot,slot+1);
  if(text.n)text.ch('\n');text.text(*line?line:a.content.world_title("intents",part.locale));}
 int lines=mn(8,c.paragraph(body,0,0,168,0,0,10)),height=31+lines*13,x=a.pointer.x>=170?8:134,y=48;
 c.rect(x,y,178,height,Panel);c.box(x,y,178,height,Gold);c.text(ui_enemy_name(a,slot),x+6,y+5,12,Gold,166);
 c.paragraph(body,x+6,y+25,166,8,0,10,White);
}
