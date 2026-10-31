#include "random_character_ui13.inc"
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
static void character_name_outline(Canvas&c,Glyph glyph,uint32_t codepoint,unsigned size,unsigned style,int x,int y){
    // One LCD pixel is sufficient; the source five-pixel dilation swallowed
    // small counters and made localized character names unreadable.
    constexpr int Radius=1,Stride=48,Height=40;
    if(!glyph.pixels||!glyph.w||!glyph.h||glyph.w+2*Radius>Stride||glyph.h+2*Radius>Height)return;
    const unsigned width=glyph.w+2*Radius,height=glyph.h+2*Radius;
    auto&cached=character_outline_cache(c.a.character_random).entry(c.a.content,codepoint,size,style);
    uint8_t*const mask=cached.mask;
    if(!cached.valid||cached.width!=width||cached.height!=height){
    for(unsigned i=0;i<width*height;++i)mask[i]=0;
    constexpr int span[3]={0,1,0};
    unsigned row=0,column=0;
    const auto pixel=[&](unsigned alpha){
        if(alpha){
        for(int dy=-Radius;dy<=Radius;++dy){const int reach=span[dy+Radius];
            auto*out=mask+(int(row)+Radius+dy)*width+column+Radius;
            for(int dx=-reach;dx<=reach;++dx)out[dx]=uint8_t(mx(out[dx],alpha));
        }}
        // Raster order already supplies the row/column. Carry once per
        // source pixel instead of dividing by a dynamic glyph width on ARM.
        if(++column==glyph.w){column=0;++row;}
    };
    const unsigned length=unsigned(glyph.w)*glyph.h;
    if(glyph.encoded_size){unsigned at=0;for(unsigned i=0;i+1<glyph.encoded_size&&at<length;i+=2){
        const unsigned stop=mn(length,at+glyph.pixels[i]);while(at<stop){pixel(glyph.pixels[i+1]);++at;}
    }}else for(unsigned at=0;at<length;++at)pixel(glyph.pixels[at]);
    cached.width=uint8_t(width);cached.height=uint8_t(height);cached.valid=true;
    }
    Canvas shade=c;shade.opacity=uint8_t(div255(shade.opacity*64));
    // The old48px stride painted transparent padding beside every letter.
    // A tight row has the same cross-dilated pixels without those visits.
    Glyph outline{};outline.pixels=mask;outline.w=uint8_t(width);outline.h=uint8_t(height);
    shade.glyph(outline,x+glyph.bearing_x-Radius,y-Radius,rgb(0,0,0));
}
static void character_info_name(Canvas&c,const char*text,int x,int y,int size){
    Canvas label=c;label.font_face=uint8_t(source_label_face(c.a.content.original_label_fonts(),4));
    for(unsigned pass=0;pass<2;++pass){RichReader reader(text);RichChar ch{};int pen=0;
        while(reader.next(ch)&&ch.codepoint!='\n'){
            const auto metric=label.text_metric(ch,size);if(pen)pen+=metric.kerning64;
            const auto glyph=c.a.content.glyph(ch.codepoint,size,label.font_style(ch));
            if(!pass)character_name_outline(label,glyph,ch.codepoint,unsigned(size),label.font_style(ch),x+text_pixel(pen),y);
            else label.character(ch.codepoint,glyph,x+text_pixel(pen),y,rgb(239,200,81));
            pen+=fitted_text::advance64(metric);
        }
    }
}
static bool character_delta_background(Canvas&c,unsigned first,unsigned frame){
    auto& raster=character_selection_raster();
    if(!raster.restore(c.a.content,first,frame,c.a.callbacks.foreground_poll))return false;
    if(!c.coverage&&!c.occlusion&&c.opacity==255&&c.brightness==255&&
       c.tint_red==255&&c.tint_green==255&&c.tint_blue==255&&!c.color_matrix&&
       c.pitch==ScreenW&&c.clip_w==ScreenW&&c.clip_h==ScreenH){
        for(unsigned y=0;y<ScreenH;y+=16){
            c.poll();__builtin_memcpy(c.pixels+y*ScreenW,raster.pixels+y*ScreenW,16*ScreenW*2);
        }
    }else c.picture(Picture{reinterpret_cast<const uint8_t*>(raster.pixels),nullptr,nullptr,
                            ScreenW,ScreenH,0},0,0);
    return true;
}
static void character_screen(Canvas&c){
    using namespace character_select_layout;
    auto&a=c.a;a.hit_count=0;
    int hover=a.pointer.visible?-2:-1;
    if(a.pointer.visible&&a.pointer.y>=PortraitTop-2&&a.pointer.y<PortraitTop+PortraitHeight+2)
        for(unsigned i=0;i<=CharacterKinds;++i)if(a.pointer.x>=portrait_x(i)-1&&a.pointer.x<portrait_x(i)+PortraitWidth+1)hover=int(i);
    const bool random=a.character_random;
    if(a.content.version()>=Ui12ContentVersion&&!a.character_departure.active()){
        // These shared source masks belong to the currently offered action.
        // Prepare them while selection is visible, before the timed wipe.
        int masks[3];unsigned count=random?3:1;
        for(unsigned i=0;i<count;++i)masks[i]=a.content.ui_image(I12_CharacterTransitionFirst+
            (random?i:a.selected_character==Character::Silent?1:a.selected_character==Character::Necrobinder?2:0));
        a.content.prepare_images(masks,count);
        for(unsigned i=0;i<count;++i)(void)a.content.image(masks[i]);
    }
    unsigned selection=CharacterKinds;
    if(!random)for(unsigned i=0;i<CharacterKinds;++i)if(RunModeCharacters[i]==a.selected_character){selection=i;break;}
    auto&motion=a.character_select_presentation;
    if(motion.active&&motion.selected!=selection)a.screen_effect(ScreenShakePresentation::Punch,ScreenShakePresentation::Weak,300,1024);
    motion.observe(true,selection,hover,a.map_clock_ms);
    const unsigned character=unsigned(a.selected_character);const int dx=motion.panel_offset(a.map_clock_ms);
    const auto&definition=character_definitions[character];
    const auto clip=a.content.version()>=RoomUi7ContentVersion?a.content.clip(int(FirstClip+character)):Clip{};
    if(random){
        if(a.content.reduced_animations()&&a.content.version()>=SourceVersion)
            c.ui(I12_RandomBackground,0,0,320,240);
        else if(random_character_background(c,a.map_clock_ms,a.map_clock_ms-motion.panel_start,motion.panel_start)){}
        else if(a.content.version()>=SourceVersion)c.ui(I12_RandomBackground,0,0,320,240);
        else{c.picture(A_Title,0,0,320,240);c.dim(0,0,320,240,110);}
    }else if(clip.count&&a.content.reduced_animations()){
        // The reduced pack stores the exact opaque first source pose. Avoid
        // replaying an identical copy-patch timeline on every menu tick.
        c.picture(int(clip.first),0,0,320,240);
    }else if(clip.count&&character_select_delta::available(a.content,character,clip.first)){
        if(!character_delta_background(c,clip.first,frame(character,a.map_clock_ms-motion.panel_start,clip.count)))
            c.rect(0,0,ScreenW,ScreenH,0);
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
    if(!random){motion.random_background.active=false;character_atmosphere(c,character,
        a.content.reduced_animations()?0:a.map_clock_ms-motion.panel_start,motion.panel_start);}
    const auto profile=a.callbacks.profile_render;
    if(profile)profile(RuntimeRenderPart::CharacterInfo,true);
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
    c.picture(A_Heart,dx+InfoX,stats_y,StatsIcon,StatsIcon);
    char hp[32]{};TextBuffer health{hp,sizeof hp,0};if(random)health.text("?" "?/??");else{health.number(definition.starting_hp);health.ch('/');health.number(definition.starting_hp);}
    const int hp_x=dx+InfoX+StatsIcon+StatsLabelGap;
    character_info_text(bold,hp,hp_x,stats_y,InfoWidth,character_info_plan(bold,hp,InfoWidth),rgb(255,100,100));
    const int gold_x=hp_x+bold.width(hp,10)+StatsGap;
    c.picture(A_Coin,gold_x,stats_y,StatsIcon,StatsIcon);
    const char*gold=random?"???":"99";
    character_info_text(bold,gold,gold_x+StatsIcon+StatsLabelGap,stats_y,30,character_info_plan(bold,gold,30),rgb(239,200,81));
    const int body_y=top+NameHeight+StatsHeight;
    const int body_height=InfoPanelY+InfoPanelHeight-14-body_y;
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
    if(overflow){const int x=dx+InfoPanelX+InfoPanelWidth-7,thumb=10;
        c.ui(I6_ScrollbarTrack,x,body_y,4,body_height);native_scrollbar_grabber(c,x-3,body_y+(body_height-thumb)*offset/overflow,thumb,thumb);}
    if(profile)profile(RuntimeRenderPart::CharacterInfo,false);
    if(profile)profile(RuntimeRenderPart::CharacterControls,true);
    native_original_back(c,motion.opened);
    if(profile)profile(RuntimeRenderPart::CharacterControls,false);
    if(profile)profile(RuntimeRenderPart::CharacterPortraits,true);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectDefect,UiAction::SelectRegent,UiAction::SelectNecrobinder,UiAction::SelectRandomCharacter};
    const int portrait_outline=a.content.world_image("character","OUTLINE");
    for(unsigned i=0;i<=CharacterKinds;++i){
        const int center=portrait_center(i);const bool selected=i==selection;
        const int scale=int(motion.portrait_scale(i,a.map_clock_ms));
        // The existing portrait payload contains its original 132:195 image
        // inside a 36x40 transparent canvas. Preserve those margins here.
        const int ph=(34*scale+512)/1024,pw=i==CharacterKinds?(ph*132+98)/196:(30*scale+512)/1024,ow=(28*scale+512)/1024,oh=(35*scale+512)/1024;
        const unsigned model=i==CharacterKinds?CharacterKinds:unsigned(RunModeCharacters[i]);
        const int y=PortraitTop+PortraitHeight/2;const int portrait=a.content.ui_image(i==CharacterKinds?unsigned(I12_RandomCharacter):character_portrait(Character(model)));
        Canvas shade=c;shade.tint_red=shade.tint_green=shade.tint_blue=0;shade.picture(portrait,center-pw/2+2,y-ph/2+2,pw,ph,64);
        if(selected&&portrait_outline>=0)c.picture(portrait_outline,center-ow/2,y-oh/2,ow,oh,motion.outline_alpha(a.map_clock_ms),presentation::Add);
        small_material_icon(c,portrait,center-pw/2,y-ph/2,pw,ph,motion.saturation(i,a.map_clock_ms),motion.value(i,a.map_clock_ms));
        c.action(portrait_x(i)-1,PortraitTop-2,PortraitWidth+2,PortraitHeight+4,actions[model]);
    }
    if(profile)profile(RuntimeRenderPart::CharacterPortraits,false);
    // Back and Confirm keep their original positions around the portraits
    // in draw order. This part has two samples per character-screen frame.
    if(profile)profile(RuntimeRenderPart::CharacterControls,true);
    native_original_confirm(c,UiAction::StartCharacter,motion.opened);
    if(profile)profile(RuntimeRenderPart::CharacterControls,false);
    if(profile)profile(RuntimeRenderPart::CharacterAscension,true);
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
    if(profile)profile(RuntimeRenderPart::CharacterAscension,false);
}

