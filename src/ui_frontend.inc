// Original scene textures and interaction flow, reflowed for 320x240.
static const char* frontend_text(const App&a,const char*table,const char*key,const char*fallback){const char*text=a.content.world_string(table,key);return *text?text:fallback;}
static const char* frontend_label(const App&a,FrontMenu item){
 static const char*keys[]={"SINGLE_PLAYER","CONTINUE","ABANDON_RUN","COMPENDIUM","SETTINGS","QUIT"};
 static const char*fallback[]={"单人游戏","继续游戏","放弃游戏","图鉴","设置","退出游戏"};
 return frontend_text(a,"main_menu_ui",keys[item],fallback[item]);
}
static void title_logo(Canvas&c){
 const auto logo=c.a.content.image(A_Logo);
 if(logo.w!=title_scene::AtlasWidth||logo.h!=title_scene::AtlasHeight){c.picture(A_Logo,3,5);return;}
 c.picture_region(logo,title_scene::TextX,title_scene::TextY,0,0,title_scene::TextWidth,title_scene::TextHeight);
 const unsigned frame=c.a.map_clock_ms/title_scene::FireFrameMs%title_scene::FireFrames;
 c.picture_region(logo,title_scene::FireX,title_scene::FireY,int(frame)*title_scene::FireWidth,title_scene::FireAtlasY,
     title_scene::FireWidth,title_scene::FireHeight);
}
static void title_screen(Canvas&c){
    auto&a=c.a;c.ui(U_TitleBackground,0,0);title_logo(c);
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
        char row[96]{};TextBuffer b{row,sizeof row,0,&a.content};b.text(frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.floor","楼层"));b.ch(' ');b.number(a.saved_floor);c.text_fitted(row,213,169,98,10,White);
        row[0]=0;b.n=0;b.text(frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.ascension","进阶"));b.ch(' ');b.number(a.saved_ascension);c.text_fitted(row,213,183,98,10,White);
        row[0]=0;b.n=0;b.number(a.saved_hp);b.ch('/');b.number(a.saved_max_hp);c.picture(A_Heart,212,201,10,11);c.text_fitted(row,224,201,41,10,White);
        row[0]=0;b.n=0;b.number(a.saved_gold);c.picture(A_Coin,267,201,10,11);c.text_fitted(row,280,201,32,10,White);
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
 auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,220);c.ui(U_RewardPanel,20,8,280,226);
 c.centered(frontend_text(a,"main_menu_ui","SETTINGS","设置"),0,17,320,20,Gold);
 const int count=a.title?5:3;
 const char*labels[]={frontend_text(a,"settings_ui","FASTMODE","加速模式"),"操作说明","组合练习","参考地图","返回"};
 const UiAction actions[]={UiAction::SettingsFastMode,UiAction::SettingsControls,UiAction::SettingsPractice,UiAction::SettingsReference,UiAction::SettingsBack};
 for(int i=0;i<count;i++){int index=!a.title&&i==2?4:i,y=55+i*29;
  c.button(labels[index],54,y,212,a.settings_item==i,26);c.action(51,y-1,218,28,actions[index]);
  if(!index){c.box(67,y+7,11,11,Gold);if(a.fast_mode){int tick=a.content.world_image("common","CONFIRM_TICK");if(tick>=0)c.picture(tick,65,y+4,15,15);else c.text("+",68,y+3,12,Gold);}}
 }
 if(!a.title)c.paragraph(frontend_text(a,"settings_ui","FASTMODE_DESCRIPTION","战斗动画和转场加快。"),44,153,234,3,0,10,White);
 c.centered("本次运行生效 · Esc 返回",24,214,272,10,Muted);
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
    c.dim(0,209,320,31,130);
    native_back(c,2,212,84,28,"返回");c.action(1,209,86,31,UiAction::Back);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectDefect,UiAction::SelectRegent,UiAction::SelectNecrobinder};
    for(int i=0;i<CharacterKinds;i++){
        int x=159+i*31;bool selected=i==(int)a.selected_character;
        int outline=a.content.world_image("character","OUTLINE");
        const int scale=int(motion.portrait_scale(unsigned(i),a.map_clock_ms));
        const int pw=(36*scale+512)/1024,ph=(40*scale+512)/1024,ow=(33*scale+512)/1024,oh=(47*scale+512)/1024;
        if(selected&&outline<0){c.rect(x+(27-ow)/2,184-oh/2,ow,oh,rgb(87,68,38));c.box(x+(27-ow)/2,184-oh/2,ow,oh,Gold);}
        if(selected&&outline>=0)c.picture(outline,x+(27-ow)/2,184-oh/2,ow,oh,motion.outline_alpha(a.map_clock_ms));
        c.ui(character_portrait((Character)i),x+(27-pw)/2,184-ph/2,pw,ph,selected||hover==i?255:145);
        c.action(x-2,159,31,48,actions[i]);
    }
    native_proceed(c,215,212,103,28,a.practice_entry?"练习":"启程");c.action(213,207,107,33,UiAction::StartCharacter);
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
    auto&a=c.a;a.hit_count=0;c.ui(U_TitleBackground,0,0);c.dim(0,0,320,240,225);
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
