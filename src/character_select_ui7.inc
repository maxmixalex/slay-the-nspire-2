// The source's 24px Kreon descriptions fit their natural height inside a
// centered VBox. Use actual available glyphs, preserving every localized line.
static fitted_text::Plan character_info_plan(Canvas&c,const char*text,int width){
    const int size=c.a.content.font_size(5,c.font_face);
    auto metric=[&](const RichChar&ch,int px){const auto g=c.a.content.glyph_metrics(ch.codepoint,px,c.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
    const int rows=fitted_text::line_count(text,width,size,metric);
    return {size,rows,rows,size+1,false};
}
static void character_info_text(Canvas&c,const char*text,int x,int y,int width,const fitted_text::Plan&plan,uint16_t color){
    const int height=plan.rows*plan.step+2;
    Canvas shadow=c;shadow.opacity=uint8_t(div255(shadow.opacity*64));shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
    shadow.fitted(text,x+1,y,width,height,plan.size,plan.size,rgb(0,0,0));
    c.fitted(text,x,y,width,height,plan.size,plan.size,color);
}
static int character_name_width(Canvas&c,const char*text,int size){
    const unsigned face=source_label_face(c.a.content.english(),4);size=c.a.content.font_size(size,face);
    RichReader reader(text);RichChar ch{};int width=0,count=0;
    while(reader.next(ch)&&ch.codepoint!='\n'){width+=c.a.content.glyph_metrics(ch.codepoint,size,face).advance;++count;}
    return width+character_select_layout::project(mx(0,count-1)*(c.a.content.english()?2:0));
}
static int character_name_size(Canvas&c,const char*text){
    constexpr int sizes[]={14,12,10};
    const unsigned face=source_label_face(c.a.content.english(),4);
    for(int size:sizes)if(character_name_width(c,text,size)<=character_select_layout::InfoWidth)return c.a.content.font_size(size,face);
    return c.a.content.font_size(10,face);
}
static void character_name_outline(Canvas&c,Glyph glyph,int x,int y){
    // Source outline_size=24 projects to five pixels. Build a bounded alpha
    // dilation once per glyph, so overlapping taps do not darken the stroke.
    constexpr int Radius=5,Stride=48,Height=40;
    if(!glyph.pixels||glyph.w+2*Radius>Stride||glyph.h+2*Radius>Height)return;
    uint8_t mask[Stride*Height]{};
    constexpr int span[11]={0,3,4,4,4,5,4,4,4,3,0};
    const auto pixel=[&](unsigned at,unsigned alpha){
        if(!alpha)return;const unsigned row=at/glyph.w,column=at-row*glyph.w;
        for(int dy=-Radius;dy<=Radius;++dy){const int reach=span[dy+Radius];
            auto*out=mask+(int(row)+Radius+dy)*Stride+int(column)+Radius;
            for(int dx=-reach;dx<=reach;++dx)out[dx]=uint8_t(mx(out[dx],alpha));
        }
    };
    const unsigned length=unsigned(glyph.w)*glyph.h;
    if(glyph.encoded_size){unsigned at=0;for(unsigned i=0;i+1<glyph.encoded_size&&at<length;i+=2){
        const unsigned stop=mn(length,at+glyph.pixels[i]);while(at<stop){pixel(at,glyph.pixels[i+1]);++at;}
    }}else for(unsigned at=0;at<length;++at)pixel(at,glyph.pixels[at]);
    Canvas shade=c;shade.opacity=uint8_t(div255(shade.opacity*64));
    Glyph outline{};outline.pixels=mask;outline.w=Stride;outline.h=uint8_t(glyph.h+2*Radius);
    shade.glyph(outline,x-Radius,y-Radius,rgb(0,0,0));
}
static void character_info_name(Canvas&c,const char*text,int x,int y,int size){
    RichReader reader(text);RichChar ch{};int at=0,index=0;const int spacing=c.a.content.english()?2:0;
    while(reader.next(ch)&&ch.codepoint!='\n'){
        const auto g=c.a.content.glyph(ch.codepoint,size,source_label_face(c.a.content.english(),4));character_name_outline(c,g,x+at+character_select_layout::project(index++*spacing),y);at+=g.advance;
    }
    reader=RichReader(text);at=index=0;
    while(reader.next(ch)&&ch.codepoint!='\n'){
        const auto g=c.a.content.glyph(ch.codepoint,size,source_label_face(c.a.content.english(),4));
        c.character(ch.codepoint,g,x+at+character_select_layout::project(index++*spacing),y,rgb(239,200,81));at+=g.advance;
    }
}
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
    Canvas body_font=c;body_font.bold_face=5;
    Canvas bold=c;bold.font_face=uint8_t(source_label_face(a.content.english(),5));
    Canvas name=c;name.font_face=uint8_t(source_label_face(a.content.english(),4));
    const auto body_plan=character_info_plan(body_font,body,InfoWidth);
    const auto relic_plan=character_info_plan(body_font,relic_text,RelicTextWidth);
    const int description_height=body_plan.rows*body_plan.step+2;
    const int top=info_top(description_height);
    if(a.content.version()>=RoomUi7ContentVersion){Canvas shade=c;shade.tint_red=shade.tint_green=shade.tint_blue=0;
        shade.nine_ui(I7_CharacterInfoPanel,dx+InfoPanelX,InfoPanelY,InfoPanelWidth,InfoPanelHeight,8,8,8,8,128);
    }else{const int skin=a.content.world_image("character","INFO_PANEL");if(skin>=0)c.nine_image(skin,dx+InfoPanelX,InfoPanelY,InfoPanelWidth,InfoPanelHeight,7,7,7,7);}
    const char*title=a.content.world_title("characters",definition.key);if(!*title)title=definition.name;
    const int title_size=character_name_size(name,title);
    character_info_name(name,title,dx+InfoX,top+(NameHeight-title_size-3)/2,title_size);
    const int stats_y=top+NameHeight+StatsOffset;
    c.picture(A_Heart,dx+InfoX,stats_y,StatsIcon,StatsIcon);
    char hp[32]{};TextBuffer health{hp,sizeof hp,0};health.number(definition.starting_hp);health.ch('/');health.number(definition.starting_hp);
    const int hp_x=dx+InfoX+StatsIcon+StatsLabelGap;
    character_info_text(bold,hp,hp_x,stats_y,InfoWidth,character_info_plan(bold,hp,InfoWidth),rgb(255,100,100));
    const int gold_x=hp_x+bold.width(hp,5)+StatsGap;
    c.picture(A_Coin,gold_x,stats_y,StatsIcon,StatsIcon);
    character_info_text(bold,"99",gold_x+StatsIcon+StatsLabelGap,stats_y,30,character_info_plan(bold,"99",30),rgb(239,200,81));
    const int body_y=top+NameHeight+StatsHeight;
    character_info_text(body_font,body,dx+InfoX,body_y,InfoWidth,body_plan,rgb(255,246,226));
    if(relic){
        const int relic_y=body_y+description_height;
        Canvas relic_shadow=c;relic_shadow.tint_red=relic_shadow.tint_green=relic_shadow.tint_blue=0;
        relic_shadow.picture(a.relic_face_image(relic_id),dx+InfoX,relic_y+RelicIconY,RelicIcon,RelicIcon,128);
        c.picture(a.relic_face_image(relic_id),dx+InfoX,relic_y+RelicIconY,RelicIcon,RelicIcon);
        const char*relic_name=a.relic_title(relic_id);
        const auto title_plan=character_info_plan(bold,relic_name,RelicTextWidth);
        character_info_text(bold,relic_name,dx+InfoX+RelicTextX,relic_y+RelicTitleY,RelicTextWidth,title_plan,rgb(239,200,81));
        character_info_text(body_font,relic_text,dx+InfoX+RelicTextX,relic_y+RelicDescriptionY,RelicTextWidth,relic_plan,rgb(255,246,226));
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
