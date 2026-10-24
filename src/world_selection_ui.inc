static void world_selection_text(const App&a,char*out,size_t capacity){
    const auto prompt=a.world_selection_prompt();
    CombatPromptContext context{{&a,prompt.table,{},int(a.world_selection_owner()),-1,int(a.world_selection_max())},a.world_selection_min(),a.world_selection_max()};
    wt_model_key(context.model,prompt.key);char raw[512]{};TextBuffer source{raw,sizeof raw,0};
    source.text(a.content.world_string(prompt.table,prompt.key));
    LocalizedTemplate(out,capacity,combat_prompt_value,&context,a.content.english()?"Energy":"能量",a.content.english()?"Stars":"辉星",a.content.english(),combat_prompt_catalog).format(raw);
}
static void world_selection_upgrades(Canvas&c){
    auto&a=c.a;if(!a.world_selection_upgrades()||!a.pointer.visible)return;
    using namespace deck_selection_layout;const char*label=a.content.world_string("card_selection","VIEW_UPGRADES");Canvas text=c;text.font_face=5;
    const int width=ToggleIcon+mn(140,text.width(label,ToggleSize));
    c.ui(a.deck_selection.show_upgrades?F_SettingsTicked:F_SettingsUnticked,ToggleX+1,ToggleY,ToggleIcon,ToggleIcon);
    text.fitted(label,ToggleX+ToggleIcon+1,ToggleY+2,width-ToggleIcon,ToggleIcon,ToggleSize,ToggleSize,rgb(239,200,81),false,true,true,false,0);
    c.action(ToggleX,ToggleY,width+2,ToggleIcon,UiAction::DeckViewUpgrades);
}
static void world_enchantment_description(Canvas&c){
    auto&a=c.a;int amount=0;const auto enchantment=a.world_selection_enchantment(&amount);
    if(enchantment==EventEnchantment::None)return;
    CardHoverTarget owner{};HoverTip tip{};tip.kind=HoverTipKind::Enchantment;tip.key=event_enchantment_key(enchantment);tip.amount=int16_t(amount);
    char title[128]{},description[512]{};format_card_hover_tip(a,owner,tip,title,sizeof title,description,sizeof description);
    const int lines=source_hover_paragraph(c,description,0,0,53,0),height=mx(25,12+lines*source_hover_step(c)),y=240-height;
    Canvas background=c;background.opacity=uint8_t(div255(unsigned(background.opacity)*128));background.rect(225,y,77,height,0);
    const int image=a.content.world_image("enchantments",tip.key);if(image>=0)c.picture(image,229,y+4,11,11);
    Canvas label=c;label.font_face=5;label.fitted(title,241,y+3,53,7,5,5,rgb(239,200,81),false,false,true);
    source_hover_paragraph(c,description,241,y+10,53,lines);
}
static void world_bundle_banner(Canvas&c){
    auto&a=c.a;const auto&returned=a.card_grids[unsigned(CardGridSurface::WorldChoice)].faces[15];
    const uint32_t start=returned.chosen?returned.selection_start:a.deck_selection.opened;
    const unsigned age=a.map_clock_ms-start;
    const unsigned alpha=a.deck_selection.preview?4096-room_curve::cubic_out(a.map_clock_ms-a.deck_selection.preview_opened,400):room_curve::cubic_out(age,400);
    Canvas banner=c;banner.opacity=uint8_t(div255(unsigned(banner.opacity)*alpha*255/4096));banner.font_face=5;
    const int y=a.deck_selection.preview?56:56+CardRewardPresentation::rounded(10*(4096-CardRewardPresentation::back(age,400)),4096);
    banner.ui(a.content.version()>=Ui6ContentVersion?I6_CommonBanner:U_RewardBanner,98,y,125,31);
    banner.fitted(a.content.world_string("gameplay_ui","CHOOSE_A_PACK"),117,y+4,86,16,6,6,rgb(255,246,226),true,true,true,false);
}
static void world_bundle_grid(Canvas&c,const uint16_t*ids,unsigned count){
    auto&a=c.a;const auto&w=a.game.world_relics;auto&grid=a.configure_card_grid(CardGridSurface::WorldChoice,int(count),a.game.revision);
    const auto&returned=grid.faces[15];
    Canvas layer=c;layer.opacity=uint8_t(div255(unsigned(c.opacity)*room_curve::cubic_out(a.map_clock_ms-a.deck_selection.opened,400)*255/4096));
    for(unsigned i=0;i<count;++i){
        const int cx=160+(int(i)*2-int(count)+1)*200*4/21;
        const bool hover=a.pointer.visible?a.pointer.x>=cx-30&&a.pointer.x<cx+31&&a.pointer.y>=84&&a.pointer.y<170:a.cursor==int(i);
        auto&motion=grid.faces[i];if(motion.index!=int(i)){motion={};motion.index=int16_t(i);motion.start=a.map_clock_ms-500;}
        if(motion.focused!=hover){motion.focused=hover;motion.start=a.map_clock_ms;}
        const int scale=hover?850:800+50*int(4096-room_curve::cubic_out(a.map_clock_ms-motion.start,500))/4096;
        for(unsigned j=0;j<3;++j){const unsigned offer=ids[i]*3+j;if(offer>=w.offer_count)continue;
            const int offset=(3-int(j)*2)*45*2*scale/21000;
            const int width=CardGridPainter::face_width(scale),height=CardGridPainter::face_height(scale);
            int x=cx+offset-width/2,y=131-offset-height/2;unsigned shade=128+j*127/2;
            if(returned.chosen&&returned.index==int(ids[i])&&a.map_clock_ms-returned.selection_start<400){
                const unsigned q=room_curve::cubic_out(a.map_clock_ms-returned.selection_start,400),previous=room_curve::cubic_out(returned.selection_start-returned.start,500);
                const int targetx=160+(1-int(j))*400*4/21-width/2,targety=131-height/2;
                const int fromx=x+(targetx-x)*int(previous)/4096,fromy=y+(targety-y)*int(previous)/4096;
                x=fromx+(x-fromx)*int(q)/4096;y=fromy+(y-fromy)*int(q)/4096;
                shade=255-(255-shade)*mn(400U,a.map_clock_ms-returned.selection_start)/400;
            }
            Canvas face=layer;face.brightness=uint8_t(div255(unsigned(face.brightness)*shade));
            draw_card_face(face,card_from_token(w.offers[offer]),x,y,width,height,-2,-2,-1,-1,&w.offer_facts[offer]);
        }
        if(!a.deck_selection.preview)c.hit(cx-194*scale*4/21000,131-307*scale*4/21000,399*scale*4/21000,560*scale*4/21000,HitKind::WorldChoice,ids[i]);
    }
}
static void native_world_selection_screen(Canvas&c,const uint16_t*ids,unsigned count){
    auto&a=c.a;const auto&s=a.game;a.sync_world_selection();const auto kind=a.world_selection_kind();
    const unsigned shade=217*room_curve::cubic_out(a.map_clock_ms-a.deck_selection.opened,500)/4096;
    Canvas backstop=c;backstop.opacity=uint8_t(div255(unsigned(backstop.opacity)*shade));backstop.rect(0,0,320,240,0);
    const bool simple=kind==WorldSelectionKind::Simple,bundle=kind==WorldSelectionKind::Bundle;
    if(bundle){a.configure_card_grid(CardGridSurface::WorldChoice,int(count),s.revision);world_bundle_banner(c);if(!a.deck_selection.preview)world_bundle_grid(c,ids,count);}
    else{
        auto&grid=a.configure_card_grid(CardGridSurface::WorldChoice,int(count),s.revision);CardGridPainter painter{c,grid};
        int portraits[24];unsigned portrait_count=0;
        for(int i=grid.first();i<grid.last();++i){const int id=ids[i];Card card=simple?card_from_token(s.phase==Phase::WorldRelicChoice?s.world_relics.offers[id]:s.original_events.offers[id]):s.deck[id];
            const EventCardFacts*facts=simple?(s.phase==Phase::WorldRelicChoice?&s.world_relics.offer_facts[id]:&s.original_events.offer_facts[id]):nullptr;
            portraits[portrait_count++]=a.card_face_image(card,-1,simple?-1:id,facts);}
        a.content.prepare_images(portraits,portrait_count);
        for(int i=grid.first();i<grid.last();++i){const int id=ids[i];Card card=simple?card_from_token(s.phase==Phase::WorldRelicChoice?s.world_relics.offers[id]:s.original_events.offers[id]):s.deck[id];
            const EventCardFacts*facts=simple?(s.phase==Phase::WorldRelicChoice?&s.world_relics.offer_facts[id]:&s.original_events.offer_facts[id]):nullptr;
            const bool upgraded=a.world_selection_upgrades()&&a.deck_selection.show_upgrades&&card_upgradeable(card);if(upgraded)card.upgraded=1;
            const int x=grid.card_x(i),y=grid.card_y(i);const bool focus=a.pointer.visible?a.pointer.x>=x-2&&a.pointer.x<x+48&&a.pointer.y>=y-2&&a.pointer.y<y+66:a.cursor==i;
            painter.card(i,card,focus&&!a.deck_selection.preview,true,-2,-2,-1,simple?-1:id,facts,world_selected(a,id)&&!a.deck_selection.preview,true,upgraded);
            if(!a.deck_selection.preview)painter.hit(i,HitKind::WorldChoice,id);
        }
        painter.finish();if(!a.deck_selection.preview)card_grid_scrollbar(c,grid,CardGridSurface::WorldChoice);
    }
    if(a.deck_selection.preview){
        a.hit_count=0;
        if(!bundle){Canvas preview=c;preview.opacity=uint8_t(div255(unsigned(preview.opacity)*(kind==WorldSelectionKind::Enchant?217:199)));preview.rect(0,0,320,240,0);}
        for(unsigned i=0;i<a.world_selection_preview_cards();++i){WorldSelectionCard model{};if(!a.world_selection_preview_card(i,model))continue;
            int x=model.x,y=model.y,w=model.w,h=model.h;
            Canvas face=c;
            if(bundle){const unsigned age=a.map_clock_ms-a.deck_selection.preview_opened,q=room_curve::cubic_out(age,500);const int cx=160+(int(a.choice_selected[0])*2-int(count)+1)*200*4/21;
                const auto&motion=a.card_grids[unsigned(CardGridSurface::WorldChoice)].faces[a.choice_selected[0]];const int scale=motion.focused?850:800;
                const int offset=(3-int(i)*2)*45*2*scale/21000;const int fromx=cx+offset-w/2,fromy=131-offset-h/2;
                x=fromx+(x-fromx)*int(q)/4096;y=fromy+(y-fromy)*int(q)/4096;
                const unsigned shade=128+i*127/2,bright=shade+(255-shade)*mn(150U,age)/150;face.brightness=uint8_t(div255(unsigned(face.brightness)*bright));}
            draw_card_face(face,model.card,x,y,w,h,-2,-2,-1,model.deck,&model.facts,nullptr,nullptr,true,model.upgraded);
            if(model.hover)c.hit(x,y,w,h,HitKind::WorldPreviewCard,int(i));
        }
        if(kind==WorldSelectionKind::Transform||((kind==WorldSelectionKind::Upgrade||kind==WorldSelectionKind::Enchant)&&a.world_selection_max()==1&&a.choice_selected_count))card_selection_upgrade_arrows(c);
        native_original_back(c,a.deck_selection.preview_opened);
        native_original_confirm(c,UiAction::ConfirmWorldChoice,a.deck_selection.preview_opened,a.world_selection_confirmable());
    }else{
        if(a.world_selection_cancelable())native_original_back(c,a.deck_selection.opened);
        native_original_confirm(c,UiAction::ConfirmWorldChoice,a.deck_selection.opened,a.world_selection_confirmable());
    }
    if(!bundle){char prompt[512]{};world_selection_text(a,prompt,sizeof prompt);card_selection_prompt(c,prompt);world_selection_upgrades(c);}
    if(kind==WorldSelectionKind::Enchant)world_enchantment_description(c);
}
