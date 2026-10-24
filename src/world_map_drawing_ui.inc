namespace {
struct MapDrawingCache {
    MapDrawingRaster raster{};
    const App*owner=nullptr;
    uint32_t revision=0;
    int origin=0;
    uint16_t count=0;
};
static MapDrawingCache map_drawing_cache;
}
static void map_drawing_ink(Canvas&c,const WorldMapView&view){
    auto&a=c.a;auto&input=a.map_drawing;const auto&document=a.progress.map_drawing;
    input.paper_origin=view.origin;input.transform_valid=true;
    if(!document.count||!document.matches(a.game.world.run_seed,a.progress.current.serial,a.game.world.act_index))return;
    auto&cache=map_drawing_cache;
    if(cache.owner!=&a||cache.revision!=input.revision||cache.origin!=view.origin||cache.count!=document.count){
        const auto draw=a.content.image(a.content.ui_image(I8_MapDrawTrail));
        const auto erase=a.content.image(a.content.ui_image(I8_MapEraseTrail));
        cache.raster.rebuild(document,{view.origin},{draw.alpha,draw.w,draw.h},{erase.alpha,erase.w,erase.h});
        cache.owner=&a;cache.revision=input.revision;cache.origin=view.origin;cache.count=document.count;
    }
    static const uint16_t colors[]={rgb(203,40,43),rgb(47,103,41),rgb(13,99,140),rgb(172,4,134),rgb(147,82,6)};
    const uint16_t ink=colors[unsigned(a.game.character)<CharacterKinds?unsigned(a.game.character):0];
    for(unsigned row=0;row<MapDrawingRaster::Height;++row)
        for(unsigned x=cache.raster.left[row];x<cache.raster.right[row];++x){
            const unsigned alpha=cache.raster.alpha[row*MapDrawingRaster::Width+x];
            if(alpha)c.paint((row+WorldMapTop)*c.pitch+x,ink,alpha);
        }
}
static void map_drawing_tools(Canvas&parent,const MapWindowSample*window){
    auto&a=parent.a;Canvas c=parent;
    if(window){c.opacity=map_window_product(c.opacity,window->drawing_tools.alpha);
        c.brightness=map_window_product(c.brightness,window->drawing_tools.rgb);}
    // DrawingTools: (56,-108)..(264,-40); 60/60/68 source-unit buttons.
    const int left=12,top=216,width=46,height=15;
    Canvas background=c;background.tint_red=background.tint_green=background.tint_blue=0;
    if(!background.nine_image(a.content.ui_image(I7_TinyPanel),left,top,width,height,2,2,2,2,192))background.rect(left,top,width,height,0);
    int hovered=-1;
    for(unsigned tool=0;tool<3;++tool){const int x=14+int(tool)*13,w=tool==2?15:13;
        const bool focus=a.map_drawing.tool_focus==int(tool)||(a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=top&&a.pointer.y<top+height);
        const bool selected=(tool==0&&a.map_drawing.mode==MapDrawingMode::Draw)||(tool==1&&a.map_drawing.mode==MapDrawingMode::Erase);
        if(focus)hovered=int(tool);
        const int base=tool==0?I8_MapDraw:tool==1?I8_MapErase:I8_MapClear;
        Canvas icon=c;const bool active=focus||selected;
        if(active){icon.tint_red=tool==0?87:255;icon.tint_green=tool==0?196:tool==1?87:229;icon.tint_blue=tool==0?255:tool==1?87:125;}
        const int size=focus?16:15,image=a.content.ui_image(base+(selected||(tool==2&&focus)?1:0));
        if(image>=0)icon.picture(image,x+(w-size)/2,top+(height-size)/2,size,size,active?255:128,presentation::Add);
        if(a.map_drawing_ready())c.hit(x-1,top-1,w+1,height+2,HitKind::MapDrawingTool,int(tool));
    }
    if(a.map_drawing.tool_focus>=0){c.text("F",2,top+2,8,Gold,9);}
    if(hovered>=0&&a.map_drawing_ready()){
        const char*keys[]={"DRAWING_BUTTON","ERASING_BUTTON","CLEAR_DRAWING"};char key[80]{};
        TextBuffer title_key{key,sizeof key,0};title_key.text(keys[hovered]);title_key.text(hovered==2?".title":a.pointer.visible?".title_mkb":".title_controller");
        const char*title=a.content.world_string("map",key);char title_copy[192]{};TextBuffer copy{title_copy,sizeof title_copy,0};copy.text(title);
        key[0]=0;TextBuffer body_key{key,sizeof key,0};body_key.text(keys[hovered]);body_key.text(".description");
        const char*body=a.content.world_string("map",key);
        // All three source controls anchor to DrawingTools+(10,-132),
        // not to the particular icon or a separate full-width reader.
        if(*title_copy)source_hover_box(c,title_copy,body,left+2,top-25);
    }
}
static bool map_drawing_cursor(Canvas&c){
    auto&a=c.a;const auto&input=a.map_drawing;
    if(!a.map_drawing_ready()||input.mode==MapDrawingMode::None||input.tool_focus>=0)return false;
    if(!input.keyboard&&!a.pointer.visible)return false;
    const bool erase=input.mode==MapDrawingMode::Erase;
    const int x=input.keyboard?input.cursor_x/256:a.pointer.x,y=input.keyboard?input.cursor_y/256:a.pointer.y;
    const int id=a.content.ui_image((erase?I8_MapCursorErase:I8_MapCursorDraw)+(input.pressed?1:0));
    if(id>=0)c.picture(id,x-(erase?6:1),y-(erase?15:14),16,16);
    else{c.line(x-2,y,x+2,y,erase?Red:White);c.line(x,y-2,x,y+2,erase?Red:White);}
    return true;
}
