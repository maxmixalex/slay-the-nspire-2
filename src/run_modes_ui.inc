static const char* modifier_localized(const App&a,RunModifier modifier,bool description){
    char key[96]{};TextBuffer b{key,sizeof key,0};b.text(run_modifier_key(modifier));
    b.text(run_modifier_character(modifier)?(description?".cardsModifierDescription":".cardsModifierTitle"):(description?".description":".title"));
    const char*value=a.content.world_string(run_modifier_character(modifier)?"characters":"modifiers",key);
    return *value?value:description?"":run_modifier_key(modifier);
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
    // Source singleplayer_submenu: three 330x490 controls at -565/-165/235.
    constexpr int width=73,height=109,top=65;
    const int bases[]={I7_MenuStandardPanel,I7_MenuDailyPanel,I7_MenuCustomPanel};
    const int icons[]={I7_MenuStandardIcon,I7_MenuDailyIcon,I7_MenuCustomIcon};
    const char*keys[]={"STANDARD","DAILY","CUSTOM"};
    const UiAction actions[]={UiAction::OpenStandardRun,UiAction::OpenDailyRun,UiAction::OpenCustomRun};
    for(unsigned i=0;i<3;++i)compendium_button(c,i,34+int(i)*89,top,width,height,bases[i],icons[i],keys[i],actions[i]);
    native_original_back(c,a.run_mode.started);
}
static void custom_run_screen(Canvas&c){
    auto&a=c.a;auto&mode=a.run_mode;title_background(c,false);c.dim(0,0,320,240,185);a.hit_count=0;
    const bool editing=a.run_seed_editing&&a.map_preview.open;
    const auto label=[&](const char*key,const char*fallback){return frontend_text(a,"main_menu_ui",key,fallback);};
    c.text_fitted(label("CUSTOM_RUN_SCREEN.CUSTOM_MODE_TITLE","Custom Mode"),8,19,120,19,Gold,10,true,true);
    c.text_fitted(label("CUSTOM_RUN_SCREEN.MODIFIERS_TITLE","Modifiers"),141,16,86,12,Gold,9);
    c.button(label("CUSTOM_RUN_SCREEN.RANDOMIZE","Randomize"),229,12,64,!a.pointer.visible&&mode.focus==8,23);
    if(!editing)c.action(228,10,66,26,UiAction::RandomizeRun);
    const auto&definition=character_definitions[unsigned(a.selected_character)];const char*name=a.content.world_title("characters",definition.key);
    c.text_fitted(*name?name:definition.name,10,51,114,11,White,8,true);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectDefect};
    for(unsigned i=0;i<CharacterKinds;++i){const Character character=RunModeCharacters[i];const int x=10+int(i)*23;
        const bool selected=character==a.selected_character;const int portrait=a.content.ui_image(character_portrait(character));
        if(selected){const int outline=a.content.world_image("character","OUTLINE");if(outline>=0)c.picture(outline,x-1,71,24,32,255,presentation::Add);}
        small_material_icon(c,portrait,x,74,22,27,selected?1024:512,selected?1024:614);
        if(!a.pointer.visible&&mode.focus==int(i))c.rect(x+2,105,18,1,Gold);
        if(!editing)c.action(x-1,69,24,37,actions[i]);
    }
    c.ui(I7_SettingsArrowLeft,14,115,12,22);c.ui(I7_SettingsArrowRight,108,115,12,22);
    c.ui(U_TopBarAscension,38,114,13,22);c.num(a.selected_ascension,41,124,8,White);
    char text[1024]{};RunModeTextValues values;values.ascension=a.selected_ascension;
    run_mode_text(a,"DAILY_RUN_MENU.ASCENSION",values,text,sizeof text);c.text_fitted(text,53,119,52,9,White,8);
    if(!editing){c.action(10,111,23,29,UiAction::RunAscensionDown,a.selected_ascension>0);c.action(103,111,23,29,UiAction::RunAscensionUp,a.selected_ascension<10);}
    c.text_fitted(label("CUSTOM_RUN_SCREEN.SEED_LABEL","Seed:"),12,149,110,9,White,8);
    c.dim(11,164,115,24,140);c.rect(12,186,113,1,editing?Gold:Muted);
    const char*seed=editing?a.map_preview.draft:a.selected_seed;
    c.text_fitted(*seed?seed:label("CUSTOM_RUN_SCREEN.SEED_RANDOM_PLACEHOLDER","Random"),16,171,105,9,*seed?White:Muted,8);
    if(!editing)c.action(10,163,117,27,UiAction::RunSeed);
    else{
        // The original control is an in-place LineEdit. Host text/Enter/Backspace
        // events already route to reference_text; no map screen is involved.
        c.text_fitted(a.content.english()?"Enter: apply  Esc: cancel":"Enter 应用  Esc 取消",10,191,118,8,Muted,8);
    }
    constexpr int left=140,top=42,width=154,view_height=155;
    if(mode.layout_generation!=a.content.generation()){
        unsigned y=0;for(unsigned i=0;i<RunModifierKinds;++i){mode.row_offsets[i]=uint16_t(y);modifier_text(a,RunModifier(i),text,sizeof text);
            const int lines=c.paragraph(text,0,0,width-17,0,0,8);y+=unsigned(mx(21,lines*11+7));}
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
        list.ui(ticked?F_SettingsTicked:F_SettingsUnticked,left,y+2,13,13);
        modifier_text(a,RunModifier(i),text,sizeof text);list.paragraph(text,left+16,y+3,width-17,(height-6)/11,0,8,White);
        if(hover&&ticked)list.picture(a.content.ui_image(F_SettingsTicked),left,y+2,13,13,50,presentation::Add);
        if(!editing)c.hit(left,hit_top,width,hit_bottom-hit_top,HitKind::RunModifier,int(i));
    }
    c.text_fitted(label("CUSTOM_RUN_SCREEN.disclaimer",""),11,204,297,8,Muted,8,true);
    if(!editing){progress_page_chrome(c);native_original_confirm(c,UiAction::StartConfiguredRun,mode.started);}
    else{a.hit_count=0;c.hit(11,163,115,27,HitKind::None);}
}
static void daily_run_screen(Canvas&c){
    auto&a=c.a;a.refresh_daily_run();const auto&mode=a.run_mode;title_background(c,false);c.dim(0,0,320,240,185);a.hit_count=0;
    const auto label=[&](const char*key,const char*fallback){return frontend_text(a,"main_menu_ui",key,fallback);};
    c.text_fitted(label("DAILY_RUN_MENU.DAILY_TITLE","Daily Challenge"),6,14,169,19,Gold,12,true,true);
    const uint32_t now=a.callbacks.wall_clock?a.callbacks.wall_clock():0;char date[64]{},time[32]{},text[1024]{};
    progress_date_text(a,now,date,sizeof date,time,sizeof time);c.text_fitted(date,9,39,163,10,Blue,8,true);
    const unsigned remaining=now?86400-now%86400:0;TextBuffer clock{time,sizeof time,0};time[0]=0;
    const unsigned hours=remaining/3600,minutes=remaining/60%60,seconds=remaining%60;
    if(hours<10)clock.ch('0');clock.number(hours);clock.ch(':');if(minutes<10)clock.ch('0');clock.number(minutes);clock.ch(':');if(seconds<10)clock.ch('0');clock.number(seconds);
    RunModeTextValues values;values.time=time;values.ascension=a.selected_ascension;
    run_mode_text(a,"DAILY_RUN_MENU.TIME_LEFT",values,text,sizeof text);c.text_fitted(text,10,54,162,9,White,8,true);
    c.ui(U_TopBarIronclad+int(a.selected_character),24,72,24,24);
    const auto&character=character_definitions[unsigned(a.selected_character)];const char*name=a.content.world_title("characters",character.key);
    c.text_fitted(*name?name:character.name,52,74,113,10,Gold,8);
    run_mode_text(a,"DAILY_RUN_MENU.ASCENSION",values,text,sizeof text);c.text_fitted(text,52,88,113,8,Blue,8);
    c.text_fitted(label("DAILY_RUN_MENU.MODIFIERS","Modifiers:"),13,111,156,10,Gold,8);
    int y=125;for(unsigned i=0;i<run_modifier_count(mode.pending)&&i<3;++i){const RunModifier modifier=run_modifier_at(mode.pending,i);
        if(a.content.version()>=HistoryUi8ContentVersion)c.ui(I8_ModifierDraft+int(run_modifier_icon(modifier)),14,y+2,16,16);
        modifier_text(a,modifier,text,sizeof text,true);const int lines=mx(2,c.paragraph(text,0,0,133,0,0,8));
        c.paragraph(text,34,y,133,mn(4,lines),0,8,White);y+=mn(4,lines)*11+5;
    }
    c.text_fitted(label("DAILY_RUN_MENU.LEADERBOARDS.title","Leaderboards"),183,24,126,15,Gold,10,true,true);
    // This offline host has neither Steam identities nor a leaderboard API.
    // Preserve the leaderboard's empty region and explicitly identify the
    // unavailable service; never manufacture ranks or submitted scores.
    c.paragraph_fitted(a.content.english()?"Leaderboards unavailable offline.":"离线时无法连接排行榜。",190,94,111,41,9,Muted,8);
    c.paragraph_fitted(a.content.english()?"Challenge uses this device’s UTC date.":"挑战使用本机 UTC 日期。",190,142,111,38,8,Muted,8);
    c.text_fitted(label("DAILY_RUN_MENU.disclaimer",""),11,209,297,8,Muted,8,true);
    native_original_back(c,mode.started);if(mode.pending.mode==RunMode::Daily)native_original_confirm(c,UiAction::StartConfiguredRun,mode.started);
}
static void run_modes_screen(Canvas&c){
    if(c.a.run_mode.screen==RunMenu::Singleplayer)singleplayer_screen(c);
    else if(c.a.run_mode.screen==RunMenu::Custom)custom_run_screen(c);
    else daily_run_screen(c);
}
