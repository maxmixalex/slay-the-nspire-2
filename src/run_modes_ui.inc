static const char* modifier_localized(const App&a,RunModifier modifier,bool description){
    return a.run_modifier_text(modifier,description);
}
struct RunModeTextValues {const char*title="",*description="",*color="gold",*time="";unsigned ascension=0;};
static LocalizedValue run_mode_text_value(const void*context,const char*key){
    const auto&v=*static_cast<const RunModeTextValues*>(context);
    const auto same=[](const char*a,const char*b){while(*a&&*b)if(*a++!=*b++)return false;return *a==*b;};
    if(same(key,"title")||same(key,"modifier_title"))return {true,0,1,v.title};
    if(same(key,"description")||same(key,"modifier_description"))return {true,0,1,v.description};
    if(same(key,"color"))return {true,0,1,v.color};if(same(key,"time"))return {true,0,1,v.time};
    if(same(key,"ascension"))return {true,v.ascension};return {};
}
static void run_mode_text(const App&a,const char*key,const RunModeTextValues&values,char*out,size_t capacity){
    LocalizedTemplate format(out,capacity,run_mode_text_value,&values,"Energy","Stars",a.content.english());
    format.for_language(a.content.language()).format(a.content.world_string("main_menu_ui",key));
}
static void modifier_text(const App&a,RunModifier modifier,char*out,size_t capacity,bool daily=false){
    RunModeTextValues values;values.title=modifier_localized(a,modifier,false);values.description=modifier_localized(a,modifier,true);
    values.color=modifier<RunModifier::DeadlyEvents?"green":"red";
    run_mode_text(a,daily?"DAILY_RUN_MENU.MODIFIER":"CUSTOM_RUN_SCREEN.MODIFIER_LABEL",values,out,capacity);
    if(!*out){TextBuffer b{out,capacity,0};b.text(values.title);b.text(": ");b.text(values.description);}
}
// Fixed source font bands, line leading and vertical centering for setup
// RichTextLabels. No resident layout or image buffer is added.
static fitted_text::Plan setup_text_plan(Canvas c,const char*text,int width,int size=10,int tracking=0){
 c.font_face=0;c.bold_face=5;size=c.a.content.font_size(size,0,false,true);
 const auto metric=[&](const RichChar&ch,int px){auto metric=c.text_metric(ch,px);
  const int extra=c.a.content.english()?reference_viewport::project(int(ch.index+1)*tracking)-reference_viewport::project(int(ch.index)*tracking):0;
  metric.advance64=fitted_text::advance64(metric)+extra*64;metric.advance+=extra;metric.width+=extra;return metric;};
 const int rows=fitted_text::line_count(text,width,size,metric);
 // FontVariation native heights at24px: Kreon30.5, Noto36. The modifier
 // scenes subtract2/4 source pixels from their baseline separation.
 return {size,mn(rows,32),rows,size+2,false};
}
static void setup_rich_text(Canvas c,const char*text,int x,int y,int width,int height,bool center=false,int tracking=0,const ProfileTransform&transform={}){
 c.font_face=0;c.bold_face=5;const auto plan=setup_text_plan(c,text,width,10,tracking);
 const auto metric=[&](const RichChar&ch,int px){auto metric=c.text_metric(ch,px);
  const int extra=c.a.content.english()?reference_viewport::project(int(ch.index+1)*tracking)-reference_viewport::project(int(ch.index)*tracking):0;
  metric.advance64=fitted_text::advance64(metric)+extra*64;metric.advance+=extra;metric.width+=extra;return metric;};
 int widths[32]{};fitted_text::visit(text,width,plan,metric,[&](const RichChar&ch,int at,int row,int px){widths[row]=at+metric(ch,px).advance;});
 const int top=y+(height-(plan.rows*plan.step+1))/2;
 const uint16_t colors[]={rgb(255,246,226),rgb(239,200,81),rgb(255,85,85),rgb(135,206,235),rgb(127,255,0),rgb(238,130,238),rgb(255,165,24),rgb(0,255,255),rgb(255,255,255),rgb(128,128,128)};
 for(unsigned shadow=0;shadow<2;++shadow){Canvas ink=c;if(!shadow)ink.opacity=uint8_t(div255(unsigned(ink.opacity)*64));
  fitted_text::visit(text,width,plan,metric,[&](const RichChar&ch,int at,int row,int px){const auto glyph=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));
   profile_glyph(ink,glyph,x+at+(center?(width-widths[row])/2:0)+int(!shadow),top+row*plan.step,shadow?rich_color(ch.style.color,colors):0,transform);});}
}
static void setup_disclaimer(Canvas&c,const char*text){
 const unsigned face=source_label_face(c.a.content.original_label_fonts(),6);
 ProfileTextStyle style{8,face,rgb(255,127,127),64};
 profile_text_box(c,text,57,211,206,27,style,true,true);
}
static void singleplayer_screen(Canvas&c){
 using namespace run_setup_layout;auto&a=c.a;title_submenu_background(c,165);a.hit_count=0;
 const bool source=a.content.has_settings_ui10_assets();
 const int old_bases[]={I7_MenuStandardPanel,I7_MenuDailyPanel,I7_MenuCustomPanel},old_icons[]={I7_MenuStandardIcon,I7_MenuDailyIcon,I7_MenuCustomIcon};
 const int bases[]={I10_StandardPanel,I10_DailyPanel,I10_CustomPanel},icons[]={I10_StandardIcon,I10_DailyIcon,I10_CustomIcon};
 const char*keys[]={"STANDARD","DAILY","CUSTOM"};const UiAction actions[]={UiAction::OpenStandardRun,UiAction::OpenDailyRun,UiAction::OpenCustomRun};
 for(unsigned i=0;i<3;++i){const int x=menu_left(i);const bool focus=progress_hover(a,x,MenuTop,MenuWidth,MenuHeight)||(!a.pointer.visible&&a.compendium_selection==i);
  const auto visual=submenu_visual(a.compendium_motion[i],focus,a.map_clock_ms);
  // NSubmenuButton's pivot is(150,250), independent of its330x705 root.
  const ProfileTransform transform{x+MenuWidth/2,MenuTop+MenuHeight/2,int(visual.scale*4096/1000)};
  const int left=x-3,width=MenuWidth+6;
  if(a.content.version()>=Ui7ContentVersion){profile_image(c,source?bases[i]:old_bases[i],left,MenuTop,width,MenuHeight,transform);
   if(visual.focus)profile_image(c,(source?bases[i]:old_bases[i])+1,left,MenuTop,width,MenuHeight,transform,visual.focus*255/4096);}
  else c.dim(left,MenuTop,width,MenuHeight,150);
  profile_image(c,source?icons[i]:old_icons[i],x+9,MenuTop+43,74,56,transform);
  char key[80]{};TextBuffer k{key,sizeof key,0};k.text(keys[i]);k.text(".title");
  ProfileTextStyle title{10,source_label_face(a.content.original_label_fonts(),5),rgb(239,200,81),32,1,rgb(61,47,32)};title.shadow_y=1;
  profile_text_box(c,frontend_text(a,"main_menu_ui",key,keys[i]),x+4,MenuTop+12,84,28,title,true,true,transform);
  key[0]=0;k.n=0;k.text(keys[i]);k.text(".description");
  // This source paragraph has extra glyph tracking, unlike modifier prose.
  const char*description=frontend_text(a,"main_menu_ui",key,"");
  profile_text_box(c,description,x+8,MenuTop+102,76,70,{8,0,rgb(255,246,226),64},true,true,transform);
  c.action(x,MenuTop,MenuWidth,MenuHeight,actions[i]);
 }
 native_original_back(c,a.run_mode.started);
}
static void custom_run_screen(Canvas&c){
 using namespace run_setup_layout;auto&a=c.a;auto&mode=a.run_mode;title_submenu_background(c,185);a.hit_count=0;
 const bool editing=a.run_seed_editing&&a.map_preview.open,english=a.content.language()!=2;const unsigned bold=source_label_face(a.content.original_label_fonts(),5);const uint16_t cream=rgb(255,246,226),gold=rgb(239,200,81);
 const auto label=[&](const char*key,const char*fallback){return frontend_text(a,"main_menu_ui",key,fallback);};
 profile_text_box(c,label("CUSTOM_RUN_SCREEN.CUSTOM_MODE_TITLE","Custom Mode"),5,14,150,25,{12,source_label_face(a.content.original_label_fonts(),4),gold,128},true,true);
 ProfileTextStyle heading{10,bold,gold,64};heading.shadow_y=1;
 profile_text_box(c,label("CUSTOM_RUN_SCREEN.MODIFIERS_TITLE","Modifiers"),164,14,80,20,heading,false,true);
 const int random_x=randomize_left(english),random_y=randomize_top();const bool random_focus=progress_hover(a,random_x,random_y,RandomizeWidth,RandomizeHeight)||(!a.pointer.visible&&mode.focus==8);
 small_material_icon(c,a.content.ui_image(a.content.has_settings_ui10_assets()?int(I10_CustomRandomize):int(U_RewardButton)),random_x,random_y,RandomizeWidth,RandomizeHeight,random_focus?1100:1000,random_focus?1100:1000);
 ProfileTextStyle random_style{8,0,cream,16,1,rgb(34,81,87)};random_style.shadow_y=1;
 profile_text_box(c,label("CUSTOM_RUN_SCREEN.RANDOMIZE","Randomize"),random_x+2,random_y,RandomizeWidth-4,RandomizeHeight,random_style,true,false);
 if(!editing)c.action(random_x,random_y,RandomizeWidth,RandomizeHeight,UiAction::RandomizeRun);
 const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectDefect};
 unsigned selected=0;int hover=-2;for(unsigned i=0;i<CharacterKinds;++i){if(RunModeCharacters[i]==a.selected_character)selected=i;
  if(!editing&&progress_hover(a,portrait_left(i),CustomPortraitTop,CustomPortraitWidth,CustomPortraitHeight))hover=int(i);}
 if(!a.pointer.visible&&mode.focus>=0&&mode.focus<int(CharacterKinds))hover=mode.focus;
 auto&portraits=a.character_select_presentation;portraits.observe(true,selected,hover,a.map_clock_ms);
 for(unsigned i=0;i<CharacterKinds;++i){const int center=portrait_center(i),cy=CustomPortraitTop+CustomPortraitHeight/2,scale=portraits.portrait_scale(i,a.map_clock_ms);
  // The retained portrait has transparent margins around its132:195 image.
  const int w=(28*scale+512)/1024,h=(33*scale+512)/1024,ow=(26*scale+512)/1024,oh=(35*scale+512)/1024;
  const int portrait=a.content.ui_image(character_portrait(RunModeCharacters[i]));Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.picture(portrait,center-w/2+2,cy-h/2+2,w,h,64);
  if(i==selected){const int outline=a.content.world_image("character","OUTLINE");if(outline>=0)c.picture(outline,center-ow/2,cy-oh/2,ow,oh,portraits.outline_alpha(a.map_clock_ms),presentation::Add);}
  small_material_icon(c,portrait,center-w/2,cy-h/2,w,h,portraits.saturation(i,a.map_clock_ms),portraits.value(i,a.map_clock_ms));
  if(!editing)c.action(portrait_left(i),CustomPortraitTop,CustomPortraitWidth,CustomPortraitHeight,actions[i]);
 }
 // AscensionPanel's six children impose844px minimum width on its764px
 // HBox. Native container layout places the description at(229,791).
 Canvas panel=c;panel.tint_red=panel.tint_green=panel.tint_blue=0;panel.nine_ui(I7_TinyPanel,17,126,128,42,2,2,2,2,128);
 if(a.selected_ascension)c.ui(I7_SettingsArrowLeft,3,137,14,24);if(a.selected_ascension<10)c.ui(I7_SettingsArrowRight,145,137,14,24);
 c.ui(U_TopBarAscension,20,140,13,13);char digits[8]{};TextBuffer number{digits,sizeof digits,0};number.number(a.selected_ascension);
 ProfileTextStyle number_style{10,bold,cream,64,1,rgb(89,52,0)};number_style.shadow_x=number_style.shadow_y=1;
 profile_text_box(c,digits,19,136,15,25,number_style,true,false);
 char level_key[40]{},text[1024]{};TextBuffer level{level_key,sizeof level_key,0};level.text("LEVEL_");level.ch(char('0'+a.selected_ascension/10));level.ch(char('0'+a.selected_ascension%10));level.text(".title");
 TextBuffer info{text,sizeof text,0};info.text("[gold][b]");info.text(frontend_text(a,"ascension",level_key,english?"Ascension":"进阶等级"));info.text("[/b][/gold]\n");level_key[8]=0;level.n=8;level.text(".description");info.text(a.content.world_string("ascension",level_key));
 Canvas description=c;description.clip_w=145;description.clip_h=168;setup_rich_text(description,text,36,129,105,36,false);
 if(!editing){if(a.selected_ascension)c.action(2,126,16,42,UiAction::RunAscensionDown);if(a.selected_ascension<10)c.action(144,126,16,42,UiAction::RunAscensionUp);}
 const int seed_left=seed_input_left(english),seed_width=SeedRight-seed_left,seed_height=SeedBottom-SeedTop;
 profile_text_box(c,label("CUSTOM_RUN_SCREEN.SEED_LABEL","Seed:"),9,SeedTop,40,seed_height,{8,bold,gold,128},false,true);
 c.dim(seed_left,SeedTop,seed_width,seed_height,140);c.rect(seed_left,SeedBottom-1,seed_width,1,editing?gold:Muted);
 const char*seed=editing?a.map_preview.draft:a.selected_seed;const bool has_seed=*seed;Canvas seed_font=c;seed_font.sparse_scene_sizes=true;
 while(*seed&&seed_font.width(seed,8)>seed_width-4)++seed;
 profile_text_box(c,has_seed?seed:label("CUSTOM_RUN_SCREEN.SEED_RANDOM_PLACEHOLDER","Random"),seed_left+2,SeedTop,seed_width-4,seed_height,{8,0,has_seed?cream:rgb(128,128,128),0},false,false,{},true);
 if(editing)c.rect(seed_left+2+mn(seed_width-4,seed_font.width(seed,8)),SeedTop+5,1,10,cream);
 else c.action(seed_left,SeedTop,seed_width,seed_height,UiAction::RunSeed);
 constexpr int left=ModifiersLeft,top=ModifiersTop,width=ModifiersWidth,view_height=ModifiersHeight;
 if(mode.layout_generation!=a.content.generation()){unsigned y=0;for(unsigned i=0;i<RunModifierKinds;++i){mode.row_offsets[i]=uint16_t(y);modifier_text(a,RunModifier(i),text,sizeof text);
   const auto plan=setup_text_plan(c,text,ModifierTextWidth);y+=unsigned(mx(project(56),plan.rows*plan.step+1)+project(8));}
  mode.row_offsets[RunModifierKinds]=uint16_t(y);mode.layout_generation=a.content.generation();}
 auto&scroller=a.progress_page_scroll;scroller.configure(mode.row_offsets[RunModifierKinds],view_height);
 if(!a.pointer.visible&&mode.focus>=9&&mode.focus<int(9+RunModifierKinds)&&mode.focus!=mode.last_focus){const unsigned i=unsigned(mode.focus-9);const int from=mode.row_offsets[i],to=mode.row_offsets[i+1],visible=scroller.offset();
  if(from<visible)scroller.target=from*256;else if(to>visible+view_height)scroller.target=(to-view_height)*256;}
 mode.last_focus=mode.focus;Canvas list=c;list.pixels+=top*list.pitch;if(list.coverage)list.coverage+=top*list.pitch;list.clip_h=view_height;
 if(!editing)c.hit(left,top,width,view_height,HitKind::ProgressScroll);
 for(unsigned i=0;i<RunModifierKinds;++i){const int y=int(mode.row_offsets[i])-scroller.offset(),height=int(mode.row_offsets[i+1]-mode.row_offsets[i])-project(8);if(y+height<=0||y>=view_height)continue;
  const int hit_top=mx(top,top+y),hit_bottom=mn(top+view_height,top+y+height);const bool hover=progress_hover(a,left,hit_top,ModifierContentWidth,hit_bottom-hit_top),focus=!a.pointer.visible&&mode.focus==int(i+9);
  if(focus){Canvas highlight=list;highlight.opacity=64;highlight.rect(left,y,ModifierContentWidth,height,rgb(255,163,114));}
  const bool ticked=(mode.pending.modifiers&(uint32_t(1)<<i))!=0,pressed=a.pointer.held&&a.pointer.captured.kind==HitKind::RunModifier&&a.pointer.captured.value==int(i);
  setup_tickbox(list,0x738b6000u+i,ticked,left+7,y+9,12,hover||focus,pressed);
  modifier_text(a,RunModifier(i),text,sizeof text);setup_rich_text(list,text,left+18,y,ModifierTextWidth,height);
  if(!editing)c.hit(left,hit_top,ModifierContentWidth,hit_bottom-hit_top,HitKind::RunModifier,int(i));
 }
 setup_disclaimer(c,label("CUSTOM_RUN_SCREEN.disclaimer",""));
 if(!editing){if(scroller.maximum){c.ui(I6_ScrollbarTrack,311,45,6,153);c.ui(I6_ScrollbarEdge,311,37,6,8);ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),314,202,6,8,0,-1024);
   c.ui(I6_ScrollbarGrabber,308,scroller.thumb_y(45,153)-7,12,14);c.hit(307,37,13,170,HitKind::ProgressThumb);}
  native_original_back(c,mode.started);native_original_confirm(c,UiAction::StartConfiguredRun,mode.started);
 }else{a.hit_count=0;c.hit(seed_left,SeedTop,seed_width,seed_height,HitKind::None);}
}
static void daily_run_screen(Canvas&c){
 auto&a=c.a;a.refresh_daily_run();const auto&mode=a.run_mode;title_submenu_background(c,185);a.hit_count=0;
 const unsigned bold=source_label_face(a.content.original_label_fonts(),5);const uint16_t cream=rgb(255,246,226),gold=rgb(239,200,81);
 const auto label=[&](const char*key,const char*fallback){return frontend_text(a,"main_menu_ui",key,fallback);};
 profile_text_box(c,label("DAILY_RUN_MENU.DAILY_TITLE","Daily Challenge"),12,14,296,24,{14,source_label_face(a.content.original_label_fonts(),4),gold,128},true,false);
 const uint32_t now=a.callbacks.wall_clock?a.callbacks.wall_clock():0;char date[64]{},time[32]{},text[1024]{};progress_date_text(a,now,date,sizeof date,time,sizeof time);
 profile_text_box(c,date,16,39,288,14,{10,bold,rgb(135,206,235),64},true,false);
 const unsigned remaining=now?86400-now%86400:0;TextBuffer clock{time,sizeof time,0};time[0]=0;const unsigned hours=remaining/3600,minutes=remaining/60%60,seconds=remaining%60;
 if(hours<10)clock.ch('0');clock.number(hours);clock.ch(':');if(minutes<10)clock.ch('0');clock.number(minutes);clock.ch(':');if(seconds<10)clock.ch('0');clock.number(seconds);
 RunModeTextValues values;values.time=time;values.ascension=a.selected_ascension;run_mode_text(a,"DAILY_RUN_MENU.TIME_LEFT",values,text,sizeof text);
 profile_text_box(c,text,16,55,288,14,{10,0,cream,64},true,false);
 c.ui(U_TopBarIronclad+int(a.selected_character),91,78,20,20);c.ui(U_TopBarAscension,107,84,14,14);char digits[8]{};TextBuffer num{digits,sizeof digits,0};num.number(a.selected_ascension);
 ProfileTextStyle number{8,0,cream,32,1,rgb(135,41,24)};number.shadow_y=1;
 profile_text_box(c,digits,109,87,10,12,number,true,false);
 const auto&character=character_definitions[unsigned(a.selected_character)];const char*name=a.content.world_title("characters",character.key);
 profile_text_box(c,*name?name:character.name,126,79,150,20,{10,bold,gold,64},false,false);
 profile_text_box(c,label("DAILY_RUN_MENU.MODIFIERS","Modifiers:"),20,104,280,18,{10,bold,gold,64},false,false);
 int row_y[4]{};const unsigned count=mn(3,run_modifier_count(mode.pending));
 for(unsigned i=0;i<count;++i){modifier_text(a,run_modifier_at(mode.pending,i),text,sizeof text,true);const auto plan=setup_text_plan(c,text,246);row_y[i+1]=row_y[i]+mx(22,plan.rows*plan.step+1)+5;}
 auto&scroller=a.progress_page_scroll;scroller.configure(row_y[count],84);Canvas modifiers=c;modifiers.pixels+=124*modifiers.pitch;if(modifiers.coverage)modifiers.coverage+=124*modifiers.pitch;modifiers.clip_h=84;
 c.hit(20,124,280,84,HitKind::ProgressScroll);
 for(unsigned i=0;i<count;++i){const RunModifier modifier=run_modifier_at(mode.pending,i);const int y=row_y[i]-scroller.offset(),height=row_y[i+1]-row_y[i]-5;if(y+height<=0||y>=84)continue;
  if(a.content.version()>=HistoryUi8ContentVersion)modifiers.ui(I8_ModifierDraft+int(run_modifier_icon(modifier)),20,y,19,19);
  modifier_text(a,modifier,text,sizeof text,true);setup_rich_text(modifiers,text,45,y,246,height);
 }
 // Online identities and empty leaderboards have no usable action on this
 // offline platform. The original daily seed, modifiers and scoring remain.
 setup_disclaimer(c,label("DAILY_RUN_MENU.disclaimer",""));native_original_back(c,mode.started);
 if(mode.pending.mode==RunMode::Daily)native_original_confirm(c,UiAction::StartConfiguredRun,mode.started);
}
static void run_modes_screen(Canvas&c){
 if(c.a.run_mode.screen==RunMenu::Singleplayer)singleplayer_screen(c);
 else if(c.a.run_mode.screen==RunMenu::Custom)custom_run_screen(c);
 else daily_run_screen(c);
}
