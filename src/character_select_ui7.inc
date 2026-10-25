static void character_atmosphere(Canvas&c,unsigned character,uint32_t elapsed,uint32_t seed){
    if(character>=5||c.a.content.version()<Ui7ContentVersion)return;
    // Decode once, then reuse the same small texture for the complete batch.
    const auto speck=c.a.content.image(c.a.content.ui_image(I7_GlowSpeck));
    for(unsigned system=0;system<2;++system){const auto&e=character_particles::Systems[character][system];
        for(unsigned i=0;i<e.count;++i){const auto p=character_particles::sample(e,i,character,system,elapsed,seed);
            if(!p.alpha||p.x<0||p.y<0||p.x>=320||p.y>=240)continue;
            Canvas layer=c;layer.tint_red=e.red;layer.tint_green=e.green;layer.tint_blue=e.blue;
            if(speck.pixels)layer.picture(speck,p.x-p.size/2,p.y-p.size/2,p.size,p.size,p.alpha,presentation::Add);
        }
    }
}
// The source's 24px Kreon descriptions fit their natural height inside a
// centered VBox. Use actual available glyphs, preserving every localized line.
static fitted_text::Plan character_info_plan(Canvas&c,const char*text,int width){
    const int size=c.a.content.font_size(10,c.font_face);
    auto metric=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
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
    const unsigned face=source_label_face(c.a.content.original_label_fonts(),4);size=c.a.content.font_size(size,face);
    Canvas label=c;label.font_face=uint8_t(face);return label.width(text,size);
}
static int character_name_size(Canvas&c,const char*text){
    constexpr int sizes[]={18,16,14,12};
    const unsigned face=source_label_face(c.a.content.original_label_fonts(),4);
    for(int size:sizes)if(character_name_width(c,text,size)<=character_select_layout::InfoWidth)return c.a.content.font_size(size,face);
    return c.a.content.font_size(12,face);
}
static void character_name_outline(Canvas&c,Glyph glyph,int x,int y){
    // One LCD pixel is sufficient; the source five-pixel dilation swallowed
    // small counters and made localized character names unreadable.
    constexpr int Radius=1,Stride=48,Height=40;
    if(!glyph.pixels||glyph.w+2*Radius>Stride||glyph.h+2*Radius>Height)return;
    uint8_t mask[Stride*Height]{};
    constexpr int span[3]={0,1,0};
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
    shade.glyph(outline,x+glyph.bearing_x-Radius,y-Radius,rgb(0,0,0));
}
static void character_info_name(Canvas&c,const char*text,int x,int y,int size){
    Canvas label=c;label.font_face=uint8_t(source_label_face(c.a.content.original_label_fonts(),4));
    for(unsigned pass=0;pass<2;++pass){RichReader reader(text);RichChar ch{};int pen=0;
        while(reader.next(ch)&&ch.codepoint!='\n'){
            const auto metric=label.text_metric(ch,size);if(pen)pen+=metric.kerning64;
            const auto glyph=c.a.content.glyph(ch.codepoint,size,label.font_style(ch));
            if(!pass)character_name_outline(label,glyph,x+text_pixel(pen),y);
            else label.character(ch.codepoint,glyph,x+text_pixel(pen),y,rgb(239,200,81));
            pen+=fitted_text::advance64(metric);
        }
    }
}
static void character_screen(Canvas&c){
    using namespace character_select_layout;
    auto&a=c.a;a.hit_count=0;
    int hover=a.pointer.visible?-2:-1;
    if(a.pointer.visible&&a.pointer.y>=PortraitTop-2&&a.pointer.y<PortraitTop+PortraitHeight+2)
        for(unsigned i=0;i<=CharacterKinds;++i)if(a.pointer.x>=portrait_x(i)-1&&a.pointer.x<portrait_x(i)+PortraitWidth+1)hover=int(i);
    const bool random=a.character_random;
    unsigned selection=CharacterKinds;
    if(!random)for(unsigned i=0;i<CharacterKinds;++i)if(RunModeCharacters[i]==a.selected_character){selection=i;break;}
    auto&motion=a.character_select_presentation;motion.observe(true,selection,hover,a.map_clock_ms);
    const unsigned character=unsigned(a.selected_character);const int dx=motion.panel_offset(a.map_clock_ms);
    const auto&definition=character_definitions[character];
    const auto clip=a.content.version()>=RoomUi7ContentVersion?a.content.clip(int(FirstClip+character)):Clip{};
    if(random){
        if(a.content.version()>=SourceVersion)c.ui(I12_RandomBackground,0,0,320,240);
        else{c.picture(A_Title,0,0,320,240);c.dim(0,0,320,240,110);}
    }else if(clip.count){
        // Exact disjoint static/dynamic planes keep the full source loop
        // without repeating unchanging background pixels in every frame.
        const int base=a.content.ui_image(I7_CharacterBackgroundStart+int(character));
        // Earlier v24 packs alias this auxiliary to the first complete frame.
        // Skip that alias so translucent pixels are never composited twice.
        if(base!=int(clip.first))c.picture(base,0,0,320,240);
        c.picture(int(clip.first)+int(frame(character,a.map_clock_ms-motion.panel_start,clip.count)),0,0,320,240);
    }
    else c.ui(character_background(a.selected_character),0,0);
    if(!random)character_atmosphere(c,character,a.map_clock_ms-motion.panel_start,motion.panel_start);
    char description_key[96]{},description[1024]{},relic_text[1024]{};
    const char*character_key=random?"RANDOM_CHARACTER":definition.key;
    TextBuffer key{description_key,sizeof description_key,0};key.text(character_key);key.text(".description");
    a.localized_text("characters",description_key,description,sizeof description);
    const char*body=*description?description:random?(a.content.language()==2?"以随机角色开始爬塔之旅。":"Embark as a randomly chosen character."):character_summary(a.selected_character);
    const auto*relic=random?nullptr:relic_definition(definition.starting_relic);const RelicId relic_id=relic?RelicId(relic-relic_definitions):RelicId(0);
    if(relic)a.relic_description(relic_id,relic_text,sizeof relic_text);
    Canvas body_font=c;body_font.bold_face=5;
    Canvas bold=c;bold.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),5));
    Canvas name=c;name.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),4));
    const auto body_plan=character_info_plan(body_font,body,InfoWidth);
    const auto relic_plan=character_info_plan(body_font,relic_text,RelicTextWidth);
    const int description_height=body_plan.rows*body_plan.step+2;
    const int top=info_top(description_height);
    if(a.content.version()>=RoomUi7ContentVersion){Canvas shade=c;shade.tint_red=shade.tint_green=shade.tint_blue=0;
        shade.nine_ui(I7_CharacterInfoPanel,dx+InfoPanelX,InfoPanelY,InfoPanelWidth,InfoPanelHeight,8,8,8,8,128);
    }else{const int skin=a.content.world_image("character","INFO_PANEL");if(skin>=0)c.nine_image(skin,dx+InfoPanelX,InfoPanelY,InfoPanelWidth,InfoPanelHeight,7,7,7,7);}
    const char*title=a.content.world_title("characters",character_key);if(!*title)title=random?(a.content.language()==2?"随机":"Random"):definition.name;
    const int title_size=character_name_size(name,title);
    character_info_name(name,title,dx+InfoX,top+(NameHeight-title_size-3)/2,title_size);
    const int stats_y=top+NameHeight+StatsOffset;
    if(!random){
    c.picture(A_Heart,dx+InfoX,stats_y,StatsIcon,StatsIcon);
    char hp[32]{};TextBuffer health{hp,sizeof hp,0};health.number(definition.starting_hp);health.ch('/');health.number(definition.starting_hp);
    const int hp_x=dx+InfoX+StatsIcon+StatsLabelGap;
    character_info_text(bold,hp,hp_x,stats_y,InfoWidth,character_info_plan(bold,hp,InfoWidth),rgb(255,100,100));
    const int gold_x=hp_x+bold.width(hp,10)+StatsGap;
    c.picture(A_Coin,gold_x,stats_y,StatsIcon,StatsIcon);
    character_info_text(bold,"99",gold_x+StatsIcon+StatsLabelGap,stats_y,30,character_info_plan(bold,"99",30),rgb(239,200,81));
    }
    const int body_y=top+NameHeight+(random?5:StatsHeight);
    const int body_height=InfoPanelY+InfoPanelHeight-10-body_y;
    const auto relic_title_plan=character_info_plan(bold,relic?a.relic_title(relic_id):"",RelicTextWidth);
    const int total_height=description_height+(relic?6+mx(RelicIcon,relic_title_plan.rows*relic_title_plan.step+2+relic_plan.rows*relic_plan.step):0);
    const int overflow=mx(0,total_height-body_height);
    const int offset=native_room_marquee_offset(overflow,a.map_clock_ms-motion.panel_start);
    // Local clipping keeps every long locale inside the padded panel. The
    // slow source-style text scroll reveals full descriptions and relics.
    Canvas article=body_font;const int article_x=mx(0,dx+InfoX);
    article.pixels+=body_y*article.pitch+article_x;article.clip_w=InfoWidth-(article_x-dx-InfoX);article.clip_h=body_height;
    Canvas article_bold=article;article_bold.font_face=bold.font_face;
    const int text_x=dx+InfoX-article_x;
    character_info_text(article,body,text_x,-offset,InfoWidth,body_plan,rgb(255,246,226));
    if(relic){
        const int relic_y=description_height+4-offset;
        Canvas relic_shadow=article;relic_shadow.tint_red=relic_shadow.tint_green=relic_shadow.tint_blue=0;
        relic_shadow.picture(a.relic_face_image(relic_id),text_x,relic_y+RelicIconY,RelicIcon,RelicIcon,128);
        article.picture(a.relic_face_image(relic_id),text_x,relic_y+RelicIconY,RelicIcon,RelicIcon);
        const char*relic_name=a.relic_title(relic_id);
        const auto title_plan=character_info_plan(bold,relic_name,RelicTextWidth);
        character_info_text(article_bold,relic_name,text_x+RelicTextX,relic_y+RelicTitleY,RelicTextWidth,title_plan,rgb(239,200,81));
        character_info_text(article,relic_text,text_x+RelicTextX,relic_y+title_plan.rows*title_plan.step+2,RelicTextWidth,relic_plan,rgb(255,246,226));
    }
    if(overflow){const int x=dx+InfoPanelX+InfoPanelWidth-7,thumb=mx(8,body_height*body_height/total_height);
        c.ui(I6_ScrollbarTrack,x,body_y,4,body_height);c.ui(I6_ScrollbarGrabber,x,body_y+(body_height-thumb)*offset/overflow,4,thumb);}
    native_original_back(c,motion.opened);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectDefect,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectRandomCharacter};
    for(unsigned i=0;i<=CharacterKinds;++i){
        const int center=portrait_center(i);const bool selected=i==selection;
        const int scale=int(motion.portrait_scale(i,a.map_clock_ms));
        // The existing portrait payload contains its original 132:195 image
        // inside a 36x40 transparent canvas. Preserve those margins here.
        const int pw=(30*scale+512)/1024,ph=(34*scale+512)/1024,ow=(28*scale+512)/1024,oh=(35*scale+512)/1024;
        const unsigned model=i==CharacterKinds?CharacterKinds:unsigned(RunModeCharacters[i]);
        const int y=PortraitTop+PortraitHeight/2;const int portrait=a.content.ui_image(i==CharacterKinds?unsigned(I12_RandomCharacter):character_portrait(Character(model)));
        Canvas shade=c;shade.tint_red=shade.tint_green=shade.tint_blue=0;shade.picture(portrait,center-pw/2+2,y-ph/2+2,pw,ph,64);
        const int outline=a.content.world_image("character","OUTLINE");
        if(selected&&outline>=0)c.picture(outline,center-ow/2,y-oh/2,ow,oh,motion.outline_alpha(a.map_clock_ms),presentation::Add);
        small_material_icon(c,portrait,center-pw/2,y-ph/2,pw,ph,motion.saturation(i,a.map_clock_ms),motion.value(i,a.map_clock_ms));
        c.action(portrait_x(i)-1,PortraitTop-2,PortraitWidth+2,PortraitHeight+4,actions[model]);
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
        band.ui(U_TopBarAscension,AscensionX+2,AscensionY+rise,16,25);
        char number[8]{};TextBuffer count{number,sizeof number,0};count.number(a.selected_ascension);
        band.text_fitted(number,AscensionX+2,AscensionY+9+rise,16,10,White,10,true,true,Ink);
        char level_key[40]{},text[512]{};TextBuffer b{level_key,sizeof level_key,0};b.text("LEVEL_");b.ch(char('0'+a.selected_ascension/10));b.ch(char('0'+a.selected_ascension%10));b.text(".title");
        TextBuffer info{text,sizeof text,0};info.text("[gold][b]");info.text(frontend_text(a,"ascension",level_key,a.content.language()!=2?"Ascension":"进阶等级"));info.text("[/b][/gold]\n");
        level_key[8]=0;b.n=8;b.text(".description");info.text(a.content.world_string("ascension",level_key));
        band.paragraph_fitted(text,AscensionX+17,AscensionY+rise,AscensionWidth-20,AscensionHeight,8,White,8);
        // NCharacterSelectScreen.ShouldShowActDropdown is always false in
        // this release. No unrelated Expedition Settings overlay is created.
    }
}

