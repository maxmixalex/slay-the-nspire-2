struct CardOpaqueMask {
    const uint8_t*owners;unsigned rank;
    bool covers(unsigned at)const{return ((owners[at>>1]>>((at&1)*4))&15u)>rank;}
};
// Continuous source materials borrow the existing image cache. The producer
// replaces the old56 sampled-phase pictures; this path allocates no frame or
// particle pool, and never asks Content for another picture while drawing.
static bool card_affliction_required(App&a,unsigned kind,bool*slots){
    card_affliction::Table table;
    return table.read(a.content.image(a.content.ui_image(card_affliction::Auxiliary)))&&table.required(kind,slots);
}
static bool card_affliction_cache_profile(const App&a){
    if(a.title||!combat_phase(a.game.phase))return false;
    // Retain the borrowed tier until this combat ends. Playing a third
    // material and drawing it again must not repeatedly evict the tile cache.
    if(a.content.selection_image_cache_profile())return true;
    unsigned kinds=0;const auto hand=a.combat_hand();
    for(unsigned i=0;i<hand.count;++i){const unsigned kind=a.game.enemy_world.affliction[hand.ids[i]];if(kind&&kind<=7)kinds|=1u<<kind;}
    // Two different materials can share most of the source fields. Three or
    // more borrow the already reviewed selection tier:12.75MiB total stays
    // fixed, leaving4.25MiB for compressed tile groups in this combat view.
    kinds&=kinds-1;return kinds&&(kinds&(kinds-1));
}
static void card_affliction_raster(Canvas&c,card_affliction::Shader*shader,const card_affliction::Field*clip,
    Picture picture,const int*inverse,const int*box,int cx,int cy,int width,int height,int sine,int cosine,unsigned mode,const CardOpaqueMask*hidden){
    using namespace card_affliction;
    if(width<=0||height<=0||c.clip_w<=0||c.clip_h<=0)return;
    const int x0=clamp(box[0],0,156),y0=clamp(box[1],0,218),x1=clamp(box[2],0,156),y1=clamp(box[3],0,218);
    if(x0>=x1||y0>=y1)return;
    int left=c.clip_w,right=0,top=c.clip_h,bottom=0;
    for(unsigned i=0;i<4;++i){const int x=(i&1)?x1:x0,y=(i&2)?y1:y0;
        const int64_t dx=int64_t(x*2-156)*width,dy=int64_t(y*2-218)*height;
        const int px=cx+int((dx*cosine/156-dy*sine/218)/2048),py=cy+int((dx*sine/156+dy*cosine/218)/2048);
        left=mn(left,px-2);right=mx(right,px+2);top=mn(top,py-2);bottom=mx(bottom,py+2);}
    left=mx(0,left);right=mn(c.clip_w,right);top=mx(0,top);bottom=mn(c.clip_h,bottom);
    if(left>=right||top>=bottom)return;
    const uint8_t*rows[218]{};
    if(!shader){
        if(!picture.pixels||picture.w!=156||picture.h!=218||picture.format>4)return;
        if(picture.format==4){const auto*p=picture.pixels;
            for(unsigned y=0;y<218;++y){rows[y]=p;unsigned spans=word(p);p+=2;
                while(spans--){const unsigned n=word(p+2),stride=p[4]+2;p+=5+n*stride;}}
        }
    }
    // Four guard bits keep affine stepping error below a fraction of a
    // source texel even for the smallest rotated particle quads.
    constexpr unsigned AQ=Q*16;
    const int du=int(int64_t(cosine)*AQ/(1024*width)),dv=int(-int64_t(sine)*AQ/(1024*height));
    const int64_t su=shader?(int64_t(inverse[0])*du+int64_t(inverse[1])*dv)/Q:0;
    const int64_t sv=shader?(int64_t(inverse[3])*du+int64_t(inverse[4])*dv)/Q:0;
    for(int y=top;y<bottom;++y){
        c.poll_row(unsigned(y-top));
        int u=int(AQ/2)+int(int64_t((2*(left-cx)+1)*cosine+(2*(y-cy)+1)*sine)*AQ/(2048*width));
        int v=int(AQ/2)+int(int64_t(-(2*(left-cx)+1)*sine+(2*(y-cy)+1)*cosine)*AQ/(2048*height));
        int64_t a=shader?(int64_t(inverse[0])*u+int64_t(inverse[1])*v)/Q+int64_t(inverse[2])*16:0;
        int64_t b=shader?(int64_t(inverse[3])*u+int64_t(inverse[4])*v)/Q+int64_t(inverse[5])*16:0;
        for(int x=left;x<right;++x,u+=du,v+=dv,a+=su,b+=sv){
            if(hidden&&hidden->covers(unsigned(y*320+x)))continue;
            if(unsigned(u)>=AQ||unsigned(v)>=AQ)continue;
            if(shader){
                if(a<0||b<0||a>=AQ||b>=AQ)continue;
                auto pixel=shader->sample(int(a>>4),int(b>>4));
                if(clip&&pixel.a)pixel.a=product(pixel.a,clip->sample(u>>4,v>>4,0));
                const unsigned alpha=(unsigned(clamp(pixel.a,0,Q))*255u+Q/2)>>20;if(!alpha)continue;
                const unsigned red=(unsigned(clamp(pixel.r,0,Q))*255u+Q/2)>>23,green=(unsigned(clamp(pixel.g,0,Q))*255u+Q/2)>>22,blue=(unsigned(clamp(pixel.b,0,Q))*255u+Q/2)>>23;
                c.paint(y*c.pitch+x,uint16_t((red<<11)|(green<<5)|blue),alpha,mode);
            }else{
                const unsigned px=unsigned(u>>4)*156u/Q,py=unsigned(v>>4)*218u/Q;
                const uint8_t*pixel=nullptr;unsigned alpha=255;
                if(picture.format==4){const auto*p=rows[py];unsigned spans=word(p);p+=2;
                    while(spans--){const unsigned sx=word(p),n=word(p+2),stride=p[4]+2;
                        if(px<sx)break;if(px<sx+n){pixel=p+5+(px-sx)*stride;alpha=stride==3?pixel[2]:255;break;}p+=5+n*stride;}
                }else{const unsigned at=py*156+px;alpha=picture.alpha?picture.alpha[at]:255;
                    pixel=picture.palette?picture.palette+2*picture.pixels[at]:picture.pixels+2*at;}
                if(pixel&&alpha)c.paint(y*c.pitch+x,uint16_t(word(pixel)),alpha,mode);
            }
        }
    }
}
static bool card_affliction_draw(Canvas&c,unsigned kind,int cx,int cy,int width,int height,int sine=0,int cosine=1024,const CardOpaqueMask*hidden=nullptr){
    using namespace card_affliction;
    if(kind<1||kind>7)return false;
    bool slots[Slots]{};if(!card_affliction_required(c.a,kind,slots))return false;
    if(c.clip_w<=0||c.clip_h<=0||width<=0||height<=0)return true;
    Picture pictures[Slots]{};
    // Complete all cache requests before binding borrowed views. Reload the
    // tiny table last and verify residency: even an unexpectedly oversized
    // external pack cannot leave dangling pointers in the shader loop.
    for(unsigned i=1;i<Slots;++i)if(slots[i])c.a.content.image(c.a.content.ui_image(Auxiliary+i));
    c.a.content.image(c.a.content.ui_image(Auxiliary));
    for(unsigned i=0;i<Slots;++i)if(slots[i]&&!c.a.content.image_cached(c.a.content.ui_image(Auxiliary+i)))return true;
    for(unsigned i=0;i<Slots;++i)if(slots[i])pictures[i]=c.a.content.image(c.a.content.ui_image(Auxiliary+i));
    Table table;if(!table.read(pictures[0]))return true;
    const uint8_t*pages[Slots]{};
    for(unsigned i=0;i<table.pages;++i)if(slots[i+1]){
        const auto&p=pictures[i+1];if(!p.pixels||p.w!=320||p.h!=205||p.format||p.alpha||p.palette)return true;pages[i]=p.pixels;}
    const Bytes bytes{pages,table.length};Field fields[24]{};bool bound[24]{};
    unsigned first,count;if(!table.family(kind,first,count))return true;
    for(unsigned n=first;n<first+count;++n){const auto*command=table.command(n);Shader shader;int parameters[13],inverse[6],box[4];
        if(command[0]==0){for(unsigned i=0;i<4;++i)box[i]=int16_t(word(command+112+i*2));
            card_affliction_raster(c,nullptr,nullptr,pictures[1+table.pages+word(command+6)],nullptr,box,cx,cy,width,height,sine,cosine,command[2],hidden);continue;}
        for(unsigned i=0;i<5;++i){const int id=int16_t(word(command+(i?6+i*2:4)));if(id<0)continue;
            if(!bound[id]){const auto*f=table.data+table.field_at+unsigned(id)*8;const unsigned at=dword(f);
                if(bytes.u32(at+28)!=dword(f+4)||!fields[id].bind(bytes,at))return true;bound[id]=true;}
            if(i)shader.field[i-1]=fields+id;}
        if(!shader.field[0])return true;
        const int clip_id=int16_t(word(command+4));const auto*clip=clip_id<0?nullptr:fields+clip_id;
        for(unsigned i=0;i<2;++i){const int id=int16_t(word(command+16+i*2));if(id<0)continue;
            const auto*q=table.data+table.curve_at+unsigned(id)*8;shader.curve[i]={nullptr,word(q+4),word(q+6),bytes,dword(q)};}
        shader.parameters=parameters;for(unsigned i=0;i<13;++i)parameters[i]=int32_t(dword(command+20+i*4));
        shader.kind=command[1];shader.repeat=command[3];
        if(command[0]==1){
            for(unsigned i=0;i<4;++i){shader.tint[i]=int32_t(dword(command+72+i*4));box[i]=int16_t(word(command+112+i*2));}
            for(unsigned i=0;i<6;++i)inverse[i]=int32_t(dword(command+88+i*4));shader.prepare(c.a.map_clock_ms);
            card_affliction_raster(c,&shader,clip,{},inverse,box,cx,cy,width,height,sine,cosine,command[2],hidden);
        }else{const unsigned at=dword(command+120),num=word(command+124);
            for(unsigned i=at;i<at+num;++i){const auto*t=table.track(i);const unsigned life=dword(t),delay=dword(t+4),fps=word(t+8),frames=word(t+10);
                const unsigned age=(c.a.map_clock_ms%life+life-delay)%life,frame=mn(int(frames)-1,int(uint64_t(age)*fps/1000));
                const unsigned start=dword(t+12)+frame*38;if(!bytes.byte(start+35))continue;
                for(unsigned j=0;j<4;++j){box[j]=int16_t(bytes.u16(start+j*2));shader.tint[j]=int(bytes.byte(start+32+j)*Q/255);}
                for(unsigned j=0;j<6;++j)inverse[j]=int32_t(bytes.u32(start+8+j*4));
                shader.custom=unsigned(uint64_t(bytes.u16(start+36))*Q/65535);shader.prepare(c.a.map_clock_ms);
                card_affliction_raster(c,&shader,clip,{},inverse,box,cx,cy,width,height,sine,cosine,command[2],hidden);}
        }
    }return true;
}
