    struct FramedDraw {int image,x,y,w,h;};
    bool framed_collect(int id,int x,int y,uint32_t milliseconds,int scale,bool loop,
                        FramedDraw*out,unsigned&count,bool cull=true){
        count=0;const int row=framed_ui20_row(id);if(row<0)return false;
        const int parameters=a.content.ui_image(FramedUi20BaseAuxiliary+unsigned(row));
        if(parameters<0)return false;
        const auto table=a.content.image(parameters);const unsigned available=unsigned(table.w)*table.h;
        if(!table.pixels||table.format||table.alpha||table.palette||available<FramedUi20HeaderWords)return false;
        const auto*data=table.pixels;
        const unsigned words=vantom_word(data,8),frames=vantom_word(data,2),frame_ms=vantom_word(data,3);
        const unsigned units=vantom_word(data,4)|(vantom_word(data,5)<<16);
        const unsigned reference=vantom_word(data,6)|(vantom_word(data,7)<<16);
        if(vantom_word(data,0)!=FramedUi20Magic||vantom_word(data,1)!=1||
           vantom_word(data,9)!=FramedUi20LayerWords||!frames||frames>256||!frame_ms||
           !units||!reference||units>=1000000||reference>=1000000||
           words>available||words<FramedUi20HeaderWords+frames*FramedUi20FrameWords)return false;
        unsigned frame=milliseconds/frame_ms;frame=loop?frame%frames:unsigned(mn(int(frame),int(frames)-1));
        const unsigned at=FramedUi20HeaderWords+frame*FramedUi20FrameWords;
        const unsigned start=vantom_word(data,at),layers=vantom_word(data,at+1);
        if(layers>FramedUi20MaxLayers||start>words||layers>(words-start)/FramedUi20LayerWords)return false;
        struct Layer {uint16_t image;int16_t x,y;};Layer copied[FramedUi20MaxLayers];
        for(unsigned i=0;i<layers;++i){const unsigned p=start+i*FramedUi20LayerWords;
            copied[i]={uint16_t(vantom_word(data,p)),int16_t(vantom_word(data,p+1)),int16_t(vantom_word(data,p+2))};}
        // Content may recycle a parameter page while loading a pixel. Copy
        // only this frame's tiny registration list before making more calls.
        scale=int(int64_t(scale)*units/reference);if(scale<=0)return true;
        for(unsigned i=0;i<layers;++i){const auto&layer=copied[i];const auto dimensions=a.content.image_dimensions(layer.image);
            if(!dimensions.w||!dimensions.h)return false;
            const int left=x+vantom_round(int64_t(layer.x)*scale,1024),top=y+vantom_round(int64_t(layer.y)*scale,1024);
            const int width=mx(1,vantom_round(int64_t(layer.x+dimensions.w)*scale,1024)-left+x);
            const int height=mx(1,vantom_round(int64_t(layer.y+dimensions.h)*scale,1024)-top+y);
            if(cull&&(left>=clip_w||top>=clip_h||left+width<=0||top+height<=0))continue;
            out[count++]={layer.image,left,top,width,height};
        }
        return true;
    }
    bool framed_clip(int id,int x,int y,uint32_t milliseconds,int scale,bool loop){
        if(framed_ui20_row(id)<0)return false;
        FramedDraw layers[FramedUi20MaxLayers];unsigned count=0;
        if(!framed_collect(id,x,y,milliseconds,scale,loop,layers,count))return false;
        for(unsigned i=0;i<count;++i){const auto&layer=layers[i];picture(layer.image,layer.x,layer.y,layer.w,layer.h);}
        return true;
    }
