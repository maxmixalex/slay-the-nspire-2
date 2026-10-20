// Original game-over flow: first Continue reveals the run summary. Its UI
// state is deliberately separate from the completed, serializable run.
bool App::game_over_available()const{
 return game.world.active&&(game.phase==Phase::Complete||game.phase==Phase::Dead)&&
  !title&&!paused&&!help&&!panel&&!library&&!character_select&&!map_preview.open&&!frontend_confirm&&!settings_open&&
  status_actor<0&&detail_card<0&&detail_item<0&&deck_candidate<0;
}
bool App::game_over_input(Key key){
 if(!game_over_available()||key!=Key::Confirm)return false;
 ui_action(game_over_summary?UiAction::GameOverMainMenu:UiAction::GameOverSummary);return true;
}
static const char* game_over_text(const App&a,const char*key,const char*fallback){
 const char*text=a.content.world_string("game_over_screen",key);return *text?text:fallback;
}
static const char* game_over_character(const App&a){
 const auto&definition=character_definitions[unsigned(a.game.character)];const char*text=a.content.world_title("characters",definition.key);return *text?text:definition.name;
}
static const char* game_over_region(const App&a){
 const auto&s=a.game;const auto act=reference::ActKind(s.world.rooms[s.world.act_index].region);
 const char*key=act==reference::ActKind::Overgrowth?"OVERGROWTH":act==reference::ActKind::Underdocks?"UNDERDOCKS":act==reference::ActKind::Hive?"HIVE":"GLORY";
 const char*text=a.content.world_title("acts",key);return *text?text:key;
}
static void game_over_backdrop(Canvas&c){
 auto&a=c.a;const auto&s=a.game;
 // Event deaths and the Architect retain their exact last event background.
 // Combat deaths retain the original region/encounter background. The hero's
 // death animation is not in the old five-clip character directory yet.
 const bool event_room=(s.phase==Phase::Complete&&s.original_events.id==OriginalEventId::TheArchitect)||
  (s.original_events.active&&!s.original_events.combat_pending);
 const int event=event_room?
  a.content.world_image("events",original_event_key(s.original_events.id)):-1;
 if(event>=0)c.picture(event,0,22,320,218);
 else c.picture(a.combat_background(),0,22,320,218);
 c.dim(0,22,320,218,a.game_over_summary?220:175);
}
static void game_over_primary(Canvas&c){
 auto&a=c.a;const bool summary=a.game_over_summary;
 native_proceed(c,96,204,128,35,game_over_text(a,summary?"BUTTON.mainMenu":"BUTTON.continue",summary?"主菜单":"继续"));
 c.action(91,204,138,35,summary?UiAction::GameOverMainMenu:UiAction::GameOverSummary);
}
static void game_over_line(Canvas&c,const char*key,int y,const char*fallback){
 char text[512]{};c.a.localized_text("game_over_screen",key,text,sizeof text);
 c.paragraph(*text?text:fallback,22,y,220,2,0,12,White);
}
static void native_game_over_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;a.hit_count=0;game_over_backdrop(c);
 c.ui(U_RewardBanner,16,27,288,47);
 // This pinned original build uses falseWin for a completed Architect run.
 // Defeat's first canonical banner is stable across redraws; presentation
 // must never consume any of the run's gameplay RNG streams.
 c.centered(game_over_text(a,s.phase==Phase::Complete?"BANNER.falseWin":"BANNER.lose0",s.phase==Phase::Complete?"远征完成":"倒在了高塔下"),31,39,258,16,Gold);
 if(!a.game_over_summary){
  c.ui(character_portrait(s.character),142,83,36,53);c.centered(game_over_character(a),20,141,280,16,White);
  char progress[256]{};TextBuffer b{progress,sizeof progress,0};b.text(game_over_region(a));b.text(" · ");b.number(s.world.total_floor);b.text(" 层");
  c.centered(progress,17,165,286,12,Muted);game_over_primary(c);return;
 }
 c.text(game_over_character(a),22,78,16,Gold,217);
 c.text(game_over_region(a),22,100,12,White,220);
 game_over_line(c,"SCORE_LINE.floorsClimbed",119,"已攀登楼层");
 if(s.world.ascension)game_over_line(c,"SCORE_LINE.ascension",142,"进阶等级");
 const int merchant=a.content.world_image("gameover","RUN_SUMMARY_MERCHANT");if(merchant>=0)c.picture(merchant,246,91,60,60);
 const int y=165;c.picture(A_Heart,22,y+1,12,13);char hp[32]{};TextBuffer h{hp,sizeof hp,0};h.number(s.hp);h.ch('/');h.number(s.max_hp);c.text(hp,38,y,10,White,62);
 c.picture(A_Coin,102,y+2,11,11);c.num(s.gold,118,y,10,Gold);
 c.ui(U_TopDeck,181,y,14,15);c.num(s.deck_count,198,y,10,White);
 c.ui(U_LibraryRelics,248,y,14,15);c.num(ui_owned_relic_count(s),265,y,10,White);
 // Balance is shown beside a coin, never under SCORE_LINE.goldGained. That
 // cumulative statistic, score, badges, unlocks and elapsed time need history
 // that this save does not contain, so this summary does not fabricate them.
 char seed[96]{};TextBuffer b{seed,sizeof seed,0};b.text("种子 ");b.text(s.world.seed_text);
 c.paragraph(seed,17,183,286,2,0,8,Muted);
 c.text("牌组 Tab",9,217,10,White);c.hit(3,207,81,32,HitKind::Command,int(Key::Inspect));
 c.text("遗物 R",248,217,10,White);c.hit(237,207,80,32,HitKind::Command,int(Key::Relics));
 game_over_primary(c);
}
