// NInspectRelicScreen keeps its paper, frame and separate image modulation.
// The LCD reading pane uses larger native text and scrolls the full prose.
static uint16_t relic_inspect_frame_color(uint16_t color,unsigned rarity,unsigned brightness=255){
    // Q16 inverse(M) * value * saturation * hue((1-h)*tau) * M, where M
    // is hsv.gdshader's authored RGB-to-YIQ matrix. Source RGB is quantized
    // once to the calculator's RGB565 image; clipping follows modulation.
    static constexpr int32_t matrices[6][9]={
        {28264,27965,2878,11935,40565,6454,19088,21514,18202},
        {-18799,64325,26980,45404,20010,6520,4514,99509,-32220},
        {64812,8875,1724,4502,69135,1717,4490,8819,62006},
        {-75970,120927,11279,50581,-23382,28388,84800,90170,-120041},
        {14236,17083,27906,30346,35255,-6752,-38932,77384,20579},
        {126187,2058,-69699,-60764,90984,29068,136674,-170179,92052}
    };
    const auto&matrix=matrices[rarity<=2?0:rarity<=7?rarity-2:0];
    const int r=((color>>11)*4096+15)/31,g=(((color>>5)&63)*4096+31)/63,b=((color&31)*4096+15)/31;
    const auto channel=[&](unsigned row,unsigned maximum){
        int q=(matrix[row]*r+matrix[row+1]*g+matrix[row+2]*b)/65536;
        q=q*int(brightness)/255;q=mn(4096,mx(0,q));return (unsigned(q)*maximum+2048)/4096;
    };
    return uint16_t(channel(0,31)<<11|channel(3,63)<<5|channel(6,31));
}
static uint16_t relic_inspect_rarity_color(unsigned rarity){
    if(rarity==3||rarity==5)return rgb(135,206,235);
    if(rarity==4)return rgb(239,200,81);
    if(rarity==6)return rgb(127,255,0);
    if(rarity==7)return rgb(255,85,85);return rgb(255,246,226);
}
static void relic_inspect_frame(Canvas c,int x,int y,unsigned rarity){
    if(!c.a.content.has_inspection_assets())return;
    const auto p=c.a.content.image(c.a.content.ui_image(I9_RelicInspectFrame));if(!p.pixels||p.format>3)return;
    const unsigned brightness=c.brightness;c.brightness=255;
    constexpr int size=relic_inspect_layout::FrameSize;
    for(int yy=mx(0,-y);yy<size&&y+yy<c.clip_h;++yy)for(int xx=mx(0,-x);xx<size&&x+xx<c.clip_w;++xx){
        const unsigned at=unsigned(yy*p.h/size)*p.w+unsigned(xx*p.w/size),alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
        const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
        c.paint((y+yy)*c.pitch+x+xx,relic_inspect_frame_color(uint16_t(pixel[0]|unsigned(pixel[1])<<8),rarity,brightness),alpha);
    }
}
static void relic_inspect_fit_image(Canvas&c,int image,int x,int y,int w,int h){
    const auto p=c.a.content.image(image);if(!p.pixels||!p.w||!p.h)return;
    const int width=int(p.w)*h>int(p.h)*w?w:int(p.w)*h/p.h;
    const int height=int(p.w)*h>int(p.h)*w?int(p.h)*w/p.w:h;
    c.picture(image,x+(w-width)/2,y+(h-height)/2,width,height);
}
static int relic_inspect_paragraph(Canvas c,const char*text,int x,int y,int width,int requested,bool center,bool draw){
    // Measure the complete original VBox. Its larger LCD presentation is
    // clipped by the caller's scrolling viewport, with no omitted prose.
    const int size=c.a.content.font_size(requested,c.font_face);
    const auto metrics=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
    const int lines=fitted_text::line_count(text,width,size,metrics),step=size+2;
    if(!draw||!lines)return lines?lines*step:0;
    const fitted_text::Plan plan{size,lines,lines,step,false};
    int widths[64]{};if(center)fitted_text::visit(text,width,plan,metrics,[&](const RichChar&ch,int at,int row,int px){if(row<64)widths[row]=at+metrics(ch,px).width;});
    const uint16_t colors[]={rgb(255,246,226),Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
    c.clip_w=mn(c.clip_w,x+width);
    fitted_text::visit(text,width,plan,metrics,[&](const RichChar&ch,int at,int row,int px){
        const int left=x+at+(center&&row<64?mx(0,(width-widths[row])/2):0),top=y+row*step;
        const auto g=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));
        Canvas shadow=c;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*64));shadow.character(ch.codepoint,g,left+1,top,0,true);
        c.character(ch.codepoint,g,left,top,rich_color(ch.style.color,colors));
    });return lines*step;
}
static void relic_inspect_text(const App&a,RelicId id,char*name,unsigned name_cap,char*description,unsigned description_cap,char*flavor,unsigned flavor_cap){
    const bool canonical=a.relic_inspect.source==RelicInspectSource::Collection||a.history_open;
    if(canonical&&!a.library_item_seen(1,int(id))){
        TextBuffer n{name,name_cap,0},d{description,description_cap,0};n.text(a.content.world_string("inspect_relic_screen","UNDISCOVERED_TITLE"));d.text(a.content.world_string("inspect_relic_screen","UNDISCOVERED_DESCRIPTION"));return;
    }
    TextBuffer n{name,name_cap,0};n.text(a.relic_title(id,canonical?int(WorldRelicCapacity):a.relic_inspect.owned));
    a.relic_description(id,description,description_cap,canonical?int(WorldRelicCapacity):a.relic_inspect.owned);
    char key[128]{},raw[1024]{};TextBuffer k{key,sizeof key,0};k.text(world_relic_key(id));k.text(".flavor");a.localized_text("relics",key,raw,sizeof raw,int(id));
    // The source FlavorText is a centered RichTextLabel. Its actual locale
    // values may contain [center]; alignment belongs to this label, not text.
    TextBuffer f{flavor,flavor_cap,0};for(const char*p=raw;*p;){
        if(p[0]=='['){const char*tags[]={"[center]","[/center]"};bool found=false;
            for(const char*tag:tags){unsigned i=0;while(tag[i]&&p[i]==tag[i])++i;if(!tag[i]){p+=i;found=true;break;}}if(found)continue;}
        f.ch(*p++);
    }
}
static void relic_inspect_arrow(Canvas c,unsigned index,int x,bool interactive){
    auto&a=c.a;auto&motion=a.relic_inspect.arrows[index];constexpr int box=relic_inspect_layout::ArrowSize,y=relic_inspect_layout::ArrowY;
    const UiAction action=index?UiAction::InspectNextRelic:UiAction::InspectPreviousRelic;
    const bool focused=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+box&&a.pointer.y>=y&&a.pointer.y<y+box;
    const bool pressed=focused&&a.pointer.held&&a.pointer.captured.kind==HitKind::UiAction&&a.pointer.captured.value==int(action);
    if(interactive)motion.observe(pressed?2:focused?1:0,a.map_clock_ms);
    const int scale=motion.scale(a.map_clock_ms),w=(13*scale+2048)/4096,h=(box*scale+2048)/4096;
    // The arrow texture has the same source 0.9/1.2/0.7 HSV shader as the
    // card inspector. Apply brightness before saturation to entering pixels.
    small_material_icon(c,a.content.ui_image(index?I7_SettingsArrowRight:I7_SettingsArrowLeft),x+(box-w)/2,y+(box-h)/2,w,h,1000,unsigned(motion.shade(a.map_clock_ms))*1000/255,true);
    if(interactive)c.action(x,y,box,box,action);
}
static void item_detail_screen(Canvas&c){
    using namespace relic_inspect_layout;
    auto&a=c.a;a.sync_relic_inspect();auto&view=a.relic_inspect;
    if(a.detail_item>=WorldRelicKinds){
        // The developer-only potion mutation tool is intentionally separate
        // from the release PotionLab, whose entries remain hover-only.
        if(a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat){
            const Potion potion=Potion(a.detail_item-WorldRelicKinds+1);bool room=false;for(unsigned i=0;i<potion_slot_count(a.game);++i)room|=potion_at(a.game,i)==Potion::Empty;
            a.hit_count=0;c.centered(potion_name(int(potion),&a.content),30,89,260,10,Gold);
            c.button(room?"获得药水":"药水栏已满",95,121,130,false,24);c.action(95,121,130,24,UiAction::LabPotion,room);
            c.hit(0,0,320,80,HitKind::Command,int(Key::Back));
        }
        return;
    }
    const bool closing=a.detail_item<0;if(closing&&!view.closing(a.map_clock_ms))return;
    const int id=closing?view.model:a.detail_item;if(id<0||id>=WorldRelicKinds)return;
    if(!closing){a.hit_count=0;c.action(0,0,320,240,UiAction::CloseRelicInspect);}
    Canvas black=c;black.opacity=uint8_t(div255(unsigned(black.opacity)*view.backstop(a.map_clock_ms)));black.rect(0,0,320,240,0);
    const int dx=view.x(a.map_clock_ms)-relic_inspect_layout::PopupX,dy=view.y(a.map_clock_ms)-relic_inspect_layout::PopupY;
    Canvas layer=c;layer.opacity=uint8_t(div255(unsigned(layer.opacity)*view.popup_alpha(a.map_clock_ms)));layer.brightness=uint8_t(view.popup_brightness(a.map_clock_ms));
    // Popup is a Control with MouseFilter=Stop, while its visual children
    // ignore the mouse. A click on the paper must not reach Backstop.
    // Close removes the backdrop/arrow filters immediately, but the paper
    // retains Stop until the screen's 250ms visibility callback completes.
    c.hit(PopupX+dx,PopupY+dy,PopupWidth,PopupHeight,HitKind::None);
    if(layer.opacity){
        layer.ui(U_RewardPanel,PopupX+dx,PopupY+dy,PopupWidth,PopupHeight);
        Canvas frame_bg=layer;frame_bg.tint_red=21;frame_bg.tint_green=71;frame_bg.tint_blue=63;frame_bg.opacity=uint8_t(div255(unsigned(frame_bg.opacity)*128));frame_bg.ui(U_RewardPanel,FrameX+1+dx,FrameY+dy,FrameSize-1,FrameSize-1);
        const bool seen=view.source!=RelicInspectSource::Collection||a.library_item_seen(1,id);
        Canvas icon=layer;if(!seen)icon.brightness=uint8_t(div255(unsigned(icon.brightness)*26));relic_inspect_fit_image(icon,a.relic_face_image(RelicId(id),a.relic_inspect_character()),IconX+dx,IconY+dy,IconSize,IconSize);
        const unsigned rarity=world_relic_definitions[id].rarity;relic_inspect_frame(layer,FrameX+dx,FrameY+dy,rarity);
        char name[256]{},description[4096]{},flavor[1024]{};relic_inspect_text(a,RelicId(id),name,sizeof name,description,sizeof description,flavor,sizeof flavor);
        Canvas title=layer;title.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),a.content.version()>=28?5:0));
        Canvas shadow=title;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*64));shadow.fitted(name,NameX+1+dx,NameY+1+dy,NameWidth,28,12,12,0,true,false);
        title.fitted(name,NameX+dx,NameY+dy,NameWidth,28,12,12,rgb(239,200,81),true,false);
        if(seen){static const char*const rarities[]={"NONE","STARTER","COMMON","UNCOMMON","RARE","SHOP","EVENT","ANCIENT"};
            char key[64]{};TextBuffer k{key,sizeof key,0};k.text("RELIC_RARITY.");k.text(rarities[rarity<8?rarity:0]);
            layer.fitted(a.content.world_string("gameplay_ui",key),RarityX+dx,RarityY+dy,RarityWidth,13,10,10,relic_inspect_rarity_color(rarity),true,true);
        }
        Canvas body=layer;body.font_face=0;body.bold_face=5;
        const int body_height=relic_inspect_paragraph(body,description,BodyX+dx,0,BodyWidth,10,false,false);
        Canvas flavor_layer=layer;flavor_layer.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),a.content.version()>=28?6:0));
        const int flavor_height=relic_inspect_paragraph(flavor_layer,flavor,BodyX+dx,0,BodyWidth,10,true,false);
        const int flavor_y=body_height+10,total=body_height+(flavor_height?10+flavor_height:0);
        view.scroll_maximum=int16_t(mx(0,total-BodyHeight));a.detail_scroll=mn(mx(0,a.detail_scroll),int(view.scroll_maximum));
        const int top=BodyY+dy,visible=mn(BodyHeight,mx(0,layer.clip_h-top));
        if(top>=0&&visible){
            body.pixels+=top*body.pitch;if(body.coverage)body.coverage+=top*body.pitch;body.clip_h=visible;
            relic_inspect_paragraph(body,description,BodyX+dx,-a.detail_scroll,BodyWidth,10,false,true);
            flavor_layer.pixels+=top*flavor_layer.pitch;if(flavor_layer.coverage)flavor_layer.coverage+=top*flavor_layer.pitch;flavor_layer.clip_h=visible;
            if(flavor_height){Canvas divider=flavor_layer;for(int i=0;i<90;++i){const unsigned edge=unsigned(mn(i,89-i));const unsigned alpha=edge>=16?128:edge*128/16;
                divider.opacity=uint8_t(div255(unsigned(layer.opacity)*alpha));divider.rect(BodyX+(BodyWidth-90)/2+dx+i,body_height+5-a.detail_scroll,1,1,rgb(255,246,226));}
                relic_inspect_paragraph(flavor_layer,flavor,BodyX+dx,flavor_y-a.detail_scroll,BodyWidth,10,true,true);
            }
        }
        if(view.scroll_maximum){
            const int track=BodyY+dy,height=BodyHeight;layer.rect(BodyX+BodyWidth+5+dx,track,2,height,rgb(44,69,77));
            layer.rect(BodyX+BodyWidth+3+dx,track+a.detail_scroll*(height-8)/view.scroll_maximum,6,8,Gold);
            char page[24]{};TextBuffer text{page,sizeof page,0};text.number(a.detail_scroll*100/view.scroll_maximum);text.ch('%');
            layer.fitted(page,54+dx,214+dy,96,15,10,10,Muted,true,true);
            layer.ui(I7_SettingsArrowLeft,24+dx,212+dy,18,18,a.detail_scroll?255:70);
            layer.ui(I7_SettingsArrowRight,164+dx,212+dy,18,18,a.detail_scroll<view.scroll_maximum?255:70);
            if(!closing){c.hit(18+dx,211+dy,30,20,HitKind::Scroll,-5,a.detail_scroll>0);c.hit(158+dx,211+dy,30,20,HitKind::Scroll,5,a.detail_scroll<view.scroll_maximum);}
        }
        if(view.reading_keywords)layer.box(212,221,104,18,Gold);
    }
    Canvas arrows=c;arrows.opacity=uint8_t(div255(unsigned(arrows.opacity)*view.arrow_alpha(a.map_clock_ms)));arrows.brightness=uint8_t(view.arrow_brightness(a.map_clock_ms));
    if(view.index>0)relic_inspect_arrow(arrows,0,view.arrow_x(0,a.map_clock_ms),!closing);
    if(view.index+1<view.count)relic_inspect_arrow(arrows,1,view.arrow_x(1,a.map_clock_ms),!closing);
}
