// The source separates the initial death/victory presentation from the
// score, badges, and discoveries. The pinned View Run control stays disabled.
void App::sync_game_over_presentation(){
 if(title||!game.world.active||(game.phase!=Phase::Dead&&game.phase!=Phase::Complete))return;
 auto&m=game_over_presentation;const auto&s=game;
 if(m.bound&&m.run==s.world.run_seed&&m.seed==s.seed&&m.floor==s.world.total_floor&&m.character==unsigned(s.character)&&m.phase==unsigned(s.phase))return;
 m.reset();m.bound=true;m.run=s.world.run_seed;m.seed=s.seed;m.floor=s.world.total_floor;m.character=uint8_t(s.character);m.phase=uint8_t(s.phase);m.opened=map_clock_ms;
 m.event_room=s.world.room_type==unsigned(WorldRoom::Event)&&!s.world.event_combat_parent&&!s.original_events.combat_pending;
}
bool App::game_over_button_ready()const{return game_over_presentation.ready(map_clock_ms);}
bool App::game_over_available()const{
 return game.world.active&&(game.phase==Phase::Complete||game.phase==Phase::Dead)&&
  !title&&!paused&&!help&&!panel&&!library&&!character_select&&!map_preview.open&&!frontend_confirm&&!settings_open&&
  !compendium_open&&!statistics_open&&!history_open&&!profiles_open&&!bestiary.open&&auxiliary.screen==AuxiliaryScreen::None&&
  status_actor<0&&detail_card<0&&detail_item<0&&deck_candidate<0;
}
const UiRunHistory* App::game_over_run()const{
 if(progress_matches(progress.current,game))return &progress.current;
 for(unsigned i=0;i<progress.count;++i){const auto*r=history_at(i);if(r&&progress_matches(*r,game))return r;}return nullptr;
}
void App::open_game_over_history(){
 if(!game_over_ui::ViewRunEnabled||!game_over_available()||!game_over_summary)return;
 sync_progress();const auto*r=game_over_run();if(!r)return;
 unsigned index=0;for(;index<progress.count;++index)if(history_at(index)->serial==r->serial)break;
 if(index==progress.count)return;
 open_history();history_index=uint8_t(index);history_from_game_over=true;compendium_open=false;
}
static unsigned game_over_badge_tier(const App&a,const UiRunHistory&r,unsigned badge){
 return a.game_over_presentation.summary?unsigned(a.game_over_presentation.badge_tiers>>(badge*2))&3u:ui_run_badge_rarity(r,UiRunBadge(badge));
}
static unsigned game_over_focus_items(const App&a,int8_t*out){
 const auto*r=a.game_over_run();if(!r)return 0;unsigned n=0;
 for(unsigned i=0;i<UiRunBadgeCount;++i)if(game_over_badge_tier(a,*r,i))out[n++]=int8_t(i);
 if(r->serial==a.progress.current.serial)for(unsigned i=0;i<4;++i)if(a.progress.current_discoveries.count[i])out[n++]=int8_t(32+i);
 return n;
}
void App::show_game_over_summary(){
 sync_game_over_presentation();if(!game_over_available()||game_over_summary||!game_over_button_ready())return;
 const auto*r=game_over_run();unsigned rows=0,badges=0,discoveries=0;uint64_t tiers=0;
 if(r){if(ui_run_facts_complete(*r)){const auto score=ui_run_score(*r);rows=2+unsigned(score.elites>0)+unsigned(score.bosses>0)+unsigned(r->ascension>0);}
  for(unsigned i=0;i<UiRunBadgeCount;++i){const unsigned tier=ui_run_badge_rarity(*r,UiRunBadge(i));tiers|=uint64_t(tier)<<(i*2);badges+=tier!=0;}
  if(r->serial==progress.current.serial)for(unsigned i=0;i<4;++i)discoveries+=progress.current_discoveries.count[i]!=0;
 }
 game_over_presentation.begin_summary(map_clock_ms,rows,badges,discoveries);game_over_presentation.badge_tiers=tiers;game_over_summary=true;pointer_cancel();
}
bool App::game_over_input(Key key){
 sync_game_over_presentation();
 if(!game_over_available())return false;
 if(key==Key::Confirm){ui_action(game_over_summary?UiAction::GameOverMainMenu:UiAction::GameOverSummary);return true;}
 if(!game_over_summary||(key!=Key::Left&&key!=Key::Right&&key!=Key::Up&&key!=Key::Down))return false;
 if(!game_over_button_ready())return true;
 int8_t items[UiRunBadgeCount+4]{};const unsigned n=game_over_focus_items(*this,items);if(!n)return true;
 unsigned at=0;for(;at<n&&items[at]!=game_over_focus;++at);
 if(at==n)game_over_focus=items[0];
 else if(key==Key::Left||key==Key::Right){
  const bool discoveries=items[at]>=32;unsigned first=at,last=at;
  while(first&&(items[first-1]>=32)==discoveries)--first;
  while(last+1<n&&(items[last+1]>=32)==discoveries)++last;
  game_over_focus=items[key==Key::Left?(at==first?last:at-1):(at==last?first:at+1)];
 }else if(key==Key::Down){for(unsigned i=0;i<n;++i)if(items[i]>=32){game_over_focus=items[i];break;}}
 else if(items[at]>=32&&items[0]<32)game_over_focus=items[0];
 return true;
}
static const char* game_over_text(const App&a,const char*key,const char*fallback=""){
 const char*text=a.content.world_string("game_over_screen",key);return *text?text:fallback;
}
static const char* game_over_character(const App&a){
 const auto&definition=character_definitions[unsigned(a.game.character)];const char*text=a.content.world_title("characters",definition.key);return *text?text:definition.name;
}
struct GameOverValues {const App*app;const UiRunHistory*run;unsigned count=0;};
static LocalizedValue game_over_value(const void*opaque,const char*name){
 const auto&v=*static_cast<const GameOverValues*>(opaque);const auto&a=*v.app;const auto&s=a.game;const auto*r=v.run;
 if(wt_equal(name,"FloorCount"))return wt_number(s.world.total_floor);
 if(wt_equal(name,"AscensionLevel"))return wt_number(s.world.ascension);
 if(wt_equal(name,"character"))return wt_string(game_over_character(a));
 if(wt_equal(name,"characterGender"))return wt_string(s.character==Character::Silent||s.character==Character::Necrobinder?"feminine":s.character==Character::Defect?"neutral":"masculine");
 if(wt_equal(name,"characterObject")||wt_equal(name,"possessiveAdjective")||wt_equal(name,"pronounObject")||wt_equal(name,"pronounPossessive")||wt_equal(name,"pronounSubject")){
  char key[80]{};TextBuffer b{key,sizeof key,0};b.text(character_definitions[unsigned(s.character)].key);b.ch('.');b.text(wt_equal(name,"characterObject")?"titleObject":name);
  return wt_string(a.content.world_string("characters",key));
 }
 if(wt_equal(name,"encounter")){const auto*d=encounter_definition(s.enemy_world.encounter);return d?wt_string(a.content.world_title("encounters",d->key)):LocalizedValue{};}
 if(wt_equal(name,"event"))return wt_string(a.content.world_title(ui_ancient_event(s.original_events.id)?"ancients":"events",original_event_key(s.original_events.id)));
 if(wt_equal(name,"CardCount")||wt_equal(name,"RelicCount")||wt_equal(name,"PotionCount")||wt_equal(name,"EnemyCount"))return wt_number(v.count);
 if(r&&ui_run_facts_complete(*r)){
  if(wt_equal(name,"GoldAmount"))return wt_number(r->gold_gained);
  if(wt_equal(name,"EliteCount"))return wt_number(ui_run_elites(*r));
  if(wt_equal(name,"BossCount"))return wt_number(ui_run_bosses(*r));
  if(wt_equal(name,"PlayerDamage"))return wt_number(ui_run_score(*r).total);
  if(wt_equal(name,"PersonalDamage"))return wt_number(a.progress.architect_damage);
 }
 return {};
}
static void game_over_format(const App&a,const char*raw,char*out,unsigned capacity,unsigned count=0){
 const GameOverValues values{&a,a.game_over_run(),count};LocalizedTemplate(out,capacity,game_over_value,&values,"Energy","Stars",a.content.english()).format(raw);
}
void App::game_over_quote(char*out,unsigned capacity)const{
 if(capacity)out[0]=0;const auto&s=game;const char*table="run_history";char key[128]{};
 reference::Random random;unsigned length=0;while(s.world.seed_text[length])++length;
 random.reset(reference::hash64(s.world.seed_text,length));
 TextBuffer k{key,sizeof key,0};
 if(s.phase==Phase::Complete){k.text("MAP_POINT_HISTORY.falseVictory.");k.number(random.below(2));}
 else if(const auto*r=game_over_run();r&&r->abandoned){k.text("MAP_POINT_HISTORY.abandon.");k.number(random.below(3));}
 else if(world_room_is_combat(s)||s.world.event_combat_parent||s.original_events.combat_pending){
  const auto*d=encounter_definition(s.enemy_world.encounter);if(d){table="encounters";k.text(d->key);k.text(".loss");}
 }else if(s.original_events.active){table=ui_ancient_event(s.original_events.id)?"ancients":"events";k.text(original_event_key(s.original_events.id));k.text(".loss");}
 char raw[2048]{};TextBuffer text{raw,sizeof raw,0};text.text(game_over_text(*this,"ENCOUNTER_QUOTE_LEFT","“"));
 const char*quote=content.world_string(table,key);if(!*quote)quote=content.world_string("run_history","MAP_POINT_HISTORY.debug");text.text(quote);
 text.text(game_over_text(*this,"ENCOUNTER_QUOTE_RIGHT","”"));game_over_format(*this,raw,out,capacity);
}
void App::game_over_detail(int focus,char*title,unsigned title_capacity,char*body,unsigned body_capacity)const{
 if(title_capacity)title[0]=0;if(body_capacity)body[0]=0;const auto*r=game_over_run();if(!r)return;
 if(focus>=0&&unsigned(focus)<UiRunBadgeCount){
  const unsigned tier=game_over_badge_tier(*this,*r,unsigned(focus));if(!tier)return;
  const auto entry=game_over_ui::BadgeTexts[focus];const char*packed=content.string(entry.first+(entry.tiers==3?tier-1:0));
  TextBuffer t{title,title_capacity,0},b{body,body_capacity,0};while(*packed&&*packed!='\n')t.ch(*packed++);if(*packed)++packed;b.text(packed);return;
 }
 if(focus<32||focus>=36||r->serial!=progress.current.serial)return;const unsigned kind=unsigned(focus-32);const auto&discoveries=progress.current_discoveries;
 if(!discoveries.count[kind])return;
 static const char*const names[]={"CARD","RELIC","POTION","ENEMY"};char key[64]{};TextBuffer k{key,sizeof key,0};k.text("DISCOVERY_HEADER_");k.text(names[kind]);
 TextBuffer heading{title,title_capacity,0};heading.text(game_over_text(*this,key));
 key[0]=0;k.n=0;k.text("DISCOVERY_BODY_");k.text(names[kind]);game_over_format(*this,game_over_text(*this,key),body,body_capacity,discoveries.count[kind]);
 TextBuffer b{body,body_capacity,0};while(b.n<body_capacity&&body&&body[b.n])++b.n;b.text("\n\n");
 for(unsigned i=0;i<discoveries.count[kind]&&i<10;++i){if(i)b.ch('\n');const auto id=discoveries.first[kind][i];
  if(!kind)b.text(content.card_string(id));else if(kind==1)b.text(relic_title(RelicId(id)));
  else if(kind==2){const auto*d=potion_definition(Potion(id+1));if(d)b.text(content.world_title("potions",d->key));}
  else b.text(monster_title(EnemyId(id)));
 }if(discoveries.count[kind]>10)b.text("\n....");
}
static void game_over_shader_backstop(Canvas&c,unsigned phase){
 if(!phase)return;const uint16_t color=rgb(31,0,11);const int cutoff=game_over_transition::cutoff(phase);
 if(cutoff>int(game_over_transition::Denominator))return;
 if(cutoff<=0){Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*game_over_transition::Alpha));layer.rect(0,0,ScreenW,ScreenH,color);return;}
 const auto mask=c.a.content.image(c.a.content.ui_image(I9_GameOverTransition));
 if(c.a.content.version()>=GameOverTransitionContentVersion&&mask.pixels&&mask.format==0&&mask.w==ScreenW&&mask.h==ScreenH){
  // Source texture_transition.gdshader: step(1-red,mix(-.1,1.1,t)).
  // The baked scalar keeps the two exact linear-filter taps, so this is one
  // unsigned comparison per output pixel. No new raster or lookup cache.
  for(int y=0;y<mn(c.clip_h,ScreenH);++y){const uint8_t*row=mask.pixels+unsigned(y)*ScreenW*2;
   for(int x=0;x<mn(c.clip_w,ScreenW);++x){const unsigned value=unsigned(row[x*2])|(unsigned(row[x*2+1])<<8);
    if(int(value)>=cutoff)c.paint(y*c.pitch+x,color,game_over_transition::Alpha);
   }
  }
 }else{
  // Pre-v31 packs lack the authored scalar texture. Keep them playable with
  // the correct color, final alpha and source clock; v31 uses the mask above.
  Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*game_over_transition::Alpha*phase/4096));layer.rect(0,0,ScreenW,ScreenH,color);
 }
}
static void game_over_backdrop(Canvas&c){
 auto&a=c.a;const auto&s=a.game;
 const bool event_room=(s.phase==Phase::Complete&&s.original_events.id==OriginalEventId::TheArchitect)||
  (s.original_events.active&&!s.original_events.combat_pending&&!s.world.event_combat_parent);
 const int event=event_room?a.content.world_image("events",original_event_key(s.original_events.id)):-1;
 if(event>=0)c.picture(event,0,22,320,218);else c.picture(a.combat_background(),0,22,320,218);
 const auto&m=a.game_over_presentation;const unsigned age=m.initial_age(a.map_clock_ms);
 // FullBlackBackstop is true black, not the general UI's green-black Ink.
 // The event's 200 ms fade precedes the 1500 ms source threshold tween.
 if(m.event_room){Canvas black=c;black.opacity=uint8_t(div255(black.opacity*m.black_alpha(a.map_clock_ms)));black.rect(0,0,ScreenW,ScreenH,0);}
 game_over_shader_backstop(c,m.backstop_phase(a.map_clock_ms));
 game_over_actor(c,age,m.event_room?m.black_alpha(a.map_clock_ms):255);
 // The separate summary backstop subsequently covers retained creatures.
 if(a.game_over_summary){Canvas summary=c;summary.opacity=uint8_t(div255(summary.opacity*224*m.summary_opacity(a.map_clock_ms)/255));summary.rect(0,22,320,218,rgb(29,3,0));}
}
static bool game_over_hovered(const Canvas&c,int x,int y,int w,int h){const auto&p=c.a.pointer;return p.visible&&p.x>=x&&p.x<x+w&&p.y>=y&&p.y<y+h;}
static void game_over_button(Canvas&c,int x,UiAction action,const char*label,InputCommand command,bool enabled=true){
 using namespace game_over_ui;auto&m=c.a.game_over_presentation;if(!m.ready(c.a.map_clock_ms))return;
 const int y=ButtonY+(c.a.game_over_summary?0:m.continue_offset(c.a.map_clock_ms));const bool hover=enabled&&game_over_hovered(c,x,y,ButtonWidth,ButtonHeight);
 Canvas button=c;button.opacity=uint8_t(div255(button.opacity*m.button_alpha(c.a.map_clock_ms)));button.brightness=enabled?(hover?255:215):110;
 button.ui(U_RewardSkip,x,y,ButtonWidth,ButtonHeight);button.font_face=5;
 button.fitted(label,x+4,y+2,ButtonWidth-8,9,5,5,rgb(253,244,227),true,true,true);
 c.action(x,y,ButtonWidth,ButtonHeight,action,enabled);
 const uint16_t code=c.a.input_bindings.keys[c.a.keyboard_only?1:0][unsigned(command)];
 if(!c.a.pointer.visible&&code){char key[16]{};input_key_label(code,key,sizeof key);
  button.text_fitted(key,x,y+14,ButtonWidth,5,Muted,5,true);}
}
static void game_over_score_line(Canvas&c,const char*key,unsigned icon,int y,const char*points,unsigned row){
 using namespace game_over_ui;char label[256]{};game_over_format(c.a,game_over_text(c.a,key),label,sizeof label);
 const auto motion=c.a.game_over_presentation.score(row,c.a.map_clock_ms);if(!motion.visible||!motion.alpha)return;
 Canvas text=c;text.opacity=uint8_t(div255(text.opacity*motion.alpha));const int x=ScoreX+motion.offset;text.ui(int(icon),x,y,8,9);text.font_face=5;
 text.fitted(label,x+10,y+1,72,8,5,5,White,false,true);
 text.fitted(points,x+82,y+1,18,8,5,5,Gold,false,true);
}
static void game_over_hover(Canvas&c,int focus,int anchor_x,int anchor_y){
 char title[192]{},body[2048]{};c.a.game_over_detail(focus,title,sizeof title,body,sizeof body);if(!*title)return;
 const int width=160,lines=c.paragraph(body,0,0,width-16,0,0,8),height=mn(208,31+lines*11);
 const int x=mn(157,mx(3,anchor_x+12)),y=mn(238-height,mx(23,anchor_y-height-4));
 c.nine_ui(U_HoverTip,x,y,width,height,7,7,7,7);c.text_fitted(title,x+8,y+6,width-16,10,Gold,8);
 c.paragraph(body,x+8,y+23,width-16,(height-27)/11,0,8,White);
}
static void native_game_over_screen(Canvas&c){
 using namespace game_over_ui;auto&a=c.a;const auto&s=a.game;a.hit_count=0;game_over_backdrop(c);
 const auto&motion=a.game_over_presentation;const auto now=a.map_clock_ms;
 const auto*r=a.game_over_run();const bool known=r&&ui_run_facts_complete(*r);
 const int banner_y=motion.banner_y(now);Canvas banner=c;banner.opacity=uint8_t(div255(banner.opacity*motion.ui_alpha(now)));
 banner.ui(I6_CommonBanner,BannerX,banner_y,BannerWidth,BannerHeight);
 char banner_key[48]{};TextBuffer bk{banner_key,sizeof banner_key,0};
 if(s.phase==Phase::Complete)bk.text("BANNER.falseWin");else{bk.text("BANNER.lose");bk.number(unsigned(s.world.run_seed>>32)%8);}
 Canvas label=c;label.font_face=5;banner.font_face=5;
 banner.fitted(game_over_text(a,banner_key,s.phase==Phase::Complete?"Victory?":"Defeat"),BannerX+11,banner_y+10,BannerWidth-22,15,10,7,Gold,true,true,true);
 char quote[2048]{};
 if(!a.game_over_summary){
  if(s.phase==Phase::Complete&&known){game_over_format(a,game_over_text(a,"VICTORY_DAMAGE_LOCAL"),quote,sizeof quote);
   const unsigned age=motion.initial_age(now),elapsed=age<motion.intro_delay()?0:age-motion.intro_delay();
   label.opacity=uint8_t(div255(label.opacity*255*room_curve::linear(elapsed,2000)/4096));
   RichReader reader(quote);RichChar ch{};unsigned count=0;while(reader.next(ch))++count;
   label.text_visible_characters=count*room_curve::sine_out(elapsed,2000)/4096;
   label.paragraph_fitted(quote,42,106,236,72,7,White,5,true);
  }else if(s.phase!=Phase::Complete){char key[24]{};TextBuffer b{key,sizeof key,0};b.text("QUOTES.");unsigned index=unsigned(s.world.run_seed)%17;if(index<10)b.ch('0');b.number(index);
   game_over_format(a,game_over_text(a,key),quote,sizeof quote);Canvas italic=label;italic.font_face=6;italic.opacity=uint8_t(div255(italic.opacity*motion.quote_alpha(now)));
   italic.paragraph_fitted(quote,26,motion.quote_y(now),268,30,5,rgb(255,100,100),5,true);
  }else label.paragraph_fitted(a.content.string(PartialHistoryText),44,111,232,28,5,Muted,5,true);
  game_over_button(c,ContinueX,UiAction::GameOverSummary,game_over_text(a,"BUTTON.continue","Continue"),InputCommand::Select);return;
 }
 if(s.phase!=Phase::Complete){
  if(motion.quote_from&&motion.summary_age(now)>=250)a.game_over_quote(quote,sizeof quote);
  else{char key[24]{};TextBuffer b{key,sizeof key,0};b.text("QUOTES.");unsigned index=unsigned(s.world.run_seed)%17;if(index<10)b.ch('0');b.number(index);game_over_format(a,game_over_text(a,key),quote,sizeof quote);}
  Canvas italic=label;italic.font_face=6;italic.opacity=uint8_t(div255(italic.opacity*motion.quote_alpha(now)));italic.paragraph_fitted(quote,26,motion.quote_y(now),268,22,5,rgb(255,100,100),5,true);
 }
 const unsigned rows=known?motion.rows:2;
 const int score_height=int(rows)*ScoreStep+2;int y=116-(score_height-16)/2;
 if(known){const auto score=ui_run_score(*r);char amount[32]{};unsigned row=0;
  const auto points=[&](unsigned n){amount[0]=0;TextBuffer b{amount,sizeof amount,0};b.ch('+');b.number(n);};
  points(score.floor_points);game_over_score_line(c,"SCORE_LINE.floorsClimbed",I8_GameOverScoreFloor,y,amount,row++);y+=ScoreStep;
  points(score.gold_points);game_over_score_line(c,"SCORE_LINE.goldGained",I8_GameOverScoreGold,y,amount,row++);y+=ScoreStep;
  if(score.elites){points(score.elites*50);game_over_score_line(c,"SCORE_LINE.elitesKilled",I8_GameOverScoreElite,y,amount,row++);y+=ScoreStep;}
  if(score.bosses){points(score.bosses*100);game_over_score_line(c,"SCORE_LINE.bossesSlain",I8_GameOverScoreBoss,y,amount,row++);y+=ScoreStep;}
  if(r->ascension){amount[0]=0;TextBuffer b{amount,sizeof amount,0};b.ch('x');b.number(1+r->ascension/10);if(r->ascension%10){b.ch('.');b.number(r->ascension%10);}game_over_score_line(c,"SCORE_LINE.ascension",I8_GameOverScoreAscension,y,amount,row++);y+=ScoreStep;}
 }else{label.paragraph_fitted(a.content.string(PartialHistoryText),72,y,176,24,5,Muted,5,true);y+=24;}
 y+=5;int8_t items[UiRunBadgeCount+4]{};const unsigned item_count=game_over_focus_items(a,items);
 unsigned badge_count=0;while(badge_count<item_count&&items[badge_count]<32)++badge_count;
 int focus=a.pointer.visible?-1:a.game_over_focus,anchor_x=150,anchor_y=y;
 for(unsigned i=0;i<badge_count;++i){const unsigned id=unsigned(items[i]),tier=game_over_badge_tier(a,*r,id);const int x=(320-int(badge_count)*BadgeSize)/2+int(i)*BadgeSize;
  const auto state=motion.badge(i,now);if(!state.visible||!state.alpha)continue;const int py=y+state.offset;
  Canvas badge=c;badge.opacity=uint8_t(div255(badge.opacity*state.alpha));if(game_over_hovered(c,x,py,BadgeSize,BadgeSize))badge.brightness=255;else badge.brightness=225;
  badge.ui(I8_GameOverBadgeBronze+int(tier)-1,x,py,BadgeSize,BadgeSize);badge.ui(I8_GameOverBadgeCombo+int(id),x,py,BadgeSize,BadgeSize);
  c.hit(x,py,BadgeSize,BadgeSize,HitKind::GameOverDetail,int(id));if(focus==int(id)){anchor_x=x;anchor_y=py;c.box(x,py,BadgeSize,BadgeSize,Gold);}
 }
 y+=badge_count?BadgeSize+9:5;
 const unsigned discoveries=item_count-badge_count;
 if(discoveries){Canvas header=label;header.opacity=uint8_t(div255(header.opacity*motion.discovery_header_alpha(now)));header.text_fitted(game_over_text(a,"DISCOVERY_HEADER"),80,y,160,5,White,5,true);y+=10;
  for(unsigned i=0;i<discoveries;++i){const unsigned kind=unsigned(items[badge_count+i]-32);const int x=(320-int(discoveries)*32)/2+int(i)*32;
   const auto state=motion.discovery(i,now);if(!state.visible||!state.alpha)continue;const int py=y+state.offset;Canvas item=label;item.opacity=uint8_t(div255(item.opacity*state.alpha));
   item.ui(I8_GameOverDiscoveryCard+int(kind),x,py,10,10);item.num(a.progress.current_discoveries.count[kind],x+12,py+2,5,White);
   c.hit(x,py,25,11,HitKind::GameOverDetail,int(kind+32));if(focus==int(kind+32)){anchor_x=x;anchor_y=py;c.box(x,py,25,11,Gold);}
  }
 }
 const char*view_label=a.content.string(ViewRunText);if(!*view_label)view_label=a.content.english()?"View Run":"查看本局";
 if(ViewRunEnabled)game_over_button(c,ViewRunX,UiAction::GameOverViewRun,view_label,InputCommand::Confirm,r&&r->outcome&&a.progress.count);
 game_over_button(c,MainMenuX,UiAction::GameOverMainMenu,game_over_text(a,"BUTTON.mainMenu","Main Menu"),InputCommand::Select);
 if(a.game_over_available()){
  if(a.pointer.visible&&!a.pointer.held){const auto*h=a.hit_at(a.pointer.x,a.pointer.y);if(h&&h->kind==HitKind::GameOverDetail){focus=h->value;anchor_x=h->x;anchor_y=h->y;}}
  if(focus>=0)game_over_hover(c,focus,anchor_x,anchor_y);
 }
}
