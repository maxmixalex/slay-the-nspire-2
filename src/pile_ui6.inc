// NCardGrid uses a continuous viewport and source-proportioned 53x75 faces.
// Battle piles are separate screens, not tabs. The deck alone owns the four
// sorters and upgrade preview. The normal top bar remains above the backstop.
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
        const int bar=I6_PileSortBar+mn(4,int(s.character)),bar_y=42-grid.offset();rows.ui(bar,25,bar_y-grid.top,270,15);
        for(int sort=0;sort<4;++sort){const int x=25+sort*68;
            const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+67&&a.pointer.y>=bar_y-2&&a.pointer.y<bar_y+17&&a.pointer.y>=grid.top;
            const bool focused=hover||(!a.pointer.visible&&a.pile_control_focus==sort);
            Canvas layer=rows;if(!focused)layer.brightness=204;
            layer.ui(bar,x,bar_y-grid.top,67,15);
            const auto*label=a.content.world_string("gameplay_ui",keys[sort]);
            layer.text_fitted(label,x+3,bar_y+2-grid.top,51,8,Gold,8,true);
            const bool descending=(a.pile_sort_descending&(1u<<sort))!=0;
            ui_picture_transform(layer,a.content.ui_image(I6_SortDescending),x+60,bar_y+8-grid.top,9,6,0,descending?1024:-1024);
            card_grid_hit(c,grid,x,bar_y-3,67,21,HitKind::PileSort,sort);
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
        const bool hover=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+55&&a.pointer.y>=y-2&&a.pointer.y<y+77&&a.pointer.y>=grid.top&&a.pointer.y<grid.bottom;
        painter.card(i,card,hover||(!a.pointer.visible&&a.pile_control_focus<0&&i==a.inspect_cursor),true,
            deck?a.card_offer_cost(card,event_card_facts(s,card,-1,deck_index)):cost(s,instance),card_star_cost(s,card,instance),instance,deck_index);
        painter.hit(i,deck?HitKind::DeckCard:HitKind::InstanceCard,ids[i]);
    }
    painter.finish();card_grid_scrollbar(c,grid,CardGridSurface::Pile);
    if(!n)c.centered("空",0,103,320,12,Muted);
    const char*info=a.content.world_string("gameplay_ui",deck?"DECK_PILE_INFO":a.pile_tab==1?"DRAW_PILE_INFO":a.pile_tab==2?"DISCARD_PILE_INFO":"EXHAUST_PILE_INFO");
    const int info_x=deck?84:53,info_w=deck?229:260;
    Canvas plaque=c;plaque.opacity=192;plaque.rect(info_x-3,211,info_w+6,28,0);
    c.paragraph_fitted(info,info_x,213,info_w,25,8,White,8,true);
    if(deck){
        c.ui(a.pile_upgraded?F_SettingsTicked:F_SettingsUnticked,5,217,13,13);
        c.text_fitted(a.content.world_string("gameplay_ui","VIEW_UPGRADES"),19,219,61,8,Gold,8);
        c.hit(3,211,78,27,HitKind::PileUpgrades);
    }
    // back_button.tscn anchors left, about two thirds down the scene. Keep
    // the cropped side tab outside the first card's hit area and the footer.
    native_back(c,-8,164,33,24,"",true,false);c.hit(0,161,23,30,HitKind::Close);
}
