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
static void title_screen(Canvas&c){
    auto&a=c.a;title_background(c);title_logo(c);
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
 const int back_item=a.settings_tab?1:a.title?3:1;
 if(!a.settings_tab){
  const bool focus=a.settings_item==0||a.pointer.visible&&a.pointer.x>=24&&a.pointer.x<296&&a.pointer.y>=61&&a.pointer.y<98;
  c.text_fitted(frontend_text(a,"settings_ui","FASTMODE",english?"Fast Mode":"加速模式"),29,68,210,12,focus?Gold:White);
  if(native)c.ui(a.fast_mode?F_SettingsTicked:F_SettingsUnticked,261,65,20,20);
  else{c.box(263,67,16,16,White);if(a.fast_mode)c.picture(a.content.world_image("common","CONFIRM_TICK"),261,65,20,20);}
  c.action(238,59,61,36,UiAction::SettingsFastMode);
  divider.rect(25,99,270,1,rgb(232,220,190));
  if(focus)c.paragraph_fitted(frontend_text(a,"settings_ui","FASTMODE_DESCRIPTION",english?"Combat actions and screen transitions happen more quickly.":"战斗动画和转场加快。"),29,108,262,44,10,White,10);
  if(a.title){
   const char*labels[]={"组合练习","参考地图"};
   for(int i=0;i<2;++i){const int y=160+i*27;const bool selected=a.settings_item==i+1;
    c.text_fitted(labels[i],29,y,220,10,selected?Gold:White);c.text("›",276,y-2,16,selected?Gold:Muted);
    divider.rect(25,y+21,270,1,rgb(232,220,190));
    c.action(24,y-4,272,26,i?UiAction::SettingsReference:UiAction::SettingsPractice);
   }
  }
 }else{
  char map_potions[128]{};TextBuffer labels_text{map_potions,sizeof map_potions,0,&a.content};
  labels_text.text(frontend_text(a,"static_hover_tips","MAP.title","Map"));labels_text.text(" / ");
  labels_text.text(frontend_text(a,"static_hover_tips","POTION_SLOT.title","Potions"));
  const char*labels[]={"方向键 / 4、6、8、2：导航",frontend_text(a,"settings_ui","INPUT_SETTINGS.INPUT_TITLE.confirm",english?"Confirm":"确认"),
      frontend_text(a,"settings_ui","INPUT_SETTINGS.INPUT_TITLE.endTurn",english?"End Turn":"结束回合"),"牌组",map_potions,"详情"};
  const char*keys[]={"","Enter / 5","Space","Tab","M / P","I"};
  for(int i=0;i<6;++i){const int y=62+i*21;c.text_fitted(labels[i],29,y,i?164:262,10,White);c.text_fitted(keys[i],200,y,91,10,White,8,true);divider.rect(25,y+17,270,1,rgb(232,220,190));}
  c.text_fitted("操作说明",191,192,104,10,a.settings_item==0?Gold:White,8,true);c.action(188,186,112,23,UiAction::SettingsControls);
 }
 native_back(c,7,211,84,28,"返回",true,a.settings_item==back_item);c.action(3,207,94,33,UiAction::SettingsBack);
}
static void frontend_confirm_screen(Canvas&c){
 auto&a=c.a;const bool quitting=a.frontend_confirm==3;a.hit_count=0;c.dim(0,0,320,240,210);c.ui(U_RewardPanel,29,43,262,165);
 c.centered(frontend_text(a,"main_menu_ui",quitting?"QUIT_CONFIRM_POPUP.header":"ABANDON_RUN_CONFIRMATION.header",quitting?"退出游戏":"你确定吗？"),41,63,238,16,Gold);
 c.paragraph(frontend_text(a,"main_menu_ui",quitting?"QUIT_CONFIRM_POPUP.body":"ABANDON_RUN_CONFIRMATION.body",quitting?"你确定要退出吗？":"放弃游戏会被视为本局失败。"),51,98,218,3,0,12,White);
 native_back(c,39,161,109,31,frontend_text(a,"main_menu_ui","GENERIC_POPUP.cancel","取消"),true,!a.frontend_choice);
 native_confirm(c,172,161,109,31,frontend_text(a,"main_menu_ui","GENERIC_POPUP.confirm","确认"),true,a.frontend_choice);
 c.action(36,151,115,46,UiAction::FrontendCancel);c.action(169,151,115,46,UiAction::FrontendAccept);
}
static void character_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;
    int hover=a.pointer.visible?-2:-1;
    if(a.pointer.visible&&!a.run_settings&&a.pointer.y>=159&&a.pointer.y<207)
        for(int i=0;i<CharacterKinds;++i)if(a.pointer.x>=157+i*31&&a.pointer.x<188+i*31)hover=i;
    auto&motion=a.character_select_presentation;motion.observe(true,unsigned(a.selected_character),hover,a.map_clock_ms);
    const int panel_x=motion.panel_offset(a.map_clock_ms);
    const auto& definition=character_definitions[(unsigned)a.selected_character];
    c.ui(character_background(a.selected_character),0,0);
    int information=a.content.world_image("character","INFO_PANEL");if(information>=0)c.picture(information,panel_x,0,155,209);else c.dim(panel_x,0,150,208,140);
    const char*title=a.content.world_title("characters",definition.key);c.text_fitted(*title?title:definition.name,panel_x+13,15,134,20,Gold,12);
    c.picture(A_Heart,panel_x+14,49,13,14);char hp[32]={};TextBuffer h{hp,sizeof hp,0,&a.content};h.number(definition.starting_hp);h.ch('/');h.number(definition.starting_hp);c.text(hp,panel_x+31,48,10,White);
    // All five pinned single-player models begin with 99 gold.
    c.picture(A_Coin,panel_x+101,50,12,12);c.num(99,panel_x+116,48,10,Gold);
    char description_key[96]{},description[1024]{};TextBuffer key{description_key,sizeof description_key,0,&a.content};key.text(definition.key);key.text(".description");a.localized_text("characters",description_key,description,sizeof description);
    c.paragraph_fitted(*description?description:character_summary(a.selected_character),panel_x+14,73,132,65,10,White);
    if(const auto*relic=relic_definition(definition.starting_relic)){
        RelicId id=RelicId(relic-relic_definitions);char text[1024]{};a.relic_description(id,text,sizeof text);
        c.picture(a.relic_face_image(id),panel_x+14,141,25,25);
        c.text_fitted(a.relic_title(id),panel_x+46,145,99,12,Gold);
        c.paragraph_fitted(text,panel_x+14,172,132,34,10,White);
    }
    native_back(c,2,212,84,28,"返回");c.action(1,209,86,31,UiAction::Back);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectDefect,UiAction::SelectRegent,UiAction::SelectNecrobinder};
    for(int i=0;i<CharacterKinds;i++){
        int x=159+i*31;bool selected=i==(int)a.selected_character;
        int outline=a.content.world_image("character","OUTLINE");
        const int scale=int(motion.portrait_scale(unsigned(i),a.map_clock_ms));
        const int pw=(36*scale+512)/1024,ph=(40*scale+512)/1024,ow=(33*scale+512)/1024,oh=(47*scale+512)/1024;
        if(selected&&outline<0){c.rect(x+(27-ow)/2,184-oh/2,ow,oh,rgb(87,68,38));c.box(x+(27-ow)/2,184-oh/2,ow,oh,Gold);}
        if(selected&&outline>=0)c.picture(outline,x+(27-ow)/2,184-oh/2,ow,oh,motion.outline_alpha(a.map_clock_ms),presentation::Add);
        c.ui(character_portrait((Character)i),x+(27-pw)/2,184-ph/2,pw,ph,selected||hover==i?255:145);
        c.action(x-2,159,31,48,actions[i]);
    }
    if(a.practice_entry)native_proceed(c,215,212,103,28,"练习");else native_confirm(c,215,212,103,28,"");c.action(213,207,107,33,UiAction::StartCharacter);
    if(!a.practice_entry){c.button("远征设置",99,216,108,false,23);c.action(96,209,114,31,UiAction::RunSettings);}
    if(a.run_settings){a.hit_count=0;c.dim(0,0,320,240,218);c.ui(U_RewardPanel,17,17,286,213);c.centered("远征设置",0,26,320,20,Gold);
        c.text("进阶等级",39,69,12,White);c.button("−",174,63,36,false,26);c.action(172,62,40,29,UiAction::RunAscensionDown,a.selected_ascension>0);c.num(a.selected_ascension,222,67,16,Gold);c.button("+",252,63,36,false,26);c.action(250,62,40,29,UiAction::RunAscensionUp,a.selected_ascension<10);
        c.text("第一幕",39,105,12,White);c.button(a.content.world_title("acts",a.selected_region==reference::ActKind::Overgrowth?"OVERGROWTH":"UNDERDOCKS"),156,98,132,false,26);c.action(154,97,136,29,UiAction::RunRegion);
        c.text("种子",39,141,12,White);c.text_fitted(*a.selected_seed?a.selected_seed:"自动生成",87,141,196,10,Gold);c.action(84,134,202,31,UiAction::RunSeed);
        c.button("编辑种子",38,170,113,false,24);c.action(36,168,117,28,UiAction::RunSeed);c.button("清除",171,170,113,false,24);c.action(169,168,117,28,UiAction::RunSeedClear,*a.selected_seed);
        c.button("返回",100,202,120,true,24);c.action(98,200,124,28,UiAction::RunSettings);
    }
}
static void library_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;title_background(c,false);c.dim(0,0,320,240,225);
    c.centered("图鉴",0,1,320,16,Gold);c.close_button(294,3);
    const char* tabs[]={"卡牌","遗物","药水"};
    const int totals[]={(int)a.library_items(),WorldRelicKinds,PotionKinds};
    for(int i=0;i<3;i++){
        int x=5+i*104;char label[32]={};TextBuffer b{label,sizeof label,0,&a.content};b.text(tabs[i]);b.ch(' ');b.number(i==0?CardKinds:totals[i]);
        c.ui(U_LibraryCards+i,x+6,24,18,18);c.text(label,x+29,25,10,i==a.library_tab?Gold:Muted);
        if(i==a.library_tab)c.rect(x+6,43,91,1,Gold);
        c.action(x,22,104,23,(UiAction)((int)UiAction::LibraryCards+i));
    }
    if(a.library_tab==0){
        const char* filters[]={"全部","铁甲","猎手","故障","储君","亡灵","无色"};
        const int filter_ids[]={0,1,2,4,5,6,3};
        const UiAction filter_actions[]={UiAction::FilterAll,UiAction::FilterIronclad,UiAction::FilterSilent,UiAction::FilterDefect,UiAction::FilterRegent,UiAction::FilterNecrobinder,UiAction::FilterColorless};
        int filter_hover=-1;
        for(int i=0;i<7;i++){int x=4+i*33;bool selected=a.library_filter==filter_ids[i];if(selected)c.rect(x,46,32,18,rgb(67,54,38));
            const bool hovered=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+33&&a.pointer.y>=45&&a.pointer.y<64;
            if(hovered)filter_hover=i;
            // card_library.tscn uses the same top-panel portraits as the HUD.
            // Icons leave the full original localized name to its hover tip.
            const int image=i?a.content.ui_image(i==6?U_ColorlessEnergy:U_TopBarIronclad+i-1):-1;
            if(image>=0){const int size=hovered?20:18;c.picture(image,x+(32-size)/2,46+(18-size)/2,size,size,selected||hovered?255:150);}
            else c.text_fitted(filters[i],x+1,48,30,10,selected?Gold:Muted,8,true);
            c.action(x,45,33,19,filter_actions[i]);}
        const char* sorts[]={"默认排序","按费用","按类型"};c.centered(sorts[a.library_sort],238,46,79,10,Gold);c.action(237,45,83,19,UiAction::CycleSort);
        CardKind ids[CardKinds];const int count=(int)a.library_items(ids),last=mx(0,(count-1)/8);
        a.library_page=mn(mx(0,a.library_page),last);a.library_selected=mn(mx(0,a.library_selected),count-1);
        // Only the visible page participates in bounded adjacent image reads.
        int portraits[8];unsigned portrait_count=0;
        for(int i=a.library_page*8;i<mn(count,a.library_page*8+8);++i)
            portraits[portrait_count++]=a.card_face_image({ids[i],uint8_t(a.library_upgraded&&card_upgradeable({ids[i],0}))});
        a.content.prepare_images(portraits,portrait_count);
        for(int i=a.library_page*8;i<mn(count,a.library_page*8+8);i++){
            int k=i-a.library_page*8,x=29+(k%4)*69,y=68+(k/4)*71;
            small_card(c,{ids[i],(uint8_t)(a.library_upgraded&&card_upgradeable({ids[i],0}))},x,y,i==a.library_selected,true,0);
            c.hit(x-3,y-2,55,70,HitKind::LibraryItem,i);
        }
        c.action(108,211,116,29,UiAction::ToggleUpgrades);c.centered(a.library_upgraded?"升级版本 +":"基础版本",108,215,116,12,Green);
        c.text("‹",12,214,16,Gold);c.action(1,211,43,29,UiAction::PreviousPage,a.library_page>0);
        c.text("›",296,214,16,Gold);c.action(277,211,43,29,UiAction::NextPage,a.library_page<last);
        char page[32]={};TextBuffer b{page,sizeof page,0,&a.content};b.number(a.library_page+1);b.text(" / ");b.number(last+1);c.text(page,51,218,10,Muted);
        if(filter_hover>=0){
            const char*name=filter_hover>0&&filter_hover<6?a.content.world_title("characters",character_definitions[filter_hover-1].key):a.content.ui_text(filters[filter_hover]);
            if(!*name)name=a.content.ui_text(filters[filter_hover]);
            const int width=mn(184,mx(92,c.width(name,10)+18)),x=mn(318-width,mx(2,20+filter_hover*33-width/2));
            const int skin=a.content.world_image("common","HOVER_TIP");
            if(skin>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.nine_image(skin,x+2,70,width,28,28,22,46,16,64);c.nine_image(skin,x,68,width,28,28,22,46,16);}
            else{c.rect(x,68,width,28,Panel);c.box(x,68,width,28,Muted);}
            c.text_fitted(name,x+8,75,width-16,10,Gold,8,true);
        }
    }else{
        const int count=totals[a.library_tab];
        const int last=(count-1)/18;a.library_page=mn(mx(0,a.library_page),last);int first=a.library_page*18;
        for(int i=first;i<mn(count,first+18);i++){
            int j=i-first;int x=22+(j%6)*49,y=53+(j/6)*35;bool selected=i==a.library_selected;
            if(selected){c.rect(x-4,y-2,38,34,rgb(75,58,37));c.box(x-4,y-2,38,34,Gold);}
            int image=a.library_tab==1?a.relic_face_image(i):a.content.potion_image((Potion)(i+1));
            c.picture(image,x+2,y,25,29);c.hit(x-5,y-3,47,35,HitKind::LibraryItem,i);
        }
        int index=mn(count-1,mx(0,a.library_selected));const char*name;const char*desc;char description[2048]{};
        if(a.library_tab==1){name=a.relic_title(RelicId(index));a.relic_description(RelicId(index),description,sizeof description);desc=description;}
        else{name=potion_name(index+1,&a.content);ui_potion_description(a,index+1,description,sizeof description);desc=description;}
        c.centered(name,10,163,300,16,Gold);c.paragraph(desc,20,189,282,2,0,12,White);
        c.text("‹",12,215,16,Gold);c.action(0,211,44,29,UiAction::PreviousPage,a.library_page>0);
        c.text("›",296,215,16,Gold);c.action(277,211,43,29,UiAction::NextPage,a.library_page<last);
        char page[64]={};TextBuffer b{page,sizeof page,0,&a.content};b.number(a.library_page+1);b.text(" / ");b.number(last+1);b.text(" · 点击查看完整详情");c.centered(page,40,222,236,10,Muted);
    }
}

static void item_detail_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,190);c.ui(U_RewardPanel,43,26,234,206);
    bool relic=a.detail_item<WorldRelicKinds;int index=relic?a.detail_item:a.detail_item-WorldRelicKinds;
    if(index<0||index>=(relic?WorldRelicKinds:PotionKinds)){a.detail_item=-1;return;}
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
    const bool lab=!relic&&a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat;
    if(lab){
        bool room=false;for(unsigned i=0;i<potion_slot_count(a.game);i++)room|=potion_at(a.game,i)==Potion::Empty;
        c.button("返回",57,201,72,false,25);c.action(53,198,80,32,UiAction::Back);
        c.button(room?"获得药水":"药水栏已满",139,201,127,room,25);
        c.action(135,198,135,32,UiAction::LabPotion,room);
    }else{
        c.button("返回",112,201,96,true,25);c.action(107,198,106,32,UiAction::Back);
    }
}
