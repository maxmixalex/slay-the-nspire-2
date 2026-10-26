static void target_reticle_corner(Canvas&,const Picture&,int,int,int,bool,bool,unsigned,bool);
// The existing bounded motion slots also hold the small Settings controls.
// NTickbox uses50ms linear focus and500ms EXPO_OUT release/press. A tab's
// focus is immediate; it has no press animation.
static ButtonMotion& setup_control_motion(App&a,uint32_t key){
 ButtonMotion*motion=nullptr,*oldest=&a.button_motions[0];
 for(auto&candidate:a.button_motions){if(candidate.identity==key){motion=&candidate;break;}
  if(!candidate.identity)oldest=&candidate;else if(oldest->identity&&uint32_t(a.map_clock_ms-candidate.seen)>uint32_t(a.map_clock_ms-oldest->seen))oldest=&candidate;}
 if(!motion)motion=oldest;
 return *motion;
}
static ButtonVisual setup_control_visual(App&a,uint32_t key,bool focus,bool pressed,bool tab=false){
 auto*motion=&setup_control_motion(a,key);
 if(motion->identity!=key){*motion=ButtonMotion{};motion->identity=key;motion->start=a.map_clock_ms;}
 const auto sample=[&](){ButtonVisual to;const bool hover=motion->state==ButtonState::Focus,press=motion->state==ButtonState::Press;
  to.scale=hover?4301:press?3891:4096;to.value=hover?1200:press?800:1000;to.hover=hover?4096:0;
  const unsigned age=a.map_clock_ms-motion->start;
  const unsigned q=tab&&hover?4096:hover?(age>=50?4096:age*4096/50):health_bar::expo_out(age>=500?1000:age*2);
  to.scale=motion->from.scale+(to.scale-motion->from.scale)*int(q)/4096;
  to.value=uint16_t(int(motion->from.value)+(int(to.value)-int(motion->from.value))*int(q)/4096);
  to.hover=motion->from.hover+(to.hover-motion->from.hover)*int(q)/4096;return to;};
 const auto next=pressed&&!tab?ButtonState::Press:focus?ButtonState::Focus:ButtonState::Rest;
 if(motion->state!=next){motion->from=sample();motion->state=next;motion->start=a.map_clock_ms;}
 motion->seen=a.map_clock_ms;return sample();
}
static void settings_reticle(Canvas&c,int x,int y,int width,int height){
 const auto picture=c.a.content.image(c.a.content.world_image("map","SELECTION_RETICLE"));
 if(picture.pixels&&picture.format<=3){for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;
  target_reticle_corner(c,picture,right?x+width-4:x,bottom?y+height-4:y,4,right,bottom,255,false);}}
 else{c.box(x,y,width,height,rgb(239,200,81));}
}
static void setup_tickbox(Canvas&c,uint32_t key,bool value,int center_x,int center_y,int size,bool focus,bool pressed){
 const auto visual=setup_control_visual(c.a,key,focus,pressed);const int extent=mx(1,(size*visual.scale+2048)/4096);
 const int image=c.a.content.has_settings_ui10_assets()?(size==settings_layout::ModifierTickSize?(value?I10_ModifierTicked:I10_ModifierUnticked):(value?I10_SettingsTicked:I10_SettingsUnticked)):(value?F_SettingsTicked:F_SettingsUnticked);
 small_material_icon(c,c.a.content.ui_image(image),center_x-extent/2,center_y-extent/2,extent,extent,1000,visual.value);
}
static void frontend_settings_screen(Canvas&c){
 using namespace settings_layout;auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,205);
 const bool english=a.content.language()!=2,source_art=a.content.has_settings_ui10_assets();
 const unsigned bold=source_label_face(a.content.original_label_fonts(),5);const uint16_t cream=rgb(255,246,226),gold=rgb(239,200,81);
 const auto label=[&](const char*key,const char*eng,const char*zhs){return frontend_text(a,"settings_ui",key,english?eng:zhs);};
 bool dropdown_layer=false;
 const auto pointed=[&](int x,int y,int w,int h){return (!a.settings_language_open||dropdown_layer)&&a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h;};
 const auto pressed=[&](UiAction action){return a.pointer.held&&a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==int(action);};
 const auto text=[&](Canvas layer,const char*value,int x,int y,int w,int h,unsigned face=0,uint16_t color=0,bool center=false,int px=LabelFont){
  profile_text_box(layer,value,x,y,w,h,{px,face,color?color:cream,128},center,true,{},true,false,0,pointed(x,y,w,h)||!a.pointer.visible);};
 const char*tabs[]={label("TAB_GENERAL","General","常规"),label("TAB_INPUT","Input","输入")};
 for(unsigned i=0;i<2;++i){const int x=tab_left(i);const bool hover=pointed(x,TabTop,TabWidth,TabHeight),selected=a.settings_tab==i;
  const auto visual=setup_control_visual(a,0x738b1000u+i,hover,false,true);
  const ProfileTransform transform{x+TabWidth/2,TabTop+TabHeight/2,visual.scale};
  const int left=transform.horizontal(x),top=transform.vertical(TabTop),w=transform.horizontal(x+TabWidth)-left,h=transform.vertical(TabTop+TabHeight)-top;
  small_material_icon(c,a.content.ui_image(source_art?I10_SettingsTab:F_SettingsTab),left,top,w,h,1000,900+300*visual.hover/4096);
  if(selected){Canvas stroke=c;if(!source_art){stroke.tint_red=93;stroke.tint_green=232;stroke.tint_blue=245;stroke.opacity=192;}
   stroke.picture(a.content.ui_image(source_art?I10_SettingsTabStroke:F_SettingsTabStroke),left,top,w,h,255,presentation::Add);}
  Canvas ink=c;ink.opacity=uint8_t(div255(unsigned(ink.opacity)*(selected?255:128+unsigned(visual.hover)*127/4096)));
  ProfileTextStyle style{TabFont,bold,blend(cream,gold,visual.hover*255/4096),0,3,0,0,64};style.tracking_source=english?2:0;
  profile_text_box(ink,tabs[i],x,TabTop-1,TabWidth,TabHeight+2,style,true,false,transform);
  c.action(x,TabTop,TabWidth,TabHeight,i?UiAction::SettingsInputTab:UiAction::SettingsGeneralTab);
 }
 Canvas divider=c;divider.opacity=uint8_t(div255(unsigned(c.opacity)*64));int hover=-1,hover_y=0;
 const auto tick=[&](bool value,unsigned item,int x,int y,UiAction action){const bool over=pointed(x,y,ControlWidth,RowHeight),focus=over||(!a.pointer.visible&&a.settings_item==item);
  setup_tickbox(c,0x738b2000u+unsigned(action),value,x+ControlWidth/2,y+RowHeight/2,TickSize,focus,pressed(action));
  if(focus&&!a.pointer.visible)settings_reticle(c,x+(ControlWidth-12)/2,y+1,12,RowHeight-1);
  c.action(x,y,ControlWidth,RowHeight,action);};
 const auto button=[&](const char*value,unsigned item,int x,int y,int asset,UiAction action){const bool focus=pointed(x,y,ControlWidth,RowHeight)||(!a.pointer.visible&&a.settings_item==item);
  const auto v=setup_control_visual(a,0x738b4000u+unsigned(action),focus,pressed(action));
  const int w=(ControlWidth*v.scale+2048)/4096,h=(RowHeight*v.scale+2048)/4096,left=x+(ControlWidth-w)/2,top=y+(RowHeight-h)/2;
  small_material_icon(c,a.content.ui_image(source_art?asset:U_RewardButton),left,top,w,h,1000,v.value);
  ProfileTextStyle style{LabelFont,bold,rgb(232,220,190),64,2,asset==I10_SettingsFeedback?rgb(32,66,36):rgb(74,37,36)};
  style.shadow_y=1;style.tracking_source=english?2:0;
  profile_text_box(c,value,x+5,y,ControlWidth-10,RowHeight,style,true,false,{x+ControlWidth/2,y+RowHeight/2,v.scale});
  if(focus&&!a.pointer.visible)settings_reticle(c,x,y,ControlWidth,RowHeight);
  c.action(x,y,ControlWidth,RowHeight,action);};
 if(!a.settings_tab){
  settings_focus::blur_rows(a.button_motions,MaxButtonMotions,a.map_clock_ms);
  if(a.settings_top>GeneralMaximum)a.settings_top=GeneralMaximum;
  if(!a.pointer.visible){if(a.settings_item<a.settings_top)a.settings_top=a.settings_item;else if(a.settings_item>=a.settings_top+GeneralVisible)a.settings_top=uint8_t(a.settings_item-GeneralVisible+1);}
  if(a.settings_top>GeneralMaximum)a.settings_top=GeneralMaximum;
  const char*keys[]={a.title?"LANGUAGE":"LANGUAGE_IN_RUN","SCREENSHAKE","FASTMODE","PHOBIA_MODE","SHOW_RUN_TIMER_HEADER","SHOW_HAND_CARD_COUNT_HEADER","LONG_PRESS_CONFIRMATION_HEADER","TEXT_EFFECTS","SKIP_INTRO_LOGO_HEADER","CREDITS","INPUT_SETTINGS.RESET_TO_DEFAULT"};
  const char*eng[]={a.title?"Language":"Language (main menu only)","Screenshake","Fast Mode","Phobia Mode","Show Run Timer","Show Hand Card Count","Long Press Confirmations","Text Effects","Skip Intro Logo","Credits","Reset to Default"};
  const char*zhs[]={a.title?"语言选项":"语言选项（仅可在主菜单调整）","屏幕抖动","加速模式","恐惧症模式","显示计时器","显示手牌计数","长按确认","文字特效","跳过开场Logo动画","制作名单","恢复默认设置"};
  const UiAction actions[]={UiAction::SettingsLanguage,UiAction::SettingsShakeUp,UiAction::SettingsFastMode,UiAction::SettingsPhobia,UiAction::SettingsRunTimer,UiAction::SettingsCardIndices,UiAction::SettingsLongPress,UiAction::SettingsTextEffects,UiAction::SettingsSkipIntroLogo,UiAction::SettingsCredits,UiAction::SettingsReset};
  const bool checked[]={false,false,a.fast_mode,a.phobia_mode,a.show_run_timer,a.show_card_indices,a.long_press,a.text_effects,a.skip_intro_logo};
  for(unsigned i=a.settings_top;i<GeneralRows&&i<a.settings_top+GeneralVisible;++i){const int row_index=int(i-a.settings_top),y=general_row(unsigned(row_index));const bool enabled=i!=0||a.title;
   const bool focus=!a.pointer.visible&&a.settings_item==i;Canvas row=c;if(!enabled)row.opacity=uint8_t(div255(unsigned(row.opacity)*115));
   text(row,label(keys[i],eng[i],zhs[i]),LabelLeft,y,ControlLeft-LabelLeft-3,RowHeight);
   if(pointed(GeneralLeft,y,GeneralWidth,RowHeight)&&i<9){hover=int(i);hover_y=y;}
   if(i==0){row.rect(ControlLeft,y,ControlWidth,RowHeight,rgb(44,67,79));
    text(row,locale_language_info(a.content.language()).name,ControlLeft+1,y,ControlWidth-11,RowHeight,bold,gold,true);
    if(a.content.version()>=RoomUi7ContentVersion)row.ui(source_art?I10_SettingsArrowDown:I7_SettingsArrowDown,ControlLeft+ControlWidth-8,y+(RowHeight-5)/2,5,5);
    if(focus&&enabled)settings_reticle(c,ControlLeft,y,ControlWidth,RowHeight);
    c.action(ControlLeft,y,ControlWidth,RowHeight,actions[i],enabled&&locale_language_count(a.available_languages())>1);
   }else if(i==1){
    static const char*levels[]={"SCREENSHAKE_NONE","SCREENSHAKE_SOME","SCREENSHAKE_NORMAL","SCREENSHAKE_LOTS","SCREENSHAKE_CAAAW"};
    static const char*en[]={"None","Some","Normal","Lots","CAAAW!"},*zh[]={"无","轻微","正常","剧烈","咔咔！"};
    text(row,label(levels[a.screen_shake],en[a.screen_shake],zh[a.screen_shake]),ControlLeft+11,y,ControlWidth-22,RowHeight,bold,cream,true);
    if(a.content.version()>=RoomUi7ContentVersion){row.ui(source_art?I10_SettingsArrowLeft:I7_SettingsArrowLeft,ControlLeft+1,y+5,10,10,a.screen_shake?255:64);
     row.ui(source_art?I10_SettingsArrowRight:I7_SettingsArrowRight,ControlLeft+ControlWidth-11,y+5,10,10,a.screen_shake<4?255:64);}
    if(focus)settings_reticle(c,ControlLeft,y,ControlWidth,RowHeight);
    c.action(ControlLeft,y,12,RowHeight,UiAction::SettingsShakeDown,a.screen_shake>0);c.action(ControlLeft+ControlWidth-12,y,12,RowHeight,UiAction::SettingsShakeUp,a.screen_shake<4);
   }else if(i<9)tick(checked[i],i,ControlLeft,y,actions[i]);
   else button(i==9?label("CREDITS_BUTTON_LABEL","View","查看"):label("RESET_SETTINGS_BUTTON","Reset","重置"),i,ControlLeft,y,i==9?I10_SettingsCredits:I10_SettingsReset,actions[i]);
   if(i+1<GeneralRows)divider.rect(GeneralLeft,general_divider(unsigned(row_index)),GeneralWidth,1,rgb(232,220,190));
  }
  const int track_y=scroll_top(false),track_h=scroll_height(false);
  c.ui(I6_ScrollbarTrack,InputTrackLeft,track_y,8,track_h);
  c.ui(I6_ScrollbarEdge,InputTrackLeft,track_y-7,8,8);
  ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),InputTrackLeft+4,track_y+track_h+3,8,8,0,-1024);
  c.ui(I6_ScrollbarGrabber,InputTrackLeft-3,track_y+int(a.settings_top)*(track_h-14)/int(GeneralMaximum),14,14);
  c.hit(InputTrackLeft-5,track_y-7,18,track_h+14,HitKind::SettingsScroll);
 }else{
  if(a.settings_top>InputMaximum)a.settings_top=InputMaximum;
  text(c,label("KEYBOARD_ONLY_MODE_HEADER","Keyboard-Only Mode","纯键盘模式"),InputPanelLeft,KeyboardTop,InputControlLeft-InputPanelLeft-3,RowHeight);
  tick(a.keyboard_only,0,InputControlLeft,KeyboardTop,UiAction::SettingsKeyboardOnly);
  if(pointed(InputPanelLeft,KeyboardTop,InputWidth,RowHeight)){hover=9;hover_y=KeyboardTop;}
  divider.rect(InputPanelLeft,62,InputWidth,1,rgb(232,220,190));divider.rect(InputPanelLeft,87,InputWidth,1,rgb(232,220,190));
  button(label("RESET_SETTINGS_BUTTON","Reset","重置"),InputCommandCount+1,InputResetLeft,InputResetTop,I10_SettingsReset,UiAction::SettingsReset);
  ProfileTextStyle header{LabelFont,bold,gold,192,1,rgb(59,44,0)};header.tracking_source=english?1:0;
  profile_text_box(c,label("INPUT_SETTINGS.COMMAND_HEADER","Command","指令"),InputPanelLeft,InputHeaderTop,InputLabelWidth,InputHeaderHeight,header,false,false);
  const char*headers[]={label("INPUT_SETTINGS.MOUSE_KEYBOARD_HEADER","Mouse + Keyboard","鼠标与键盘"),label("INPUT_SETTINGS.KEYBOARD_ONLY_MODE_HEADER","Keyboard-Only","纯键盘")};
  for(unsigned b=0;b<2;++b)profile_text_box(c,headers[b],binding_left(b),InputHeaderTop,BindingWidth,InputHeaderHeight,header,true,true,{},true);
  const unsigned bank=a.keyboard_only&&!a.pointer.visible?1:0;
  int focused_command=-1;
  for(unsigned i=a.settings_top;i<InputCommandCount&&i<a.settings_top+InputVisible;++i){const unsigned visible=i-a.settings_top;
   const int y=input_row(visible),height=input_row(visible+1)-y;
   if(pointed(InputPanelLeft,y,InputWidth,height)||(!a.pointer.visible&&a.settings_item==i+1))focused_command=int(i);
  }
  // Commands retain identity when they scroll. An offscreen old focus starts
  // its release here, rather than waiting until that row becomes visible.
  settings_focus::blur_rows(a.button_motions,MaxButtonMotions,a.map_clock_ms,focused_command<0?0:settings_focus::identity(unsigned(focused_command)));
  for(unsigned i=a.settings_top;i<InputCommandCount&&i<a.settings_top+InputVisible;++i){const unsigned visible=i-a.settings_top;const int y=input_row(visible),height=input_row(visible+1)-y;
   const bool enabled=input_remappable(bank,i),focus=focused_command==int(i);
   const uint32_t key=settings_focus::identity(i);
   const unsigned alpha=settings_focus::observe(setup_control_motion(a,key),key,focus,a.map_clock_ms);
   if(alpha){Canvas highlight=c;highlight.opacity=uint8_t(div255(unsigned(c.opacity)*alpha));highlight.rect(InputPanelLeft,y,InputWidth,height,rgb(183,255,255));}
   ProfileTextStyle item{LabelFont,0,cream,64};item.tracking_source=english?1:0;
   const char*command=a.input_command_title(i);int command_width=0,characters=0;RichReader reader(command);RichChar ch{};
   while(reader.next(ch)){command_width+=a.content.glyph_metrics(ch.codepoint,LabelFont,0).advance;++characters;}
   command_width+=project(mx(0,characters-1)*item.tracking_source);
   ProfileTextStyle command_style=item;
   // Keep long input names readable by wrapping within the taller LCD row.
   if(command_width>InputLabelWidth)command_style.size=10;
   profile_text_box(c,command,InputLabelLeft,y,InputLabelWidth,height,command_style,false,true,{},true);
   for(unsigned b=0;b<2;++b){char binding[16]{};input_key_label(a.input_bindings.keys[b][i],binding,sizeof binding,a.calculator_keyboard);
    Canvas value=c;if(b!=bank){value.tint_red=191;value.tint_green=value.tint_blue=48;}
    if(!input_remappable(b,i))value.brightness=128;else if(!b&&i==unsigned(InputCommand::EndTurn))value.brightness=153;
    profile_text_box(value,binding,binding_left(b),y,BindingWidth,height,item,true,false);}
   c.hit(InputPanelLeft,y,InputWidth,height,HitKind::InputBinding,int(i),enabled);
  }
  const int top=scroll_top(true),height=scroll_height(true);
  c.ui(I6_ScrollbarTrack,InputTrackLeft,top,8,height);
  c.ui(I6_ScrollbarEdge,InputTrackLeft,top-7,8,8);
  ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),InputTrackLeft+4,top+height+3,8,8,0,-1024);
  c.ui(I6_ScrollbarGrabber,InputTrackLeft-3,top+int(a.settings_top)*(height-14)/int(InputMaximum),14,14);
  c.hit(InputTrackLeft-5,top-7,18,height+14,HitKind::SettingsScroll);
 }
 const bool back_pointer=a.pointer.visible;if(a.settings_language_open)a.pointer.visible=false;
 native_original_back(c);a.pointer.visible=back_pointer;
 if(hover>=1&&!a.settings_language_open&&!a.frontend_confirm){
  const char*titles[]={"","SCREENSHAKE_HEADER","FASTMODE","PHOBIA_MODE","SHOW_RUN_TIMER_HEADER","SHOW_HAND_CARD_COUNT_HEADER","LONG_PRESS_CONFIRMATION_HEADER","TEXT_EFFECTS","SKIP_INTRO_LOGO_HEADER","KEYBOARD_ONLY_MODE_HEADER"};
  const char*descriptions[]={"","SCREENSHAKE_DESCRIPTION","FASTMODE_DESCRIPTION","PHOBIA_MODE_DESCRIPTION","SHOW_RUN_TIMER_DESCRIPTION","SHOW_HAND_CARD_COUNT_DESCRIPTION","LONG_PRESS_CONFIRMATION_DESCRIPTION","TEXT_EFFECTS_DESCRIPTION","SKIP_INTRO_LOGO_DESCRIPTION","KEYBOARD_ONLY_MODE_DESCRIPTION"};
  const char*body=hover==8?label(descriptions[hover],"Go directly to the main menu when the game starts.","在游戏启动后直接进入主菜单。"):a.content.world_string("settings_ui",descriptions[hover]);
  if(*body)source_hover_box(c,hover==8?label(titles[hover],"Skip Intro Logo","跳过开场Logo动画"):a.content.world_string("settings_ui",titles[hover]),body,TooltipLeft,hover_y);
 }
 if(a.settings_language_open){
  dropdown_layer=true;
  const unsigned count=locale_language_count(a.available_languages()),visible=DropdownVisible;
  const unsigned maximum=language_maximum(count);if(a.settings_language_top>maximum)a.settings_language_top=uint8_t(maximum);
  const unsigned top=a.settings_language_top;
  constexpr int left=DropdownLeft,width=DropdownWidth,row_height=DropdownRowHeight,first_y=DropdownTop;
  const int item_width=maximum?width-24:width;
  a.hit_count=0;c.action(0,0,320,240,UiAction::SettingsLanguageClose);
  // SettingsDropdown and NLanguageDropdownItem author these ColorRects;
  // the scrollbar uses their separate small_* source textures below.
  c.rect(left,first_y,width,language_height(count),rgb(18,33,41));
  for(unsigned i=top;i<count&&i<top+visible;++i){const unsigned id=locale_language_at(a.available_languages(),i);
   const int y=first_y+int(i-top)*row_height;const bool focus=a.pointer.visible?pointed(left,y,item_width,row_height):a.settings_language_item==i;
   if(focus)c.rect(left,y,item_width,row_height,rgb(44,88,112));
   profile_text_box(c,locale_language_info(id).name,left+6,y,item_width-12,row_height,{10,0,cream,32},true,false);
   c.action(left,y,item_width,row_height,UiAction(unsigned(UiAction::SettingsLanguageFirst)+id-1));
  }
  if(maximum){
   const int track_x=language_track_left(),height=language_height(count);
   const bool original=a.content.ui_image(I13_SmallScrollbarTrain)>=0;
   Canvas track=c;if(original){track.tint_red=track.tint_green=track.tint_blue=0;track.opacity=uint8_t(div255(unsigned(track.opacity)*128));}
   track.ui(original?I13_SmallScrollbarTrackEdge:I6_ScrollbarEdge,track_x,first_y+1,8,8);
   track.ui(original?I13_SmallScrollbarTrackCenter:I6_ScrollbarTrack,track_x,first_y+9,8,height-18);
   ui_picture_transform(track,a.content.ui_image(original?I13_SmallScrollbarTrackEdge:I6_ScrollbarEdge),track_x+4,first_y+height-5,8,8,0,-1024);
   const int thumb_y=first_y+4+int(top)*language_track_span(count)/int(maximum);
   const bool focus=pointed(track_x-4,first_y,20,height)||(a.pointer.held&&a.pointer.captured.kind==HitKind::SettingsScroll&&a.pointer.captured.value==1);
   Canvas thumb=c;if(focus){thumb.tint_red=239;thumb.tint_green=200;thumb.tint_blue=81;}else thumb.opacity=uint8_t(div255(unsigned(thumb.opacity)*64));
   if(original)thumb.nine_ui(I13_SmallScrollbarTrain,track_x,thumb_y,8,DropdownThumbHeight,0,6,0,6);
   else thumb.ui(I6_ScrollbarGrabber,track_x-2,thumb_y,12,DropdownThumbHeight);
   c.hit(track_x-4,first_y,20,height,HitKind::SettingsScroll,1);
  }
 }
 if(a.binding_listening>=0){a.hit_count=0;
  Canvas backstop=c;backstop.opacity=242;backstop.rect(27,101,267,38,rgb(0,73,68));
  char prompt[160]{};TextBuffer b{prompt,sizeof prompt,0};b.text("[sine]");b.text(label("LISTENING_INPUT","Listening for input...","等待输入……"));b.text("[/sine]");
  Canvas ink=c;ink.sparse_scene_sizes=true;ink.centered(prompt,37,113,248,12,cream);
  native_original_back(c);
 }
}
