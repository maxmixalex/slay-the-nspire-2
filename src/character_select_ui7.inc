static void character_screen(Canvas&c){
    using namespace character_select_layout;
    auto&a=c.a;a.hit_count=0;
    int hover=a.pointer.visible?-2:-1;
    if(a.pointer.visible&&a.pointer.y>=PortraitTop-2&&a.pointer.y<PortraitTop+PortraitHeight+2)
        for(unsigned i=0;i<CharacterKinds;++i)if(a.pointer.x>=portrait_x(i)-1&&a.pointer.x<portrait_x(i)+PortraitWidth+1)hover=int(i);
    auto&motion=a.character_select_presentation;motion.observe(true,unsigned(a.selected_character),hover,a.map_clock_ms);
    const unsigned character=unsigned(a.selected_character);const int dx=motion.panel_offset(a.map_clock_ms);
    const auto&definition=character_definitions[character];
    const auto clip=a.content.version()>=RoomUi7ContentVersion?a.content.clip(int(FirstClip+character)):Clip{};
    if(clip.count){
        // Exact disjoint static/dynamic planes keep the full source loop
        // without repeating unchanging background pixels in every frame.
        const int base=a.content.ui_image(I7_CharacterBackgroundStart+int(character));
        // Earlier v24 packs alias this auxiliary to the first complete frame.
        // Skip that alias so translucent pixels are never composited twice.
        if(base!=int(clip.first))c.picture(base,0,0,320,240);
        c.picture(int(clip.first)+int(frame(character,a.map_clock_ms-motion.panel_start,clip.count)),0,0,320,240);
    }
    else c.ui(character_background(a.selected_character),0,0);

    char description_key[96]{},description[1024]{},relic_text[1024]{};
    TextBuffer key{description_key,sizeof description_key,0};key.text(definition.key);key.text(".description");
    a.localized_text("characters",description_key,description,sizeof description);
    const char*body=*description?description:character_summary(a.selected_character);
    const auto*relic=relic_definition(definition.starting_relic);const RelicId relic_id=relic?RelicId(relic-relic_definitions):RelicId(0);
    if(relic)a.relic_description(relic_id,relic_text,sizeof relic_text);
    const int body_rows=mn(5,c.paragraph(body,0,0,InfoWidth,0,0,8));
    const int relic_rows=relic?mn(4,c.paragraph(relic_text,0,0,InfoWidth-18,0,0,8)):0;
    const int height=24+14+body_rows*11+(relic?5+11+relic_rows*11:0);
    // Preserve the original left-center information block and 133px fuzzy
    // panel width. The minimum readable 8px body grows upward, keeping the
    // bottom clear of the source ascension strip and centered portrait row.
    const int top=InfoBottom-height,panel_top=top-6,panel_height=height+12;
    if(a.content.version()>=RoomUi7ContentVersion){Canvas shade=c;shade.tint_red=shade.tint_green=shade.tint_blue=0;
        shade.nine_ui(I7_CharacterInfoPanel,dx+InfoPanelX,panel_top,InfoPanelWidth,panel_height,8,8,8,8,128);
    }else{const int skin=a.content.world_image("character","INFO_PANEL");if(skin>=0)c.nine_image(skin,dx+InfoPanelX,panel_top,InfoPanelWidth,panel_height,7,7,7,7);}
    const char*title=a.content.world_title("characters",definition.key);if(!*title)title=definition.name;
    Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=64;
    shadow.text_fitted(title,dx+InfoX+1,top+1,InfoWidth,20,0,12);
    c.text_fitted(title,dx+InfoX,top,InfoWidth,20,rgb(239,200,81),12);
    int y=top+24;
    c.picture(A_Heart,dx+InfoX,y,9,9);char hp[32]{};TextBuffer health{hp,sizeof hp,0};health.number(definition.starting_hp);health.ch('/');health.number(definition.starting_hp);
    c.text(hp,dx+InfoX+11,y,8,rgb(255,100,100));
    const int gold_x=dx+InfoX+11+c.width(hp,8)+11;c.picture(A_Coin,gold_x,y,9,9);c.num(99,gold_x+11,y,8,rgb(255,210,80));
    y+=14;c.paragraph(body,dx+InfoX,y,InfoWidth,body_rows,0,8,White);y+=body_rows*11;
    if(relic){
        y+=5;Canvas relic_shadow=c;relic_shadow.tint_red=relic_shadow.tint_green=relic_shadow.tint_blue=0;
        relic_shadow.picture(a.relic_face_image(relic_id),dx+InfoX+1,y+2,14,14,128);
        c.picture(a.relic_face_image(relic_id),dx+InfoX,y+1,14,14);
        c.text_fitted(a.relic_title(relic_id),dx+InfoX+18,y,InfoWidth-18,8,rgb(239,200,81),8);
        c.paragraph(relic_text,dx+InfoX+18,y+11,InfoWidth-18,relic_rows,0,8,White);
    }
    native_original_back(c,motion.opened);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectDefect,UiAction::SelectRegent,UiAction::SelectNecrobinder};
    for(unsigned i=0;i<CharacterKinds;++i){
        const int center=portrait_center(i);const bool selected=i==character;
        const int scale=int(motion.portrait_scale(i,a.map_clock_ms));
        // The existing portrait payload contains its original 132:195 image
        // inside a 36x40 transparent canvas. Preserve those margins here.
        const int pw=(22*scale+512)/1024,ph=(25*scale+512)/1024,ow=(20*scale+512)/1024,oh=(26*scale+512)/1024;
        const int y=PortraitTop+PortraitHeight/2;const int portrait=a.content.ui_image(character_portrait(Character(i)));
        Canvas shade=c;shade.tint_red=shade.tint_green=shade.tint_blue=0;shade.picture(portrait,center-pw/2+2,y-ph/2+2,pw,ph,64);
        const int outline=a.content.world_image("character","OUTLINE");
        if(selected&&outline>=0)c.picture(outline,center-ow/2,y-oh/2,ow,oh,motion.outline_alpha(a.map_clock_ms),presentation::Add);
        small_material_icon(c,portrait,center-pw/2,y-ph/2,pw,ph,motion.saturation(i,a.map_clock_ms),motion.value(i,a.map_clock_ms));
        c.action(portrait_x(i)-1,PortraitTop-2,PortraitWidth+2,PortraitHeight+4,actions[i]);
    }
    native_original_confirm(c,UiAction::StartCharacter,motion.opened);
    if(!a.practice_entry){
        const uint32_t elapsed=a.map_clock_ms-motion.panel_start;
        const unsigned alpha=elapsed>=200?255:unsigned(elapsed)*255/200;
        const int rise=elapsed>=300?0:int((7*(4096-int(room_curve::back_out(room_curve::linear(elapsed,300))))+2048)/4096);
        Canvas band=c;band.opacity=uint8_t(div255(c.opacity*alpha));
        Canvas panel=band;panel.tint_red=panel.tint_green=panel.tint_blue=0;
        if(a.content.version()>=RoomUi7ContentVersion)panel.nine_ui(I7_TinyPanel,AscensionX,AscensionY+rise,AscensionWidth,AscensionHeight,3,3,3,3,128);
        else panel.dim(AscensionX,AscensionY+rise,AscensionWidth,AscensionHeight,128);
        if(a.selected_ascension){band.ui(I7_SettingsArrowLeft,AscensionX-12,AscensionY+rise,12,22);c.action(AscensionX-14,AscensionY-2,16,26,UiAction::RunAscensionDown);}
        if(a.selected_ascension<10){band.ui(I7_SettingsArrowRight,AscensionX+AscensionWidth,AscensionY+rise,12,22);c.action(AscensionX+AscensionWidth-2,AscensionY-2,16,26,UiAction::RunAscensionUp);}
        band.ui(U_TopBarAscension,AscensionX+3,AscensionY+rise,12,22);band.num(a.selected_ascension,AscensionX+5,AscensionY+10+rise,8,White);
        char level_key[40]{},text[512]{};TextBuffer b{level_key,sizeof level_key,0};b.text("LEVEL_");b.ch(char('0'+a.selected_ascension/10));b.ch(char('0'+a.selected_ascension%10));b.text(".title");
        TextBuffer info{text,sizeof text,0};info.text("[gold][b]");info.text(frontend_text(a,"ascension",level_key,a.content.english()?"Ascension":"进阶等级"));info.text("[/b][/gold]\n");
        level_key[8]=0;b.n=8;b.text(".description");info.text(a.content.world_string("ascension",level_key));
        band.paragraph_fitted(text,AscensionX+17,AscensionY+rise,AscensionWidth-20,AscensionHeight,8,White,8);
        // NCharacterSelectScreen.ShouldShowActDropdown is always false in
        // this release. No unrelated Expedition Settings overlay is created.
    }
}
