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
struct CardGridPainter {
    Canvas&c;CardGridScroll&grid;int hover=-1;
    struct Face {int index=-1,fee=-2,stars=-2,instance=-1,deck=-1,scale=800;Card card{};const EventCardFacts*facts=nullptr;bool playable=true,chosen=false,known=true;} front;
    struct CardHit {int index=-1,value=0;HitKind kind=HitKind::None;bool enabled=true;} front_hit;
    CardGridPainter(Canvas&canvas,CardGridScroll&scroll):c(canvas),grid(scroll){
        const auto&p=c.a.pointer;
        if(!p.visible||(grid.dragging&&p.dragging)||grid.thumb_drag||p.y<grid.top||p.y>=grid.bottom)return;
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
    static int face_width(int scale){return (300*scale*2+4500)/9000;}
    static int face_height(int scale){return (422*scale*2+4500)/9000;}
    void draw(const Face&face){
        Canvas layer=card_grid_canvas(c,grid);const int w=face_width(face.scale),h=face_height(face.scale);
        const int x=grid.card_x(face.index)+(CardGridScroll::CardW-w)/2,y=grid.card_y(face.index)-grid.top+(CardGridScroll::CardH-h)/2;
        draw_card_face(layer,face.card,x,y,w,h,face.fee,face.stars,face.instance,face.deck,face.facts,nullptr,nullptr,face.known);
        if(!face.playable)layer.dim(x,y,w,h,125);
        if(face.chosen){layer.box(x-3,y-3,w+6,h+6,Green);layer.rect(x+w-16,y+h-18,14,17,Panel);layer.text("✓",x+w-15,y+h-18,12,Green);}
    }
    void card(int index,Card card,bool focused,bool playable=true,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,bool chosen=false,bool known=true){
        if(c.a.pointer.visible)focused=index==hover;
        const int scale=grid.observe_face(index,focused,c.a.map_clock_ms);
        Face face{index,fee,stars,instance,deck,scale,card,facts,playable,chosen,known};
        if(focused){front=face;grid.focused_index=int16_t(index);}else draw(face);
    }
    void draw_hit(const CardHit&hit){
        const int scale=grid.face_scale(hit.index,c.a.map_clock_ms),w=face_width(scale),h=face_height(scale);
        const int x=grid.card_x(hit.index)+(CardGridScroll::CardW-w)/2,y=grid.card_y(hit.index)+(CardGridScroll::CardH-h)/2;
        card_grid_hit(c,grid,x-2,y-2,w+4,h+4,hit.kind,hit.value,hit.enabled);
    }
    void hit(int index,HitKind kind,int value,bool enabled=true){
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
    c.ui(I6_ScrollbarTrack,300,top,11,bottom-top);c.ui(I6_ScrollbarEdge,300,top-11,11,11);
    ui_picture_transform(c,c.a.content.ui_image(I6_ScrollbarEdge),305,bottom+5,11,11,0,-1024);
    const bool hover=c.a.pointer.visible&&c.a.pointer.x>=295&&c.a.pointer.x<316&&c.a.pointer.y>=center-10&&c.a.pointer.y<center+10;
    if(hover!=grid.thumb_hover){grid.thumb_hover=hover;grid.hover_changed=c.a.map_clock_ms;}
    const unsigned elapsed=c.a.map_clock_ms-grid.hover_changed;
    const int recovery=elapsed>=500?0:4096-int(health_bar::expo_out(elapsed*2));
    const int scale=hover?1178:1024+154*recovery/4096;
    const int size=CardGridScroll::ThumbSize*scale/1024;
    c.ui(I6_ScrollbarGrabber,305-size/2,center-size/2,size,size);
    c.hit(294,top-8,23,bottom-top+16,HitKind::GridScroll,int(surface));
}
