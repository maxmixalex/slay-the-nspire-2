#include "card_highlight_ui.inc"
// A sub-canvas clips the moving rows before rasterization. It shares the
// screen storage and adds no viewport image or render-time allocation.
static Canvas card_grid_canvas(Canvas&c,const CardGridScroll&grid){
    Canvas layer=c;layer.pixels+=grid.top*layer.pitch;
    if(layer.coverage)layer.coverage+=grid.top*layer.pitch;
    layer.clip_h=grid.height();return layer;
}
static void card_grid_hit(Canvas&c,const CardGridScroll&grid,int x,int y,int w,int h,HitKind kind,int value,bool enabled=true){
    const int top=mx(y,int(grid.top)),bottom=mn(y+h,int(grid.bottom));
    if(bottom>top)c.hit(x,top,w,bottom-top,kind,value,enabled);
}
static LocalizedValue card_grid_victory_value(const void*opaque,const char*key){return wt_equal(key,"Victories")?LocalizedValue{true,*static_cast<const uint32_t*>(opaque),1,nullptr}:LocalizedValue{};}
static void card_grid_stats(Canvas&c,CardKind kind,int cx,int cy,int scale){
    const int x=cx-117*scale*4/21000,y=cy-138*scale*4/21000,w=231*scale*4/21000,h=117*scale*4/21000;
    Canvas background=c;background.opacity=uint8_t(div255(unsigned(background.opacity)*186));background.rect(x,y,w,h,0);
    char label[128]{};LocalizedTemplate text(label,sizeof label,card_grid_victory_value,&c.a.progress.card_wins[kind]);text.for_language(c.a.content.language()).format(c.a.content.locale_string("card_library","VICTORIES"));
    // The source30px label inherits the actual holder scale (.8 normally,
    // 1.05 on focus). Render its native5/6px glyphs directly, keeping rich
    // color and alpha; the old8px temporary bitmap changed both shapes.
    const int size=c.a.content.font_size((30*scale*4+10500)/21000),width=c.width(label,size),left=x+(w-width)/2,top=y+(h-size-2)/2;
    const int dx=(3*scale*4+10500)/21000,dy=(2*scale*4+10500)/21000;
    // RichTextLabel defaults to clip_contents=true. It has no horizontal
    // autosize bound here: a very large counter keeps the source band and
    // is clipped to the Label, never rescaled or replaced with an ellipsis.
    const int clip_x=mx(0,x),clip_y=mx(0,y),clip_right=mn(c.clip_w,x+w),clip_bottom=mn(c.clip_h,y+h);
    if(clip_right<=clip_x||clip_bottom<=clip_y)return;
    Canvas label_layer=c;label_layer.pixels+=clip_y*c.pitch+clip_x;if(label_layer.coverage)label_layer.coverage+=clip_y*c.pitch+clip_x;
    label_layer.clip_w=clip_right-clip_x;label_layer.clip_h=clip_bottom-clip_y;
    for(unsigned pass=0;pass<2;++pass){
        Canvas ink=label_layer;if(!pass)ink.opacity=uint8_t(div255(unsigned(ink.opacity)*128));
        RichReader reader(label);RichChar ch{};int advance64=0;
        while(reader.next(ch)){
            const auto glyph=c.a.content.glyph(ch.codepoint,size,0);
            if(advance64)advance64+=c.a.content.glyph_kerning64(ch.previous,ch.codepoint,size,0);
            const uint16_t color=ch.style.color==1?rgb(239,200,81):rgb(255,246,226);
            if(ch.index<c.text_visible_characters)ink.character(ch.codepoint,glyph,left+text_pixel(advance64)-clip_x+(!pass?dx:0),top-clip_y+(!pass?dy:0),pass?color:0,!pass);
            advance64+=glyph.advance64?glyph.advance64:int(glyph.advance)*64;
        }
    }
}
struct CardGridPainter {
    Canvas&c;CardGridScroll&grid;int hover=-1;bool interactive=true;
    struct Face {int index=-1,fee=-2,stars=-2,instance=-1,deck=-1,scale=800;Card card{};const EventCardFacts*facts=nullptr;bool playable=true,chosen=false,known=true,upgrade_preview=false;} front;
    struct CardHit {int index=-1,value=0;HitKind kind=HitKind::None;bool enabled=true;} front_hit;
    CardGridPainter(Canvas&canvas,CardGridScroll&scroll,bool enabled=true):c(canvas),grid(scroll),interactive(enabled){
        const auto&p=c.a.pointer;
        if(!interactive||!p.visible||(grid.dragging&&p.dragging)||grid.thumb_drag||p.y<grid.top||p.y>=grid.bottom)return;
        // NGridCardHolder.MoveToFront keeps the enlarged card above its
        // neighbours. Test that holder first so crossing the overlap cannot
        // focus two cards or alternate ownership on consecutive frames.
        if(contains(grid.focused_index)){hover=grid.focused_index;return;}
        for(int i=grid.last()-1;i>=grid.first();--i)if(contains(i)){hover=i;break;}
    }
    bool contains(int index)const{
        if(index<grid.first()||index>=grid.last())return false;
        const int scale=grid.face_scale(index,c.a.map_clock_ms),w=face_width(scale),h=face_height(scale);
        const int x=grid.card_x(index)+(CardGridScroll::CardW-w)/2,y=grid.card_y(index)+(CardGridScroll::CardH-h)/2;
        return c.a.pointer.x>=x-2&&c.a.pointer.x<x+w+2&&c.a.pointer.y>=y-2&&c.a.pointer.y<y+h+2;
    }
    static int face_width(int scale){return (300*scale*4+10500)/21000;}
    static int face_height(int scale){return (422*scale*4+10500)/21000;}
    void draw(const Face&face){
        Canvas layer=card_grid_canvas(c,grid);const int w=face_width(face.scale),h=face_height(face.scale);
        const int x=grid.card_x(face.index)+(CardGridScroll::CardW-w)/2,y=grid.card_y(face.index)-grid.top+(CardGridScroll::CardH-h)/2;
        card_selection_highlight(layer,x,y,w,h,grid.observe_selection(face.index,face.chosen,c.a.map_clock_ms));
        draw_card_face(layer,face.card,x,y,w,h,face.fee,face.stars,face.instance,face.deck,face.facts,nullptr,nullptr,face.known,face.upgrade_preview);
        if(c.a.library&&!c.a.library_tab&&c.a.card_library.show_stats)card_grid_stats(layer,face.card.kind,x+w/2,y+h/2,face.scale);
        if(!face.playable)layer.dim(x,y,w,h,125);
    }
    void card(int index,Card card,bool focused,bool playable=true,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,bool chosen=false,bool known=true,bool upgrade_preview=false){
        if(!interactive)focused=false;else if(c.a.pointer.visible)focused=index==hover;
        const int scale=grid.observe_face(index,focused,c.a.map_clock_ms);
        Face face{index,fee,stars,instance,deck,scale,card,facts,playable,chosen,known,upgrade_preview};
        if(focused){front=face;grid.focused_index=int16_t(index);}else draw(face);
    }
    void draw_hit(const CardHit&hit){
        const int scale=grid.face_scale(hit.index,c.a.map_clock_ms),w=face_width(scale),h=face_height(scale);
        const int x=grid.card_x(hit.index)+(CardGridScroll::CardW-w)/2,y=grid.card_y(hit.index)+(CardGridScroll::CardH-h)/2;
        card_grid_hit(c,grid,x-2,y-2,w+4,h+4,hit.kind,hit.value,hit.enabled);
    }
    void hit(int index,HitKind kind,int value,bool enabled=true){
        if(!interactive)return;
        const CardHit hit{index,value,kind,enabled};if(front.index==index)front_hit=hit;else draw_hit(hit);
    }
    void finish(){
        if(front.index>=0){draw(front);front.index=-1;}
        if(front_hit.index>=0){draw_hit(front_hit);front_hit.index=-1;}
    }
};
static void card_grid_scrollbar(Canvas&c,CardGridScroll&grid,CardGridSurface surface){
    if(!grid.has_scrollbar())return;
    const int top=grid.bar_top(),bottom=grid.bar_bottom(),center=grid.thumb_y();
    const int x=surface==CardGridSurface::WorldChoice&&c.a.world_selection_kind()==WorldSelectionKind::Enchant?187:301;
    c.ui(I6_ScrollbarTrack,x,top,10,bottom-top);c.ui(I6_ScrollbarEdge,x,top-10,10,10);
    ui_picture_transform(c,c.a.content.ui_image(I6_ScrollbarEdge),x+5,bottom+5,10,10,0,-1024);
    native_scrollbar_grabber(c,x+5-CardGridScroll::ThumbSize/2,center-CardGridScroll::ThumbSize/2,
                             CardGridScroll::ThumbSize,CardGridScroll::ThumbSize);
    c.hit(x-7,top-8,23,bottom-top+16,HitKind::GridScroll,int(surface));
}
