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
 auto&m=game_over_presentation;
 if(key==Key::Up||key==Key::Down){m.scroll=uint16_t(mx(0,mn(int(m.scroll)+(key==Key::Up?-26:26),m.scroll_maximum)));game_over_focus=-1;return true;}
 if(!game_over_button_ready())return true;
 int8_t items[UiRunBadgeCount+4]{};const unsigned n=game_over_focus_items(*this,items);if(!n)return true;
 unsigned at=0;for(;at<n&&items[at]!=game_over_focus;++at);
 game_over_focus=items[at==n?0:key==Key::Left?(at?at-1:n-1):(at+1)%n];m.reveal_focus=true;
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
 const GameOverValues values{&a,a.game_over_run(),count};LocalizedTemplate(out,capacity,game_over_value,&values,"Energy","Stars",a.content.english()).for_language(a.content.language()).format(raw);
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
 if(s.phase==Phase::Complete&&s.original_events.id==OriginalEventId::TheArchitect){
  // NGameOverScreen reparents every VisualOnly combat creature above its
  // backstops. RunManager.WinRun then kills the player at this same origin;
  // hand its original left-side pose to die without a duplicate idle body.
  const bool terminal=int32_t(a.map_clock_ms-m.opened)>=0;
  architect_scene_actors(c,!terminal);if(terminal)game_over_actor(c,age,255);
 }else{combat_doom_layers(c,false);game_over_actor(c,age,m.event_room?m.black_alpha(a.map_clock_ms):255);combat_doom_layers(c,true);combat_doom_overlay(c);}
 // The separate summary backstop subsequently covers retained creatures.
 if(a.game_over_summary){Canvas summary=c;summary.opacity=uint8_t(div255(summary.opacity*224*m.summary_opacity(a.map_clock_ms)/255));summary.rect(0,22,320,218,rgb(29,3,0));}
}
static bool game_over_hovered(const Canvas&c,int x,int y,int w,int h){const auto&p=c.a.pointer;return p.visible&&p.x>=x&&p.x<x+w&&p.y>=y&&p.y<y+h;}
static void game_over_button(Canvas&c,int x,UiAction action,const char*label,InputCommand command,bool enabled=true){
 using namespace game_over_ui;auto&m=c.a.game_over_presentation;if(!m.ready(c.a.map_clock_ms))return;
 const int y=ButtonY+(c.a.game_over_summary?0:m.continue_offset(c.a.map_clock_ms));const bool hover=enabled&&game_over_hovered(c,x,y,ButtonWidth,ButtonHeight);
 Canvas button=c;button.opacity=uint8_t(div255(button.opacity*m.button_alpha(c.a.map_clock_ms)));button.brightness=enabled?(hover?255:215):110;
 button.ui(U_RewardSkip,x,y,ButtonWidth,ButtonHeight);button.font_face=source_label_face(c.a.content.original_label_fonts(),5);
 button.fitted(label,x+16,y+2,ButtonWidth-32,22,10,10,rgb(253,244,227),true,false,true);
 c.action(x,y,ButtonWidth,ButtonHeight,action,enabled);
 const uint16_t code=c.a.input_bindings.keys[c.a.keyboard_only?1:0][unsigned(command)];
 if(!c.a.pointer.visible&&code){char key[16]{};input_key_label(code,key,sizeof key);
  button.text_fitted(key,x+ButtonWidth+4,y+8,38,8,Muted,8);}
}
static void game_over_score_line(Canvas&c,const char*key,unsigned icon,int y,const char*points,unsigned row){
 using namespace game_over_ui;char label[256]{};game_over_format(c.a,game_over_text(c.a,key),label,sizeof label);
 const auto motion=c.a.game_over_presentation.score(row,c.a.map_clock_ms);if(!motion.visible||!motion.alpha)return;
 Canvas text=c;text.opacity=uint8_t(div255(text.opacity*motion.alpha));const int x=ScoreX+motion.offset;text.ui(int(icon),x,y+4,14,16);text.font_face=source_label_face(c.a.content.original_label_fonts(),5);
 text.fitted(label,x+20,y+1,ScoreWidth-64,22,10,10,White);
 text.fitted(points,x+ScoreWidth-40,y+5,40,15,10,10,Gold,false,true);
}
static void game_over_hover(Canvas&c,int focus,int anchor_x,int anchor_y){
 char title[192]{},body[2048]{};c.a.game_over_detail(focus,title,sizeof title,body,sizeof body);if(!*title)return;
 using namespace game_over_ui;
 source_hover_box(c,title,body,anchor_x+(focus<32?BadgeTipX:DiscoveryTipX),anchor_y+(focus<32?BadgeTipY:DiscoveryTipY));
}
static void native_game_over_screen(Canvas&c){
 using namespace game_over_ui;auto&a=c.a;const auto&s=a.game;a.hit_count=0;game_over_backdrop(c);
 auto&motion=a.game_over_presentation;const auto now=a.map_clock_ms;
 const auto*r=a.game_over_run();const bool known=r&&ui_run_facts_complete(*r);
 const int banner_y=motion.banner_y(now);Canvas banner=c;banner.opacity=uint8_t(div255(banner.opacity*motion.ui_alpha(now)));
 banner.ui(I6_CommonBanner,BannerX,banner_y,BannerWidth,BannerHeight);
 char banner_key[48]{};TextBuffer bk{banner_key,sizeof banner_key,0};
 if(s.phase==Phase::Complete)bk.text("BANNER.falseWin");else{bk.text("BANNER.lose");bk.number(unsigned(s.world.run_seed>>32)%8);}
 Canvas label=c;label.font_face=source_label_face(c.a.content.original_label_fonts(),5);banner.font_face=source_label_face(c.a.content.original_label_fonts(),5);
 banner.fitted(game_over_text(a,banner_key,s.phase==Phase::Complete?"Victory?":"Defeat"),BannerX+11,banner_y+5,BannerWidth-22,20,12,10,Gold,true,true,true);
 char quote[2048]{};
 if(!a.game_over_summary){
  if(s.phase==Phase::Complete&&known){game_over_format(a,game_over_text(a,"VICTORY_DAMAGE_LOCAL"),quote,sizeof quote);
   const unsigned age=motion.initial_age(now),elapsed=age<motion.intro_delay()?0:age-motion.intro_delay();
   label.opacity=uint8_t(div255(label.opacity*255*room_curve::linear(elapsed,2000)/4096));
   RichReader reader(quote);RichChar ch{};unsigned count=0;while(reader.next(ch))++count;
   label.text_visible_characters=count*room_curve::sine_out(elapsed,2000)/4096;
   label.font_face=source_label_face(a.content.original_label_fonts(),6);label.paragraph_fitted(quote,26,96,268,105,10,White,10,true);
  }else if(s.phase!=Phase::Complete){char key[24]{};TextBuffer b{key,sizeof key,0};b.text("QUOTES.");unsigned index=unsigned(s.world.run_seed)%17;if(index<10)b.ch('0');b.number(index);
   game_over_format(a,game_over_text(a,key),quote,sizeof quote);Canvas italic=label;italic.font_face=source_label_face(a.content.original_label_fonts(),6);italic.opacity=uint8_t(div255(italic.opacity*motion.quote_alpha(now)));
   const int height=mn(100,italic.paragraph(quote,26,0,268,0,0,10)*13+2);
   italic.paragraph_fitted(quote,26,motion.quote_y(now)+8,268,height,10,rgb(255,100,100),10,true);
  }else{
   // A compatible older save lacks score facts, but its explanatory text
   // still belongs to the victory quote stage. Do not cover the retained
   // Architect sequence or the200ms terminal fade with an opaque fallback.
   const unsigned age=motion.initial_age(now),elapsed=age<motion.intro_delay()?0:age-motion.intro_delay();
   label.opacity=uint8_t(div255(label.opacity*255*room_curve::linear(elapsed,2000)/4096));
   label.paragraph_fitted(a.content.string(PartialHistoryText),26,96,268,105,10,Muted,10,true);
  }
  game_over_button(c,ContinueX,UiAction::GameOverSummary,game_over_text(a,"BUTTON.continue","Continue"),InputCommand::Select);return;
 }
 // The original terminal sequence remains timed; its readable summary is a
 // clipped 10px article with a fixed confirmation button on the LCD.
 if(s.phase!=Phase::Complete){
  if(motion.quote_from&&motion.summary_age(now)<250){char key[24]{};TextBuffer b{key,sizeof key,0};b.text("QUOTES.");unsigned index=unsigned(s.world.run_seed)%17;if(index<10)b.ch('0');b.number(index);game_over_format(a,game_over_text(a,key),quote,sizeof quote);}
  else a.game_over_quote(quote,sizeof quote);
 }
 Canvas body=label;body.pixels+=BodyTop*body.pitch;if(body.coverage)body.coverage+=BodyTop*body.pitch;body.clip_h=BodyHeight;
 Canvas quote_measure=body;quote_measure.font_face=source_label_face(a.content.original_label_fonts(),6);
 const int quote_height=*quote?quote_measure.paragraph(quote,26,0,268,0,0,10)*13+9:0;
 const int score_top=quote_height;
 int8_t items[UiRunBadgeCount+4]{};const unsigned item_count=game_over_focus_items(a,items);
 unsigned badge_count=0;while(badge_count<item_count&&items[badge_count]<32)++badge_count;
 const unsigned discoveries=item_count-badge_count;
 const int badge_top=score_top+(known?int(motion.rows)*ScoreStep:52)+6;
 const int discovery_top=badge_top+(badge_count?int((badge_count+BadgesPerRow-1)/BadgesPerRow)*(BadgeSize+4)+6:0);
 const int full_height=discovery_top+(discoveries?45:0);
 motion.scroll_maximum=uint16_t(mx(0,full_height-BodyHeight));
 if(motion.reveal_focus){
  for(unsigned i=0;i<item_count;++i)if(items[i]==a.game_over_focus){
   const int top=i<badge_count?badge_top+int(i/BadgesPerRow)*(BadgeSize+4):discovery_top+18;
   if(top<int(motion.scroll))motion.scroll=uint16_t(top);
   else if(top+BadgeSize>int(motion.scroll)+BodyHeight)motion.scroll=uint16_t(top+BadgeSize-BodyHeight);
  }motion.reveal_focus=false;
 }
 motion.scroll=uint16_t(mx(0,mn(motion.scroll,motion.scroll_maximum)));const int scroll=motion.scroll;
 if(*quote){Canvas italic=body;italic.font_face=source_label_face(a.content.original_label_fonts(),6);italic.opacity=uint8_t(div255(italic.opacity*motion.quote_alpha(now)));
  italic.paragraph(quote,26,-scroll,268,200,0,10,rgb(255,100,100));}
 int y=score_top-scroll;
 if(known){const auto score=ui_run_score(*r);char amount[32]{};unsigned row=0;
  const auto points=[&](unsigned n){amount[0]=0;TextBuffer b{amount,sizeof amount,0};b.ch('+');b.number(n);};
  points(score.floor_points);game_over_score_line(body,"SCORE_LINE.floorsClimbed",I8_GameOverScoreFloor,y,amount,row++);y+=ScoreStep;
  points(score.gold_points);game_over_score_line(body,"SCORE_LINE.goldGained",I8_GameOverScoreGold,y,amount,row++);y+=ScoreStep;
  if(score.elites){points(score.elites*50);game_over_score_line(body,"SCORE_LINE.elitesKilled",I8_GameOverScoreElite,y,amount,row++);y+=ScoreStep;}
  if(score.bosses){points(score.bosses*100);game_over_score_line(body,"SCORE_LINE.bossesSlain",I8_GameOverScoreBoss,y,amount,row++);y+=ScoreStep;}
  if(r->ascension){amount[0]=0;TextBuffer b{amount,sizeof amount,0};b.ch('x');b.number(1+r->ascension/10);if(r->ascension%10){b.ch('.');b.number(r->ascension%10);}game_over_score_line(body,"SCORE_LINE.ascension",I8_GameOverScoreAscension,y,amount,row++);}
 }else body.paragraph_fitted(a.content.string(PartialHistoryText),28,y,264,52,10,Muted,10,true);
 int focus=a.pointer.visible?-1:a.game_over_focus,anchor_x=150,anchor_y=BodyTop;bool focus_visible=false;
 const auto detail_hit=[&](int x,int py,int w,int h,int id){
  const int top=mx(0,py),bottom=mn(BodyHeight,py+h);if(bottom<=top)return;
  c.hit(x,BodyTop+top,w,bottom-top,HitKind::GameOverDetail,id);
  if(focus==id){anchor_x=x;anchor_y=BodyTop+top;focus_visible=true;body.box(x,py,w,h,Gold);}
 };
 for(unsigned i=0;i<badge_count;++i){const unsigned id=unsigned(items[i]),tier=game_over_badge_tier(a,*r,id);
  const unsigned inrow=mn(BadgesPerRow,int(badge_count-i/BadgesPerRow*BadgesPerRow));
  const int x=(320-int(inrow)*(BadgeSize+4))/2+int(i%BadgesPerRow)*(BadgeSize+4);
  const auto state=motion.badge(i,now);if(!state.visible||!state.alpha)continue;
  const int py=badge_top+int(i/BadgesPerRow)*(BadgeSize+4)-scroll+state.offset;
  Canvas badge=body;badge.opacity=uint8_t(div255(badge.opacity*state.alpha));badge.brightness=game_over_hovered(c,x,BodyTop+py,BadgeSize,BadgeSize)?255:225;
  badge.ui(I8_GameOverBadgeBronze+int(tier)-1,x,py,BadgeSize,BadgeSize);badge.ui(I8_GameOverBadgeCombo+int(id),x,py,BadgeSize,BadgeSize);
  detail_hit(x,py,BadgeSize,BadgeSize,int(id));
 }
 if(discoveries){y=discovery_top-scroll;Canvas header=body;header.opacity=uint8_t(div255(header.opacity*motion.discovery_header_alpha(now)));
  header.text_fitted(game_over_text(a,"DISCOVERY_HEADER"),28,y,264,10,White,10,true);y+=18;
  for(unsigned i=0;i<discoveries;++i){const unsigned kind=unsigned(items[badge_count+i]-32);const int x=(320-int(discoveries)*59)/2+int(i)*59;
   const auto state=motion.discovery(i,now);if(!state.visible||!state.alpha)continue;const int py=y+state.offset;Canvas item=body;item.opacity=uint8_t(div255(item.opacity*state.alpha));
   item.ui(I8_GameOverDiscoveryCard+int(kind),x,py,20,20);item.num(a.progress.current_discoveries.count[kind],x+24,py+5,10,White);
   detail_hit(x,py,53,22,int(kind+32));
  }
 }
 if(motion.scroll_maximum){c.ui(I6_ScrollbarTrack,302,BodyTop,6,BodyHeight);const int thumb=mx(10,BodyHeight*BodyHeight/full_height);
  native_scrollbar_grabber(c,299,BodyTop+(BodyHeight-thumb)*motion.scroll/motion.scroll_maximum,12,thumb);}
 const char*view_label=a.content.string(ViewRunText);if(!*view_label)view_label=a.content.language()!=2?"View Run":"查看本局";
 if(ViewRunEnabled)game_over_button(c,ViewRunX,UiAction::GameOverViewRun,view_label,InputCommand::Confirm,r&&r->outcome&&a.progress.count);
 game_over_button(c,MainMenuX,UiAction::GameOverMainMenu,game_over_text(a,"BUTTON.mainMenu","Main Menu"),InputCommand::Select);
 if(a.game_over_available()){
  if(a.pointer.visible&&!a.pointer.held){const auto*h=a.hit_at(a.pointer.x,a.pointer.y);if(h&&h->kind==HitKind::GameOverDetail){focus=h->value;anchor_x=h->x;anchor_y=h->y;focus_visible=true;}}
  if(focus>=0&&focus_visible)game_over_hover(c,focus,anchor_x,anchor_y);
 }
}