static void character_departure_screen(Canvas&c){
    const auto&m=c.a.character_departure;if(!m.active())return;c.a.hit_count=0;
    const unsigned phase=m.progress(c.a.map_clock_ms);
    if(m.phase==2){c.dim(0,0,320,240,unsigned(255*(4096-phase)/4096));return;}
    if(!phase)return;
    // Original texture_transition shader: step(1-red, mix(-.1,1.1,t)).
    // The RGB565 grayscale ordering permits one comparison per LCD pixel;
    // no full-screen intermediate buffer is allocated for this 800 ms wipe.
    const int cutoff=(255*(11*4096-12*int(phase))+40959)/40960;
    if(cutoff>255)return;if(cutoff<=0){c.rect(0,0,320,240,0);return;}
    const unsigned asset=I12_CharacterTransitionFirst+(m.character==unsigned(Character::Silent)?1:m.character==unsigned(Character::Necrobinder)?2:0);
    const auto mask=c.a.content.image(c.a.content.ui_image(asset));
    if(c.a.content.version()<Ui12ContentVersion||!mask.pixels||mask.w!=320||mask.h!=240){c.dim(0,0,320,240,phase*255/4096);return;}
    const uint16_t threshold=rgb(cutoff,cutoff,cutoff);
    if(mask.format==4){const uint8_t*run=mask.pixels;
        for(int y=0;y<240;++y){const unsigned count=run[0]|unsigned(run[1])<<8;run+=2;
            for(unsigned span=0;span<count;++span){const unsigned left=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,stride=run[4]?3:2;run+=5;
                for(unsigned i=0;i<n;++i,run+=stride)if((unsigned(run[0])|unsigned(run[1])<<8)>=threshold)c.paint(y*c.pitch+left+i,0);
            }
        }
    }else for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){const unsigned at=y*320+x;const uint8_t*p=mask.palette?mask.palette+mask.pixels[at]*2:mask.pixels+at*2;
        if((unsigned(p[0])|unsigned(p[1])<<8)>=threshold)c.paint(y*c.pitch+x,0);
    }
}
