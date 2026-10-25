// Original card_library.tscn at the 1680x1260 FourByThree viewport.
// Labels keep the scene's4/5/6px font bands. Their single-line source boxes
// never pass through the general card fitter's5px minimum.
static void library_hover_box(Canvas&c,const char*title,const char*body,int x,int y,int width=SourceHoverWidth){source_hover_box(c,title,body,x,y,width);}
static void library_edge_fade(Canvas&c,int x,int width){
    for(int y=0;y<12;++y){const unsigned alpha=unsigned(230*(12-y)/12);c.dim(x,y,width,1,alpha);c.dim(x,239-y,width,1,alpha);}
}
static bool library_control_focused(const App&a,int control,int x,int y,int w,int h){
    if(a.pointer.visible)return a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h;
    return a.card_library.control==control;
}
static bool library_control_pressed(const App&a,int control){
    return a.pointer.held&&a.pointer.captured.kind==HitKind::LibraryControl&&a.pointer.captured.value==control&&a.pointer.context==a.input_context();
}
static LocalizedValue library_count_value(const void*opaque,const char*key){return wt_equal(key,"Amount")?LocalizedValue{true,*static_cast<const int*>(opaque),1,nullptr}:LocalizedValue{};}
// Scale the existing native glyph about the Label pivot for its source1.1
// hover. This changes no font band and needs no temporary bitmap or cache.
static void library_label_glyph(Canvas&c,Glyph g,int x,int y,uint16_t color,int pivot_x,int pivot_y,int scale){
    if(scale==1000){c.glyph(g,x,y,color);return;}if(!g.pixels)return;
    const auto project=[&](int value,int pivot){const int q=(value-pivot)*scale;return pivot+(q+(q<0?-500:500))/1000;};
    unsigned column=0,row=0;const auto emit=[&](unsigned alpha){
        if(alpha){const int left=project(x+int(column),pivot_x),right=project(x+int(column)+1,pivot_x),top=project(y+int(row),pivot_y),bottom=project(y+int(row)+1,pivot_y);
            for(int yy=mx(0,top);yy<mn(c.clip_h,bottom);++yy)for(int xx=mx(0,left);xx<mn(c.clip_w,right);++xx)c.paint(yy*c.pitch+xx,color,alpha);}
        if(++column==g.w){column=0;++row;}
    };
    if(g.encoded_size){for(unsigned i=0;i+1<g.encoded_size&&row<g.h;i+=2)for(unsigned n=0;n<g.pixels[i]&&row<g.h;++n)emit(g.pixels[i+1]);}
    else for(unsigned i=0;i<unsigned(g.w)*g.h;++i)emit(g.pixels[i]);
}
// The source Label is vertically centered and does not clip its own outline.
// Both shadow and glyph passes reuse the entire text, with no ellipsis and
// no substituted face. The bold family is the original KreonBold variation.
static int library_fixed_label(Canvas c,const char*text,int x,int y,int w,int h,int requested,unsigned face,uint16_t color,
                               unsigned shadow_alpha=128,int shadow_x=1,int shadow_y=1,int outline=0,bool center=false,int scale=1000){
    c.font_face=uint8_t(face);c.bold_face=5;
    const int size=c.a.content.font_size(requested,face),width=c.width(text,size),left=x+(center?(w-width)/2:0),top=y+(h-size-2)/2;
    const auto draw=[&](int dx,int dy,bool shadow){
        Canvas ink=c;if(shadow)ink.opacity=uint8_t(div255(unsigned(ink.opacity)*shadow_alpha));
        RichReader reader(text);RichChar ch{};int at=left;
        while(reader.next(ch)){
            const auto g=c.a.content.glyph(ch.codepoint,size,c.font_style(ch));
            const uint16_t rich=ch.style.color==1?rgb(239,200,81):ch.style.color==3?rgb(135,206,235):color;
            if(ch.index<c.text_visible_characters)library_label_glyph(ink,g,at+dx,top+dy,shadow?0:rich,x,y+h/2,scale);
            at+=g.advance;
        }
    };
    if(outline){for(int dy=-outline;dy<=outline;++dy)for(int dx=-outline;dx<=outline;++dx)
        if((dx||dy)&&dx*dx+dy*dy<=outline*outline)draw(dx,dy,true);}
    else if(shadow_alpha)draw(shadow_x,shadow_y,true);
    draw(0,0,false);return width;
}
static void library_sort_button(Canvas&c,int control,int sorter,const char*key,int source_y){
    auto&a=c.a;const int x=reference_viewport::project(16),y=reference_viewport::project(source_y),w=reference_viewport::project(256),h=reference_viewport::project(42);
    const bool focus=library_control_focused(a,control,x,y,w,h),pressed=library_control_pressed(a,control);
    Canvas button=c;button.brightness=uint8_t(div255(unsigned(button.brightness)*204));
    const int width=reference_viewport::project(pressed?256:269),height=reference_viewport::project(focus&&!pressed?44:pressed?40:42);
    small_material_icon(button,a.content.ui_image(U_RewardButton),x+(w-width)/2,y+(h-height)/2,width,height,focus&&!pressed?1000:800,focus&&!pressed?1000:800,true);
    const int label_x=reference_viewport::project(24),label_h=reference_viewport::project(48);
    const int text_width=library_fixed_label(c,a.content.world_string("gameplay_ui",key),label_x,y,w,label_h,4,source_label_face(a.content.english(),5),rgb(239,200,81),128,0,0,reference_viewport::project(12));
    Canvas arrow=c;arrow.tint_red=239;arrow.tint_green=200;arrow.tint_blue=81;
    const int image=a.content.ui_image(I6_SortDescending),size=reference_viewport::project(36),arrow_x=label_x+text_width+reference_viewport::project(12);
    ui_picture_transform(arrow,image,arrow_x+size/2,y+label_h/2,size,size,0,a.card_library.sort_descending&(1u<<sorter)?1024:-1024);
    c.hit(x,y,w,h,HitKind::LibraryControl,control);
}
static void library_search(Canvas&c){
    auto&a=c.a;const int x=3,y=3,w=49,h=9;
    const bool editing=a.library_search_editing,focus=library_control_focused(a,LibrarySearch,x,y,w,h);
    c.rect(x,y,w,h,rgb(23,32,35));if(editing||focus)c.box(x,y,w,h,rgb(153,156,150));
    const char*query=*a.card_library.draft?a.card_library.draft:a.content.locale_string("card_library","SEARCH_PLACEHOLDER");
    // LineEdit displays plain text and scrolls to the caret. Brackets typed
    // into a search must not become RichReader markup or an ellipsis.
    const int size=a.content.font_size(5);int width=0;for(const char*p=query;*p;)width+=a.content.glyph_metrics(utf8(p),size).advance;
    constexpr int text_width=36;const int scroll=*a.card_library.draft?mx(0,width-text_width+1):0;
    Canvas field=c;field.pixels+=4*field.pitch+5;if(field.coverage)field.coverage+=4*field.pitch+5;field.clip_w=text_width;field.clip_h=7;
    int at=-scroll;unsigned index=0;for(const char*p=query;*p;){const uint32_t cp=utf8(p);const auto g=a.content.glyph(cp,size);
        if(index++<field.text_visible_characters)field.character(cp,g,at,0,*a.card_library.draft?rgb(255,255,255):rgb(153,156,150));at+=g.advance;}
    if(editing&&(a.map_clock_ms/500)%2==0)field.rect(mn(text_width-1,width-scroll),0,1,6,rgb(255,255,255));
    c.hit(x,y,40,h,HitKind::LibraryControl,LibrarySearch);c.ui(I6_CloseX,44,4,6,6);c.hit(43,y,9,h,HitKind::LibraryControl,LibraryClearSearch);
}
static void library_tick_label(Canvas&c,int control,const char*key,int source_y,bool checked){
    auto&a=c.a;const int y=reference_viewport::project(source_y),h=reference_viewport::project(42);
    const bool pressed=library_control_pressed(a,control),focus=library_control_focused(a,control,3,y,49,h);
    c.ui(checked?F_SettingsTicked:F_SettingsUnticked,4,y+1,6,6);
    Canvas label=c;if(pressed)label.brightness=uint8_t(div255(unsigned(label.brightness)*191));
    library_fixed_label(label,a.content.locale_string("card_library",key),reference_viewport::project(60),y,40,h,4,source_label_face(a.content.english(),5),rgb(239,200,81),128,1,1,0,false,focus&&!pressed?1100:1000);
    c.hit(3,y,49,h,HitKind::LibraryControl,control);
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
        painter.card(i,{ids[i],uint8_t(a.library_upgraded&&card_upgradeable({ids[i],0}))},focused,true,-2,-2,-1,-1,&canonical,false,a.card_library.was_seen(ids[i]),a.library_upgraded);
        painter.hit(i,HitKind::LibraryItem,i);
    }
    painter.finish();library_edge_fade(c,55,265);card_grid_scrollbar(c,grid,CardGridSurface::Library);
    if(!count)library_fixed_label(c,a.content.locale_string("card_library","NO_RESULTS"),54,0,266,240,6,0,rgb(255,246,226),64,1,0,0,true);
    char count_text[96]{};LocalizedTemplate count_label(count_text,sizeof count_text,library_count_value,&count);count_label.format(a.content.locale_string("card_library","CARD_COUNT"));
    Canvas counter=c;counter.opacity=uint8_t(div255(unsigned(counter.opacity)*129));
    library_fixed_label(counter,count_text,reference_viewport::project(1556),reference_viewport::project(1198),reference_viewport::project(102),reference_viewport::project(31),4,0,rgb(255,246,226),64,1,0,0,true);
    Canvas panel=c;panel.opacity=uint8_t(div255(unsigned(panel.opacity)*128));panel.rect(0,0,55,240,rgb(46,66,71));c.rect(55,0,1,240,rgb(60,83,87));
    library_search(c);
    static constexpr int filters[]={1,2,5,6,4,3,7,8};
    static constexpr int pool_images[]={U_TopBarIronclad,U_TopBarSilent,U_TopBarRegent,U_TopBarNecrobinder,U_TopBarDefect,U_ColorlessEnergy,I7_LibraryAncient,I7_LibraryMisc};
    int hovered=-1;
    for(int i=0;i<8;++i){const int x=reference_viewport::project(16+(i%4)*64),y=reference_viewport::project(76+(i/4)*64),control=LibraryPoolFirst+i;
        const bool focus=library_control_focused(a,control,x,y,12,12),selected=a.library_filter==filters[i],pressed=library_control_pressed(a,control);
        const int scale=selected?1100:950,multiplier=!selected&&pressed?800:focus?1200:1000;
        const int size=(56*4*scale*multiplier+10500000)/21000000,cx=reference_viewport::project(48+(i%4)*64),cy=reference_viewport::project(108+(i/4)*64);
        Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*64));shadow.ui(pool_images[i],cx-size/2+1,cy-size/2+1,size,size);
        small_material_icon(c,a.content.ui_image(pool_images[i]),cx-size/2,cy-size/2,size,size,selected?1000:300,selected?1000:550,true);
        c.hit(x,y,12,12,HitKind::LibraryControl,control);if(focus)hovered=control;
    }
    library_sort_button(c,LibrarySortType,0,"SORT_TYPE",216);
    for(int i=0;i<4;++i){const int x=reference_viewport::project(24+i*48),y=reference_viewport::project(262),control=LibraryTypeFirst+i;
        const bool focused=library_control_focused(a,control,x,y,9,10),checked=a.card_library.types&(1u<<i);
        Canvas image=c;if(!checked&&!focused)image.brightness=190;image.ui(I7_LibraryType+i,x,y,9,10);
        if(checked){Canvas outline=c;outline.tint_red=239;outline.tint_green=200;outline.tint_blue=81;outline.ui(I7_LibraryTypeOutline,x,y,9,10);}
        c.hit(x,y,9,10,HitKind::LibraryControl,control);if(focused)hovered=control;
    }
    library_sort_button(c,LibrarySortRarity,1,"SORT_RARITY",338);
    const char*rarity_keys[]={"RARITY_COMMON","RARITY_UNCOMMON","RARITY_RARE","RARITY_OTHER"};
    const uint8_t rarity_tints[][3]={{255,255,255},{135,206,235},{239,200,81},{238,130,238}};
    const bool rarity_enabled=a.library_filter>0&&a.library_filter<7;
    for(int i=0;i<4;++i){const int y=reference_viewport::project(384+i*38),control=LibraryRarityFirst+i,h=reference_viewport::project(38);
        const bool focus=library_control_focused(a,control,3,y,49,h),checked=a.card_library.rarities&(1u<<i);
        Canvas row=c;if(!rarity_enabled)row.brightness=uint8_t(div255(unsigned(row.brightness)*128));
        row.ui(checked?F_SettingsTicked:F_SettingsUnticked,2,y-1,8,8);
        Canvas label=row;label.tint_red=rarity_tints[i][0];label.tint_green=rarity_tints[i][1];label.tint_blue=rarity_tints[i][2];
        if(library_control_pressed(a,control))label.brightness=uint8_t(div255(unsigned(label.brightness)*191));
        library_fixed_label(label,a.content.locale_string("card_library",rarity_keys[i]),reference_viewport::project(68),y,37,h,4,source_label_face(a.content.english(),5),rgb(255,246,226),128,1,1,0,false,focus&&rarity_enabled&&!library_control_pressed(a,control)?1100:1000);
        c.hit(3,y,49,h,HitKind::LibraryControl,control,rarity_enabled);if(focus&&rarity_enabled)hovered=control;
    }
    library_sort_button(c,LibrarySortCost,2,"SORT_COST",558);
    const char*costs[]={"0","1","2","3+","X"};
    for(int i=0;i<5;++i){const int x=reference_viewport::project(24+i*48),y=reference_viewport::project(600),control=LibraryCostFirst+i;
        const bool focus=library_control_focused(a,control,x,y,9,9),checked=a.card_library.costs&(1u<<i);
        Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*64));shadow.ui(I7_LibraryCost,x+1,y+1,10,10);
        Canvas image=c;image.tint_red=85;image.tint_green=148;image.tint_blue=165;
        small_material_icon(image,a.content.ui_image(I7_LibraryCost),x,y,10,10,1000,1200,true);
        if(checked){Canvas outline=c;outline.tint_red=239;outline.tint_green=200;outline.tint_blue=81;outline.ui(I7_LibraryCostOutline,x,y,10,10);}
        library_fixed_label(c,costs[i],x,y,10,10,5,source_label_face(a.content.english(),5),rgb(255,246,226),128,0,0,reference_viewport::project(12),true);
        c.hit(x,y,9,9,HitKind::LibraryControl,control);if(focus)hovered=control;
    }
    library_sort_button(c,LibrarySortAlphabet,3,"SORT_ALPHABET",666);
    library_tick_label(c,LibraryViewStats,"VIEW_STATS",1156,a.card_library.show_stats);
    library_tick_label(c,LibraryViewUpgrades,"VIEW_UPGRADES",1202,a.library_upgraded);
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
