static void native_camp_screen(Canvas&c){
 auto&a=c.a;auto&s=a.game;c.picture(A_CampBackground,0,22,320,218);c.clip(character_camp_animation(s.character),188,61,a.animation*33,false);
 if(s.character==Character::Necrobinder)c.clip(AnimOstyCamp,247,100,a.animation*33,false);
 c.picture(a.content.ui_image(U_CampFlames)+(int)((a.animation*33/125)%160),130,76,46,58);c.picture(A_CampFire,128,111,55,34);
 c.centered("休息处",0,39,320,20,Gold);const int count=world_rest_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));const int first=a.cursor/4*4;
 for(int i=first;i<mn(count,first+4);i++){const char*key=world_rest_option_key(s,i);bool enabled=world_rest_option_enabled(s,i);int x=5+(i-first)*79;
  if(a.cursor==i)c.ui(U_RestOutline,x-1,130,80,57);c.picture(a.content.world_image("rest",key),x+3,133,72,51,enabled?255:95);
  char title[96]{};TextBuffer b{title,sizeof title,0};b.text("OPTION_");b.text(key);b.text(".name");const char*name=a.content.world_string("rest_site_ui",title);
  c.centered(name,x,187,78,12,a.cursor==i?Gold:enabled?White:Muted);c.hit(x,129,78,73,HitKind::Choice,i,enabled);
 }
 const bool can_proceed=world_rest_can_proceed(s);
 char key[128]{},description[1024]{};TextBuffer b{key,sizeof key,0};
 if(count){
  const bool used=(s.world.rest_used&(uint32_t(1)<<a.cursor))!=0;
  const bool enabled=world_rest_option_enabled(s,a.cursor);
  const auto option=WorldRestOption(s.world.rest_options[a.cursor]);
  b.text("OPTION_");b.text(world_rest_option_key(s,a.cursor));
  b.text(!enabled&&!used&&(option==WorldRestOption::Smith||option==WorldRestOption::Cook)?".descriptionDisabled":".description");
  if(used){TextBuffer done{description,sizeof description,0};done.text("已完成此选项。");}
  else a.localized_text("rest_site_ui",key,description,sizeof description,int(option));
 }
 c.dim(0,204,320,36,200);c.paragraph(description,8,207,can_proceed?190:304,2,0,10,White);
 if(can_proceed){
  c.ui(U_RewardSkip,211,211,103,27);const char*text=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
  c.centered(*text?text:"前进",212,214,101,12,Gold);c.action(207,208,111,32,UiAction::LeaveCamp);
  c.text("空格",285,203,8,Muted);
 }
 if(count>4){c.text("‹",5,115,12,Gold);c.hit(0,111,31,24,HitKind::Command,(int)Key::Left);c.text("›",304,115,12,Gold);c.hit(289,111,31,24,HitKind::Command,(int)Key::Right);}
}
