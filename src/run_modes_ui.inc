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
    format.format(a.content.world_string("main_menu_ui",key));
}
static void modifier_text(const App&a,RunModifier modifier,char*out,size_t capacity,bool daily=false){
    RunModeTextValues values;values.title=modifier_localized(a,modifier,false);values.description=modifier_localized(a,modifier,true);
    values.color=modifier<RunModifier::DeadlyEvents?"green":"red";
    run_mode_text(a,daily?"DAILY_RUN_MENU.MODIFIER":"CUSTOM_RUN_SCREEN.MODIFIER_LABEL",values,out,capacity);
    if(!*out){TextBuffer b{out,capacity,0};b.text(values.title);b.text(": ");b.text(values.description);}
}
static void singleplayer_screen(Canvas&c){
    auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,165);a.hit_count=0;
    // Singleplayer inherits the 330x705 submenu_button, not the shorter
    // 368x490 compendium controls. Project the authored 4:3 viewport.
    constexpr int width=reference_viewport::project(330),height=reference_viewport::project(705);
    constexpr int top=reference_viewport::project(reference_viewport::Height/2-353);
    const int bases[]={I7_MenuStandardPanel,I7_MenuDailyPanel,I7_MenuCustomPanel};
    const int icons[]={I7_MenuStandardIcon,I7_MenuDailyIcon,I7_MenuCustomIcon};
    const char*keys[]={"STANDARD","DAILY","CUSTOM"};
    const UiAction actions[]={UiAction::OpenStandardRun,UiAction::OpenDailyRun,UiAction::OpenCustomRun};
    for(unsigned i=0;i<3;++i){
        const int x=reference_viewport::project(reference_viewport::Width/2-565+int(i)*400);
        const bool focus=progress_hover(a,x,top,width,height)||(!a.pointer.visible&&a.compendium_selection==i);
        const bool pressed=focus&&a.pointer.held&&a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==int(actions[i]);
        auto&motion=a.compendium_motion[i];const unsigned scale=motion.observe(pressed?2:focus?1:0,a.map_clock_ms,false),alpha=motion.alpha(a.map_clock_ms);
        const int w=(width*int(scale)+500)/1000,h=(height*int(scale)+500)/1000,left=x+(width-w)/2,y=top+(height-h)/2;
        if(a.content.version()>=Ui7ContentVersion){c.ui(bases[i]+motion.previous,left,y,w,h);c.ui(bases[i]+motion.state,left,y,w,h,alpha);}else c.dim(left,y,w,h,150);
        c.ui(icons[i],left+w/2-27,y+27,54,41);
        char key[80]{};TextBuffer text{key,sizeof key,0};text.text(keys[i]);text.text(".title");
        c.text_fitted(frontend_text(a,"main_menu_ui",key,keys[i]),left+2,y+13,w-4,12,Gold,8,true,true);
        key[0]=0;text.n=0;text.text(keys[i]);text.text(".description");
        c.fitted(frontend_text(a,"main_menu_ui",key,""),left+4,y+69,w-8,h-74,8,8,White,false,true,true);
        c.action(x,top,width,height,actions[i]);
    }
    native_original_back(c,a.run_mode.started);
}
static void custom_run_screen(Canvas&c){
    auto&a=c.a;auto&mode=a.run_mode;title_background(c,false);c.dim(0,0,320,240,185);a.hit_count=0;
    const bool editing=a.run_seed_editing&&a.map_preview.open;
    const auto label=[&](const char*key,const char*fallback){return frontend_text(a,"main_menu_ui",key,fallback);};
    c.text_fitted(label("CUSTOM_RUN_SCREEN.CUSTOM_MODE_TITLE","Custom Mode"),6,22,148,12,Gold,9,true,true);
    c.text_fitted(label("CUSTOM_RUN_SCREEN.MODIFIERS_TITLE","Modifiers"),165,22,66,10,Gold,8);
    const bool random_focus=progress_hover(a,237,20,50,14)||(!a.pointer.visible&&mode.focus==8);
    Canvas random=c;random.brightness=random_focus?255:205;random.ui(U_RewardButton,237,20,50,14);
    c.text_fitted(label("CUSTOM_RUN_SCREEN.RANDOMIZE","Randomize"),239,23,46,8,White,8,true);
    if(!editing)c.action(237,19,50,16,UiAction::RandomizeRun);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectDefect};
    for(unsigned i=0;i<CharacterKinds;++i){const Character character=RunModeCharacters[i];const int x=32+int(i)*22;
        const bool selected=character==a.selected_character;const int portrait=a.content.ui_image(character_portrait(character));
        if(selected){const int outline=a.content.world_image("character","OUTLINE");if(outline>=0)c.picture(outline,x-1,172,22,30,255,presentation::Add);}
        small_material_icon(c,portrait,x,174,20,26,selected?1024:512,selected?1024:614);
        if(!a.pointer.visible&&mode.focus==int(i))c.rect(x+2,203,16,1,Gold);
        if(!editing)c.action(x-1,172,22,32,actions[i]);
    }
    Canvas ascension=c;ascension.tint_red=ascension.tint_green=ascension.tint_blue=0;
    ascension.nine_ui(I7_TinyPanel,24,149,121,22,3,3,3,3,128);
    if(a.selected_ascension)c.ui(I7_SettingsArrowLeft,12,149,12,22);
    if(a.selected_ascension<10)c.ui(I7_SettingsArrowRight,145,149,12,22);
    c.ui(U_TopBarAscension,27,149,12,22);c.num(a.selected_ascension,29,159,8,White);
    char level_key[40]{},text[1024]{};TextBuffer level{level_key,sizeof level_key,0};level.text("LEVEL_");level.ch(char('0'+a.selected_ascension/10));level.ch(char('0'+a.selected_ascension%10));level.text(".title");
    TextBuffer info{text,sizeof text,0};info.text("[gold][b]");info.text(frontend_text(a,"ascension",level_key,a.content.english()?"Ascension":"进阶等级"));info.text("[/b][/gold]\n");
    level_key[8]=0;level.n=8;level.text(".description");info.text(a.content.world_string("ascension",level_key));
    c.paragraph_fitted(text,41,149,101,22,8,White,8);
    if(!editing){if(a.selected_ascension)c.action(10,147,16,26,UiAction::RunAscensionDown);if(a.selected_ascension<10)c.action(143,147,16,26,UiAction::RunAscensionUp);}
    c.text_fitted(label("CUSTOM_RUN_SCREEN.SEED_LABEL","Seed:"),48,105,26,9,Gold,8);
    c.dim(77,102,65,15,140);c.rect(77,116,65,1,editing?Gold:Muted);
    const char*seed=editing?a.map_preview.draft:a.selected_seed;
    const bool has_seed=*seed;while(*seed&&c.width(seed,8)>61)++seed;
    c.text_fitted(has_seed?seed:label("CUSTOM_RUN_SCREEN.SEED_RANDOM_PLACEHOLDER","Random"),79,105,61,9,has_seed?White:Muted,8);
    if(!editing)c.action(76,100,68,20,UiAction::RunSeed);
    else{
        // The original control is an in-place LineEdit. Host text/Enter/Backspace
        // events already route to reference_text; no map screen is involved.
        c.text_fitted(a.content.english()?"Enter: apply  Esc: cancel":"Enter 应用  Esc 取消",28,124,130,8,Muted,8);
    }
    constexpr int left=166,top=36,width=108,view_height=173;
    if(mode.layout_generation!=a.content.generation()){
        unsigned y=0;for(unsigned i=0;i<RunModifierKinds;++i){mode.row_offsets[i]=uint16_t(y);modifier_text(a,RunModifier(i),text,sizeof text);
            const int lines=c.paragraph(text,0,0,width-14,0,0,8);y+=unsigned(mx(21,lines*11+7));}
        mode.row_offsets[RunModifierKinds]=uint16_t(y);mode.layout_generation=a.content.generation();
    }
    auto&scroller=a.progress_page_scroll;scroller.configure(mode.row_offsets[RunModifierKinds],view_height);
    if(!a.pointer.visible&&mode.focus>=9&&mode.focus<int(9+RunModifierKinds)&&mode.focus!=mode.last_focus){
        const unsigned i=unsigned(mode.focus-9);const int from=mode.row_offsets[i],to=mode.row_offsets[i+1],visible=scroller.offset();
        if(from<visible)scroller.target=from*256;else if(to>visible+view_height)scroller.target=(to-view_height)*256;
    }
    mode.last_focus=mode.focus;
    Canvas list=c;list.pixels+=top*list.pitch;if(list.coverage)list.coverage+=top*list.pitch;list.clip_h=view_height;
    if(!editing)c.hit(left,top,width,view_height,HitKind::ProgressScroll);
    for(unsigned i=0;i<RunModifierKinds;++i){const int y=int(mode.row_offsets[i])-scroller.offset(),height=int(mode.row_offsets[i+1]-mode.row_offsets[i]);
        if(y+height<=0||y>=view_height)continue;
        const int hit_top=mx(top,top+y),hit_bottom=mn(top+view_height,top+y+height);
        const bool hover=progress_hover(a,left,hit_top,width,hit_bottom-hit_top),focus=!a.pointer.visible&&mode.focus==int(i+9);
        if(focus){Canvas highlight=list;highlight.opacity=120;highlight.rect(left,y,width,height,rgb(51,51,58));}
        const bool ticked=(mode.pending.modifiers&(uint32_t(1)<<i))!=0;
        list.ui(ticked?F_SettingsTicked:F_SettingsUnticked,left,y+2,10,10);
        modifier_text(a,RunModifier(i),text,sizeof text);list.paragraph(text,left+13,y+3,width-14,(height-6)/11,0,8,White);
        if(hover&&ticked)list.picture(a.content.ui_image(F_SettingsTicked),left,y+2,10,10,50,presentation::Add);
        if(!editing)c.hit(left,hit_top,width,hit_bottom-hit_top,HitKind::RunModifier,int(i));
    }
    c.paragraph_fitted(label("CUSTOM_RUN_SCREEN.disclaimer",""),37,218,246,20,8,rgb(255,97,97),8);
    if(!editing){
        if(scroller.maximum){c.ui(I6_ScrollbarTrack,277,42,7,158);c.ui(I6_ScrollbarEdge,277,34,7,8);
            ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),280,204,7,8,0,-1024);
            c.ui(I6_ScrollbarGrabber,274,scroller.thumb_y(42,158)-7,13,14);c.hit(275,34,12,174,HitKind::ProgressThumb);}
        native_original_back(c,mode.started);native_original_confirm(c,UiAction::StartConfiguredRun,mode.started);
    }else{a.hit_count=0;c.hit(76,100,68,20,HitKind::None);}
}
static void daily_run_screen(Canvas&c){
    auto&a=c.a;a.refresh_daily_run();const auto&mode=a.run_mode;title_background(c,false);c.dim(0,0,320,240,185);a.hit_count=0;
    const auto label=[&](const char*key,const char*fallback){return frontend_text(a,"main_menu_ui",key,fallback);};
    c.text_fitted(label("DAILY_RUN_MENU.DAILY_TITLE","Daily Challenge"),6,22,148,12,Gold,9,true,true);
    const uint32_t now=a.callbacks.wall_clock?a.callbacks.wall_clock():0;char date[64]{},time[32]{},text[1024]{};
    progress_date_text(a,now,date,sizeof date,time,sizeof time);c.text_fitted(date,5,36,150,10,Blue,8,true);
    const unsigned remaining=now?86400-now%86400:0;TextBuffer clock{time,sizeof time,0};time[0]=0;
    const unsigned hours=remaining/3600,minutes=remaining/60%60,seconds=remaining%60;
    if(hours<10)clock.ch('0');clock.number(hours);clock.ch(':');if(minutes<10)clock.ch('0');clock.number(minutes);clock.ch(':');if(seconds<10)clock.ch('0');clock.number(seconds);
    RunModeTextValues values;values.time=time;values.ascension=a.selected_ascension;
    run_mode_text(a,"DAILY_RUN_MENU.TIME_LEFT",values,text,sizeof text);c.text_fitted(text,22,49,132,9,White,8,true);
    c.ui(U_TopBarIronclad+int(a.selected_character),53,96,13,13);
    c.ui(U_TopBarAscension,60,100,10,13);c.num(a.selected_ascension,62,108,8,White);
    const auto&character=character_definitions[unsigned(a.selected_character)];const char*name=a.content.world_title("characters",character.key);
    c.text_fitted(*name?name:character.name,73,95,77,10,Gold,8);
    run_mode_text(a,"DAILY_RUN_MENU.ASCENSION",values,text,sizeof text);c.text_fitted(text,73,108,77,8,Blue,8);
    c.text_fitted(label("DAILY_RUN_MENU.MODIFIERS","Modifiers:"),31,139,118,10,Gold,8);
    int row_y[4]{};const unsigned count=mn(3,run_modifier_count(mode.pending));
    for(unsigned i=0;i<count;++i){modifier_text(a,run_modifier_at(mode.pending,i),text,sizeof text,true);row_y[i+1]=row_y[i]+mx(2,c.paragraph(text,0,0,95,0,0,8))*11+5;}
    auto&scroller=a.progress_page_scroll;scroller.configure(row_y[count],59);
    Canvas modifiers=c;modifiers.pixels+=150*modifiers.pitch;if(modifiers.coverage)modifiers.coverage+=150*modifiers.pitch;modifiers.clip_h=59;
    c.hit(34,150,111,59,HitKind::ProgressScroll);
    for(unsigned i=0;i<count;++i){const RunModifier modifier=run_modifier_at(mode.pending,i);const int y=row_y[i]-scroller.offset(),height=row_y[i+1]-row_y[i];
        if(y+height<=0||y>=59)continue;
        if(a.content.version()>=HistoryUi8ContentVersion)modifiers.ui(I8_ModifierDraft+int(run_modifier_icon(modifier)),34,y+2,12,12);
        modifier_text(a,modifier,text,sizeof text,true);modifiers.paragraph(text,49,y,95,(height-5)/11,0,8,White);
    }
    c.text_fitted(label("DAILY_RUN_MENU.LEADERBOARDS.title","Leaderboards"),165,40,150,12,Gold,8,true,true);
    // This offline host has neither Steam identities nor a leaderboard API.
    // Preserve the leaderboard's empty region and explicitly identify the
    // unavailable service; never manufacture ranks or submitted scores.
    c.paragraph_fitted(a.content.english()?"Leaderboards unavailable offline.":"离线时无法连接排行榜。",191,102,98,34,8,Muted,8);
    c.paragraph_fitted(a.content.english()?"Challenge uses this device's UTC date.":"挑战使用本机 UTC 日期。",191,142,98,38,8,Muted,8);
    c.paragraph_fitted(label("DAILY_RUN_MENU.disclaimer",""),37,218,246,20,8,rgb(255,97,97),8);
    native_original_back(c,mode.started);if(mode.pending.mode==RunMode::Daily)native_original_confirm(c,UiAction::StartConfiguredRun,mode.started);
}
static void run_modes_screen(Canvas&c){
    if(c.a.run_mode.screen==RunMenu::Singleplayer)singleplayer_screen(c);
    else if(c.a.run_mode.screen==RunMenu::Custom)custom_run_screen(c);
    else daily_run_screen(c);
}
