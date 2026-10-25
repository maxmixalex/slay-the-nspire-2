// Original compendium hierarchy and scrollable statistics/history. UI history
// is observed metadata; opening these screens never adopts a historical State.
struct ProgressTextValues {
    const char*amount=nullptr,*playtime=nullptr,*fastest=nullptr,*seed=nullptr,*date=nullptr,*time=nullptr;
    int64_t wins=0,losses=0,total=0,hp=0,max_hp=0,gold=0,floor=0,turns=0;
};
static LocalizedValue progress_text_value(const void*context,const char*key){
    const auto&v=*static_cast<const ProgressTextValues*>(context);
    const auto same=[](const char*a,const char*b){while(*a&&*b)if(*a++!=*b++)return false;return *a==*b;};
    if(same(key,"Amount"))return v.amount?LocalizedValue{true,0,1,v.amount}:LocalizedValue{};
    if(same(key,"Playtime"))return {true,0,1,v.playtime?v.playtime:"—"};
    if(same(key,"FastestWin"))return {true,0,1,v.fastest?v.fastest:"—"};
    if(same(key,"Wins"))return {true,v.wins};if(same(key,"Losses"))return {true,v.losses};
    if(same(key,"totalCards")||same(key,"totalRelics"))return {true,v.total};
    if(same(key,"Seed"))return {true,0,1,v.seed?v.seed:"—"};
    if(same(key,"Date"))return {true,0,1,v.date?v.date:"—"};
    if(same(key,"Time"))return {true,0,1,v.time?v.time:"—"};
    if(same(key,"HP"))return {true,v.hp};if(same(key,"MaxHP"))return {true,v.max_hp};if(same(key,"Gold"))return {true,v.gold};
    if(same(key,"FloorNum"))return {true,v.floor};if(same(key,"Turns"))return {true,v.turns};
    return {};
}
static void progress_text(const App&a,const char*table,const char*key,const ProgressTextValues&values,char*out,size_t capacity){
    LocalizedTemplate format(out,capacity,progress_text_value,&values,"Energy","Stars",a.content.english());
    format.format(a.content.world_string(table,key));
}
static void progress_time_text(char*out,size_t capacity,uint32_t seconds){
    TextBuffer text{out,capacity,0};const unsigned hours=seconds/3600,minutes=seconds/60%60;
    if(hours){text.number(hours);text.ch(':');}if(minutes<10)text.ch('0');text.number(minutes);text.ch(':');
    if(seconds%60<10)text.ch('0');text.number(seconds%60);
}
static void progress_date_text(const App&a,uint32_t timestamp,char*date,size_t date_size,char*time,size_t time_size){
    if(!timestamp){TextBuffer d{date,date_size,0};d.text("—");TextBuffer t{time,time_size,0};t.text("—");return;}
    unsigned days=timestamp/86400,year=1970;const auto leap=[](unsigned y){return y%4==0&&(y%100!=0||y%400==0);};
    while(days>=(leap(year)?366u:365u)){days-=leap(year)?366:365;++year;}
    const unsigned lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};unsigned month=0;
    while(month<11&&days>=lengths[month]+unsigned(month==1&&leap(year))){days-=lengths[month]+unsigned(month==1&&leap(year));++month;}
    TextBuffer d{date,date_size,0};
    if(a.content.english()){static const char*names[]={"January","February","March","April","May","June","July","August","September","October","November","December"};d.text(names[month]);d.ch(' ');d.number(days+1);d.text(", ");d.number(year);}
    else{static const char*names[]={"一月","二月","三月","四月","五月","六月","七月","八月","九月","十月","十一月","十二月"};d.number(year);d.ch(' ');d.text(names[month]);d.ch(' ');d.number(days+1);}
    TextBuffer t{time,time_size,0};const unsigned hour=timestamp/3600%24,minute=timestamp/60%60;
    t.number(hour%12?hour%12:12);t.ch(':');if(minute<10)t.ch('0');t.number(minute);
    t.text(a.content.english()?(hour<12?" AM":" PM"):(hour<12?" 上午":" 下午"));
}
static bool progress_hover(const App&a,int x,int y,int width,int height){return a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+width&&a.pointer.y>=y&&a.pointer.y<y+height;}
static void compendium_button(Canvas&c,unsigned index,int x,int y,int width,int height,int base,int icon,const char*prefix,UiAction action,bool bottom=false){
    auto&a=c.a;const bool focused=progress_hover(a,x,y,width,height)||(!a.pointer.visible&&a.compendium_selection==index);
    const bool pressed=focused&&a.pointer.held&&a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==int(action);
    auto&motion=a.compendium_motion[index];const unsigned scale=motion.observe(pressed?2:focused?1:0,a.map_clock_ms,bottom,bottom?950:980),alpha=motion.alpha(a.map_clock_ms);
    const ProfileTransform transform{x+reference_viewport::project(bottom?136:180),y+reference_viewport::project(bottom?96:240),int((scale*4096+500)/1000)};
    if(a.content.version()>=Ui7ContentVersion){profile_image(c,base+motion.previous,x,y,width,height,transform);profile_image(c,base+motion.state,x,y,width,height,transform,alpha);}
    else c.dim(x,y,width,height,150);
    if(bottom){const bool history=index==5;profile_image(c,icon,x+(history?4:10),y+(history?6:5),history?45:33,history?20:21,transform);}
    else profile_image(c,icon,x+9,y+17,50,38,transform);
    char key[128]{};TextBuffer k{key,sizeof key,0};k.text(prefix);k.text(".title");
    const char*title=frontend_text(a,"main_menu_ui",key,prefix);
    ProfileTextStyle title_style{5,source_label_face(a.content.english(),5),rgb(239,200,81),32,2,rgb(51,49,39),1};title_style.shadow_y=1;
    profile_text_box(c,title,x+(bottom?3:4),y+(bottom?26:7),bottom?47:60,10,title_style,true,false,transform);
    if(!bottom){key[0]=0;k.n=0;k.text(prefix);k.text(".description");const char*description=frontend_text(a,"main_menu_ui",key,"");
        profile_text_box(c,description,x+8,y+56,53,29,{5,0,rgb(255,246,226),64},true,true,transform);}
    c.action(x,y,width,height,action);
}
static void compendium_screen(Canvas&c){
    auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,165);a.hit_count=0;
    const bool bestiary=a.bestiary_available();const int count=bestiary?4:3;
    // 368x490 source panels, a 12px HBox gap, and centered 714px VBox.
    // The source margin container remains 1920 wide even in the 1680 viewport.
    constexpr int width=reference_viewport::project(368),height=reference_viewport::project(490);
    const int source_width=count*368+(count-1)*12,source_left=(reference_viewport::Width-source_width)/2;
    constexpr int top=reference_viewport::project((reference_viewport::Height-714)/2);
    const int panels[]={I7_MenuCardPanel,I7_MenuRelicPanel,I7_MenuPotionPanel,I7_MenuBestiaryPanel};
    const int icons[]={I7_MenuCardIcon,I7_MenuRelicIcon,I7_MenuPotionIcon,I7_MenuBestiaryIcon};
    const char*names[]={"COMPENDIUM_CARD_LIBRARY","COMPENDIUM_RELIC_COLLECTION","COMPENDIUM_POTION_LAB","COMPENDIUM_BESTIARY"};
    const UiAction actions[]={UiAction::LibraryCards,UiAction::LibraryRelics,UiAction::LibraryPotions,UiAction::OpenBestiary};
    for(int i=0;i<count;++i)compendium_button(c,unsigned(i),reference_viewport::project(source_left+i*380),top,width,height,panels[i],icons[i],names[i],actions[i]);
    constexpr int right=reference_viewport::project(1600),bottom=reference_viewport::project(787);
    constexpr int bottom_width=reference_viewport::project(280),bottom_height=reference_viewport::project(200),bottom_gap=reference_viewport::project(24);
    const int stats_x=right-bottom_width-(a.progress.count?bottom_width+bottom_gap:0);
    compendium_button(c,4,stats_x,bottom,bottom_width,bottom_height,I7_MenuStatsPanel,I7_MenuStatsIcon,"STATISTICS",UiAction::OpenStatistics,true);
    if(a.progress.count)compendium_button(c,5,right-bottom_width,bottom,bottom_width,bottom_height,I7_MenuHistoryPanel,I7_MenuHistoryIcon,"RUN_HISTORY",UiAction::OpenHistory,true);
    native_original_back(c,a.progress_page_started);
}
static Canvas progress_content_canvas(Canvas&c){Canvas layer=c;layer.pixels+=40*layer.pitch;if(layer.coverage)layer.coverage+=40*layer.pitch;layer.clip_h=174;return layer;}
static void progress_content_hit(Canvas&c,int x,int y,int width,int height,HitKind kind,int value){
    const int top=mx(40,y),bottom=mn(214,y+height);if(bottom>top)c.hit(x,top,width,bottom-top,kind,value);
}
static void progress_page_chrome(Canvas&c){
    auto&a=c.a;auto&s=a.progress_page_scroll;
    native_original_back(c,a.progress_page_started);
    if(s.maximum){c.ui(I6_ScrollbarTrack,301,61,10,128);c.ui(I6_ScrollbarEdge,301,50,10,11);
        ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),306,195,10,11,0,-1024);
        const int center=s.thumb_y();c.ui(I6_ScrollbarGrabber,298,center-8,16,16);c.hit(294,51,23,148,HitKind::ProgressThumb);
    }
}
static unsigned progress_bit_count(const uint8_t*bits,unsigned count){unsigned n=0;for(unsigned i=0;i<count;++i)n+=(bits[i/8]>>(i%8))&1;return n;}
static void statistics_entry(Canvas&c,const UiStatisticsLayout&layout,unsigned index,int icon,const char*top,const char*bottom,const ProgressTextValues&top_values,const ProgressTextValues&bottom_values,bool top_red=false){
    using reference_viewport::project;auto&a=c.a;const auto point=UiStatisticsLayout::projected(layout.entries[index]);
    const int y=point.y-a.progress_page_scroll.offset(),origin=30;const bool visible=y+point.h>origin&&y<240;
    const bool hover=visible&&!a.pointer.dragging&&progress_hover(a,point.x,y,point.w,point.h);
    const bool focus=hover||(!a.pointer.visible&&a.statistics_presentation.selected==index);
    const int weight=a.statistics_presentation.entries[index].observe(focus,a.map_clock_ms);
    if(!visible)return;const ProfileTransform transform{point.x+project(50),y-origin+point.h/2,4096+205*weight/4096};
    const auto source=layout.entries[index];
    const int icon_y=project(source.y+(source.h-72)/2)-a.progress_page_scroll.offset()-origin;
    profile_image(c,icon,project(source.x+12),icon_y,project(80),project(72),transform);
    const int label_x=project(source.x+96),label_width=project(404);const bool pair=top&&bottom;
    const auto label=[&](const char*key,const ProgressTextValues&values,bool second){if(!key)return;char text[192]{};progress_text(a,"stats_screen",key,values,text,sizeof text);
        Canvas metrics=c;metrics.font_face=0;metrics.bold_face=5;int size=5;if(metrics.width(text,size)>label_width)size=4;
        const int source_y=source.y+(pair?(second?36:8):(source.h-48)/2);
        profile_text_box(c,text,label_x,project(source_y)-a.progress_page_scroll.offset()-origin,label_width,project(48),{size,0,top_red&&!second?rgb(255,85,85):rgb(255,246,226),64},false,false,transform,true);
    };
    label(top,top_values,false);label(bottom,bottom_values,true);
    if(focus&&!a.pointer.visible){const auto image=a.content.image(a.content.world_image("map","SELECTION_RETICLE"));
        if(image.pixels&&image.format<=3)for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom_corner=corner&1;
            target_reticle_corner(c,image,right?point.x+point.w-1:point.x,bottom_corner?y-origin+point.h-1:y-origin,4,right,bottom_corner,255,false);}}
    const int hit_top=mx(origin,y),hit_bottom=mn(240,y+point.h);
    if(hit_bottom>hit_top){Canvas hits=c;hits.hit(point.x,hit_top,point.w,hit_bottom-hit_top,HitKind::StatisticsEntry,int(index));}
}
static void statistics_screen(Canvas&c){
    using reference_viewport::project;auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,210);a.hit_count=0;
    const unsigned age=a.map_clock_ms-a.progress_page_started;Canvas foreground=c;foreground.opacity=uint8_t(div255(unsigned(foreground.opacity)*(age>=400?255:age*255/400)));
    const UiStatisticsLayout layout(a.progress);a.progress_page_scroll.configure(layout.height,240);
    c.hit(UiStatisticsLayout::Left,30,UiStatisticsLayout::Width,210,HitKind::ProgressScroll);
    Canvas page=foreground;page.pixels+=30*page.pitch;if(page.coverage)page.coverage+=30*page.pitch;page.clip_h=210;
    const int offset=a.progress_page_scroll.offset(),origin=30;
    ProfileTextStyle header{7,source_label_face(a.content.english(),5),rgb(239,200,81),64,2,rgb(77,59,34)};header.shadow_x=2;header.shadow_y=1;
    profile_text_box(page,frontend_text(a,"main_menu_ui","STATISTICS.OVERALL.title","Overall Stats"),UiStatisticsLayout::Left,project(200)-offset-origin,UiStatisticsLayout::Width,project(64),header,false,false);
    Canvas panel=page;panel.opacity=uint8_t(div255(unsigned(panel.opacity)*128));
    panel.rect(UiStatisticsLayout::Left,project(UiStatisticsLayout::OverallTop)-offset-origin,UiStatisticsLayout::Width,project(UiStatisticsLayout::OverallHeight),rgb(30,43,48));
    uint32_t seconds=0,fastest=0,wins=0,losses=0,slain=0;unsigned ascensions=0,monsters=0;
    for(const auto&s:a.progress.characters){seconds=progress_add(seconds,s.play_seconds);wins=progress_add(wins,s.wins);losses=progress_add(losses,s.losses);ascensions+=s.highest_ascension;if(s.fastest_win_seconds&&(!fastest||s.fastest_win_seconds<fastest))fastest=s.fastest_win_seconds;}
    for(const auto&model:a.progress.enemies){slain=progress_add(slain,model[0].wins);monsters+=bool(model[0].seen);}
    char elapsed[32]{},record[32]{};progress_time_text(elapsed,sizeof elapsed,seconds);if(fastest)progress_time_text(record,sizeof record,fastest);
    const int icons[]={I7_StatsAchievements,I7_StatsClock,I7_StatsCards,I7_StatsSwords,I7_StatsMonsters,I7_StatsRelics,I7_StatsPotions,I7_StatsEvents,I7_StatsStreak};
    const char*top[]={"ENTRY_ACHIEVEMENTS.top","ENTRY_PLAYTIME.top","ENTRY_CARDS.top","ENTRY_WIN_LOSS.top","ENTRY_MONSTER.top","ENTRY_RELIC.top","ENTRY_POTION.top","ENTRY_EVENTS.top","ENTRY_STREAK.top"};
    const char*bottom[]={"ENTRY_ACHIEVEMENTS.bottom","ENTRY_PLAYTIME.bottom","ENTRY_CARDS.bottom","ENTRY_WIN_LOSS.bottom","ENTRY_MONSTER.bottom","ENTRY_RELIC.bottom","ENTRY_POTION.bottom","ENTRY_EVENTS.bottom",nullptr};
    for(unsigned i=0;i<9;++i){
        char amount[40]{},other[40]{};TextBuffer n{amount,sizeof amount,0},d{other,sizeof other,0};
        ProgressTextValues values;values.wins=wins;values.losses=losses;values.playtime=elapsed;values.fastest=fastest?record:nullptr;
        switch(i){
        case 0:n.text("—");d.text("—");break; // Excluded Timeline/platform achievement progress is unknown.
        case 2:n.text("—");d.number(progress_bit_count(a.progress.discovered_cards,CardKinds));d.ch('/');d.number(CardKinds);break;
        case 3:n.number(ascensions);n.ch('/');n.number(10*CharacterKinds);break;
        case 4:n.number(slain);d.number(monsters);d.ch('/');d.number(101);break;
        case 5:n.text("—");d.number(progress_bit_count(a.progress.discovered_relics,WorldRelicKinds));d.ch('/');d.number(WorldRelicKinds);break;
        case 6:n.text("—");d.number(PotionKinds);break;
        case 7:n.text("N/A");d.number(progress_bit_count(a.progress.discovered_events,OriginalEventKinds));d.ch('/');d.number(OriginalEventKinds);break;
        case 8:n.number(a.progress.best_streak);break;
        default:break;
        }
        values.amount=amount;ProgressTextValues lower=values;lower.amount=other;
        statistics_entry(page,layout,i,icons[i],i==3&&!ascensions?nullptr:top[i],i==1&&!fastest?nullptr:bottom[i],values,lower);
    }
    profile_text_box(page,frontend_text(a,"main_menu_ui","STATISTICS.title","Character Stats"),UiStatisticsLayout::Left,project(UiStatisticsLayout::CharacterHeader)-offset-origin,UiStatisticsLayout::Width,project(64),header,false,false);
    static constexpr uint32_t colors[]={0xff5555,0x7fff00,0x87ceeb,0xffa518,0xee82ee};
    for(unsigned slot=0;slot<layout.character_count;++slot){const unsigned model=layout.models[slot];const auto&source=layout.characters[slot];const auto point=UiStatisticsLayout::projected(source);const int y=point.y-offset-origin;
        const auto&s=a.progress.characters[model];panel.rect(point.x,y,point.w,point.h,rgb(30,43,48));
        page.ui(U_TopBarIronclad+int(model),project(source.x+20),project(source.y+12)-offset-origin,project(48),project(48));
        const auto&ch=character_definitions[model];const char*name=a.content.world_title("characters",ch.key);const uint32_t color=colors[model];
        profile_text_box(page,*name?name:ch.name,project(source.x+76),project(source.y+12)-offset-origin,project(444),7,
            {5,source_label_face(a.content.english(),5),rgb(color>>16,(color>>8)&255,color&255),64},false,false);
        char time[32]{},best[32]{},amount[24]{},best_streak[24]{};progress_time_text(time,sizeof time,s.play_seconds);if(s.fastest_win_seconds)progress_time_text(best,sizeof best,s.fastest_win_seconds);
        ProgressTextValues values;values.playtime=time;values.fastest=s.fastest_win_seconds?best:nullptr;values.wins=s.wins;values.losses=s.losses;
        statistics_entry(page,layout,9+slot*3,I7_StatsClock,"ENTRY_CHAR_PLAYTIME.top",s.fastest_win_seconds?"ENTRY_CHAR_PLAYTIME.bottom":nullptr,values,values);
        TextBuffer b{amount,sizeof amount,0};b.number(s.highest_ascension);values.amount=amount;
        statistics_entry(page,layout,10+slot*3,I7_StatsSwords,s.highest_ascension?"ENTRY_CHAR_WIN_LOSS.top":nullptr,"ENTRY_CHAR_WIN_LOSS.bottom",values,values,true);
        b.n=0;amount[0]=0;b.number(s.current_streak);values.amount=amount;
        TextBuffer bs{best_streak,sizeof best_streak,0};bs.number(s.best_streak);ProgressTextValues lower=values;lower.amount=best_streak;
        statistics_entry(page,layout,11+slot*3,I7_StatsStreak,"ENTRY_CHAR_STREAK.top","ENTRY_CHAR_STREAK.bottom",values,lower);
    }
    Canvas tab=foreground;tab.brightness=uint8_t(div255(unsigned(tab.brightness)*229));tab.ui(F_SettingsTab,110,25,49,17);
    foreground.picture(a.content.ui_image(F_SettingsTabStroke),110,25,49,17,255,presentation::Add);
    profile_text_box(foreground,frontend_text(a,"stats_screen","TAB_STATS.header","Statistics"),110,24,49,19,{6,source_label_face(a.content.english(),5),rgb(255,246,226),128,2,0},true,false);
    Canvas disabled=foreground;disabled.brightness=uint8_t(div255(unsigned(disabled.brightness)*105));disabled.ui(F_SettingsTab,161,25,49,17);
    Canvas disabled_text=disabled;disabled_text.opacity=uint8_t(div255(unsigned(disabled_text.opacity)*128));
    profile_text_box(disabled_text,frontend_text(a,"stats_screen","TAB_ACHIEVEMENT.header","Achievements"),161,24,49,19,{6,source_label_face(a.content.english(),5),rgb(255,246,226),128,2,0},true,false);
    disabled.ui(I7_MenuLock,175,24,21,21);
    if(a.progress_page_scroll.maximum){foreground.ui(I6_ScrollbarTrack,268,UiStatisticsLayout::ScrollTop,9,UiStatisticsLayout::ScrollLength);
        foreground.ui(I6_ScrollbarEdge,268,49,9,9);ui_picture_transform(foreground,a.content.ui_image(I6_ScrollbarEdge),272,216,9,9,0,-1024);
        const int center=a.progress_page_scroll.thumb_y(UiStatisticsLayout::ScrollTop,UiStatisticsLayout::ScrollLength);foreground.ui(I6_ScrollbarGrabber,265,center-7,14,14);
        c.hit(263,49,19,171,HitKind::ProgressThumb);}
    native_original_back(foreground,a.progress_page_started);
    int focus=-1;if(a.pointer.visible){const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(hit&&hit->kind==HitKind::StatisticsEntry&&!a.pointer.held)focus=hit->value;}
    else focus=a.statistics_presentation.selected;
    if(focus==1||(focus==3&&ascensions)){const auto point=UiStatisticsLayout::projected(layout.entries[focus]);const int y=point.y-offset;
        if(y+point.h>30&&y<240){const bool playtime=focus==1;source_hover_box(foreground,
            frontend_text(a,"stats_screen",playtime?"TIP_PLAYTIME.header":"TIP_WIN_LOSS.header",""),
            frontend_text(a,"stats_screen",playtime?"TIP_PLAYTIME.description":"TIP_WIN_LOSS.description",""),
            point.x+(point.x<128?-project(392):project(532)),y);}}
}
// NTinyCard is a color-coded silhouette. It never scales the full readable
// card renderer into a few pixels or rebuilds a card description per history row.
static void history_tiny_card(Canvas&c,const EventDeckSnapshot&entry,int x,int y,int size){
    if(c.a.content.version()<HistoryUi8ContentVersion){
        c.picture(c.a.content.frame_image(entry.card),x,y,size,size);return;
    }
    const Card card=entry.card;const auto pool=world_card_pool(CardKind(card.kind));
    uint32_t color=0xffffff;
    switch(pool){
    case WorldCardPool::Ironclad:color=0xd62000;break;case WorldCardPool::Silent:color=0x5ebd00;break;
    case WorldCardPool::Defect:color=0x3eb3ed;break;case WorldCardPool::Regent:color=0xe36600;break;
    case WorldCardPool::Necrobinder:color=0xcd4eed;break;case WorldCardPool::Colorless:case WorldCardPool::Event:color=0xa3a3a3;break;
    case WorldCardPool::Curse:color=0x585b61;break;case WorldCardPool::Quest:color=0x24476a;break;default:break;
    }
    Canvas back=c;back.tint_red=uint8_t(color>>16);back.tint_green=uint8_t(color>>8);back.tint_blue=uint8_t(color);
    back.ui(I8_TinyCardBack,x,y,size,size);
    Canvas description=c;description.tint_red=242;description.tint_green=235;description.tint_blue=177;
    description.ui(I8_TinyCardDescription,x,y,size,size);
    const auto type=c.a.card_face_type(card,-1,-1,&entry.facts);
    const int portrait=type==CardType::Attack?I8_TinyCardAttack:type==CardType::Power?I8_TinyCardPower:I8_TinyCardSkill;
    Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(c.opacity*64));
    shadow.ui(portrait+1,x,y,size,size);c.ui(portrait,x,y,size,size);
    shadow.ui(I8_TinyCardBannerShadow,x,y,size,size);
    const uint32_t rarity_colors[]={0xffffff,0x9c9c9c,0x9c9c9c,0x64ffff,0xffda36,0xe669ff,0x13be1a,0xf46836};
    const unsigned rarity=unsigned(card_rarity(card.kind));color=rarity<8?rarity_colors[rarity]:0xffffff;
    Canvas banner=c;banner.tint_red=uint8_t(color>>16);banner.tint_green=uint8_t(color>>8);banner.tint_blue=uint8_t(color);
    banner.ui(I8_TinyCardBanner,x,y,size,size);
}
static void history_hit(Canvas&c,const UiHistoryLayout::Point&p,int dx,int offset,HitKind kind,int value){
    const int x=p.x+dx,y=UiHistoryLayout::ViewTop+p.y-offset;
    const int left=mx(0,x),right=mn(320,x+p.w),top=mx(UiHistoryLayout::ViewTop,y),bottom=mn(UiHistoryLayout::ViewTop+UiHistoryLayout::ViewHeight,y+p.h);
    if(right>left&&bottom>top)c.hit(left,top,right-left,bottom-top,kind,value);
}
static const char* history_room_key(const UiFloorHistory&f){
    if(f.room==unsigned(WorldRoom::Boss)){const auto*d=encounter_definition(EncounterId(f.encounter));if(d)return d->key;}
    if(f.room==unsigned(WorldRoom::Event)&&f.event<OriginalEventKinds){
        const auto id=OriginalEventId(f.event);if(ui_ancient_event(id))return original_event_key(id);
    }
    switch(WorldRoom(f.room)){case WorldRoom::Monster:return "MONSTER";case WorldRoom::Elite:return "ELITE";
    case WorldRoom::Boss:return "ELITE";case WorldRoom::Rest:return "REST_SITE";case WorldRoom::Shop:return "SHOP";
    case WorldRoom::Treasure:return "TREASURE";default:return "EVENT";}
}
static int history_focus_id(const App&a,const UiRunHistory&r,const uint16_t*indices,unsigned groups){
    if(!a.pointer.visible)return a.history_presentation.focus;
    if(a.history_tooltip.source==1&&a.history_tooltip.serial==r.serial&&a.history_tooltip.contains(a.pointer.x,a.pointer.y))return a.history_tooltip.floor;
    const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit)return -1;
    if(hit->kind==HitKind::HistoryFloor&&hit->value>=0&&hit->value<r.floor_count)return hit->value;
    if(hit->kind==HitKind::HistoryRelic&&hit->value>=0&&hit->value<r.relic_count)return UiHistoryLayout::RelicFirst+hit->value;
    if(hit->kind==HitKind::HistoryCard)for(unsigned i=0;i<groups;++i)if(indices[i]==hit->value)return UiHistoryLayout::CardFirst+int(i);
    return -1;
}
static void history_tooltip(Canvas&c,const UiRunHistory&r,int focus,const UiHistoryLayout&layout,int dx,int offset){
    if(focus<0||focus>=UiHistoryLayout::CardFirst)return;
    UiHistoryLayout::Point point{};if(!layout.point(r,focus,point))return;
    const int py=point.y-offset+UiHistoryLayout::ViewTop;
    if(py+point.h<=UiHistoryLayout::ViewTop||py>=UiHistoryLayout::ViewTop+UiHistoryLayout::ViewHeight)return;
    if(focus<UiHistoryLayout::RelicFirst){journal_floor_tooltip(c,r,unsigned(focus),point.x+dx+point.w/2,py+point.h/2,1);return;}
    // NRelicBasicHolder.Model.HoverTips is drawn by the shared keyword stack,
    // including the source's associated cards and mechanic descriptions.
}
static void history_screen(Canvas&c){
    auto&a=c.a;const auto*record=a.history_at(a.history_index);if(!record){a.history_open=false;compendium_screen(c);return;}const auto&r=*record;
    title_background(c,false);c.dim(0,0,320,240,210);a.hit_count=0;
    uint16_t indices[MaxDeck]{},counts[MaxDeck]{};const unsigned groups=ui_history_groups(r,indices,counts);
    const UiHistoryLayout layout(r,groups);auto&motion=a.history_presentation;
    a.progress_page_scroll.configure(layout.height,UiHistoryLayout::ViewHeight);
    c.hit(UiHistoryLayout::Left,UiHistoryLayout::ViewTop,UiHistoryLayout::Width,UiHistoryLayout::ViewHeight,HitKind::ProgressScroll);
    Canvas page=c;page.pixels+=UiHistoryLayout::ViewTop*page.pitch;if(page.coverage)page.coverage+=UiHistoryLayout::ViewTop*page.pitch;page.clip_h=UiHistoryLayout::ViewHeight;
    page.opacity=uint8_t(div255(page.opacity*motion.alpha(a.map_clock_ms)));
    const int offset=a.progress_page_scroll.offset(),dx=motion.offset(a.map_clock_ms);
    for(unsigned i=0;i<r.floor_count;++i)history_hit(c,layout.floor(r,i),dx,offset,HitKind::HistoryFloor,int(i));
    for(unsigned i=0;i<r.relic_count;++i)history_hit(c,layout.relic(i),dx,offset,HitKind::HistoryRelic,int(i));
    for(unsigned i=0;i<groups;++i)history_hit(c,layout.card(i),dx,offset,HitKind::HistoryCard,indices[i]);
    const int focused=history_focus_id(a,r,indices,groups);motion.observe(focused,a.map_clock_ms);
    page.ui(U_TopBarIronclad+int(r.character),29+dx,2-offset,22,22);
    const auto&ch=character_definitions[r.character];const char*name=a.content.world_title("characters",ch.key);
    page.text_fitted(*name?name:ch.name,54+dx,3-offset,109,11,Gold,8);
    char text[384]{},date[64]{},time[32]{},elapsed[32]{};progress_date_text(a,r.started_at,date,sizeof date,time,sizeof time);
    ProgressTextValues values;values.seed=r.seed_text;values.date=date;values.time=time;values.hp=r.hp;values.max_hp=r.max_hp;values.gold=r.gold;values.floor=r.floor;
    progress_text(a,"run_history","INFO.DATE_TIME",values,text,sizeof text);page.text_fitted(text,168+dx,1-offset,123,8,White,8);
    progress_text(a,"run_history","INFO.SEED",values,text,sizeof text);page.text_fitted(text,168+dx,12-offset,123,8,White,8);
    const char*mode_key=r.mode==unsigned(RunMode::Custom)?"GAME_MODE.custom":r.mode==unsigned(RunMode::Daily)?"GAME_MODE.daily":"GAME_MODE.standard";
    const char*mode=frontend_text(a,"run_history",mode_key,r.mode==unsigned(RunMode::Custom)?"Custom":r.mode==unsigned(RunMode::Daily)?"Daily":"Standard");page.text_fitted(mode,168+dx,23-offset,123,8,White,8);
    page.text_fitted("v0.111.0 / 41cef1ea",168+dx,34-offset,123,8,Muted,8);
    page.picture(A_Heart,54+dx,17-offset,9,10);TextBuffer line{text,sizeof text,0};text[0]=0;line.number(r.hp);line.ch('/');line.number(r.max_hp);page.text(text,65+dx,17-offset,8,White);
    page.picture(A_Coin,103+dx,17-offset,9,10);text[0]=0;line.n=0;line.number(r.gold);page.text(text,114+dx,17-offset,8,Gold);
    page.ui(U_TopFloor,29+dx,31-offset,10,11);page.num(r.floor,42+dx,32-offset,8,White);
    progress_time_text(elapsed,sizeof elapsed,r.seconds);page.ui(I7_StatsClock,65+dx,30-offset,12,12);page.text(r.partial?"—":elapsed,79+dx,32-offset,8,White);
    for(unsigned i=0;i<r.potion_count&&i<10;++i){const int x=127+dx+int(i)*10;
        page.ui(U_PotionPlaceholder,x,30-offset,10,13);
        if(r.potions[i])page.picture(a.content.potion_image(Potion(r.potions[i])),x,30-offset,10,13);
    }
    uint32_t quests[3]{};const int journal_owner=a.progress.journal.owner(r.serial);
    for(unsigned i=0;i<a.progress.journal.count;++i){const auto&e=a.progress.journal.at(i);if(e.owner==journal_owner&&e.floor<UiHistoryFloors&&(e.kind==RunHistoryEvent::QuestCompleted||e.kind==RunHistoryEvent::FurCoat))quests[e.floor/32]|=uint32_t(1)<<(e.floor%32);}
    for(unsigned i=0;i<r.floor_count;++i){const auto&f=r.floors[i];const auto point=layout.floor(r,i);const int x=point.x+dx,y=point.y-offset;
        if(y+point.h<=0||y>=page.clip_h)continue;
        if(!i||r.floors[i-1].act!=f.act){const char*act=a.content.world_title("acts",map_colors(f.region).region);page.text_fitted(act,27+dx,y+3,30,8,Gold,8);}
        const int size=mn(19,12*int(motion.scale(int(i),a.map_clock_ms))/1000);
        if(a.content.version()>=HudUi7ContentVersion)hud_room_icon(page,history_room_key(f),x+6-size/2,y+7-size/2,size,size);
        else page.picture(f.room==unsigned(WorldRoom::Rest)?A_MapCamp:f.room==unsigned(WorldRoom::Shop)?A_MapShop:f.room==unsigned(WorldRoom::Treasure)?A_MapTreasure:f.room==unsigned(WorldRoom::Event)?A_MapEvent:A_MapCombat,x,y,12,12);
        if(quests[i/32]&(uint32_t(1)<<(i%32)))page.picture(a.content.world_image("map","QUEST"),x+7,y-1,5,5);
    }
    values.total=r.relic_count;progress_text(a,"run_history","RELIC_HISTORY.header",values,text,sizeof text);page.text_fitted(text,29+dx,layout.relic_header-offset,257,10,Gold,8);
    for(unsigned i=0;i<r.relic_count;++i){const auto point=layout.relic(i);const int x=point.x+dx,y=point.y-offset;if(y+point.h<=0||y>=page.clip_h)continue;
        const int size=12*int(motion.scale(UiHistoryLayout::RelicFirst+int(i),a.map_clock_ms))/1000;
        page.picture(a.relic_face_image(r.relics[i],r.character),x+6-size/2,y+6-size/2,size,size);
    }
    values.total=r.deck_count;progress_text(a,"run_history","DECK_HISTORY.header",values,text,sizeof text);page.text_fitted(text,29+dx,layout.deck_header-offset,257,10,Gold,8);
    for(unsigned i=0;i<groups;++i){const auto point=layout.card(i);const int x=point.x+dx,y=point.y-offset;if(y+point.h<=0||y>=page.clip_h)continue;
        const auto&entry=r.deck[indices[i]];const unsigned scale=motion.scale(UiHistoryLayout::CardFirst+int(i),a.map_clock_ms);const int size=8*int(scale)/1000;
        history_tiny_card(page,entry,x+4-size/2,y+6-size/2,size);
        text[0]=0;TextBuffer title{text,sizeof text,0};if(counts[i]>1){title.number(counts[i]);title.text("x ");}title.text(a.content.card_string(entry.card.kind));if(entry.card.upgraded)title.ch('+');
        const int label_shift=int(scale-1000)*2/500;
        page.text_fitted(text,x+10+label_shift,y+2,point.w-10-label_shift,8,entry.facts.enchantment!=EventEnchantment::None?rgb(197,127,255):entry.card.upgraded?Green:White,8);
    }
    if(a.history_index+1<a.progress.count){c.ui(I7_SettingsArrowLeft,7,111,12,24);c.action(0,100,26,49,UiAction::HistoryPrevious);}
    if(a.history_index){c.ui(I7_SettingsArrowRight,307,111,12,24);c.action(297,100,23,49,UiAction::HistoryNext);}
    progress_page_chrome(c);history_tooltip(c,r,focused,layout,dx,offset);
}
