// hover_tip.tscn / NHoverTipSet: a 360px column in the 1680px viewport.
// Text is actual native 5px Kreon, with the source 22/16/45/28 margins and
// zero VBox separation. Old packs select their available font band instead.
static constexpr int SourceHoverWidth=69,SourceHoverLeft=4,SourceHoverRight=9,SourceHoverTop=3,SourceHoverBottom=5;
static int source_hover_size(const Canvas&c){return c.a.content.font_size(5);}
static int source_hover_step(const Canvas&c){return source_hover_size(c)+1;}
static int source_hover_overhead(const Canvas&c,bool title=true){return SourceHoverTop+SourceHoverBottom+(title?source_hover_step(c):0);}
static int source_hover_body_width(int width){return mx(1,width-SourceHoverLeft-SourceHoverRight);}
static int source_hover_paragraph(Canvas&c,const char*body,int x,int y,int width,int lines,int skip=0,uint16_t color=rgb(255,246,226)){
    Canvas text=c;text.font_face=0;text.bold_face=5;const int size=source_hover_size(c);
    char localized[1024];body=c.a.ui_text(body,localized,sizeof localized);
    const auto metrics=[&](const RichChar&ch,int px){const auto g=c.a.content.glyph_metrics(ch.codepoint,px,text.font_style(ch));return fitted_text::Metric{g.advance,g.advance};};
    if(!lines)return fitted_text::line_count(body,width,size,metrics);
    const uint16_t colors[]={color,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
    const fitted_text::Plan plan{size,skip+lines,skip+lines,size+1,false};
    text.clip_w=mn(text.clip_w,x+width);text.clip_h=mn(text.clip_h,y+lines*(size+1)+2);
    fitted_text::visit(body,width,plan,metrics,[&](const RichChar&ch,int at,int row,int px){
        if(row<skip||ch.index>=c.text_visible_characters)return;
        static const int wave[]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
        int dy=c.a.text_effects&&ch.style.wave?wave[(c.a.animation/2+ch.index*2)%16]:0;
        if(c.a.text_effects&&ch.style.thinky_dots)dy+=rich_thinky_offset(c.a.map_clock_ms,ch.index-ch.style.thinky_start);
        const int dx=c.a.text_effects&&ch.style.jitter?int((c.a.animation/2+ch.index*7)%3)-1:0;
        const auto ink=c.a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,c.a.map_clock_ms,at*26000/mx(1,size)):colors[ch.style.color];
        const auto g=c.a.content.glyph(ch.codepoint,px,text.font_style(ch));
        text.character(ch.codepoint,g,x+at+dx,y+(row-skip)*(size+1)+dy,ink);
    });return skip+lines;
}
static void source_hover_skin(Canvas&c,HoverRect r,bool debuff=false){
    int id=c.a.content.world_image("common",debuff?"HOVER_TIP_DEBUFF":"HOVER_TIP");
    if(id<0)id=c.a.content.ui_image(U_HoverTip);const auto p=c.a.content.image(id);
    if(!p.pixels||p.format>3){c.rect(r.x,r.y,r.w,r.h,debuff?rgb(59,35,45):Panel);c.box(r.x,r.y,r.w,r.h,Gold);return;}
    // The stored texture is larger than its 4:3 projection. Keep source
    // patch margins in the projected 65x20 domain, then sample existing art.
    uint16_t columns[320];const int left=mx(0,-r.x),right=mn(r.w,c.clip_w-r.x);
    for(int xx=left;xx<right;++xx)columns[xx-left]=uint16_t(nine_patch::coordinate(xx,r.w,65,10,17)*p.w/65);
    auto draw=[&](int dx,int dy,bool shadow){
        for(int yy=mx(0,-r.y-dy);yy<mn(r.h,c.clip_h-r.y-dy);++yy){
            const int row=nine_patch::coordinate(yy,r.h,20,8,6)*p.h/20*p.w;
            for(int xx=left;xx<right&&r.x+xx+dx<c.clip_w;++xx){
                const int at=row+columns[xx-left];unsigned alpha=p.alpha?p.alpha[at]:255;if(shadow)alpha=div255(alpha*64);if(!alpha)continue;
                const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
                c.paint((r.y+yy+dy)*c.pitch+r.x+xx+dx,shadow?0:uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha);
            }
        }
    };draw(1,1,true);draw(0,0,false);
}
static void source_hover_title(Canvas&c,const char*title,HoverRect r,int icon=-1){
    if(!*title)return;Canvas label=c;label.font_face=source_label_face(c.a.content.english(),c.a.content.version()>=28?5:0);
    const int size=source_hover_size(c),width=source_hover_body_width(r.w)-(icon>=0?7:0);
    label.text_fitted(title,r.x+SourceHoverLeft,r.y+SourceHoverTop,width,size,rgb(239,200,81),size);
    if(icon>=0)c.picture(icon,r.x+r.w-SourceHoverRight-6,r.y+SourceHoverTop,6,6);
}
static int source_hover_height(Canvas&c,const char*title,const char*body,int width=SourceHoverWidth){
    return mn(234,source_hover_overhead(c,*title)+source_hover_paragraph(c,body,0,0,source_hover_body_width(width),0)*source_hover_step(c));
}
static void source_hover_box(Canvas&c,const char*title,const char*body,int x,int y,int width=SourceHoverWidth,int icon=-1,bool debuff=false){
    width=mn(width,SourceHoverWidth);const int step=source_hover_step(c),overhead=source_hover_overhead(c,*title);
    const int lines=source_hover_paragraph(c,body,0,0,source_hover_body_width(width),0);
    const int height=mn(234,overhead+lines*step);x=mx(3,mn(317-width,x));y=mx(3,mn(237-height,y));
    const HoverRect r{x,y,width,height};source_hover_skin(c,r,debuff);source_hover_title(c,title,r,icon);
    source_hover_paragraph(c,body,x+SourceHoverLeft,y+SourceHoverTop+(*title?step:0),source_hover_body_width(width),mx(0,(height-overhead)/step));
}
