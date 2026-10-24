// The library is an independent submenu, with its original narrow sidebar.
// Grid geometry uses the actual 1680x1260 4:3 viewport. Sidebar labels retain
// the 8px readable font floor, with their controls fitted to its 55px width.
static void library_hover_box(Canvas&c,const char*title,const char*body,int x,int y,int width=132){
    const int title_height=*title?16:0,lines=c.paragraph(body,0,0,width-16,0,0,8);
    const int height=mn(218,16+title_height+lines*11);x=mx(3,mn(317-width,x));y=mx(3,mn(237-height,y));
    if(!c.nine_ui(U_HoverTip,x,y,width,height,7,7,7,7)){c.rect(x,y,width,height,Panel);c.box(x,y,width,height,Gold);}
    if(*title)c.text_fitted(title,x+8,y+5,width-16,10,Gold,8);
    c.paragraph(body,x+8,y+7+title_height,width-16,mx(0,(height-12-title_height)/11),0,8,White);
}
static void library_edge_fade(Canvas&c,int x,int width){
    for(int y=0;y<12;++y){const unsigned alpha=unsigned(230*(12-y)/12);c.dim(x,y,width,1,alpha);c.dim(x,239-y,width,1,alpha);}
}
static bool library_control_focused(const App&a,int control,int x,int y,int w,int h){
    if(a.pointer.visible)return a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h;
    return a.card_library.control==control;
}
static LocalizedValue library_count_value(const void*opaque,const char*key){return wt_equal(key,"Amount")?LocalizedValue{true,*static_cast<const int*>(opaque),1,nullptr}:LocalizedValue{};}
static void library_sort_button(Canvas&c,int control,int sorter,const char*key,int y){
    auto&a=c.a;const bool focus=library_control_focused(a,control,3,y,49,11);
    Canvas button=c;if(!focus)button.brightness=200;button.ui(U_RewardButton,3,y,49,11);
    c.text_fitted(a.content.locale_string("gameplay_ui",key),5,y,37,8,Gold,8);
    const int image=a.content.ui_image(I6_SortDescending);
    ui_picture_transform(c,image,47,y+5,8,8,0,a.card_library.sort_descending&(1u<<sorter)?1024:-1024);
    c.hit(3,y,49,11,HitKind::LibraryControl,control);
}
static void card_library_screen(Canvas&c){
    auto&a=c.a;CardKind ids[CardKinds];const int count=int(a.library_items(ids));
    auto&grid=a.configure_card_grid(CardGridSurface::Library,count);CardGridPainter painter{c,grid};
    a.library_selected=mn(mx(0,a.library_selected),mx(0,count-1));
    int portraits[24];unsigned prepared=0;const EventCardFacts canonical{};
    for(int i=grid.first();i<grid.last()&&prepared<24;++i)portraits[prepared++]=a.card_face_image({ids[i],uint8_t(a.library_upgraded&&card_upgradeable({ids[i],0}))},-1,-1,&canonical);
    a.content.prepare_images(portraits,prepared);
    for(int i=grid.first();i<grid.last();++i){
        const bool focused=!a.pointer.visible&&a.card_library.control<0&&i==a.library_selected;
        painter.card(i,{ids[i],uint8_t(a.library_upgraded&&card_upgradeable({ids[i],0}))},focused,true,-2,-2,-1,-1,&canonical,false,a.card_library.was_seen(ids[i]));
        painter.hit(i,HitKind::LibraryItem,i);
    }
    painter.finish();library_edge_fade(c,55,265);card_grid_scrollbar(c,grid,CardGridSurface::Library);
    if(!count)c.centered(a.content.locale_string("card_library","NO_RESULTS"),70,114,220,10,White);
    char count_text[96]{};LocalizedTemplate count_label(count_text,sizeof count_text,library_count_value,&count);count_label.format(a.content.locale_string("card_library","CARD_COUNT"));
    c.text_fitted(count_text,246,226,68,8,Muted,8,true);
    c.dim(0,0,55,240,128);c.rect(0,0,55,240,rgb(36,49,53));c.rect(54,0,1,240,rgb(60,83,87));
    const bool editing=a.library_search_editing;const bool search_focus=library_control_focused(a,LibrarySearch,3,3,49,13);
    c.rect(3,3,49,13,rgb(23,32,35));if(editing||search_focus)c.box(3,3,49,13,Muted);
    const char*query=*a.card_library.draft?a.card_library.draft:a.content.locale_string("card_library","SEARCH_PLACEHOLDER");
    c.text_fitted(query,5,4,36,8,*a.card_library.draft?White:Muted,8);
    if(editing&&(a.map_clock_ms/500)%2==0)c.rect(mn(38,5+c.width(a.card_library.draft,8)),5,1,9,White);
    c.hit(3,3,38,13,HitKind::LibraryControl,LibrarySearch);c.ui(I6_CloseX,42,5,8,8);c.hit(41,3,11,13,HitKind::LibraryControl,LibraryClearSearch);
    static constexpr int filters[]={1,2,5,6,4,3,7,8};
    static constexpr int pool_images[]={U_TopBarIronclad,U_TopBarSilent,U_TopBarRegent,U_TopBarNecrobinder,U_TopBarDefect,U_ColorlessEnergy,I7_LibraryAncient,I7_LibraryMisc};
    int hovered=-1;
    for(int i=0;i<8;++i){const int x=3+(i%4)*12,y=18+(i/4)*12,control=LibraryPoolFirst+i;
        const bool focus=library_control_focused(a,control,x,y,12,12),selected=a.library_filter==filters[i];
        Canvas image=c;if(!selected&&!focus)image.brightness=140;const int size=focus?15:12;
        image.ui(pool_images[i],x+6-size/2,y+6-size/2,size,size);c.hit(x,y,12,12,HitKind::LibraryControl,control);
        if(focus)hovered=control;
    }
    library_sort_button(c,LibrarySortType,0,"SORT_TYPE",48);
    for(int i=0;i<4;++i){const int x=4+i*11,y=58,control=LibraryTypeFirst+i;
        const bool focused=library_control_focused(a,control,x,y,11,13),checked=a.card_library.types&(1u<<i);
        Canvas image=c;if(!checked&&!focused)image.brightness=190;image.ui(I7_LibraryType+i,x,y,11,13);
        if(checked)c.ui(I7_LibraryTypeOutline,x,y,11,13);c.hit(x,y,11,13,HitKind::LibraryControl,control);if(focused)hovered=control;
    }
    library_sort_button(c,LibrarySortRarity,1,"SORT_RARITY",75);
    const char*rarity_keys[]={"RARITY_COMMON","RARITY_UNCOMMON","RARITY_RARE","RARITY_OTHER"};
    const uint16_t rarity_colors[]={White,rgb(135,206,235),rgb(239,200,81),rgb(238,130,238)};
    const bool rarity_enabled=a.library_filter>0&&a.library_filter<7;
    for(int i=0;i<4;++i){const int y=85+i*76/9,control=LibraryRarityFirst+i;
        const bool focus=library_control_focused(a,control,4,y,48,9),checked=a.card_library.rarities&(1u<<i);
        Canvas row=c;if(!rarity_enabled)row.opacity=90;row.ui(checked?F_SettingsTicked:F_SettingsUnticked,3,y-1,10,10);
        row.text_fitted(a.content.locale_string("card_library",rarity_keys[i]),14,y,38,8,focus?Gold:rarity_colors[i],8);
        c.hit(3,y,49,9,HitKind::LibraryControl,control,rarity_enabled);if(focus&&rarity_enabled)hovered=control;
    }
    library_sort_button(c,LibrarySortCost,2,"SORT_COST",124);
    const char*costs[]={"0","1","2","3+","X"};
    for(int i=0;i<5;++i){const int x=3+i*10,y=133,control=LibraryCostFirst+i;
        const bool focus=library_control_focused(a,control,x,y,10,12),checked=a.card_library.costs&(1u<<i);
        Canvas image=c;if(!checked&&!focus)image.brightness=190;image.ui(I7_LibraryCost,x,y,10,12);if(checked)c.ui(I7_LibraryCostOutline,x,y,10,12);
        c.text_fitted(costs[i],x,y+1,10,8,White,8,true);c.hit(x,y,10,12,HitKind::LibraryControl,control);if(focus)hovered=control;
    }
    library_sort_button(c,LibrarySortAlphabet,3,"SORT_ALPHABET",148);
    const bool stats_focus=library_control_focused(a,LibraryViewStats,3,212,49,12);
    c.ui(a.card_library.show_stats?F_SettingsTicked:F_SettingsUnticked,3,212,11,11);
    c.text_fitted(a.content.locale_string("card_library","VIEW_STATS"),15,213,37,8,stats_focus?Gold:White,8);
    c.hit(3,211,49,12,HitKind::LibraryControl,LibraryViewStats);
    const bool upgrade_focus=library_control_focused(a,LibraryViewUpgrades,3,224,49,13);
    c.ui(a.library_upgraded?F_SettingsTicked:F_SettingsUnticked,3,224,11,11);
    c.text_fitted(a.content.locale_string("card_library","VIEW_UPGRADES"),15,225,37,8,upgrade_focus?Gold:White,8);
    c.hit(3,223,49,14,HitKind::LibraryControl,LibraryViewUpgrades);
    native_original_back(c,a.card_library.opened);
    if(hovered>=0&&!a.pointer.held){
        const char*body="";char name[128]{};
        if(hovered>=LibraryPoolFirst&&hovered<=LibraryPoolLast){const int pool=hovered-LibraryPoolFirst;
            if(pool<5){static constexpr uint8_t characters[]={0,1,3,4,2};body=a.content.world_title("characters",character_definitions[characters[pool]].key);}
            else{const char*keys[]={"POOL_COLORLESS_TIP","POOL_ANCIENT_TIP","POOL_MISC_TIP"};body=a.content.locale_string("card_library",keys[pool-5]);}
        }else if(hovered>=LibraryTypeFirst&&hovered<=LibraryTypeLast){const char*keys[]={"TYPE_ATTACK_TIP","TYPE_SKILL_TIP","TYPE_POWER_TIP","TYPE_OTHER_TIP"};body=a.content.locale_string("card_library",keys[hovered-LibraryTypeFirst]);}
        else if(hovered>=LibraryRarityFirst&&hovered<=LibraryRarityLast){const char*keys[]={"RARITY_COMMON_TIP","RARITY_UNCOMMON_TIP","RARITY_RARE_TIP","RARITY_OTHER_TIP"};body=a.content.locale_string("card_library",keys[hovered-LibraryRarityFirst]);}
        else if(hovered>=LibraryCostFirst&&hovered<=LibraryCostLast){const char*keys[]={"COST_ZERO_TIP","COST_ONE_TIP","COST_TWO_TIP","COST_THREE_TIP","COST_X_TIP"};body=a.content.locale_string("card_library",keys[hovered-LibraryCostFirst]);}
        const HitRegion*hit=nullptr;for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::LibraryControl&&a.hits[i].value==hovered){hit=&a.hits[i];break;}
        if(*body&&hit)library_hover_box(c,name,body,67,hit->y);
    }
}
