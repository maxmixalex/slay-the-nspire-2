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
    if(hours){text.number(hours);text.ch(':');if(minutes<10)text.ch('0');}text.number(minutes);text.ch(':');
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
    else{d.number(year);d.ch('/');d.number(month+1);d.ch('/');d.number(days+1);}
    TextBuffer t{time,time_size,0};const unsigned hour=timestamp/3600%24,minute=timestamp/60%60;
    t.number(a.content.english()?(hour%12?hour%12:12):hour);t.ch(':');if(minute<10)t.ch('0');t.number(minute);
    if(a.content.english())t.text(hour<12?" AM":" PM");
}
static bool progress_hover(const App&a,int x,int y,int width,int height){return a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+width&&a.pointer.y>=y&&a.pointer.y<y+height;}
static void compendium_button(Canvas&c,unsigned index,int x,int y,int width,int height,int base,int icon,const char*prefix,UiAction action,bool bottom=false){
    auto&a=c.a;const bool focused=progress_hover(a,x,y,width,height)||(!a.pointer.visible&&a.compendium_selection==index);
    const bool pressed=focused&&a.pointer.held&&a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==int(action);
    auto&motion=a.compendium_motion[index];const unsigned scale=motion.observe(pressed?2:focused?1:0,a.map_clock_ms,bottom),alpha=motion.alpha(a.map_clock_ms);
    const int w=(width*int(scale)+500)/1000,h=(height*int(scale)+500)/1000,left=x+(width-w)/2,top=y+(height-h)/2;
    if(a.content.version()>=Ui7ContentVersion){c.ui(base+motion.previous,left,top,w,h);c.ui(base+motion.state,left,top,w,h,alpha);}
    else c.dim(left,top,w,h,150);
    c.ui(icon,left+w/2-(bottom?17:27),top+(bottom?4:23),bottom?34:53,bottom?22:40);
    char key[128]{};TextBuffer k{key,sizeof key,0};k.text(prefix);k.text(".title");
    const char*title=frontend_text(a,"main_menu_ui",key,prefix);
    c.text_fitted(title,left+4,top+(bottom?h-13:9),w-8,bottom?10:11,Gold,8,true,true);
    if(!bottom){key[0]=0;k.n=0;k.text(prefix);k.text(".description");const char*description=frontend_text(a,"main_menu_ui",key,"");c.fitted(description,left+7,top+63,w-14,h-70,8,8,White,false,true,true);}
    c.action(x,y,width,height,action);
}
static void compendium_screen(Canvas&c){
    auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,165);a.hit_count=0;
    const bool bestiary=a.bestiary_available();const int count=bestiary?4:3,width=74,gap=2,left=(320-count*width-(count-1)*gap)/2;
    const int panels[]={I7_MenuCardPanel,I7_MenuRelicPanel,I7_MenuPotionPanel,I7_MenuBestiaryPanel};
    const int icons[]={I7_MenuCardIcon,I7_MenuRelicIcon,I7_MenuPotionIcon,I7_MenuBestiaryIcon};
    const char*names[]={"COMPENDIUM_CARD_LIBRARY","COMPENDIUM_RELIC_COLLECTION","COMPENDIUM_POTION_LAB","COMPENDIUM_BESTIARY"};
    const UiAction actions[]={UiAction::LibraryCards,UiAction::LibraryRelics,UiAction::LibraryPotions,UiAction::OpenBestiary};
    for(int i=0;i<count;++i)compendium_button(c,unsigned(i),left+i*(width+gap),50,width,110,panels[i],icons[i],names[i],actions[i]);
    const int right=left+count*width+(count-1)*gap;
    const int stats_x=right-(a.progress.count?116:56);
    compendium_button(c,4,stats_x,166,56,40,I7_MenuStatsPanel,I7_MenuStatsIcon,"STATISTICS",UiAction::OpenStatistics,true);
    if(a.progress.count)compendium_button(c,5,right-56,166,56,40,I7_MenuHistoryPanel,I7_MenuHistoryIcon,"RUN_HISTORY",UiAction::OpenHistory,true);
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
static void progress_stat_entry(Canvas&c,int x,int y,int icon,const char*top,const char*bottom,const ProgressTextValues&values){
    if(y+27<=0||y>=c.clip_h)return;
    c.ui(icon,x,y+5,18,16);char text[192]{};
    if(top){progress_text(c.a,"stats_screen",top,values,text,sizeof text);c.text_fitted(text,x+22,y+2,101,9,White,8);}
    if(bottom){progress_text(c.a,"stats_screen",bottom,values,text,sizeof text);c.text_fitted(text,x+22,y+14,101,9,White,8);}
}
static void statistics_screen(Canvas&c){
    auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,210);a.hit_count=0;
    c.ui(F_SettingsTab,50,3,110,35);c.ui(F_SettingsTabStroke,50,3,110,35);
    c.text_fitted(frontend_text(a,"stats_screen","TAB_STATS.header","Statistics"),56,13,98,12,Gold,8,true);
    Canvas disabled=c;disabled.opacity=120;disabled.ui(F_SettingsTab,162,3,110,35);
    disabled.text_fitted(frontend_text(a,"stats_screen","TAB_ACHIEVEMENT.header","Achievements"),166,13,102,12,White,8,true);
    disabled.ui(I7_MenuLock,204,8,19,19); // NStatsScreen disables this tab.
    unsigned character_count=0;for(unsigned i=0;i<CharacterKinds;++i)character_count+=(a.progress.characters_started>>i)&1;
    a.progress_page_scroll.configure(185+int((character_count+1)/2)*103);
    c.hit(23,40,271,174,HitKind::ProgressScroll);
    Canvas page=progress_content_canvas(c);const int offset=a.progress_page_scroll.offset();
    page.text_fitted(frontend_text(a,"main_menu_ui","STATISTICS.OVERALL.title","Overall Stats"),29,3-offset,251,12,Gold,10);
    page.dim(26,21-offset,261,148,125);
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
        case 6:n.text("—");d.number(PotionKinds);break; // Original bottom uses the complete potion model count.
        case 7:n.text("N/A");d.number(progress_bit_count(a.progress.discovered_events,OriginalEventKinds));d.ch('/');d.number(OriginalEventKinds);break;
        case 8:n.number(a.progress.best_streak);break;
        default:break;
        }
        const int x=29+int(i%2)*130,y=24+int(i/2)*28-offset;values.amount=amount;
        progress_stat_entry(page,x,y,icons[i],top[i],nullptr,values);values.amount=other;
        progress_stat_entry(page,x,y,icons[i],nullptr,bottom[i],values);
    }
    page.text_fitted(frontend_text(a,"main_menu_ui","STATISTICS.title","Character Stats"),29,173-offset,251,12,Gold,10);
    static const unsigned order[]={0,1,3,4,2};unsigned visible=0;
    for(unsigned model:order){if(!(a.progress.characters_started&(1u<<model)))continue;
        const auto&s=a.progress.characters[model];const int x=27+int(visible%2)*132,y=192+int(visible/2)*103-offset;++visible;
        if(y>=174||y+96<=0)continue;
        page.dim(x,y,128,96,125);page.ui(U_TopBarIronclad+int(model),x+3,y+4,19,19);
        const auto&ch=character_definitions[model];const char*name=a.content.world_title("characters",ch.key);
        page.text_fitted(*name?name:ch.name,x+25,y+8,99,10,Gold,8);
        char time[32]{},best[32]{},amount[24]{},best_streak[24]{};progress_time_text(time,sizeof time,s.play_seconds);if(s.fastest_win_seconds)progress_time_text(best,sizeof best,s.fastest_win_seconds);
        ProgressTextValues values;values.playtime=time;values.fastest=s.fastest_win_seconds?best:nullptr;values.wins=s.wins;values.losses=s.losses;
        progress_stat_entry(page,x+2,y+28,I7_StatsClock,"ENTRY_CHAR_PLAYTIME.top","ENTRY_CHAR_PLAYTIME.bottom",values);
        TextBuffer b{amount,sizeof amount,0};b.number(s.highest_ascension);values.amount=amount;
        progress_stat_entry(page,x+2,y+50,I7_StatsSwords,"ENTRY_CHAR_WIN_LOSS.top","ENTRY_CHAR_WIN_LOSS.bottom",values);
        b.n=0;amount[0]=0;b.number(s.current_streak);values.amount=amount;
        progress_stat_entry(page,x+2,y+72,I7_StatsStreak,"ENTRY_CHAR_STREAK.top",nullptr,values);
        TextBuffer bs{best_streak,sizeof best_streak,0};bs.number(s.best_streak);values.amount=best_streak;
        progress_stat_entry(page,x+2,y+72,I7_StatsStreak,nullptr,"ENTRY_CHAR_STREAK.bottom",values);
    }
    progress_page_chrome(c);
}
static unsigned history_card_groups(const UiRunHistory&r,uint16_t*indices,uint16_t*counts){
    unsigned n=0;for(unsigned i=0;i<r.deck_count;++i){const auto&card=r.deck[i];unsigned at=0;
        for(;at<n;++at){const auto&other=r.deck[indices[at]];
            if(card.card.kind==other.card.kind&&card.card.upgraded==other.card.upgraded&&card.facts.enchantment==other.facts.enchantment&&card.facts.enchantment_amount==other.facts.enchantment_amount&&card.facts.energy_offset==other.facts.energy_offset&&card.facts.science_type==other.facts.science_type&&card.facts.rider==other.facts.rider&&card.facts.progress==other.facts.progress&&card.genetic==other.genetic&&card.scythe==other.scythe)break;
        }
        if(at==n){indices[n]=uint16_t(i);counts[n++]=1;}else ++counts[at];
    }return n;
}
static void history_screen(Canvas&c){
    auto&a=c.a;const auto*record=a.history_at(a.history_index);if(!record){a.history_open=false;compendium_screen(c);return;}const auto&r=*record;
    title_background(c,false);c.dim(0,0,320,240,210);a.hit_count=0;
    c.centered(frontend_text(a,"main_menu_ui","RUN_HISTORY.title","Run History"),24,10,272,16,Gold);
    uint16_t indices[MaxDeck]{},counts[MaxDeck]{};const unsigned groups=history_card_groups(r,indices,counts);
    const int relic_rows=(r.relic_count+12)/13,deck_y=105+relic_rows*19,map_y=deck_y+20+int((groups+2)/3)*18;
    a.progress_page_scroll.configure(map_y+32+int((r.floor_count+12)/13)*22);
    c.hit(27,40,266,174,HitKind::ProgressScroll);
    Canvas page=progress_content_canvas(c);const int offset=a.progress_page_scroll.offset();
    page.ui(U_TopBarIronclad+int(r.character),29,2-offset,26,26);
    const auto&ch=character_definitions[r.character];const char*name=a.content.world_title("characters",ch.key);
    page.text_fitted(*name?name:ch.name,61,5-offset,148,12,Gold,10);
    char text[256]{},date[64]{},time[32]{},elapsed[32]{};progress_date_text(a,r.started_at,date,sizeof date,time,sizeof time);
    ProgressTextValues values;values.seed=r.seed_text;values.date=date;values.time=time;values.hp=r.hp;values.max_hp=r.max_hp;values.gold=r.gold;values.floor=r.floor;
    progress_text(a,"run_history","INFO.DATE_TIME",values,text,sizeof text);page.text_fitted(text,61,20-offset,215,8,White,8);
    page.picture(A_Heart,29,35-offset,10,11);TextBuffer hp{text,sizeof text,0};text[0]=0;hp.number(r.hp);hp.ch('/');hp.number(r.max_hp);page.text(text,42,35-offset,10,White);
    page.picture(A_Coin,102,35-offset,10,11);text[0]=0;hp.n=0;hp.number(r.gold);page.text(text,115,35-offset,10,Gold);
    progress_time_text(elapsed,sizeof elapsed,r.seconds);page.ui(I7_StatsClock,184,33-offset,15,14);page.text(r.partial?"—":elapsed,201,35-offset,10,White);
    progress_text(a,"run_history","INFO.SEED",values,text,sizeof text);page.text_fitted(text,29,52-offset,258,9,White,8);
    const char*mode=frontend_text(a,"run_history","GAME_MODE.standard","Standard");page.text_fitted(mode,29,66-offset,180,9,White,8);
    text[0]=0;hp.n=0;hp.number(r.floor);page.ui(U_TopFloor,224,64-offset,12,12);page.text(text,241,66-offset,10,Gold);
    for(unsigned i=0;i<r.potion_count&&i<10;++i)if(r.potions[i])page.picture(a.content.potion_image(Potion(r.potions[i])),29+int(i)*16,79-offset,13,16);
    values.total=r.relic_count;progress_text(a,"run_history","RELIC_HISTORY.header",values,text,sizeof text);page.text_fitted(text,29,99-offset,257,10,Gold,8);
    for(unsigned i=0;i<r.relic_count;++i){const int x=29+int(i%13)*20,y=114+int(i/13)*19-offset;if(y+18<=0||y>=174)continue;
        page.picture(a.content.relic_image(r.relics[i]),x,y,17,17);progress_content_hit(c,x,y+40,19,18,HitKind::HistoryRelic,int(i));}
    values.total=r.deck_count;progress_text(a,"run_history","DECK_HISTORY.header",values,text,sizeof text);page.text_fitted(text,29,deck_y+9-offset,257,10,Gold,8);
    for(unsigned i=0;i<groups;++i){const int x=29+int(i%3)*86,y=deck_y+27+int(i/3)*18-offset;if(y+17<=0||y>=174)continue;
        const auto&card=r.deck[indices[i]];draw_card_face(page,card.card,x,y,10,14,-2,-2,-1,-1,&card.facts);
        text[0]=0;TextBuffer title{text,sizeof text,0,&a.content};if(counts[i]>1){title.number(counts[i]);title.text("x ");}title.text(a.content.card_string(card.card.kind));if(card.card.upgraded)title.ch('+');
        const bool hover=progress_hover(a,x,y+40,83,17);page.text_fitted(text,x+13,y+2,70,9,hover?Gold:White,8);
        progress_content_hit(c,x,y+40,83,17,HitKind::HistoryCard,indices[i]);
    }
    for(unsigned i=0;i<r.floor_count;++i){const auto&floor=r.floors[i];const int x=31+int(i%13)*20,y=map_y+14+int(i/13)*22-offset;if(y+20<=0||y>=174)continue;
        const int icon=floor.room==unsigned(WorldRoom::Monster)?A_MapCombat:floor.room==unsigned(WorldRoom::Elite)||floor.room==unsigned(WorldRoom::Boss)?A_MapFinal:floor.room==unsigned(WorldRoom::Rest)?A_MapCamp:floor.room==unsigned(WorldRoom::Shop)?A_MapShop:floor.room==unsigned(WorldRoom::Treasure)?A_MapTreasure:A_MapEvent;
        page.picture(icon,x,y,16,16);
        progress_content_hit(c,x,y+40,18,19,HitKind::HistoryFloor,int(i));
    }
    if(a.history_index+1<a.progress.count){c.ui(I7_SettingsArrowLeft,7,111,12,24);c.action(0,100,26,49,UiAction::HistoryPrevious);}
    if(a.history_index){c.ui(I7_SettingsArrowRight,307,111,12,24);c.action(297,100,23,49,UiAction::HistoryNext);}
    progress_page_chrome(c);
    if(a.pointer.visible){const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(hit&&hit->kind==HitKind::HistoryFloor&&hit->value>=0&&hit->value<r.floor_count){
        const auto&f=r.floors[hit->value];values.floor=f.floor;values.hp=f.hp;values.max_hp=f.max_hp;values.gold=f.gold;values.turns=f.turns;
        const int x=mn(161,mx(4,hit->x)),y=hit->y>125?hit->y-64:hit->y+21;c.nine_ui(I7_TinyPanel,x,y,154,59,3,3,3,3);
        progress_text(a,"run_history","MAP_POINT_HISTORY.header",values,text,sizeof text);c.text_fitted(text,x+7,y+5,140,10,Gold,8);
        progress_text(a,"run_history","MAP_POINT_HISTORY.player_stats",values,text,sizeof text);c.text_fitted(text,x+7,y+22,140,9,White,8);
        progress_text(a,"run_history","MAP_POINT_HISTORY.turnsTaken",values,text,sizeof text);if(f.turns)c.text_fitted(text,x+7,y+39,140,9,White,8);
    }}
}
