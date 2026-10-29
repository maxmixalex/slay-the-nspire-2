// NCardGrid uses the actual 1680x1260 viewport and 46x64 small faces.
// Battle piles are separate screens, not tabs. The deck alone owns the four
// sorters and upgrade preview. The normal top bar remains above the backstop.
static void pile_sort_label(Canvas,const char*,int,int,int,uint32_t);
static void pile_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;HitRegion top[16]{};unsigned top_count=0;
    for(int i=0;i<a.hit_count&&top_count<16;++i){const auto&h=a.hits[i];if(h.y==0&&h.h<=22)top[top_count++]=h;}
    a.hit_count=0;
    Canvas backdrop=c;backdrop.opacity=192;backdrop.rect(0,22,320,218,0);
    for(unsigned i=0;i<top_count;++i){const auto&h=top[i];c.hit(h.x,h.y,h.w,h.h,h.kind,h.value,h.enabled);}
    const bool deck=a.pile_tab==0;
    int ids[MaxCards];const int n=int(a.pile_items(ids));
    auto&grid=a.configure_card_grid(CardGridSurface::Pile,n,s.revision);CardGridPainter painter{c,grid};
    a.inspect_cursor=mn(mx(0,a.inspect_cursor),mx(0,n-1));a.scroll=grid.first();
    Canvas rows=card_grid_canvas(c,grid);
    if(deck){
        static const char*keys[]={"SORT_OBTAINED","SORT_TYPE","SORT_COST","SORT_ALPHABET"};
        const int character=mn(4,int(s.character)),bar=I6_PileSortBar+character,bar_y=33-grid.offset();rows.ui(bar,44,bar_y-grid.top,236,18);
        // SortingOptions has a 60px strip around a centred HBox of four
        // 250x42 controls, separated by 50px. Keep their horizontal spacing,
        // with a taller LCD strip for readable8px labels. Text stays left-
        // aligned; only ButtonImage receives the source HSV treatment.
        const int first=44+(236-reference_viewport::project(1150))/2;
        for(int sort=0;sort<4;++sort){const int x=first+reference_viewport::project(sort*300);
            const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+48&&a.pointer.y>=bar_y+2&&a.pointer.y<bar_y+16&&a.pointer.y>=grid.top;
            const bool focused=hover||(!a.pointer.visible&&a.pile_control_focus==sort);
            const bool pressed=hover&&a.pointer.held&&a.pointer.captured.kind==HitKind::PileSort&&a.pointer.captured.value==sort;
            Canvas button=rows;button.brightness=uint8_t(div255(unsigned(button.brightness)*186));
            static const unsigned source_s[]={850,450,900,1500,550},source_v[]={1000,1200,1000,1200,1200};
            const unsigned target=focused&&!pressed?1000:800;
            small_material_icon(button,a.content.ui_image(bar),x+1,bar_y+2-grid.top,pressed?46:48,focused&&!pressed?15:14,
                target*1000/source_s[character],target*1000/source_v[character],true);
            const auto*label=a.content.world_string("gameplay_ui",keys[sort]);
            Canvas layer=rows;layer.font_face=source_label_face(a.content.original_label_fonts(),5);layer.sparse_scene_sizes=true;
            const uint32_t identity=0x50494c30u+unsigned(sort),text_hash=RichLabelClocks::hash(label);
            const unsigned age=a.overflow_label_clocks.observe(identity,text_hash,a.map_clock_ms);
            const int tracking=a.content.original_label_fonts()?1:0;
            pile_sort_label(layer,label,x+2,bar_y+1-grid.top,tracking,a.map_clock_ms-age);
            const bool descending=(a.pile_sort_descending&(1u<<sort))!=0;
            Canvas arrow=rows;arrow.tint_red=239;arrow.tint_green=200;arrow.tint_blue=81;
            int characters=0;RichReader reader(label);RichChar glyph{};while(reader.next(glyph))++characters;
            const int text_width=layer.width(label,8)+reference_viewport::project(mx(0,characters-1)*tracking);
            const int arrow_x=x+2+mn(37,text_width)+2+3;
            ui_picture_transform(arrow,a.content.ui_image(I6_SortDescending),arrow_x,bar_y+9-grid.top,7,7,0,descending?1024:-1024);
            card_grid_hit(c,grid,x,bar_y+1,48,16,HitKind::PileSort,sort);
        }
    }
    // Batch the exact visible faces (portrait/frame/border/banner/energy),
    // including shared assets, before the first face triggers small reads.
    int images[32];unsigned image_count=0;
    auto add=[&](int id){if(id<0)return;for(unsigned i=0;i<image_count;++i)if(images[i]==id)return;if(image_count<32)images[image_count++]=id;};
    for(int i=a.scroll;i<grid.last();++i){Card card=deck?s.deck[ids[i]]:s.cards[ids[i]];
        if(deck&&a.pile_upgraded&&card_upgradeable(card))card.upgraded=1;
        const int instance=deck?-1:ids[i],deck_index=deck?ids[i]:-1;
        add(a.card_face_image(card,instance,deck_index));add(a.card_face_frame(card,instance,deck_index));add(a.content.energy_image(card));
        const unsigned group=card_face::material_group(unsigned(card_rarity(card.kind)));const int base=U_CardBorderCommonAttack+int(group)*5;
        const auto type=a.card_face_type(card,instance,deck_index);add(a.content.ui_image(base+(type==CardType::Attack?0:type==CardType::Power?2:1)));
        add(a.content.ui_image(base+3));add(a.content.ui_image(base+4));
    }
    a.content.prepare_images(images,image_count);
    for(int i=a.scroll;i<grid.last();++i){
        Card card=deck?s.deck[ids[i]]:s.cards[ids[i]];if(deck&&a.pile_upgraded&&card_upgradeable(card))card.upgraded=1;
        const int instance=deck?-1:ids[i],deck_index=deck?ids[i]:-1;
        const int x=grid.card_x(i),y=grid.card_y(i);
        const bool hover=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+CardGridScroll::CardW+2&&a.pointer.y>=y-2&&a.pointer.y<y+CardGridScroll::CardH+2&&a.pointer.y>=grid.top&&a.pointer.y<grid.bottom;
        painter.card(i,card,hover||(!a.pointer.visible&&a.pile_control_focus<0&&i==a.inspect_cursor),true,
            deck?a.card_offer_cost(card,event_card_facts(s,card,-1,deck_index)):cost(s,instance),card_star_cost(s,card,instance),instance,deck_index,nullptr,false,true,deck&&a.pile_upgraded&&!s.deck[ids[i]].upgraded);
        painter.hit(i,deck?HitKind::DeckCard:HitKind::InstanceCard,ids[i]);
    }
    painter.finish();card_grid_scrollbar(c,grid,CardGridSurface::Pile);
    if(!n)c.centered("空",0,103,320,12,Muted);
    const char*info=a.content.world_string("gameplay_ui",deck?"DECK_PILE_INFO":a.pile_tab==1?"DRAW_PILE_INFO":a.pile_tab==2?"DISCARD_PILE_INFO":"EXHAUST_PILE_INFO");
    const int info_w=mn(156,mx(1,c.width(info,8))),info_x=160-info_w/2;
    Canvas plaque=c;plaque.opacity=192;plaque.rect(info_x-2,225,info_w+4,13,0);
    c.text_fitted(info,info_x,227,info_w,8,White,8,true);
    if(deck){
        // Center the entire icon/label group in its dedicated footer row.
        // A bounded label keeps long locales clear of the Back tab.
        Canvas label=c;label.font_face=source_label_face(a.content.original_label_fonts(),5);
        const char*text=a.content.world_string("gameplay_ui","VIEW_UPGRADES");
        const int label_width=mn(180,mx(1,label.width(text,10))),width=14+label_width,x=(320-width)/2;
        c.ui(a.pile_upgraded?F_SettingsTicked:F_SettingsUnticked,x,208,10,10);
        label.text_fitted(text,x+14,208,label_width,10,Gold,8);
        c.hit(x-2,206,width+4,15,HitKind::PileUpgrades);
    }
    // back_button.tscn anchors left, about two thirds down the scene. Keep
    // the cropped side tab outside the first card's hit area and the footer.
    native_original_back(c);
}
