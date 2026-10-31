    // Each piece keeps the common actor-space sampling grid. Rounding a
    // piece's target width and then rescaling it independently changes the
    // nearest sample phase at non-integer sizes.
    struct FramedDraw {int image,x,y,w,h,source_x,source_y,origin_x,origin_y,scale;};
    static int framed_floor(int64_t value,int divisor){
        return value<0?-int((-value+divisor-1)/divisor):int(value/divisor);
    }
    static int framed_edge(int coordinate,int scale){
        // Destination pixel centres select source pixels. This is the first
        // destination whose inverse sample is at least coordinate.
        return framed_floor(int64_t(coordinate)*scale+511,1024);
    }
    int framed_parameters(int id){
        const int auxiliary=framed_ui22_auxiliary(id);
        const int parameters=auxiliary<0?-1:a.content.ui_image(auxiliary);
        if(parameters>0)return parameters;
        const int row=framed_ui20_row(id);
        return row<0?-1:a.content.ui_image(FramedUi20BaseAuxiliary+unsigned(row));
    }
    bool framed_collect(int id,int x,int y,uint32_t milliseconds,int scale,bool loop,
                        FramedDraw*out,unsigned&count,bool cull=true,int exact_frame=-1){
        count=0;const int parameters=framed_parameters(id);
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
        unsigned frame=milliseconds/frame_ms;
        frame=exact_frame>=0?unsigned(mn(exact_frame,int(frames)-1)):
            loop?frame%frames:(frame<frames?frame:frames-1);
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
            const int left=x+framed_edge(layer.x,scale),top=y+framed_edge(layer.y,scale);
            const int width=x+framed_edge(layer.x+dimensions.w,scale)-left;
            const int height=y+framed_edge(layer.y+dimensions.h,scale)-top;
            if(width<=0||height<=0)continue;
            if(cull&&(left>=clip_w||top>=clip_h||left+width<=0||top+height<=0))continue;
            out[count++]={layer.image,left,top,width,height,layer.x,layer.y,x,y,scale};
        }
        return true;
    }
    void framed_picture(const FramedDraw&layer,unsigned opacity=255,unsigned blend_mode=0,
                        uint16_t(*transform)(uint16_t)=nullptr){
        if(!opacity||!this->opacity||layer.scale<=0||layer.w<=0||layer.h<=0)return;
        const int left=mx(0,layer.x),right=mn(clip_w,layer.x+layer.w);
        const int top=mx(0,layer.y),bottom=mn(clip_h,layer.y+layer.h);
        if(left>=right||top>=bottom)return;
        if(layer.scale==1024&&!transform){
            picture(layer.image,layer.x,layer.y,layer.w,layer.h,opacity,blend_mode);return;
        }
        const auto p=a.content.image(layer.image);if(!p.pixels||!p.w||!p.h)return;
        const int columns=right-left;if(columns>ScreenW)return;
        uint16_t source[ScreenW];
        const int64_t first=int64_t(left-layer.origin_x)*1024+512;
        const int initial=framed_floor(first,layer.scale);
        unsigned remainder=unsigned(first-int64_t(initial)*layer.scale);
        const unsigned step=1024u/unsigned(layer.scale),advance=1024u%unsigned(layer.scale);
        int column=initial-layer.source_x;
        for(int xx=0;xx<columns;++xx){
            if(column<0||column>=p.w)return;source[xx]=uint16_t(column);
            column+=int(step);remainder+=advance;
            if(remainder>=unsigned(layer.scale)){remainder-=unsigned(layer.scale);++column;}
        }
        const bool unmodulated=!coverage&&this->opacity==255&&opacity==255&&brightness==255&&
            tint_red==255&&tint_green==255&&tint_blue==255&&!color_matrix&&blend_mode==presentation::Normal;
        const auto draw=[&](int at,uint16_t color,unsigned alpha){
            if(opacity!=255)alpha=div255(alpha*opacity);if(!alpha)return;
            if(transform)color=transform(color);
            if(unmodulated){if(!occlusion||occlusion[at]!=255)pixels[at]=alpha==255?color:blend(pixels[at],color,alpha);}
            else paint(at,color,alpha,blend_mode);
        };
        if(p.format==4){
            const auto lower_column=[&](unsigned value){
                int lo=0,hi=columns;while(lo<hi){const int mid=lo+(hi-lo)/2;
                    if(source[mid]<value)lo=mid+1;else hi=mid;}return lo;
            };
            const int64_t first_row=int64_t(layer.source_y)*layer.scale+511;
            const int initial_row=framed_floor(first_row,1024);
            unsigned row_remainder=unsigned(first_row-int64_t(initial_row)*1024);
            const unsigned row_step=unsigned(layer.scale)/1024u,row_advance=unsigned(layer.scale)%1024u;
            int y0=layer.origin_y+initial_row;const uint8_t*run=p.pixels;
            for(unsigned sy=0;sy<p.h;++sy){
                poll_row(sy);int y1=y0+int(row_step);row_remainder+=row_advance;
                if(row_remainder>=1024u){row_remainder-=1024u;++y1;}
                const int row_top=mx(top,y0),row_bottom=mn(bottom,y1);
                unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
                for(unsigned span=0;span<spans;++span){
                    const unsigned sx=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,stride=run[4]?3u:2u;run+=5;
                    if(row_top<row_bottom&&source[0]<sx+n&&source[columns-1]>=sx){
                        const int begin=lower_column(sx),end=lower_column(sx+n);
                        for(int yy=row_top;yy<row_bottom;++yy){int at=yy*pitch+left+begin;
                            for(int xx=begin;xx<end;++xx,++at){const uint8_t*pixel=run+(unsigned(source[xx])-sx)*stride;
                                draw(at,uint16_t(pixel[0]|unsigned(pixel[1])<<8),stride==3?pixel[2]:255);}
                        }
                    }
                    run+=n*stride;
                }
                y0=y1;
            }
            return;
        }
        if(p.format>3)return;
        const int64_t first_row=int64_t(top-layer.origin_y)*1024+512;
        const int initial_row=framed_floor(first_row,layer.scale);
        unsigned row_remainder=unsigned(first_row-int64_t(initial_row)*layer.scale);
        int row=initial_row-layer.source_y;
        for(int yy=top;yy<bottom;++yy){
            poll_row(unsigned(yy-top));if(row<0||row>=p.h)return;
            int at=yy*pitch+left;
            for(int xx=0;xx<columns;++xx,++at){const unsigned index=unsigned(row)*p.w+source[xx];
                const uint8_t*pixel=p.palette?p.palette+unsigned(p.pixels[index])*2:p.pixels+index*2;
                draw(at,uint16_t(pixel[0]|unsigned(pixel[1])<<8),p.alpha?p.alpha[index]:255);}
            row+=int(step);row_remainder+=advance;
            if(row_remainder>=unsigned(layer.scale)){row_remainder-=unsigned(layer.scale);++row;}
        }
    }
    bool framed_clip(int id,int x,int y,uint32_t milliseconds,int scale,bool loop,
                     unsigned blend_mode=0,int exact_frame=-1){
        FramedDraw layers[FramedUi20MaxLayers];unsigned count=0;
        if(!framed_collect(id,x,y,milliseconds,scale,loop,layers,count,true,exact_frame))return false;
        for(unsigned i=0;i<count;++i)framed_picture(layers[i],255,blend_mode);
        return true;
    }
