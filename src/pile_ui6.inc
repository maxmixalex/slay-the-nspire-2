// Adapt the original CardGrid to readable 47x66 cards on the 320x240 display.
// Battle piles are separate screens, not tabs. The deck alone owns the four
// sorters and upgrade preview. The normal top bar remains above the backstop.
static void pile_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;HitRegion top[16]{};unsigned top_count=0;
    for(int i=0;i<a.hit_count&&top_count<16;++i){const auto&h=a.hits[i];if(h.y==0&&h.h<=22)top[top_count++]=h;}
    a.hit_count=0;
    Canvas backdrop=c;backdrop.opacity=192;backdrop.rect(0,22,320,218,0);
    for(unsigned i=0;i<top_count;++i){const auto&h=top[i];c.hit(h.x,h.y,h.w,h.h,h.kind,h.value,h.enabled);}
    const bool deck=a.pile_tab==0;
    if(deck){
        static const char*keys[]={"SORT_OBTAINED","SORT_TYPE","SORT_COST","SORT_ALPHABET"};
        const int bar=I6_PileSortBar+mn(4,int(s.character));c.ui(bar,25,32,270,15);
        for(int sort=0;sort<4;++sort){const int x=25+sort*68;
            const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+67&&a.pointer.y>=30&&a.pointer.y<49;
            const bool focused=hover||(!a.pointer.visible&&a.pile_control_focus==sort);
            Canvas layer=c;if(!focused)layer.brightness=204;
            layer.ui(bar,x,32,67,15);
            const auto*label=a.content.world_string("gameplay_ui",keys[sort]);
            layer.text_fitted(label,x+3,34,51,8,Gold,8,true);
            const bool descending=(a.pile_sort_descending&(1u<<sort))!=0;
            ui_picture_transform(layer,a.content.ui_image(I6_SortDescending),x+60,40,9,6,0,descending?1024:-1024);
            c.hit(x,29,67,21,HitKind::PileSort,sort);
        }
    }
    int ids[MaxCards];const int n=int(a.pile_items(ids));
    a.inspect_cursor=mn(mx(0,a.inspect_cursor),mx(0,n-1));a.scroll=mn((mx(0,a.scroll)/10)*10,mx(0,((n-1)/10)*10));
    // Batch the exact visible faces (portrait/frame/border/banner/energy),
    // including shared assets, before the first face triggers small reads.
    int images[32];unsigned image_count=0;
    auto add=[&](int id){if(id<0)return;for(unsigned i=0;i<image_count;++i)if(images[i]==id)return;if(image_count<32)images[image_count++]=id;};
    for(int i=a.scroll;i<mn(n,a.scroll+10);++i){Card card=deck?s.deck[ids[i]]:s.cards[ids[i]];
        if(deck&&a.pile_upgraded&&card_upgradeable(card))card.upgraded=1;
        const int instance=deck?-1:ids[i],deck_index=deck?ids[i]:-1;
        add(a.card_face_image(card,instance,deck_index));add(a.card_face_frame(card,instance,deck_index));add(a.content.energy_image(card));
        const unsigned group=card_face::material_group(unsigned(card_rarity(card.kind)));const int base=U_CardBorderCommonAttack+int(group)*5;
        const auto type=a.card_face_type(card,instance,deck_index);add(a.content.ui_image(base+(type==CardType::Attack?0:type==CardType::Power?2:1)));
        add(a.content.ui_image(base+3));add(a.content.ui_image(base+4));
    }
    a.content.prepare_images(images,image_count);
    const int start_y=deck?59:45;
    for(int i=a.scroll;i<mn(n,a.scroll+10);++i){
        Card card=deck?s.deck[ids[i]]:s.cards[ids[i]];if(deck&&a.pile_upgraded&&card_upgradeable(card))card.upgraded=1;
        const int instance=deck?-1:ids[i],deck_index=deck?ids[i]:-1;
        const int x=24+(i-a.scroll)%5*54,y=start_y+(i-a.scroll)/5*74;
        const bool hover=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+49&&a.pointer.y>=y-2&&a.pointer.y<y+68;
        small_card(c,card,x,y,hover||(!a.pointer.visible&&a.pile_control_focus<0&&i==a.inspect_cursor),true,0,
            deck?a.card_offer_cost(card,event_card_facts(s,card,-1,deck_index)):cost(s,instance),card_star_cost(s,card,instance),instance,deck_index);
        c.hit(x-2,y-2,51,70,deck?HitKind::DeckCard:HitKind::InstanceCard,ids[i]);
    }
    const int bar_top=deck?59:45,bar_height=140;
    if(n>10){
        c.ui(I6_ScrollbarTrack,302,bar_top,6,bar_height);c.ui(I6_ScrollbarEdge,300,bar_top-4,10,5);
        ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),305,bar_top+bar_height+2,10,5,0,-1024);
        const int pages=(n+9)/10,h=mx(12,bar_height/pages),y=bar_top+(bar_height-h)*(a.scroll/10)/mx(1,pages-1);
        c.ui(I6_ScrollbarGrabber,300,y,10,h);
        c.hit(297,bar_top,18,mx(0,y-bar_top),HitKind::Scroll,-1,a.scroll>0);
        c.hit(297,y+h,18,mx(0,bar_top+bar_height-y-h),HitKind::Scroll,1,a.scroll+10<n);
    }
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