static uint16_t character_departure_black(uint16_t color,unsigned keep){
    // Independent red/blue lanes share one multiply and exact /255 rounding.
    // The intervening eleven bits prevent either channel carrying into the
    // other, including the endpoints of the source full-screen black fade.
    uint32_t rb=(((uint32_t(color)&0xf800u)<<5)|(color&31u))*keep+0x00800080u;
    rb=(rb+((rb>>8)&0x00ff00ffu))>>8;
    return uint16_t(((rb>>5)&0xf800u)|(div255(((color>>5)&63u)*keep)<<5)|(rb&31u));
}
struct CharacterDepartureShade {
    uint16_t red[32],green[64],blue[32];
    explicit CharacterDepartureShade(unsigned keep){
        for(unsigned i=0;i<64;++i){green[i]=uint16_t(div255(i*keep)<<5);
            if(i<32){blue[i]=uint16_t(div255(i*keep));red[i]=uint16_t(blue[i]<<11);}}
    }
    uint16_t apply(uint16_t color)const{return red[color>>11]|green[(color>>5)&63]|blue[color&31];}
};
static bool character_departure_direct(const Canvas&c){
    return c.opacity==255&&!c.coverage&&!c.occlusion&&!c.color_matrix;
}
static void character_departure_dim(Canvas&c,unsigned alpha){
    if(!alpha)return;if(alpha>=255){c.rect(0,0,320,240,0);return;}
    const bool direct=character_departure_direct(c);const unsigned keep=255-alpha;
    if(direct){const CharacterDepartureShade shade(keep);
        const int width=mn(c.clip_w,320),height=mn(c.clip_h,240);
        for(int y=0;y<height;++y){c.poll_row(unsigned(y));auto*out=c.pixels+y*c.pitch;
            for(int x=0;x<width;++x)out[x]=shade.apply(out[x]);}
        return;
    }
    for(int y=0;y<mn(c.clip_h,240);++y){c.poll_row(unsigned(y));for(int x=0;x<mn(c.clip_w,320);++x){
        const unsigned at=y*c.pitch+x;
        c.paint(at,0,alpha);
    }}
}
static void character_departure_screen(Canvas&c){
    const auto&m=c.a.character_departure;if(!m.active())return;c.a.hit_count=0;
    const unsigned phase=m.progress(c.a.map_clock_ms);
    if(m.phase==2){
        // NTransition.FadeIn: default fade material, CUBIC_IN threshold 1→0.
        const unsigned cubic=unsigned((uint64_t(phase)*phase*phase+8388608u)>>24);
        character_departure_dim(c,255*(4096-cubic)/4096);return;
    }
    if(!phase)return;
    if(m.phase==3){
        // Default fade material has a linear threshold in parallel with
        // SimpleTransition's QuadIn black overlay.
        const unsigned threshold=phase*255u/4096u,shade=(phase*phase*255u+8388608u)>>24;
        character_departure_dim(c,255-div255((255-threshold)*(255-shade)));return;
    }
    // FadeOut runs the texture wipe and SimpleTransition QUAD_IN in parallel.
    const unsigned shade=(phase*phase*255u+8388608u)>>24,keep=255-shade;
    // Original texture_transition shader: step(1-red, mix(-.1,1.1,t)).
    // The RGB565 grayscale ordering permits one comparison per LCD pixel;
    // no full-screen intermediate buffer is allocated for this 800 ms wipe.
    const int cutoff=(255*(11*4096-12*int(phase))+40959)/40960;
    if(cutoff>255){character_departure_dim(c,shade);return;}if(cutoff<=0){c.rect(0,0,320,240,0);return;}
    const unsigned asset=I12_CharacterTransitionFirst+(m.character==unsigned(Character::Silent)?1:m.character==unsigned(Character::Necrobinder)?2:0);
    const auto mask=c.a.content.image(c.a.content.ui_image(asset));
    if(c.a.content.version()<Ui12ContentVersion||!mask.pixels||mask.w!=320||mask.h!=240){character_departure_dim(c,phase*255/4096);return;}
    const uint16_t threshold=rgb(cutoff,cutoff,cutoff);
    const bool direct=character_departure_direct(c);
    if(direct&&mask.format!=4){
        const CharacterDepartureShade fade(keep);const int width=mn(c.clip_w,320),height=mn(c.clip_h,240);
        if(mask.palette){uint8_t opaque[256];
            for(unsigned i=0;i<256;++i)opaque[i]=(unsigned(mask.palette[i*2])|unsigned(mask.palette[i*2+1])<<8)>=threshold;
            for(int y=0;y<height;++y){c.poll_row(unsigned(y));auto*out=c.pixels+y*c.pitch;const auto*row=mask.pixels+y*320;
                for(int x=0;x<width;++x)out[x]=opaque[row[x]]?0:fade.apply(out[x]);}
        }else for(int y=0;y<height;++y){c.poll_row(unsigned(y));auto*out=c.pixels+y*c.pitch;const auto*row=mask.pixels+y*640;
            for(int x=0;x<width;++x)out[x]=(unsigned(row[x*2])|unsigned(row[x*2+1])<<8)>=threshold?0:fade.apply(out[x]);}
        return;
    }
    if(mask.format==4){character_departure_dim(c,shade);const uint8_t*run=mask.pixels;
        for(int y=0;y<240;++y){c.poll_row(unsigned(y));const unsigned count=run[0]|unsigned(run[1])<<8;run+=2;
            for(unsigned span=0;span<count;++span){const unsigned left=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,stride=run[4]?3:2;run+=5;
                for(unsigned i=0;i<n;++i,run+=stride)if(left+i<unsigned(c.clip_w)&&y<c.clip_h&&(unsigned(run[0])|unsigned(run[1])<<8)>=threshold){
                    if(direct)c.pixels[y*c.pitch+left+i]=0;else c.paint(y*c.pitch+left+i,0);
                }
            }
        }
    }else{
        uint8_t opaque[256];if(mask.palette)for(unsigned i=0;i<256;++i)opaque[i]=(unsigned(mask.palette[i*2])|unsigned(mask.palette[i*2+1])<<8)>=threshold;
        for(int y=0;y<mn(c.clip_h,240);++y){c.poll_row(unsigned(y));for(int x=0;x<mn(c.clip_w,320);++x){
            const unsigned at=y*320+x;const bool clear=mask.palette?opaque[mask.pixels[at]]:
                (unsigned(mask.pixels[at*2])|unsigned(mask.pixels[at*2+1])<<8)>=threshold;
            const unsigned out=y*c.pitch+x;
            if(direct)c.pixels[out]=clear?0:character_departure_black(c.pixels[out],keep);
            else if(clear||shade)c.paint(out,0,clear?255:shade);
        }}
    }
}
