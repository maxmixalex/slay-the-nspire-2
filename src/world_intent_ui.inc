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
static unsigned ui_intent_face(const App&a){return a.content.version()>=HistoryUi8ContentVersion&&a.content.original_label_fonts()?5u:0u;}
static int ui_intent_size(const App&a,bool repeat=false){return a.content.font_size(repeat?8:10,ui_intent_face(a));}
static int ui_intent_text_width(const App&a,const char*text,int size){
 int width64=0;uint32_t previous=0;const unsigned face=ui_intent_face(a);
 while(*text){const uint32_t cp=utf8(text);const auto g=a.content.glyph_metrics(cp,size,face);
  width64+=a.content.glyph_kerning64(previous,cp,size,face)+(g.advance64?g.advance64:int(g.advance)*64);previous=cp;}
 return text_pixel(width64);
}
static UiIntentValue ui_intent_value(const App&a,const UiIntentPart&part,const EnemyIntent&intent){
 UiIntentValue label;TextBuffer value{label.value,sizeof label.value,0},repeat{label.repeat,sizeof label.repeat,0};
 if(part.flag==EnemyIntentAttack){value.number(intent.damage);if(intent.hits>1){repeat.text(a.content.language()!=2?"x":"×");repeat.number(intent.hits);}}
 else if(part.flag==EnemyIntentStatus&&intent.status_cards>0)value.number(intent.status_cards);
 label.value_width=ui_intent_text_width(a,label.value,ui_intent_size(a));
 label.width=label.value_width+ui_intent_text_width(a,label.repeat,ui_intent_size(a,true));return label;
}
struct UiIntentCell {int x,y,w,h,size;uint16_t flag;};
static unsigned ui_intent_cells(const App&a,int slot,CreaturePose pos,UiIntentCell*cells){
 const auto intent=a.combat_intent(slot);unsigned count=0;
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag)++count;
 if(!count)return 0;const int size=mn(20,mx(14,pos.width/int(count)));int width=0;
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag)width+=mx(size,ui_intent_value(a,part,intent).width+2);
 int next=mx(1,mn(319-width,pos.intent_x-width/2));unsigned index=0;
 for(const auto&part:ui_intent_parts)if(intent.flags&part.flag){
  const int cell=mx(size,ui_intent_value(a,part,intent).width+2);
  // NIntent's child moves UP at PI radians/sec, with10px amplitude and
  //8px offset. Adjacent controls have the original0.3-radian phase offset;
  // the process-object hash's arbitrary first phase remains deterministic.
  const int phase=int((a.animation*33u%2000u)*4096u/2000u)+int(index)*196;
  const int y=pos.intent-(8+10*CombatCardMotion::sine(phase)/1024)*4/21;
  cells[index++]={next,y,cell,mx(size,size-2+ui_intent_size(a)),size,part.flag};next+=cell;
 }
 return index;
}
static void native_enemy_intent(Canvas&c,int slot,CreaturePose pos){
 const auto intent=c.a.combat_intent(slot);UiIntentCell cells[14]{};const unsigned count=ui_intent_cells(c.a,slot,pos,cells);
 Canvas faded=c;faded.opacity=uint8_t(unsigned(c.opacity)*c.a.combat_intent_alpha(slot)/255);faded.font_face=uint8_t(ui_intent_face(c.a));
 const int main_size=ui_intent_size(c.a),repeat_size=ui_intent_size(c.a,true);
 for(unsigned i=0;i<count;++i){const auto&cell=cells[i];const UiIntentPart*part=nullptr;
  for(const auto&candidate:ui_intent_parts)if(candidate.flag==cell.flag){part=&candidate;break;}
  if(!part)continue;const auto label=ui_intent_value(c.a,*part,intent);const int center=cell.x+cell.w/2;
  faded.picture(ui_intent_image(c.a,*part,intent),center-cell.size/2,cell.y,cell.size,cell.size);
  if(*label.value){
   // Source22/18px normal labels use authored Kreon Bold in English, then
   // MegaRichTextLabel's Regular locale substitution in Chinese. The10/8px
   // LCD bands keep both values readable after the agreed UI enlargement.
   const int x=center-label.width/2,y=cell.y+cell.size-2,repeat_y=y+mx(0,main_size-repeat_size);
   Canvas shadow=faded;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*144));
   shadow.text(label.value,x+1,y+1,main_size,Ink);shadow.text(label.repeat,x+label.value_width+1,repeat_y+1,repeat_size,Ink);
   faded.text(label.value,x,y,main_size,rgb(255,246,226));faded.text(label.repeat,x+label.value_width,repeat_y,repeat_size,rgb(255,246,226));
  }
  // Each NIntent owns a separate hitbox. Its slot identity still routes
  // card/potion targeting to the same creature; the hover selects one tip.
  c.hit(cell.x,cell.y-2,cell.w,cell.h+4,HitKind::EnemyIntent,slot);
 }
}
bool creature_hover_target(const App&a,CardHoverTarget&target){
 if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.keyboard_card>=0||a.title||a.library||a.paused||a.help||a.panel||
    a.settings_open||a.frontend_confirm||a.map_window.is_open||a.map_preview.open||a.event_reader||a.ftue.active()||
    a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||(!combat_phase(a.game.phase)&&!ui_feedback_bound(a)&&!a.choice_peeking)||!a.game.enemy_world.active)return false;
 const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit||(hit->kind!=HitKind::EnemyIntent&&hit->kind!=HitKind::Enemy))return false;
 const int slot=hit->value;if(slot<0||slot>=a.game.enemy_count||a.combat_health(slot+1).hp<=0)return false;
 const auto pose=a.combat_enemy_pose(slot);target={};target.creature=slot+1;
 target.owner={pose.body_left,pose.body_top,pose.body_width,pose.body_height};target.power_left=pose.x>=160;
 if(hit->kind==HitKind::EnemyIntent){
  UiIntentCell cells[14]{};const unsigned count=ui_intent_cells(a,slot,pose,cells);
  for(unsigned i=count;i;i--){const auto&cell=cells[i-1];
   if(a.pointer.x>=cell.x&&a.pointer.x<cell.x+cell.w&&a.pointer.y>=cell.y-2&&a.pointer.y<cell.y+cell.h+2){target.intent=cell.flag;break;}}
  if(!target.intent)return false;
 }
 target.identity=0x74000000u|(unsigned(slot)<<16)|target.intent;return true;
}
void collect_creature_hover_tips(const App&a,const CardHoverTarget&owner,HoverTipList&out){
 out=HoverTipList{};const int slot=owner.creature-1;if(slot<0||slot>=a.game.enemy_count)return;
 const auto intent=a.combat_intent(slot);
 for(const auto&part:ui_intent_parts)if((intent.flags&part.flag)&&(!owner.intent||owner.intent==part.flag)){
  HoverTip tip{};tip.kind=HoverTipKind::Intent;tip.key=part.locale;tip.actor=int16_t(owner.creature);tip.amount=int16_t(part.flag);out.add(tip);
 }
 if(!owner.intent){
  // Creature.HoverTips appends each actual power's associated set after
  // its intents. Retained effect-time tokens also survive canonical reuse.
  uint16_t tokens[MaxPlayerPowerEntries]{};const unsigned count=a.combat_power_entries(owner.creature,tokens,MaxPlayerPowerEntries);
  for(unsigned i=0;i<count;++i){HoverTipList powers{};collect_power_hover_tips(a,tokens[i],powers);for(unsigned j=0;j<powers.count;++j)out.add(powers.items[j]);out.overflow|=powers.overflow;}
 }
 out.finish();
}
