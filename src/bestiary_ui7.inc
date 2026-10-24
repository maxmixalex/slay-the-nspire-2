// The pinned Bestiary list is a single-player compendium screen. Only actual
// wins unlock entries; hovering and preview actions never touch the run model.
struct BestiaryEntrySpec {uint8_t model,encounter,region,room;};
struct BestiaryModelSpec {uint16_t first;uint8_t count;};
struct BestiaryMoveSpec {const char*label;const char*clip;int16_t node;};
#include "bestiary_catalog_ui7.inc"
static constexpr unsigned BestiaryEntries=sizeof bestiary_entries/sizeof bestiary_entries[0];
#include "bestiary_order_ui7.inc"
static_assert(BestiaryEntries+5<=BestiaryPresentation::Capacity,"Bestiary list capacity");
static const BestiaryEntrySpec* bestiary_entry(int index){return index>=0&&unsigned(index)<BestiaryEntries?&bestiary_entries[index]:nullptr;}
static const UiEnemyStatistics& bestiary_statistics(const App&a,const BestiaryEntrySpec&e,int character=-1){
 return e.model==255?a.encounter_statistics(e.encounter,character):a.enemy_statistics(e.model,character);
}
static bool bestiary_known(const App&a,int index){const auto*e=bestiary_entry(index);return e&&bestiary_statistics(a,*e).wins>0;}
static const char* bestiary_text(const App&a,const char*key,const char*english,const char*chinese){
 const char*text=a.content.world_string("bestiary",key);return *text?text:a.content.english()?english:chinese;
}
static const char* bestiary_title(const App&a,const BestiaryEntrySpec&e,bool encounter=false){
 if(!encounter&&e.model!=255){const auto*d=enemy_definition(EnemyId(e.model));return d?a.content.world_title("monsters",d->key):"";}
 const auto*d=encounter_definition(EncounterId(e.encounter));return d?a.content.world_title("encounters",d->key):"";
}
static BestiaryModelSpec bestiary_moves_for(const BestiaryEntrySpec*e){return !e?BestiaryModelSpec{}:e->model==255?bestiary_decimillipede:bestiary_models[e->model];}
unsigned App::bestiary_move_count()const{return bestiary_moves_for(bestiary_entry(bestiary.selected)).count;}
unsigned App::bestiary_entry_model(int row)const{
 const auto*e=row>=0&&row<bestiary.row_count?bestiary_entry(bestiary.rows[row]):nullptr;return e?e->model:255;
}
// Animation predicates read a detached display context. Missing special move
// effects remain an explicit parity limit; they never borrow the live battle.
static bool bestiary_fact(void*,const char*,int&value){value=0;return true;}
static int bestiary_clip(unsigned model,const char*name,unsigned skin=0,int move=-1){
 if(!name)return -1;
 const int exact=world_enemy_resolve_clip(model,name,skin,move,bestiary_fact,nullptr);if(exact>=0)return exact;
 const auto*v=world_enemy_visual(model);if(!v)return -1;
 for(unsigned i=0;i<v->clip_count;++i){const auto*r=world_enemy_clip_row(model,i);
  if(r&&r->skin==skin&&ui_visual_equal(r->native_clip,name)&&(r->move==move||r->move<0))return r->directory;}
 return -1;
}
static int bestiary_rest(unsigned model,int state=-1){
 const auto*v=world_enemy_visual(model);if(!v)return -1;
 if(v->shared_group)return bestiary_clip(model,v->shared_side==1?"left/idle_loop":"right/idle_loop");
 for(unsigned pass=0;pass<v->state_count&&state>=0;++pass){const auto*n=world_enemy_state(model,state);if(!n)break;
  if(n->loop)return bestiary_clip(model,n->clip);if(n->next<0)break;state=n->next;}
 int idle=bestiary_clip(model,"idle_loop");if(idle>=0)return idle;
 for(unsigned i=0;i<v->state_count;++i){const auto*n=world_enemy_state(model,i);if(n&&n->loop){idle=bestiary_clip(model,n->clip);if(idle>=0)return idle;}}
 return -1;
}
static void bestiary_prepare_actor(BestiaryPresentation::Preview&actor,unsigned model,uint32_t now){
 actor={};actor.model=uint8_t(model);actor.start=now;
 actor.state=int16_t(world_enemy_initial(model,bestiary_fact,nullptr));
 actor.idle=int16_t(bestiary_rest(model,actor.state));
 if(const auto*state=world_enemy_state(model,actor.state)){const int clip=bestiary_clip(model,state->clip);
  if(clip>=0&&!state->loop){actor.clips[0]=int16_t(clip);actor.count=1;}}
}
static void bestiary_select(App&a,int row){
 auto&b=a.bestiary;if(row<0||row>=b.row_count||!bestiary_known(a,b.rows[row]))return;
 b.focused=int16_t(row);b.reveal(row);const int entry=b.rows[row];if(entry==b.selected)return;
 b.arrow_from=b.selected_row<0?row*BestiaryPresentation::RowHeight*BestiaryPresentation::Unit:
  (b.arrow_y(a.map_clock_ms)-BestiaryPresentation::Top-BestiaryPresentation::RowHeight/2)*BestiaryPresentation::Unit+b.position;
 b.selected=int16_t(entry);b.selected_row=int16_t(row);b.selected_at=b.mode_at=b.arrow_at=a.map_clock_ms;b.action=0;
 const auto&e=bestiary_entries[entry];const int filters[]={-1,0,1,3,4,2};
 if(!bestiary_statistics(a,e,filters[b.filter]).wins&&!bestiary_statistics(a,e,filters[b.filter]).losses)b.filter=0;
 b.filter_at=a.map_clock_ms;
 if(e.model==255){b.actor_count=3;const EnemyId models[]={EnemyId::DecimillipedeSegmentBack,EnemyId::DecimillipedeSegmentFront,EnemyId::DecimillipedeSegmentMiddle};
  for(unsigned i=0;i<3;++i)bestiary_prepare_actor(b.actors[i],unsigned(models[i]),a.map_clock_ms);
 }else{b.actor_count=1;bestiary_prepare_actor(b.actors[0],e.model,a.map_clock_ms);}
}
bool App::open_bestiary(bool force){
 if(!force&&!bestiary_available())return false;
 pointer_cancel();bestiary={};auto&b=bestiary;b.open=true;b.actions=bestiary_actions_preferred;b.selected_at=b.mode_at=b.filter_at=map_clock_ms;
 // NBestiary uses CurrentCulture; NGame pins that to InvariantCulture.
 // The two101-byte tables contain the original localized order, verified
 // with .NET. Opening needs one pass and no title decoding or sort scratch.
 const auto&order=bestiary_order[content.english()?0:1];int previous_region=-1;
 for(unsigned position=0;position<BestiaryEntries;++position){const unsigned entry=order[position],region=bestiary_entries[entry].region;
  if(region<4&&!act_discovered(region))continue;
  if(int(region)!=previous_region){b.rows[b.row_count++]=int16_t(-1-int(region));previous_region=int(region);}
  b.rows[b.row_count++]=int16_t(entry);
 }
 for(int row=0;row<b.row_count;++row)if(bestiary_known(*this,b.rows[row])){bestiary_select(*this,row);break;}
 hit_count=0;return true;
}
static int bestiary_transition(unsigned model,int state,const char*trigger){
 int next=world_enemy_transition(model,state,trigger,bestiary_fact,nullptr);if(next>=0)return next;
 const auto*v=world_enemy_visual(model);if(!v)return -1;
 for(unsigned i=0;i<v->transition_count;++i){const auto*t=world_enemy_transition_row(model,i);
  if(t&&(t->from<0||t->from==state)&&ui_visual_equal(t->trigger,trigger))return t->to;}
 return -1;
}
static void bestiary_append(BestiaryPresentation::Preview&actor,int clip){if(clip>=0&&actor.count<12)actor.clips[actor.count++]=int16_t(clip);}
static void bestiary_play(App&a,unsigned index){
 auto&b=a.bestiary;const auto*entry=bestiary_entry(b.selected);const auto list=bestiary_moves_for(entry);if(index>=list.count)return;
 b.action=uint8_t(index);const auto&move=bestiary_moves[list.first+index];
 for(unsigned i=0;i<b.actor_count;++i){auto&actor=b.actors[i];actor.count=actor.current=0;actor.start=a.map_clock_ms;actor.hold=false;
  if(move.clip){bestiary_append(actor,bestiary_clip(actor.model,move.clip,actor.skin,move.node));
   actor.hold=ui_visual_equal(move.clip,"die")||ui_visual_equal(move.clip,"left/die")||ui_visual_equal(move.clip,"right/die");
   actor.idle=int16_t(bestiary_clip(actor.model,ui_visual_equal(move.clip,"wither")?"dead_loop":"idle_loop",actor.skin));
   if(ui_visual_equal(move.clip,"sleep_loop"))actor.idle=int16_t(bestiary_clip(actor.model,"sleep_loop",actor.skin));
   if(actor.idle<0)actor.idle=int16_t(bestiary_rest(actor.model));
  }else if(const auto*definition=world_enemy_move(actor.model,move.node)){
   int state=actor.state;
   for(unsigned action=0;action<definition->action_count;++action){const auto*command=world_enemy_action(*definition,action);if(!command)continue;
    if(command->direct)bestiary_append(actor,bestiary_clip(actor.model,command->name,actor.skin,move.node));
    else{
     if(const auto*before=world_enemy_state(actor.model,state))if(!before->loop&&before->next>=0)state=before->next;
     const int next=bestiary_transition(actor.model,state,command->name);
     if(const auto*node=world_enemy_state(actor.model,next)){state=next;bestiary_append(actor,bestiary_clip(actor.model,node->clip,actor.skin,move.node));}
    }
   }
   actor.state=int16_t(state);actor.idle=int16_t(definition->next_clip?bestiary_clip(actor.model,definition->next_clip,actor.skin):bestiary_rest(actor.model,state));
  }
 }
}
static void bestiary_toggle(App&a){
 auto&b=a.bestiary;b.actions=!b.actions;b.mode_at=a.map_clock_ms;b.focus=0;
 a.bestiary_actions_preferred=b.actions;a.persist_preferences();
}
static bool bestiary_filter_enabled(const App&a,unsigned filter){
 const auto*entry=bestiary_entry(a.bestiary.selected);if(!entry||filter>5)return false;
 const int characters[]={-1,0,1,3,4,2};const auto&s=bestiary_statistics(a,*entry,characters[filter]);return s.wins||s.losses;
}
static void bestiary_choose_filter(App&a,unsigned filter){if(!bestiary_filter_enabled(a,filter))return;
 a.bestiary.filter=uint8_t(filter);a.bestiary.filter_at=a.map_clock_ms;
}
bool App::bestiary_pointer(const HitRegion&hit){
 if(!bestiary.open)return false;
 switch(hit.kind){
 case HitKind::BestiaryEntry:bestiary_select(*this,hit.value);bestiary.focus=0;break;
 case HitKind::BestiaryMove:bestiary_play(*this,unsigned(hit.value));bestiary.focus=1;break;
 case HitKind::BestiaryFilter:bestiary_choose_filter(*this,unsigned(hit.value));bestiary.focus=1;break;
 case HitKind::BestiaryMode:bestiary_toggle(*this);break;
 case HitKind::Close:bestiary.open=false;pointer_cancel();hit_count=0;break;
 default:break;
 }return true;
}
void App::bestiary_scroll(int rows){if(bestiary.open)bestiary.wheel(rows);}
bool App::bestiary_input(Key key){
 auto&b=bestiary;if(!b.open)return false;
 if(key==Key::Back){b.open=false;pointer_cancel();hit_count=0;return true;}
 if(key==Key::Quit){quit=true;return true;}
 if(key==Key::EndTurn){bestiary_toggle(*this);return true;}
 const int direction=key==Key::Up?-1:key==Key::Down?1:0;
 if(direction){
  if(!b.focus){int row=b.focused;for(int i=0;i<b.row_count;++i){row=(row+direction+b.row_count)%b.row_count;
    if(bestiary_known(*this,b.rows[row])){b.focused=int16_t(row);b.reveal(row);break;}}
  }else if(b.actions){const unsigned count=bestiary_move_count();if(count)b.action=uint8_t((b.action+int(count)+direction)%int(count));}
  else{int filter=b.filter;for(unsigned i=0;i<6;++i){filter=(filter+direction+6)%6;if(bestiary_filter_enabled(*this,unsigned(filter))){bestiary_choose_filter(*this,unsigned(filter));break;}}}
  return true;
 }
 if(key==Key::Left){b.focus=1;return true;}if(key==Key::Right){b.focus=0;return true;}
 if(key==Key::Confirm){if(!b.focus)bestiary_select(*this,b.focused);else if(b.actions)bestiary_play(*this,b.action);return true;}
 // Original directional-controller action bindings, in their source order.
 if(b.actions){const Key hotkeys[]={Key::Inspect,Key::ExhaustPile,Key::DiscardPile,Key::DrawPile,Key::Potions};
  for(unsigned i=0;i<sizeof hotkeys/sizeof hotkeys[0];++i)if(key==hotkeys[i]){bestiary_play(*this,i);return true;}
 }else if(key==Key::Inspect||key==Key::ExhaustPile){
  const int step=key==Key::Inspect?-1:1;int filter=b.filter;for(unsigned i=0;i<6;++i){filter=(filter+step+6)%6;if(bestiary_filter_enabled(*this,unsigned(filter))){bestiary_choose_filter(*this,unsigned(filter));break;}}
 }
 return true;
}
static const char* bestiary_move_label(const App&a,const BestiaryMoveSpec&move){
 const char*key=move.label;const char*table=nullptr;
 if(key[0]=='m'&&key[1]=='o'){table="monsters";key+=9;}else if(key[0]=='b'&&key[1]=='e'){table="bestiary";key+=9;}
 const char*label=table?a.content.world_string(table,key):"";
 if(*label)return label;
 if(move.clip){if(ui_visual_equal(move.clip,"hurt")||ui_visual_equal(move.clip,"left/hurt")||ui_visual_equal(move.clip,"right/hurt"))return a.content.english()?"Hurt":"受伤";
  if(ui_visual_equal(move.clip,"die")||ui_visual_equal(move.clip,"left/die")||ui_visual_equal(move.clip,"right/die"))return a.content.english()?"Die":"死亡";
  if(ui_visual_equal(move.clip,"revive"))return a.content.english()?"Revive":"复生";}
 return move.label;
}
static unsigned bestiary_duration(const App&a,int clip){
 const auto*row=clip>=0?world_enemy_clip_directory(unsigned(clip)):nullptr;if(row)return row->duration_ms?row->duration_ms:1;
 const auto cl=a.content.clip(clip);return cl.count&&cl.frame_ms?unsigned(cl.count)*cl.frame_ms:1;
}
static int bestiary_current_clip(App&a,BestiaryPresentation::Preview&actor,uint32_t&elapsed,bool&loop){
 const uint32_t now=a.map_clock_ms;
 while(actor.current<actor.count){const int clip=actor.clips[actor.current];const unsigned duration=bestiary_duration(a,clip);
  if(now-actor.start<duration||(actor.hold&&actor.current+1==actor.count)){elapsed=now-actor.start;loop=false;return clip;}
  actor.start+=duration;++actor.current;
 }
 elapsed=now-actor.start;loop=true;return actor.idle;
}
static int bestiary_phobia(const App&a,const BestiaryPresentation::Preview&actor,int clip,bool dead){
 if(!a.phobia_mode||clip<0)return clip;const auto kind=EnemyId(actor.model);
 switch(kind){
 case EnemyId::TheObscura:case EnemyId::FossilStalker:case EnemyId::SkulkingColony:case EnemyId::TwigSlimeM:case EnemyId::HauntedShip:
  if(const auto*row=world_enemy_clip_directory(unsigned(clip))){const int alternate=a.content.world_clip("phobia",row->key);if(alternate>=0)return alternate;}break;
 case EnemyId::DecimillipedeSegmentBack:case EnemyId::DecimillipedeSegmentFront:case EnemyId::DecimillipedeSegmentMiddle:
 case EnemyId::Entomancer:case EnemyId::PhrogParasite:case EnemyId::TerrorEel:case EnemyId::TheInsatiable:case EnemyId::Wriggler:{
  const auto*model=world_enemy_visual(actor.model);char key[96]{};TextBuffer b{key,sizeof key,0};if(model)b.text(model->name);
  const bool segment=kind==EnemyId::DecimillipedeSegmentBack||kind==EnemyId::DecimillipedeSegmentFront||kind==EnemyId::DecimillipedeSegmentMiddle;
  b.text(segment&&dead?".dead_body":".body");const int alternate=a.content.world_clip("phobia",key);if(alternate>=0)return alternate;break;}
 default:break;
 }return clip;
}
static void bestiary_actor(Canvas&c,BestiaryPresentation::Preview&actor,int x,int feet,int scale){
 uint32_t elapsed=0;bool loop=true;int clip=bestiary_current_clip(c.a,actor,elapsed,loop);
 const auto*row=clip>=0?world_enemy_clip_directory(unsigned(clip)):nullptr;
 const bool dead=actor.hold||(row&&ui_visual_equal(row->native_clip,"dead_loop"));clip=bestiary_phobia(c.a,actor,clip,dead);
 if(clip<0)return;const auto animation=c.a.content.clip(clip);if(!animation.count||!animation.frame_ms)return;
 unsigned frame=elapsed/animation.frame_ms;frame=loop?frame%animation.count:unsigned(mn(int(frame),animation.count-1));
 const int id=int(animation.first)+int(frame);const auto image=c.a.content.image(id);if(!image.pixels)return;
 c.picture(image,x-animation.pivot_x*scale/1024,feet-animation.pivot_y*scale/1024,mx(1,image.w*scale/1024),mx(1,image.h*scale/1024));
}
static void bestiary_actors(Canvas&c){
 auto&a=c.a;auto&b=a.bestiary;const auto*entry=bestiary_entry(b.selected);if(!entry)return;
 Canvas layer=c;const unsigned age=a.map_clock_ms-b.selected_at;layer.opacity=uint8_t(age>=250?255:age*255/250);
 if(entry->model==255){const auto*layout=encounter_positions(EncounterId::DecimillipedeElite);
  for(unsigned i=0;i<b.actor_count;++i){const auto bounds=creature_bounds(EnemyId(b.actors[i].model));
   // Original encounter slots are translated left459px in this layout.
   const int x=layout?(layout->points[i][0]-459)/6:132+int(i)*36;
   // Setup assigns GlobalPosition from the marker: the creature container's
   // own88px offset is consequently cancelled, not added a second time.
   const int y=layout?120+(layout->points[i][1]-540)/6:145;
   bestiary_actor(layer,b.actors[i],x,y,mx(160,int(bounds.height)*1024/(6*84)));}
 }else{
  const auto*v=world_enemy_visual(entry->model);
  if(v&&v->shared_group){
   // The shared Kaiser image contains both arms and its body. Its baked
   //109/720 scale and(2773/360,18203/540) origin are the existing world
   // converter contract. Bestiary's0.75/6 scale is90/109 of that image.
   uint32_t elapsed=0;bool loop=true;int clip=bestiary_current_clip(a,b.actors[0],elapsed,loop);const auto animation=a.content.clip(clip);
   if(animation.count&&animation.frame_ms){unsigned frame=elapsed/animation.frame_ms;frame=loop?frame%animation.count:unsigned(mn(int(frame),animation.count-1));
    const int numerator=(265+(v->shared_side==1?510:-620))*218-8319;
    const int left=numerator>=0?(numerator+654)/1308:-((-numerator+654)/1308);
    layer.picture(int(animation.first)+int(frame),left,50,264,180);}
  }else{const auto bounds=creature_bounds(EnemyId(entry->model));const int scale=mx(160,int(bounds.height)*1024/(6*84));
   bestiary_actor(layer,b.actors[0],160,128+int(bounds.height)/12,scale);}
 }
}
struct BestiaryNumbers {uint32_t wins,losses;char rate[16]{};};
static LocalizedValue bestiary_number(const void*context,const char*key){
 const auto&n=*static_cast<const BestiaryNumbers*>(context);
 if(ui_visual_equal(key,"kills"))return {true,n.wins,1,nullptr};
 if(ui_visual_equal(key,"deaths"))return {true,n.losses,1,nullptr};
 if(ui_visual_equal(key,"total"))return {true,int64_t(n.wins)+n.losses,1,nullptr};
 if(ui_visual_equal(key,"winrate"))return {true,0,1,n.rate};return {};
}
static void bestiary_rate(BestiaryNumbers&n){
 const uint64_t total=uint64_t(n.wins)+n.losses;if(!total){n.rate[0]=n.rate[1]='-';n.rate[2]=0;return;}
 // Division stays32bit on ARM. Repeated subtraction has a fixed1000 bound
 // even when a migrated UI statistic reaches its saturateduint32 maximum.
 uint64_t remainder=uint64_t(n.wins)*1000u+total/2;unsigned tenths=0;
 while(remainder>=total&&tenths<1000){remainder-=total;++tenths;}
 TextBuffer b{n.rate,sizeof n.rate,0};b.number(tenths/10);if(tenths%10){b.ch('.');b.number(tenths%10);}
}
static void bestiary_dialogue(Canvas&c,const BestiaryEntrySpec&entry){
 auto&a=c.a;auto&b=a.bestiary;if(!b.filter)return;const unsigned age=a.map_clock_ms-b.filter_at;if(age<100)return;
 const unsigned characters[]={0,0,1,3,4,2},character=characters[b.filter];const auto&stats=bestiary_statistics(a,entry,int(character));
 const auto&definition=character_definitions[character];char key[96]{};TextBuffer text{key,sizeof key,0};text.text(definition.key);
 text.text(stats.wins?".bestiaryKillQuote":".bestiaryQuote");const char*quote=a.content.world_string("characters",key);
 if(!*quote)quote=bestiary_text(a,"QUOTE_PLACEHOLDER","…","……");
 const uint32_t colors[]={0x590700,0x284719,0x13446b,0x52371d,0x6b4658};const uint32_t color=colors[character];
 const int slide=age>=500?0:7-int(health_bar::expo_out((age-100)*1000/400))*7/4096;
 Canvas layer=c;layer.opacity=uint8_t(age>=200?255:(age-100)*255/100);
 const int x=81-slide,y=190;
 layer.ui(U_TopBarIronclad+int(character),x,y+7,18,18);
 Canvas bubble=layer;bubble.tint_red=uint8_t(color>>16);bubble.tint_green=uint8_t(color>>8);bubble.tint_blue=uint8_t(color);
 if(a.content.version()>=Ui7ContentVersion){bubble.nine_ui(I7_BestiaryDialogue,x+25,y,119,34,13,14,13,14);
  bubble.ui(character==unsigned(Character::Silent)?I7_BestiaryThoughtTail:I7_BestiaryTail,x+18,y+11,9,10);}
 else bubble.rect(x+25,y,119,34,White);
 layer.paragraph(quote,x+31,y+5,106,3,0,8,White);
}
static void target_reticle_corner(Canvas&,const Picture&,int,int,int,bool,bool,unsigned,bool);
static void bestiary_controls(Canvas&c){
 auto&a=c.a;auto&b=a.bestiary;const auto*entry=bestiary_entry(b.selected);const unsigned age=a.map_clock_ms-b.mode_at;
 const int eased=age>=500?4096:int(health_bar::expo_out(age*2));const int left=15+(32*eased/4096-32)/6;
 Canvas panel=c;panel.opacity=uint8_t(age>=500?255:age*255/500);
 if(entry){
  if(b.actions){const auto moves=bestiary_moves_for(entry);const int top=mx(48,120-int(moves.count)*8-9);
   panel.text_fitted(bestiary_text(a,"ACTIONS.header","Actions","动作"),left,top-18,84,10,White,8);
   for(unsigned i=0;i<moves.count;++i){const int y=top+int(i)*16;const char*name=bestiary_move_label(a,bestiary_moves[moves.first+i]);
    const bool hover=a.pointer.visible&&a.pointer.x>=left&&a.pointer.x<left+84&&a.pointer.y>=y-2&&a.pointer.y<y+14;
    const bool selected=!a.pointer.visible&&b.focus==1&&b.action==i;
    panel.text_fitted(name,left+(hover?1:0),y,84,hover?10:8,hover||selected?Gold:White,8);
    c.hit(left-3,y-2,90,16,HitKind::BestiaryMove,int(i));}
  }else{
   const unsigned characters[]={0,0,1,3,4,2};const int x=left+9,y=60;
   for(unsigned filter=0;filter<6;++filter){const int px=x+int(filter%3)*23,py=y+int(filter/3)*23;
    const bool enabled=bestiary_filter_enabled(a,filter),chosen=b.filter==filter;
    Canvas icon=panel;if(!enabled)icon.opacity=uint8_t(unsigned(icon.opacity)*51/255);else if(!chosen)icon.brightness=191;
    icon.ui(filter?U_TopBarIronclad+int(characters[filter]):U_ColorlessEnergy,px,py,18,18);
    if(chosen){const auto image=a.content.image(a.content.world_image("map","SELECTION_RETICLE"));
     if(image.pixels&&image.format<=3)for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;
      target_reticle_corner(panel,image,right?px+16:px-3,bottom?py+16:py-3,6,right,bottom,255,false);}}
    c.hit(px-3,py-3,23,23,HitKind::BestiaryFilter,int(filter),enabled);
   }
   const int character=b.filter?int(characters[b.filter]):-1;const auto&s=bestiary_statistics(a,*entry,character);BestiaryNumbers numbers{s.wins,s.losses,{}};bestiary_rate(numbers);
   char text[384]{};const char*format=bestiary_text(a,"STATS.layout","Encounters: {total}\nKills: {kills}\nDeaths: {deaths}\nWin rate: {winrate}%","遭遇：{total}\n击杀：{kills}\n死亡：{deaths}\n胜率：{winrate}%");
   LocalizedTemplate formatter(text,sizeof text,bestiary_number,&numbers);formatter.format(format);
   panel.paragraph(text,left+4,113,86,8,0,8,White);bestiary_dialogue(c,*entry);
  }
 }
 const char*mode=b.actions?bestiary_text(a,"MODE.viewStats","View Stats","查看统计"):bestiary_text(a,"MODE.viewActions","View Actions","查看动作");
 const bool focused=!a.pointer.visible&&b.focus==2;const auto visual=c.control_visual(mode,15,211,94,22,ButtonFamily::Action,focused,true);
 const int width=94*visual.scale/4096,height=22*visual.scale/4096;
 c.ui(U_RewardSkip,15+(94-width)/2,211+(22-height)/2,width,height);
 c.text_fitted(mode,20,216,84,10,Gold,8,true);c.hit(12,208,100,28,HitKind::BestiaryMode);
 native_back(c,3,3,35,22,"");c.hit(3,3,35,22,HitKind::Close);
}
static void bestiary_list(Canvas&c){
 auto&a=c.a;auto&b=a.bestiary;constexpr int left=BestiaryPresentation::Left,top=BestiaryPresentation::Top,bottom=BestiaryPresentation::Bottom;
 Canvas layer=c;layer.pixels+=top*layer.pitch;layer.clip_h=bottom-top;
 const int first=mx(0,b.position/BestiaryPresentation::Unit/BestiaryPresentation::RowHeight-1),last=mn(b.row_count,first+(bottom-top)/BestiaryPresentation::RowHeight+4);
 for(int row=first;row<last;++row){const int index=b.rows[row],y=b.row_y(row);const auto*entry=bestiary_entry(index);
  if(!entry){const int region=-1-index;const char*regions[]={"OVERGROWTH","UNDERDOCKS","HIVE","GLORY"};
   const char*label=region>=0&&region<4?a.content.world_title("acts",regions[region]):bestiary_text(a,"EVENTS.title","Events","事件");
   layer.text_fitted(label,left+1,y-top,68,10,Gold,8);continue;}
  const bool enabled=bestiary_known(a,index),hover=enabled&&a.pointer.visible&&!b.thumb_drag&&!(b.dragging&&a.pointer.dragging)&&a.pointer.x>=left&&a.pointer.x<303&&a.pointer.y>=y&&a.pointer.y<y+14;
  const int scale=b.scale(unsigned(row),hover,a.map_clock_ms);const int font=scale>1090?10:8;
  const char*label=enabled?bestiary_title(a,*entry):bestiary_text(a,"UNSEEN.monsterName","Unknown","未知");
  const uint16_t color=!enabled?Muted:entry->room==3?rgb(242,92,90):entry->room==2?rgb(204,147,225):White;
  layer.text_fitted(label,left,y-top+2,71,font,color,8);
  if(!a.pointer.visible&&!b.focus&&b.focused==row)layer.box(left-3,y-top,76,14,rgb(135,225,225));
  const int hit_top=mx(y,top),hit_bottom=mn(y+14,bottom);if(hit_bottom>hit_top)c.hit(left-3,hit_top,76,hit_bottom-hit_top,HitKind::BestiaryEntry,row,enabled);
 }
 if(b.selected_row>=0){const int y=b.arrow_y(a.map_clock_ms)-top;
  if(a.content.version()>=Ui7ContentVersion)layer.ui(I7_BestiaryPointer,left-12,y-4,8,8);}
 if(b.maximum()>0){const int thumb=b.thumb_y();c.ui(I6_ScrollbarTrack,308,top+8,7,bottom-top-16);
  c.ui(I6_ScrollbarEdge,308,top,7,7);ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),311,bottom-4,7,7,0,-1024);
  c.ui(I6_ScrollbarGrabber,305,thumb-7,14,14);c.hit(302,top,18,bottom-top,HitKind::BestiaryScroll);}
}
static void bestiary_screen(Canvas&c){
 auto&a=c.a;auto&b=a.bestiary;a.hit_count=0;title_background(c,false);
 bestiary_actors(c);const auto*entry=bestiary_entry(b.selected);
 const unsigned age=a.map_clock_ms-b.selected_at;const int eased=age>=500?4096:int(health_bar::expo_out(age*2));
 Canvas title=c;title.font_face=6;title.opacity=uint8_t(age>=500?255:age*255/500);
 const char*name=entry?bestiary_title(a,*entry):bestiary_text(a,"LOCKED.monsterTitle","Unknown","未知");
 title.text_fitted(name,43,5+11*eased/4096,191,16,rgb(239,200,81),10,true);
 bestiary_controls(c);bestiary_list(c);
}
