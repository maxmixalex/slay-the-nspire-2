// Original scene textures and interaction flow, reflowed for 320x240.
static const char* frontend_text(const App&a,const char*table,const char*key,const char*fallback){const char*text=a.content.world_string(table,key);return *text?text:fallback;}
static const char* frontend_label(const App&a,FrontMenu item){
 static const char*keys[]={"SINGLE_PLAYER","CONTINUE","ABANDON_RUN","COMPENDIUM","SETTINGS","QUIT"};
 static const char*fallback[]={"单人游戏","继续游戏","放弃游戏","图鉴","设置","退出游戏"};
 return frontend_text(a,"main_menu_ui",keys[item],fallback[item]);
}
static void frontend_number(TextBuffer&b,const char*label,int value){
 b.text(label);RichReader reader(label);RichChar character{};uint32_t last=0;
 while(reader.next(character))last=character.codepoint;
 if(last&&last!=' '&&last!='\t'&&last!='\n')b.ch(' ');
 b.number(value);
}
// A clipped view of a full-width atlas band scales one authored sprite. The
// view shares decoded storage; no per-particle image request or bitmap exists.
static void title_particle(Canvas&c,Picture band,const title_scene::ParticleSample&p,int source_x,int source_width,unsigned mode){
 const int left=mx(0,p.x),right=mn(c.clip_w,p.x+p.width),top=mx(0,p.y),bottom=mn(c.clip_h,p.y+p.height);
 if(!p.opacity||left>=right||top>=bottom||!band.pixels)return;
 Canvas layer=c;layer.pixels+=top*c.pitch+left;
 if(layer.coverage)layer.coverage+=top*c.pitch+left;
 layer.clip_w=right-left;layer.clip_h=bottom-top;
 layer.tint_red=uint8_t(div255(unsigned(c.tint_red)*p.red));
 layer.tint_green=uint8_t(div255(unsigned(c.tint_green)*p.green));
 layer.tint_blue=uint8_t(div255(unsigned(c.tint_blue)*p.blue));
 layer.picture(band,p.x-left-source_x*p.width/source_width,p.y-top,
     int(band.w)*p.width/source_width,p.height,p.opacity,mode);
}
static void title_background(Canvas&c,bool particles=true){
 auto&a=c.a;
 if(!particles||a.content.version()<TitleEffectsContentVersion){c.ui(U_TitleBackground,0,0);return;}
 c.ui(U_TitleBackdrop,0,0);
 if(particles){
  const auto atlas=a.content.image(a.content.ui_image(U_TitleParticles));
  if(atlas.w==title_scene::ParticleAtlasWidth&&atlas.h==title_scene::ParticleAtlasHeight){
   const auto dot=Canvas::picture_rows(atlas,title_scene::DotAtlasY,title_scene::DotSize);
   for(unsigned i=0;i<title_scene::StarCount;++i)
    title_particle(c,dot,title_scene::star_sample(i,a.map_clock_ms),title_scene::DotAtlasX,title_scene::DotSize,presentation::Normal);
   Picture fog[4];for(unsigned row=0;row<4;++row)fog[row]=Canvas::picture_rows(atlas,int(row)*title_scene::FogFrameHeight,title_scene::FogFrameHeight);
   for(unsigned layer=0;layer<2;++layer)for(unsigned i=0;i<title_scene::FogCount;++i){
    const auto particle=title_scene::fog_sample(layer,i,a.map_clock_ms);
    title_particle(c,fog[particle.frame/2],particle,int(particle.frame%2)*title_scene::FogFrameWidth,
        title_scene::FogFrameWidth,presentation::Add);
   }
  }
 }
 c.ui(U_TitleForeground,0,0);
}
static void title_logo(Canvas&c){
 const auto logo=c.a.content.image(A_Logo);
 if(logo.w!=title_scene::AtlasWidth||(logo.h!=title_scene::LegacyAtlasHeight&&logo.h!=title_scene::AtlasHeight)){c.picture(A_Logo,3,5);return;}
 if(logo.h==title_scene::AtlasHeight){
  const auto glow=title_scene::glow_sample(c.a.map_clock_ms);
  Canvas light=c;light.brightness=uint8_t(div255(unsigned(light.brightness)*glow.brightness));
  light.tint_red=64;light.tint_green=205;light.tint_blue=215;
  light.picture(Canvas::picture_rows(logo,title_scene::GlowAtlasY,title_scene::GlowHeight),
      title_scene::GlowCenterX-glow.width/2,title_scene::GlowCenterY-glow.height/2,
      glow.width,glow.height,title_scene::GlowOpacity,presentation::Add);
 }
 c.picture_region(logo,title_scene::TextX,title_scene::TextY,0,0,title_scene::TextWidth,title_scene::TextHeight);
 const unsigned frame=c.a.map_clock_ms/title_scene::FireFrameMs%title_scene::FireFrames;
 c.picture_region(logo,title_scene::FireX,title_scene::FireY,int(frame)*title_scene::FireWidth,title_scene::FireAtlasY,
     title_scene::FireWidth,title_scene::FireHeight);
}
#include "profiles_ui.inc"
static void title_screen(Canvas&c){
    auto&a=c.a;title_background(c);title_logo(c);
    profile_title_button(c);
    auxiliary_patch_button(c);
    const int count=a.title_menu_count();
    for(int i=0;i<count;i++){
        const int y=title_scene::menu_top(count)+i*title_scene::MenuRowHeight;
        const bool selected=a.menu==i;const char*label=frontend_label(a,front_item(a,i));
        const auto fit=c.fitted(label,title_scene::MenuCenter-title_scene::MenuLabelWidth/2,y,
            title_scene::MenuLabelWidth,19,16,12,selected?Gold:White,true,true,true);
        if(selected){
            const int width=mn(title_scene::MenuLabelWidth,c.width(label,fit.size));
            const int left=title_scene::MenuCenter-width/2-22,right=title_scene::MenuCenter+(width+1)/2+2;
            const int mark=a.content.world_image("menu","HIGHLIGHT");
            if(mark>=0){const auto reticle=a.content.image(mark);
                c.picture_region(reticle,left,y-2,0,0,20,20);
                c.picture_region(reticle,right,y-2,0,0,20,20,true);
            }else{c.text("›",left+5,y,16,Gold);c.text("‹",right+5,y,16,Gold);}
        }
        c.hit(title_scene::MenuCenter-title_scene::MenuHitWidth/2,y-2,title_scene::MenuHitWidth,title_scene::MenuRowHeight,HitKind::Menu,i);
    }
    if(a.has_save&&front_item(a,a.menu)==FrontContinue){
        int panel=a.content.world_image("menu","CONTINUE_PANEL");if(panel>=0)c.picture(panel,207,137,110,81);else c.dim(207,137,110,81,160);
        int portrait=a.content.ui_image(U_TopBarIronclad+int(a.saved_character));
        if(portrait<0)portrait=a.content.ui_image(character_portrait(a.saved_character));
        c.picture(portrait,212,145,18,18);
        const auto&ch=character_definitions[unsigned(a.saved_character)];const char*name=a.content.world_title("characters",ch.key);
        c.text_fitted(*name?name:ch.name,234,147,78,12,White);
        char row[160]{};TextBuffer b{row,sizeof row,0,&a.content};
        if(a.saved_world){const char*act=a.content.world_title("acts",map_colors(a.saved_current_region).region);if(*act){b.text(act);b.text(" [blue]- ");}else b.text("[blue]");}
        else b.text("[blue]");
        frontend_number(b,frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.floor","楼层"),a.saved_floor);b.text("[/blue]");c.text_fitted(row,213,169,98,10,White);
        if(a.saved_ascension){row[0]=0;b.n=0;frontend_number(b,frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.ascension","进阶 "),a.saved_ascension);c.text_fitted(row,213,183,98,10,White);}
        row[0]=0;b.n=0;b.text("[red]");b.number(a.saved_hp);b.ch('/');b.number(a.saved_max_hp);b.text("[/red]");c.picture(A_Heart,212,201,10,11);c.text_fitted(row,224,201,41,10,White);
        row[0]=0;b.n=0;b.text("[gold]");b.number(a.saved_gold);b.text("[/gold]");c.picture(A_Coin,267,201,10,11);c.text_fitted(row,280,201,32,10,White);
    }
    c.text("nSpire II 0.17",235,229,8,White,83);
}
static void native_pause_screen(Canvas&c){
 auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,185);
 c.centered(frontend_text(a,"gameplay_ui","PAUSE_MENU.PAUSED","暂停"),0,16,320,20,Gold);
 const char*keys[]={"PAUSE_MENU.RESUME","PAUSE_MENU.SETTINGS","PAUSE_MENU.COMPENDIUM","PAUSE_MENU.GIVE_UP","PAUSE_MENU.SAVE_AND_QUIT"};
 const char*fallback[]={"继续游戏","设置","图鉴","放弃游戏","保存并返回主菜单"};
 for(int i=0;i<5;i++){const int y=54+i*32;const bool enabled=i!=3||resumable_run(a.game),selected=a.menu==i;
  c.button(frontend_text(a,"gameplay_ui",keys[i],fallback[i]),61,y,198,selected,28,enabled);c.hit(58,y-1,204,31,HitKind::Menu,i,enabled);
 }
}
static void frontend_settings_screen(Canvas&c){
 auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,205);
 const bool english=a.content.english(),native=a.content.version()>=FrontendContentVersion;
 // settings_screen.tscn is an unframed list behind a horizontal tab manager.
 // Only implemented settings have tabs; the calculator has no audio/video rows.
 const char*tab_labels[]={frontend_text(a,"settings_ui","TAB_GENERAL","General"),
                         frontend_text(a,"settings_ui","TAB_INPUT","Input")};
 for(int i=0;i<2;++i){
  const int x=48+i*112,y=13;const bool selected=a.settings_tab==i;
  const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+112&&a.pointer.y>=y&&a.pointer.y<y+39;
  // NSettingsTab has no press-size override: focus is 1.05, followed by
  // a 0.5s EXPO_OUT release. Reuse the bounded UI tween slots, not new storage.
  ButtonMotion*motion=nullptr;ButtonMotion*oldest=&a.button_motions[0];const uint32_t key=0x738b1000u+unsigned(i);
  for(auto&candidate:a.button_motions){if(candidate.identity==key){motion=&candidate;break;}if(!candidate.identity)oldest=&candidate;else if(oldest->identity&&uint32_t(a.map_clock_ms-candidate.seen)>uint32_t(a.map_clock_ms-oldest->seen))oldest=&candidate;}
  if(!motion)motion=oldest;const auto visual=motion->observe(key,ButtonFamily::Action,hover?ButtonState::Focus:ButtonState::Rest,a.map_clock_ms);
  const int w=(112*visual.scale+2048)/4096,h=(39*visual.scale+2048)/4096,tx=x+(112-w)/2,ty=y+(39-h)/2;
  if(native){
   Canvas tab=c;tab.brightness=uint8_t(div255(tab.brightness*229));tab.ui(F_SettingsTab,tx,ty,w,h);
   if(visual.hover)c.picture(a.content.ui_image(F_SettingsTab),tx,ty,w,h,unsigned(visual.hover)*77/4096,presentation::Add);
   if(selected)c.picture(a.content.ui_image(F_SettingsTabStroke),tx,ty,w,h,255,presentation::Add);
  }else if(selected)c.rect(x+10,y+33,92,1,Gold);
  Canvas label=c;label.opacity=uint8_t(div255(label.opacity*(selected||hover?255:128)));
  label.text_fitted(tab_labels[i],tx+7,ty+12,w-14,12,hover?Gold:White,10,true,true);
  c.action(x,y,112,39,i?UiAction::SettingsInputTab:UiAction::SettingsGeneralTab);
 }
 Canvas divider=c;divider.opacity=uint8_t(div255(divider.opacity*64));
 const auto label=[&](const char*key,const char*eng,const char*zhs){return frontend_text(a,"settings_ui",key,english?eng:zhs);};
 const auto checkbox=[&](bool value,int y){
  if(native)c.ui(value?F_SettingsTicked:F_SettingsUnticked,264,y+3,20,20);
  else{c.box(266,y+5,16,16,White);if(value)c.picture(a.content.world_image("common","CONFIRM_TICK"),264,y+3,20,20);}
 };
 int hover=-1;
 if(!a.settings_tab){
  const char*keys[]={a.title?"LANGUAGE":"LANGUAGE_IN_RUN","SCREENSHAKE","FASTMODE","PHOBIA_MODE","SHOW_HAND_CARD_COUNT_HEADER","TEXT_EFFECTS","SEND_FEEDBACK","CREDITS"};
  const char*eng[]={a.title?"Language":"Language (main menu only)","Screenshake","Fast Mode","Phobia Mode","Show Hand Card Count","Text Effects","Send Feedback","Credits"};
  const char*zhs[]={a.title?"语言选项":"语言选项（仅可在主菜单调整）","屏幕抖动","加速模式","恐惧症模式","显示手牌计数","文字特效","发送反馈","制作名单"};
  const UiAction actions[]={UiAction::SettingsLanguage,UiAction::SettingsShakeUp,UiAction::SettingsFastMode,UiAction::SettingsPhobia,UiAction::SettingsCardIndices,UiAction::SettingsTextEffects,UiAction::SettingsFeedback,UiAction::SettingsCredits};
  const bool checked[]={false,false,a.fast_mode,a.phobia_mode,a.show_card_indices,a.text_effects};
  if(a.settings_top>3)a.settings_top=3;
  for(unsigned i=a.settings_top;i<8&&i<a.settings_top+5;++i){
   const int y=60+int(i-a.settings_top)*28;
   const bool pointed=a.pointer.visible&&a.pointer.x>=25&&a.pointer.x<295&&a.pointer.y>=y&&a.pointer.y<y+27;
   if(pointed&&i<6)hover=int(i);
   const bool focus=pointed||(!a.pointer.visible&&a.settings_item==i),enabled=i!=0||a.title;
   Canvas row=c;if(!enabled)row.opacity=uint8_t(div255(row.opacity*115));
   row.text_fitted(label(keys[i],eng[i],zhs[i]),29,y+7,i==0?161:i==1?146:i>=6?174:222,11,focus?Gold:White,8);
   if(i==0){
    row.rect(196,y+3,93,22,rgb(44,67,79));
    row.text_fitted(english?"English":"中文",201,y+7,71,10,Gold,8,true);
    if(a.content.version()>=RoomUi7ContentVersion)row.ui(I7_SettingsArrowDown,276,y+10,6,6);
    c.action(194,y,99,27,actions[i],enabled&&a.available_languages()==3);
   }else if(i==1){
    static const char*levels[]={"SCREENSHAKE_NONE","SCREENSHAKE_SOME","SCREENSHAKE_NORMAL","SCREENSHAKE_LOTS","SCREENSHAKE_CAAAW"};
    static const char*levels_en[]={"None","Some","Normal","Lots","CAAAW!"};
    static const char*levels_zh[]={"无","轻微","正常","剧烈","咔咔！"};
    c.text_fitted(label(levels[a.screen_shake],levels_en[a.screen_shake],levels_zh[a.screen_shake]),198,y+7,72,10,Gold,8,true);
    if(a.content.version()>=RoomUi7ContentVersion){c.ui(I7_SettingsArrowLeft,185,y+4,9,17,a.screen_shake?255:70);c.ui(I7_SettingsArrowRight,279,y+4,9,17,a.screen_shake<4?255:70);}
    c.action(178,y,21,27,UiAction::SettingsShakeDown,a.screen_shake>0);c.action(272,y,22,27,UiAction::SettingsShakeUp,a.screen_shake<4);
   }else if(i<6){checkbox(checked[i],y);c.action(252,y,42,27,actions[i]);}
   else{
    c.button(i==6?label("SEND_FEEDBACK_BUTTON_LABEL","Open","打开"):label("CREDITS_BUTTON_LABEL","View","查看"),209,y+2,80,focus,23);
    c.action(205,y,90,27,actions[i]);
   }
   divider.rect(25,y+27,270,1,rgb(232,220,190));
  }
  c.rect(303,61,2,139,rgb(18,33,41));c.rect(301,61+int(a.settings_top)*52/3,6,87,rgb(80,115,132));
  c.action(296,57,17,72,UiAction::SettingsScrollUp,a.settings_top>0);c.action(296,129,17,74,UiAction::SettingsScrollDown,a.settings_top<3);
 }else{
  const bool focus=!a.pointer.visible&&a.settings_item==0;
  c.text_fitted(label("KEYBOARD_ONLY_MODE_HEADER","Keyboard-Only Mode","纯键盘模式"),29,67,218,11,focus?Gold:White);
  checkbox(a.keyboard_only,60);c.action(252,60,42,27,UiAction::SettingsKeyboardOnly);
  divider.rect(25,87,270,1,rgb(232,220,190));
  if(a.pointer.visible&&a.pointer.x>=25&&a.pointer.x<295&&a.pointer.y>=60&&a.pointer.y<87)hover=6;
  const unsigned bank=a.keyboard_only&&!a.pointer.visible?1:0;
  c.text_fitted(label("INPUT_SETTINGS.COMMAND_HEADER","Command","指令"),29,92,139,10,Muted,8);
  c.paragraph(label("INPUT_SETTINGS.MOUSE_KEYBOARD_HEADER","Mouse + Keyboard","鼠标与键盘"),174,91,59,2,0,8,bank==0?Gold:Muted);
  c.paragraph(label("INPUT_SETTINGS.KEYBOARD_ONLY_MODE_HEADER","Keyboard-Only","纯键盘"),239,91,56,2,0,8,bank==1?Gold:Muted);
  for(unsigned i=a.settings_top;i<InputCommandCount&&i<a.settings_top+5;++i){const int y=113+int(i-a.settings_top)*18;
   const bool enabled=input_remappable(bank,i),pointed=a.pointer.visible&&a.pointer.x>=25&&a.pointer.x<296&&a.pointer.y>=y&&a.pointer.y<y+18;
   const bool selected=pointed||(!a.pointer.visible&&a.settings_item==i+1);Canvas row=c;if(!enabled)row.opacity=uint8_t(div255(row.opacity*120));
   if(selected&&enabled)row.rect(25,y,271,17,rgb(31,51,62));
   row.text_fitted(a.input_command_title(i),29,y+3,139,10,selected?Gold:White,8);
   for(unsigned b=0;b<2;++b){char binding[16]{};input_key_label(a.input_bindings.keys[b][i],binding,sizeof binding);
    row.text_fitted(binding,174+int(b)*65,y+3,56,10,selected&&b==bank?Gold:White,8,true);}
   divider.rect(25,y+17,270,1,rgb(232,220,190));c.hit(25,y,271,18,HitKind::InputBinding,int(i),enabled);
  }
  c.rect(303,113,2,90,rgb(18,33,41));c.rect(301,113+int(a.settings_top)*72/int(InputCommandCount-5),6,18,rgb(80,115,132));
  c.action(296,108,17,45,UiAction::SettingsScrollUp,a.settings_top>0);c.action(296,153,17,50,UiAction::SettingsScrollDown,a.settings_top<InputCommandCount-5);
 }
 const unsigned reset_item=a.settings_tab?InputCommandCount+1:8,back_item=reset_item+1;
 native_back(c,7,211,84,28,"返回",true,!a.pointer.visible&&a.settings_item==back_item);c.action(3,207,94,33,UiAction::SettingsBack);
 if(a.settings_tab){c.button(label("INPUT_SETTINGS.RESET_TO_DEFAULT","Reset to Default","恢复默认"),210,211,100,!a.pointer.visible&&a.settings_item==reset_item,25);c.action(206,207,109,33,UiAction::SettingsReset);}
 else{c.button(label("RESET_SETTINGS_BUTTON","Reset","重置"),241,211,69,!a.pointer.visible&&a.settings_item==reset_item,25);c.action(237,207,78,33,UiAction::SettingsReset);}
 if(hover>=1&&!a.settings_language_open&&!a.frontend_confirm){
  const char*descriptions[]={"","SCREENSHAKE_DESCRIPTION","FASTMODE_DESCRIPTION","PHOBIA_MODE_DESCRIPTION","SHOW_HAND_CARD_COUNT_DESCRIPTION","TEXT_EFFECTS_DESCRIPTION","KEYBOARD_ONLY_MODE_DESCRIPTION"};
  const char*body=a.content.world_string("settings_ui",descriptions[hover]);
  if(*body){const int width=154,lines=c.paragraph(body,0,0,width-16,0,0,10),height=mn(112,12+lines*13);
   const int y=hover<3?102:55;
   c.nine_ui(U_HoverTip,137,y,width,height,7,7,7,7);c.paragraph(body,145,y+6,width-16,(height-12)/13,0,10,White);
  }
 }
 if(a.settings_language_open){
  a.hit_count=0;c.action(0,0,320,240,UiAction::SettingsLanguageClose);
  const int top=86;
  c.rect(196,top,93,46,rgb(18,33,41));
  for(unsigned i=0;i<2;++i){const int y=top+2+int(i)*21;
   const bool selected=a.pointer.visible?a.pointer.x>=198&&a.pointer.x<287&&a.pointer.y>=y&&a.pointer.y<y+21:a.settings_language_item==i;
   if(selected)c.rect(198,y,89,21,rgb(44,88,112));
   c.text_fitted(i?"中文":"English",201,y+5,83,10,White,8,true);
   c.action(198,y,89,21,i?UiAction::SettingsLanguageChinese:UiAction::SettingsLanguageEnglish);
  }
 }
 if(a.binding_listening>=0){
  a.hit_count=0;c.dim(0,0,320,240,200);
  c.text_fitted(a.input_command_title(unsigned(a.binding_listening)),30,87,260,12,Gold,10,true);
  c.text_fitted(label("LISTENING_INPUT","Listening for input...","等待输入……"),30,113,260,12,White,10,true);
  native_back(c,7,211,84,28,"返回",true);c.action(3,207,94,33,UiAction::SettingsBack);
 }
}
static void frontend_confirm_screen(Canvas&c){
 auto&a=c.a;const bool quitting=a.frontend_confirm==3,reset=a.frontend_confirm==4,erase_profile=a.frontend_confirm==5;a.hit_count=0;c.dim(0,0,320,240,210);
 const char*header=frontend_text(a,reset?"settings_ui":"main_menu_ui",reset?"RESET_CONFIRMATION.header":quitting?"QUIT_CONFIRM_POPUP.header":"ABANDON_RUN_CONFIRMATION.header",reset?"重置设置":quitting?"退出游戏":"你确定吗？");
 const char*body=frontend_text(a,reset?"settings_ui":"main_menu_ui",reset?"RESET_GAMEPLAY_CONFIRMATION.body":quitting?"QUIT_CONFIRM_POPUP.body":"ABANDON_RUN_CONFIRMATION.body",reset?"恢复默认设置？":quitting?"你确定要退出吗？":"放弃游戏会被视为本局失败。");
 char profile_header[128]{},profile_body[1024]{};
 if(erase_profile){profile_text(a,"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.title",a.content.english()?"Delete Save Profile {Id}?":"是否删除{Id}号存档？",{a.profile_target},profile_header,sizeof profile_header);
  profile_text(a,"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.description",a.content.english()?"All data in Save Profile {Id} will be deleted. This cannot be undone.":"{Id}号存档内的所有数据将被删除。这一操作无法逆转。",{a.profile_target},profile_body,sizeof profile_body);header=profile_header;body=profile_body;}
 if(a.content.version()>=Ui6ContentVersion){
  // NGenericPopup instantiates the 522x600 vertical popup and independent
  // cancel/confirm skins, not the navigation BackButton/ConfirmButton.
  c.ui(I6_PopupVertical,73,20,174,200);
  c.text_fitted(header,80,39,160,12,Gold,10,true);
  const int lines=c.paragraph(body,90,61,140,0,0,10,White);
  c.paragraph(body,90,61+mx(0,(101-lines*12)/2),140,8,0,10,White);
  for(int i=0;i<2;++i){const int x=73+i*114,y=169;const bool selected=!a.pointer.visible&&int(a.frontend_choice)==i;
   const char*label=frontend_text(a,"main_menu_ui",erase_profile?(i?"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.delete":"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.cancel"):(i?"GENERIC_POPUP.confirm":"GENERIC_POPUP.cancel"),i?"确认":"取消");
   const auto visual=c.control_visual(label,x,y,60,24,ButtonFamily::Action,selected,true);
   const int w=60*visual.scale/4096,h=24*visual.scale/4096,left=x+(60-w)/2,top=y+(24-h)/2;
   Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
   if(visual.hover)layer.picture(a.content.ui_image(i?I6_PopupConfirmOutline:I6_PopupCancelOutline),left,top,w,h,unsigned(visual.hover)*128/4096,presentation::Add);
   layer.ui(i?I6_PopupConfirm:I6_PopupCancel,left,top,w,h);
   const int label_x=left+(i?10:5),label_w=w-15;
   layer.text_fitted(label,label_x+1,top+(h-10)/2,label_w,10,i?rgb(31,71,0):rgb(89,18,0),8,true);
   layer.text_fitted(label,label_x,top+(h-10)/2-1,label_w,10,White,8,true);
   c.action(x-3,y-4,66,32,i?UiAction::FrontendAccept:UiAction::FrontendCancel);
  }
 }else{
  c.ui(U_RewardPanel,29,43,262,165);c.centered(header,41,63,238,16,Gold);c.paragraph(body,51,98,218,3,0,12,White);
  c.button(frontend_text(a,"main_menu_ui","GENERIC_POPUP.cancel","取消"),39,161,109,!a.frontend_choice,31);
  c.button(frontend_text(a,"main_menu_ui","GENERIC_POPUP.confirm","确认"),172,161,109,a.frontend_choice,31);
  c.action(36,151,115,46,UiAction::FrontendCancel);c.action(169,151,115,46,UiAction::FrontendAccept);
 }
}
#include "character_select_ui7.inc"
#include "card_library_ui.inc"
#include "collection_ui.inc"

static void item_detail_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,190);c.ui(U_RewardPanel,43,26,234,206);
    bool relic=a.detail_item<WorldRelicKinds;int index=relic?a.detail_item:a.detail_item-WorldRelicKinds;
    if(index<0||index>=(relic?WorldRelicKinds:PotionKinds)){a.detail_item=-1;return;}
    if(!a.library&&!a.history_open){if(relic)a.discover_relic(RelicId(index));else a.discover_potion(Potion(index+1));}
    const char*name=relic?a.relic_title(RelicId(index),a.detail_owned):potion_name(index+1,&a.content);
    char description[4096]{};if(relic)a.relic_description(RelicId(index),description,sizeof description,a.detail_owned);
    if(!relic)ui_potion_description(a,index+1,description,sizeof description);
    const char*desc=description;
    int image=relic?a.relic_face_image(index):a.content.potion_image((Potion)(index+1));
    c.picture(image,140,40,40,46);c.centered(name,58,91,204,16,Gold);
    bool owned=relic&&a.detail_owned>=0&&a.detail_owned<int(ui_owned_relic_count(a.game))&&ui_owned_relic_id(a.game,a.detail_owned)==index;
    char progress[128]{};if(owned)ui_relic_progress(a.game,a.detail_owned,progress,sizeof progress,&a.content);int top=*progress?136:119;
    if(*progress)c.centered(progress,57,116,206,10,Green);
    int visible=top==136?4:5;
    int lines=c.paragraph(desc,67,top,188,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-visible));
    c.paragraph(desc,67,top,188,visible,a.detail_scroll,12,White);
    if(lines>visible){c.text("↑",260,top,10,Gold);c.hit(251,top-2,26,20,HitKind::Scroll,-1,a.detail_scroll>0);
        c.text("↓",260,178,10,Gold);c.hit(251,175,26,21,HitKind::Scroll,1,a.detail_scroll+visible<lines);}
#if SPIRE_DEBUG_COMMANDS
    const bool lab=!relic&&a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat;
#else
    const bool lab=false;
#endif
    if(lab){
        bool room=false;for(unsigned i=0;i<potion_slot_count(a.game);i++)room|=potion_at(a.game,i)==Potion::Empty;
        c.button("返回",57,201,72,false,25);c.action(53,198,80,32,UiAction::Back);
        c.button(room?"获得药水":"药水栏已满",139,201,127,room,25);
        c.action(135,198,135,32,UiAction::LabPotion,room);
    }else{
        c.button("返回",112,201,96,true,25);c.action(107,198,106,32,UiAction::Back);
    }
}
