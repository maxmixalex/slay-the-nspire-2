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
 if(!encounter&&e.model!=255)return a.monster_title(EnemyId(e.model));
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
 b.arrow_from=b.selected_row<0?b.row_center(row):
  (b.arrow_y(a.map_clock_ms)-BestiaryPresentation::Top)*BestiaryPresentation::Unit+b.position;
 b.selected=int16_t(entry);b.selected_row=int16_t(row);b.selected_at=b.mode_at=b.arrow_at=a.map_clock_ms;b.action=0;
 ++b.input_serial;for(auto&motion:b.moves)motion={};
 const auto&e=bestiary_entries[entry];const int filters[]={-1,0,1,3,4,2};
 if(!bestiary_statistics(a,e,filters[b.filter]).wins&&!bestiary_statistics(a,e,filters[b.filter]).losses)b.filter=0;
 b.filter_at=a.map_clock_ms;
 if(e.model==255){b.actor_count=3;const EnemyId models[]={EnemyId::DecimillipedeSegmentBack,EnemyId::DecimillipedeSegmentFront,EnemyId::DecimillipedeSegmentMiddle};
  for(unsigned i=0;i<3;++i)bestiary_prepare_actor(b.actors[i],unsigned(models[i]),a.map_clock_ms);
 }else{b.actor_count=1;bestiary_prepare_actor(b.actors[0],e.model,a.map_clock_ms);}
}
bool App::open_bestiary(bool force){
 if(!force&&!bestiary_available())return false;
 pointer_cancel();physical_keys_cancel();bestiary={};auto&b=bestiary;b.open=true;b.actions=bestiary_actions_preferred;b.opened=b.selected_at=b.mode_at=b.filter_at=map_clock_ms;
 // NBestiary uses CurrentCulture; NGame pins that to InvariantCulture.
 // The two101-byte tables contain the original localized order, verified
 // with .NET. Opening needs one pass and no title decoding or sort scratch.
 const auto&order=bestiary_order[content.english()?0:1];int previous_region=-1;
 for(unsigned position=0;position<BestiaryEntries;++position){const unsigned entry=order[position],region=bestiary_entries[entry].region;
  if(region<4&&!act_discovered(region))continue;
  if(int(region)!=previous_region){b.rows[b.row_count++]=int16_t(-1-int(region));previous_region=int(region);}
  b.rows[b.row_count++]=int16_t(entry);
 }
 b.layout_rows();
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
 auto&b=a.bestiary;const auto*entry=bestiary_entry(b.selected);const auto list=bestiary_moves_for(entry);if(!b.actions||index>=list.count)return;
 b.action=uint8_t(index);const auto&move=bestiary_moves[list.first+index];
 if(index<9)b.moves[index].press(false,a.map_clock_ms);
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
 auto&b=a.bestiary;b.actions=!b.actions;b.mode_at=a.map_clock_ms;b.focus=0;++b.input_serial;
 for(auto&motion:b.moves)motion={};
}
static bool bestiary_filter_enabled(const App&a,unsigned filter){
 const auto*entry=bestiary_entry(a.bestiary.selected);if(!entry||filter>5)return false;
 const int characters[]={-1,0,1,3,4,2};const auto&s=bestiary_statistics(a,*entry,characters[filter]);return s.wins||s.losses;
}
static void bestiary_choose_filter(App&a,unsigned filter){if(!bestiary_filter_enabled(a,filter))return;
 a.bestiary.filter=uint8_t(filter);a.bestiary.filter_at=a.map_clock_ms;
}
#include "bestiary_input.inc"
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
  for(unsigned i=0;i<b.actor_count;++i){
   // Original encounter slots are translated left459px in this layout.
   const int x=layout?160+reference_viewport::project(layout->points[i][0]-960-459):132+int(i)*36;
   // Setup assigns GlobalPosition from the marker: the creature container's
   // own88px offset is consequently cancelled, not added a second time.
   const int y=layout?120+reference_viewport::project(layout->points[i][1]-540):145;
   bestiary_actor(layer,b.actors[i],x,y,mx(1,reference_viewport::project(creature_sprite_units(EnemyId(b.actors[i].model)))));}
 }else{
  const auto*v=world_enemy_visual(entry->model);
  if(v&&v->shared_group){
   // The shared Kaiser image contains both arms and its body. Its baked
   //109/720 scale and(2773/360,18203/540) origin are the existing world
   // converter contract. The source's0.75 scale at4:3 is720/763 of it.
   // Setup divides its510/-620 arm offset by0.75 before adding265.
   uint32_t elapsed=0;bool loop=true;int clip=bestiary_current_clip(a,b.actors[0],elapsed,loop);const auto animation=a.content.clip(clip);
   if(animation.count&&animation.frame_ms){unsigned frame=elapsed/animation.frame_ms;frame=loop?frame%animation.count:unsigned(mn(int(frame),animation.count-1));
    const int left=v->shared_side==1?173:-114;
    layer.picture(int(animation.first)+int(frame),left,23,302,206);}
  }else{const auto bounds=creature_bounds(EnemyId(entry->model));
   const int scale=mx(1,reference_viewport::project(creature_sprite_units(EnemyId(entry->model))));
   // The500px holder is centered48 source px below the viewport center;
   // Setup adds half the actual hitbox height. Sprite registration is
   // independent of that hitbox, just as it is in the combat renderer.
   bestiary_actor(layer,b.actors[0],160,120+reference_viewport::project(48)+reference_viewport::project_half(bounds.height),scale);}
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
// Render hover/press scaling from the authored native glyph band. A small
// foreground tile preserves glyph identity instead of switching to an8/10px
// font every time a5/6px label is focused. No retained buffer is introduced.
static void bestiary_label(Canvas&c,const char*text,int x,int y,int width,int face,int size,uint16_t color,int scale=1024,bool centered=false,int minimum=0){
 Canvas label=c;label.font_face=uint8_t(face);label.bold_face=5;
 if(!minimum)minimum=size;
 if(scale==1024){Canvas shadow=label;shadow.opacity=uint8_t(div255(shadow.opacity*128));
  shadow.fitted(text,x+1,y+1,width,size+5,size,minimum,0,centered,true);
  label.fitted(text,x,y,width,size+5,size,minimum,color,centered,true);return;}
 constexpr int Width=128,Height=16;const int w=mn(width,Width);
 uint16_t pixels[Width*Height]{};uint8_t alpha[Width*Height]{};
 Canvas tile{c.a,pixels,Width,w,Height,alpha};tile.font_face=uint8_t(face);tile.bold_face=5;
 Canvas shadow=tile;shadow.opacity=128;shadow.fitted(text,1,1,w-1,Height,size,minimum,0,centered,true);
 tile.fitted(text,0,0,w-1,Height,size,minimum,color,centered,true);
 // Picture rows must be tightly packed when the label's source width is
 // less than128. Compact in place before presenting the temporary image.
 if(w<Width)for(int row=1;row<Height;++row)for(int col=0;col<w;++col){pixels[row*w+col]=pixels[row*Width+col];alpha[row*w+col]=alpha[row*Width+col];}
 const Picture picture{reinterpret_cast<const uint8_t*>(pixels),alpha,nullptr,uint16_t(w),uint16_t(Height),1};
 const int h=mx(1,Height*scale/1024),outw=mx(1,w*scale/1024);
 c.picture(picture,x+(centered?(w-outw)/2:0),y+(Height-h)/2,outw,h);
}
static void bestiary_keycap(Canvas&c,InputCommand command,int x,int y){
 if(!c.a.keyboard_only||c.a.pointer.visible)return;
 char text[20]{};input_key_label(c.a.input_bindings.keys[1][unsigned(command)],text,sizeof text);
 const int width=mx(9,mn(24,c.width(text,5)+3));c.ui(I8_KeyboardKeycap,x,y,width,9);
 Canvas label=c;label.font_face=0;label.fitted(text,x+1,y+1,width-2,8,5,5,rgb(255,246,226),true,true);
}
static void bestiary_dialogue(Canvas&c,const BestiaryEntrySpec&entry){
 using reference_viewport::project;
 auto&a=c.a;auto&b=a.bestiary;if(!b.filter)return;const unsigned age=a.map_clock_ms-b.filter_at;if(age<100)return;
 const unsigned characters[]={0,0,1,3,4,2},character=characters[b.filter];const auto&stats=bestiary_statistics(a,entry,int(character));
 const auto&definition=character_definitions[character];char key[96]{};TextBuffer text{key,sizeof key,0};text.text(definition.key);
 text.text(stats.wins?".bestiaryKillQuote":".bestiaryQuote");const char*quote=a.content.world_string("characters",key);
 const bool placeholder=!*quote;if(placeholder)quote=bestiary_text(a,"QUOTE_PLACEHOLDER","…","……");
 const uint32_t colors[]={0x590700,0x284719,0x13446b,0x52371d,0x6b4658};const uint32_t color=colors[character];
 const int slide=age>=500?0:project(32)-int(health_bar::expo_out((age-100)*1000/400))*project(32)/4096;
 Canvas layer=c;layer.opacity=uint8_t(age>=200?255:(age-100)*255/100);layer.bold_face=5;
 // Only the original placeholder is wholly italic. The actual seen quotes
 // use Kreon Regular; do not apply the dialogue's italic family globally.
 if(placeholder)layer.font_face=6;
 const int x=project(560)-slide,y=240-project(134),bubble_x=x+project(60),width=project(692);
 const auto metrics=[&](const RichChar&ch,int size){const auto g=a.content.glyph_metrics(ch.codepoint,size,layer.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
 const int rows=fitted_text::line_count(quote,project(634),5,metrics),height=mx(project(56),rows*6+project(22));
 const int top=y+(project(78)-height)/2;
 layer.ui(U_TopBarIronclad+int(character),x,y+project(11),project(56),project(56));
 Canvas bubble=layer;bubble.tint_red=uint8_t(color>>16);bubble.tint_green=uint8_t(color>>8);bubble.tint_blue=uint8_t(color);
 if(a.content.version()>=Ui7ContentVersion){bubble.nine_ui(I7_BestiaryDialogue,bubble_x+project(16),top,width-project(16),height,5,5,5,5);
  bubble.ui(character==unsigned(Character::Silent)?I7_BestiaryThoughtTail:I7_BestiaryTail,bubble_x,top+(height-project(28))/2,project(28),project(28));}
 else bubble.rect(bubble_x,top,width,height,White);
 Canvas shadow=layer;shadow.opacity=uint8_t(div255(shadow.opacity*64));
 const int tx=bubble_x+project(40),ty=top+project(10);
 shadow.fitted(quote,tx+1,ty+1,project(634),mx(12,rows*6+5),5,5,0);
 layer.fitted(quote,tx,ty,project(634),mx(12,rows*6+5),5,5,rgb(255,246,226));
}
static void target_reticle_corner(Canvas&,const Picture&,int,int,int,bool,bool,unsigned,bool);
static void bestiary_controls(Canvas&c){
 using reference_viewport::project;
 auto&a=c.a;auto&b=a.bestiary;const auto*entry=bestiary_entry(b.selected);const unsigned age=a.map_clock_ms-b.mode_at;
 const int eased=age>=500?4096:int(health_bar::expo_out(age*2));const int left=project(210+24+32*eased/4096);
 Canvas panel=c;panel.opacity=uint8_t(age>=500?255:age*255/500);panel.bold_face=5;
 if(entry){
  if(b.actions){const auto moves=bestiary_moves_for(entry);const int source_height=36+4+int(moves.count)*56;
   const int top=120-reference_viewport::project_half(source_height),first=top+project(40);
   bestiary_label(panel,bestiary_text(a,"ACTIONS.header","Actions","行动"),left,top,100,5,5,rgb(239,200,81));
   static constexpr InputCommand shortcuts[]={InputCommand::Deck,InputCommand::Exhaust,InputCommand::Discard,InputCommand::Draw,InputCommand::TopPanel};
   for(unsigned i=0;i<moves.count;++i){const int y=first+project(int(i)*56),h=project(int(i+1)*56)-project(int(i)*56);const char*name=bestiary_move_label(a,bestiary_moves[moves.first+i]);
    const int width=mx(project(155),mn(124,panel.width(name,6)+project(36)));
    const bool hover=a.pointer.visible&&a.pointer.x>=left&&a.pointer.x<left+width&&a.pointer.y>=y&&a.pointer.y<y+h;
    const bool selected=!a.pointer.visible&&b.focus==1&&b.action==i;
    auto&motion=b.moves[i];motion.focus(hover||selected,a.map_clock_ms);
    const bool pointer_press=hover&&a.pointer.held&&a.pointer.captured.kind==HitKind::BestiaryMove&&a.pointer.captured.value==int(i);
    if(pointer_press&&motion.phase!=1)motion.press(true,a.map_clock_ms);
    unsigned color_age=a.map_clock_ms-motion.press_at;uint16_t color=rgb(255,246,226);
    if(motion.phase==1)color=blend(color,rgb(191,191,191),mn(255,int(color_age)*255/50));
    else if(motion.phase==2)color=blend(rgb(127,255,0),color,mn(255,int(color_age)*255/200));
    const int scale=motion.focus_scale(a.map_clock_ms)*motion.press_scale(a.map_clock_ms)/1024;
    bestiary_label(panel,name,left+project(12),y+(h-6)/2,width-project(36),0,6,color,scale);
    if(i<5)bestiary_keycap(panel,shortcuts[i],left-project(49),y+(h-9)/2);
    c.hit(left,y,width,h,HitKind::BestiaryMove,int(i));
   }
  }else{
   const unsigned characters[]={0,0,1,3,4,2};
   const int character=b.filter?int(characters[b.filter]):-1;const auto&s=bestiary_statistics(a,*entry,character);BestiaryNumbers numbers{s.wins,s.losses,{}};bestiary_rate(numbers);
   char text[384]{};const char*format=bestiary_text(a,"STATS.layout","Encounters: {total}\nKills: {kills}\nDeaths: {deaths}\nWin rate: {winrate}%","遭遇：{total}\n击杀：{kills}\n死亡：{deaths}\n胜率：{winrate}%");
   LocalizedTemplate formatter(text,sizeof text,bestiary_number,&numbers);formatter.format(format);
   const int top=120-(project(36+4+128+4)+24)/2,filters_y=top+project(40),stats_y=filters_y+project(132);
   // StatsHeader is an authored literal in the pinned scene; _Ready only
   // localizes MoveHeader, not this label.
   bestiary_label(panel,"Stats",left,top,80,5,5,rgb(239,200,81));
   for(unsigned filter=0;filter<6;++filter){const int px=left+project(int(filter%3)*64),py=filters_y+project(int(filter/3)*64);
    const bool enabled=bestiary_filter_enabled(a,filter),chosen=b.filter==filter;
    Canvas icon=panel;if(!enabled)icon.opacity=uint8_t(unsigned(icon.opacity)*51/255);else if(!chosen)icon.brightness=191;
    icon.ui(filter?U_TopBarIronclad+int(characters[filter]):U_ColorlessEnergy,px+1,py+1,10,10);
    if(chosen){const auto image=a.content.image(a.content.world_image("map","SELECTION_RETICLE"));
     if(image.pixels&&image.format<=3)for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;
      target_reticle_corner(panel,image,right?px+10:px,bottom?py+10:py,4,right,bottom,255,false);}}
    c.hit(px,py,project(64),project(64),HitKind::BestiaryFilter,int(filter),enabled);
   }
   panel.fitted(text,left+project(12),stats_y,91,34,5,5,rgb(255,246,226));
   bestiary_keycap(panel,InputCommand::Deck,left-project(52),filters_y+4);
   bestiary_keycap(panel,InputCommand::Exhaust,left+project(203),filters_y+4);
   bestiary_dialogue(c,*entry);
  }
 }
 const char*mode=b.actions?bestiary_text(a,"MODE.viewStats","View Stats","查看统计"):bestiary_text(a,"MODE.viewActions","View Actions","查看行动");
 constexpr int x=BestiaryPresentation::ModeX,y=BestiaryPresentation::ModeY,w=BestiaryPresentation::ModeWidth,h=BestiaryPresentation::ModeHeight;
 const bool focused=a.pointer.visible?a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h:b.focus==2;
 bool pressed=focused&&a.pointer.held&&a.pointer.captured.kind==HitKind::BestiaryMode;
 for(const auto&held:a.held_inputs)if(held.code&&held.key==BestiaryPresentation::PhysicalBase+9&&held.context==b.input_serial)pressed=true;
 b.mode_motion.observe(focused,pressed,a.map_clock_ms);const int scale=b.mode_motion.scale(a.map_clock_ms);
 const int width=w*scale/1024,height=h*scale/1024;
 c.ui(U_RewardSkip,x+(w-width)/2,y+(h-height)/2,width,height);
 bestiary_label(c,mode,x+project(34),y+(h-5)/2,w-2*project(34),5,5,rgb(232,220,190),scale,true);
 bestiary_keycap(c,InputCommand::Confirm,x+project(216),y+project(23));
 c.hit(x,y,w,h,HitKind::BestiaryMode);
 native_original_back(c,b.opened);
}
static void bestiary_list(Canvas&c){
 using reference_viewport::project;
 auto&a=c.a;auto&b=a.bestiary;constexpr int left=BestiaryPresentation::Left,top=BestiaryPresentation::Top,bottom=BestiaryPresentation::Bottom;
 Canvas layer=c;layer.pixels+=top*layer.pitch;layer.clip_h=bottom-top;
 for(int row=0;row<b.row_count;++row){const int y=b.row_y(row),height=b.row_height(row);if(y+height<=top)continue;if(y>=bottom)break;
  const int index=b.rows[row];const auto*entry=bestiary_entry(index);
  if(!entry){const int region=-1-index;const char*regions[]={"OVERGROWTH","UNDERDOCKS","HIVE","GLORY"};
   const char*label=region>=0&&region<4?a.content.world_title("acts",regions[region]):bestiary_text(a,"EVENTS.title","Events","事件");
   bestiary_label(layer,label,left,y-top+height-8,project(280),5,6,rgb(255,246,226));continue;}
  const bool enabled=bestiary_known(a,index),hover=enabled&&a.pointer.visible&&!b.thumb_drag&&!(b.dragging&&a.pointer.dragging)&&a.pointer.x>=left&&a.pointer.x<BestiaryPresentation::Right&&a.pointer.y>=y&&a.pointer.y<y+height;
  const bool focused=enabled&&!a.pointer.visible&&!b.focus&&b.focused==row;
  const int scale=b.scale(unsigned(row),hover||focused,a.map_clock_ms);
  const char*label=enabled?bestiary_title(a,*entry):bestiary_text(a,"UNSEEN.monsterName","Unknown","未知");
  const uint16_t color=!enabled?rgb(128,128,128):entry->room==3?rgb(255,85,85):entry->room==2?rgb(238,130,238):rgb(255,246,226);
  if(focused){Canvas highlight=layer;highlight.opacity=75;highlight.rect(left-3,y-top,project(326),height+1,rgb(0,197,242));}
  bestiary_label(layer,label,left,y-top+(height-5)/2,project(280),0,5,color,scale,false,a.content.english()?4:5);
  const int hit_top=mx(y,top),hit_bottom=mn(y+height,bottom);if(hit_bottom>hit_top)c.hit(left,hit_top,project(280),hit_bottom-hit_top,HitKind::BestiaryEntry,row,enabled);
 }
 if(b.selected_row>=0){const int y=b.arrow_y(a.map_clock_ms)-top;
  if(a.content.version()>=Ui7ContentVersion){Canvas arrow=layer;arrow.tint_red=239;arrow.tint_green=200;arrow.tint_blue=81;
   arrow.ui(I7_BestiaryPointer,left-project(42),y-project(18),project(36),project(36));}}
 if(b.maximum()>0){constexpr int sx=320-project(100),sy=BestiaryPresentation::ScrollTop,sh=BestiaryPresentation::ScrollBottom-sy;const int thumb=b.thumb_y();
  Canvas track=c;track.tint_red=42;track.tint_green=74;track.tint_blue=82;
  track.ui(I6_ScrollbarTrack,sx,sy,project(50),sh);track.ui(I6_ScrollbarEdge,sx,sy-project(48),project(50),project(48));
  ui_picture_transform(track,a.content.ui_image(I6_ScrollbarEdge),sx+project(25),sy+sh+project(24),project(50),project(48),0,-1024);
  c.ui(I6_ScrollbarGrabber,sx+project(25)-project(36),thumb-project(36),project(72),project(72));
  c.hit(sx-project(11),sy-project(36),project(72),sh+project(72),HitKind::BestiaryScroll);
 }
}
static void bestiary_screen(Canvas&c){
 auto&a=c.a;auto&b=a.bestiary;a.hit_count=0;title_background(c,false);
 bestiary_actors(c);const auto*entry=bestiary_entry(b.selected);
 const unsigned age=a.map_clock_ms-b.selected_at;const int eased=age>=500?4096:int(health_bar::expo_out(age*2));
 Canvas title=c;title.font_face=4;title.opacity=uint8_t(age>=500?255:age*255/500);
 const char*name=entry?bestiary_title(a,*entry):bestiary_text(a,"LOCKED.monsterTitle","Unknown","未知");
 const int title_y=reference_viewport::project(24+64*eased/4096);
 bestiary_label(title,name,0,title_y+2,320,4,10,rgb(239,200,81),1024,true);
 bestiary_controls(c);bestiary_list(c);
}
