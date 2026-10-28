static bool collection_ancient_known(const App&a,unsigned category){
    static constexpr OriginalEventId events[]={OriginalEventId::Neow,OriginalEventId::Orobas,OriginalEventId::Pael,OriginalEventId::Tezcatara,OriginalEventId::Nonupeipe,OriginalEventId::Tanx,OriginalEventId::Vakuu,OriginalEventId::Darv};
    if(category>=8&&category<16){const unsigned id=unsigned(events[category-8])-1;if(a.progress.discovered_events[id/8]&(1u<<(id%8)))return true;}
    for(unsigned row=0;row<a.collection.row_count;++row){const auto&r=a.collection.rows[row];
        if(r.category==category)for(unsigned i=0;i<r.count;++i)if(a.library_item_seen(1,a.collection.ids[r.first+i]))return true;
    }return false;
}
static LocalizedValue collection_header_value(const void*opaque,const char*key){
    if(wt_equal(key,"Ancient"))return {true,0,1,static_cast<const char*>(opaque)};return {};
}
static const char*collection_header(Canvas&c,unsigned category,char*out,unsigned capacity){
    static constexpr const char*relic_keys[]={"STARTER","COMMON","UNCOMMON","RARE","SHOP","ANCIENT","EVENT"};
    static constexpr const char*potion_keys[]={"COMMON","UNCOMMON","RARE","SPECIAL"};
    if(c.a.library_tab==2)return c.a.content.locale_string("potion_lab",potion_keys[category<4?category:0]);
    if(category<7)return c.a.content.locale_string("relic_collection",relic_keys[category]);
    static constexpr const char*ancients[]={"NEOW","OROBAS","PAEL","TEZCATARA","NONUPEIPE","TANX","VAKUU","DARV"};
    const char*name=collection_ancient_known(c.a,category)?c.a.content.world_title("ancients",ancients[category-8]):c.a.content.locale_string("relic_collection","UNKNOWN_ANCIENT");
    LocalizedTemplate formatter(out,capacity,collection_header_value,name);formatter.for_language(c.a.content.language()).format(c.a.content.locale_string("relic_collection","ANCIENT_SUBCATEGORY"));return out;
}
static void collection_header_line(Canvas&c,const char*text,int x,int y,bool relic){
    // Both category scenes fix their body to24 source pixels; inline28px
    // titles project to the same5px band at1680x1260. Relic headings use
    // Kreon Regular even for [b], while PotionCategory supplies Kreon Bold.
    // MegaRichTextLabel substitutes zhs bold spans with Source Han Serif.
    // Header.clip_contents=false and autowrap_mode=0: the full heading may
    // extend past its1020px category width, without fitting or ellipsizing.
    const int size=8;
    const int shadow_x=reference_viewport::project(relic?3:2),shadow_y=reference_viewport::project(relic?2:3);
    for(unsigned pass=0;pass<2;++pass){
        Canvas ink=c;if(!pass)ink.opacity=uint8_t(div255(c.opacity*128));
        RichReader reader(text);RichChar ch{};int advance64=0;unsigned previous_face=0;
        while(reader.next(ch)){
            const unsigned face=ch.style.bold&&(!relic||!c.a.content.original_label_fonts())?5u:0u;
            const auto glyph=c.a.content.glyph(ch.codepoint,size,face);
            if(advance64&&previous_face==face)advance64+=c.a.content.glyph_kerning64(ch.previous,ch.codepoint,size,face);
            const uint16_t color=ch.style.color==1?rgb(239,200,81):ch.style.color==3?rgb(135,206,235):rgb(255,246,226);
            ink.character(ch.codepoint,glyph,x+text_pixel(advance64)+(!pass?shadow_x:0),y+(!pass?shadow_y:0),pass?color:0,!pass);
            advance64+=glyph.advance64?glyph.advance64:int(glyph.advance)*64;previous_face=face;
        }
    }
}
static int collection_item_scale(App&a,int index,bool focused,bool relic){
    return a.collection.observe_hover(index,focused,relic,a.map_clock_ms);
}
static void collection_tooltip(Canvas&c,int index,int x,int y,int width){
    auto&a=c.a;if(index<0||index>=a.collection.count)return;
    const int id=a.collection.ids[index];const bool relic=a.library_tab==1,seen=a.library_item_seen(a.library_tab,id);
    if(seen)return; // The shared source hover stack also includes related models.
    const char*title="";char body[2048]{};
    if(seen){
        if(relic){title=a.relic_title(RelicId(id));a.relic_description(RelicId(id),body,sizeof body);}
        else{title=potion_name(id+1,&a.content);ui_potion_description(a,id+1,body,sizeof body);}
    }else{
        const char*title_key=relic?"COMPENDIUM_RELIC_COLLECTION.unknown.title":"POTION_LAB_COLLECTION.unknown.title";
        const char*body_key=relic?"COMPENDIUM_RELIC_COLLECTION.unknown.description":"POTION_LAB_COLLECTION.unknown.description";
        title=a.content.locale_string("main_menu_ui",title_key);TextBuffer text{body,sizeof body,0};text.text(a.content.locale_string("main_menu_ui",body_key));
    }
    library_hover_box(c,title,body,x,y,width);
}
static void collection_screen(Canvas&c){
    auto&a=c.a;a.prepare_collection();auto&grid=a.card_grids[unsigned(CardGridSurface::Library)];const bool relic=a.library_tab==1;
    const int offset=grid.offset(),base_size=relic?24:17;int focused=-1,focus_x=0,focus_y=0;
    // The category width is 1020 source pixels, centered in the 1680 viewport.
    for(unsigned i=0;i<a.collection.category_count;++i){const int y=a.collection.category_y[i]-offset;const unsigned category=a.collection.category_kind[i];
        if(y<-12||y>=240)continue;char text[512];const char*header=collection_header(c,category,text,sizeof text);
        const int icon=category>=8?collection_assets::AncientIcon[category-8]:-1;
        if(icon>=0)c.ui(icon,47,y-2,14,14);
        collection_header_line(c,header,icon>=0?64:47,y,relic);
    }
    int images[144];unsigned prepared=0;
    for(unsigned row=0;row<a.collection.row_count;++row){const auto&r=a.collection.rows[row];const int y=r.y-offset;
        if(y<-base_size-4||y>=244)continue;
        for(unsigned column=0;column<r.count&&prepared+2<144;++column){const int id=a.collection.ids[r.first+column];
            images[prepared++]=relic?a.relic_face_image(RelicId(id)):a.content.potion_image(Potion(id+1));
            const int outline=relic?(id==WorldRelicYummyCookie?collection_assets::CookieOutline[a.canonical_cookie_variant()]:collection_assets::RelicOutline[id]):collection_assets::PotionOutline[id];if(outline>=0)images[prepared++]=a.content.ui_image(outline);
        }
    }a.content.prepare_images(images,prepared);
    for(unsigned row=0;row<a.collection.row_count;++row){const auto&r=a.collection.rows[row];const int y=r.y-offset;
        if(y<-base_size-4||y>=244)continue;
        for(unsigned column=0;column<r.count;++column){const int index=r.first+column,id=a.collection.ids[index];
            const int x=51+int(column)*30,hit_size=relic?23:21;
            const bool mouse=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+hit_size+2&&a.pointer.y>=y-2&&a.pointer.y<y+hit_size+2;
            const bool selected=!a.pointer.held&&(mouse||(!a.pointer.visible&&index==a.library_selected));
            const int scale=collection_item_scale(a,index,selected,relic),size=base_size*scale/4096,cx=x+hit_size/2,cy=y+hit_size/2;
            int outline=relic?collection_assets::RelicOutline[id]:collection_assets::PotionOutline[id];
            if(relic&&id==WorldRelicYummyCookie)outline=collection_assets::CookieOutline[a.canonical_cookie_variant()];
            const bool seen=a.library_item_seen(a.library_tab,id);
            if(outline>=0){Canvas border=c;const unsigned character=a.library_item_character(a.library_tab,id);
                static constexpr uint8_t colors[6][3]={{0,0,0},{255,85,85},{127,255,0},{255,165,24},{255,120,160},{135,206,235}};
                border.opacity=seen&&character?168:128;
                if(seen){border.tint_red=colors[character][0];border.tint_green=colors[character][1];border.tint_blue=colors[character][2];}
                border.ui(outline,cx-size/2,cy-size/2,size,size);}
            Canvas icon=c;if(!seen)icon.tint_red=icon.tint_green=icon.tint_blue=25;
            icon.picture(relic?a.relic_face_image(RelicId(id)):a.content.potion_image(Potion(id+1)),cx-size/2,cy-size/2,size,size);
            card_grid_hit(c,grid,x-2,y-2,hit_size+4,hit_size+4,HitKind::LibraryItem,index);
            if(selected){focused=index;focus_x=cx;focus_y=y;}
        }
    }
    library_edge_fade(c,0,320);card_grid_scrollbar(c,grid,CardGridSurface::Library);
    native_original_back(c,a.card_library.opened);
    if(focused>=0){const int width=SourceHoverWidth,x=focus_x<160?focus_x+15:focus_x-width-15;collection_tooltip(c,focused,x,focus_y,width);}
}
static void library_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;title_submenu_background(c,185);
    if(!a.library_tab)card_library_screen(c);else collection_screen(c);
}
