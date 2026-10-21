// Original map material operations on bounded RGB565 pictures. Static icons
// can rotate without an intermediate framebuffer; sparse Boss clips preserve
// the baker's RGB mask, alpha, fixed pivot and full original animation cycle.
enum class MapShader { Original,Tint,Normal,Boss };
static uint16_t map_shade(uint16_t source,MapShader shader,uint16_t ink,uint16_t paper){
    if(shader==MapShader::Original)return source;
    const unsigned r=source>>11,g=(source>>5)&63,b=source&31;
    if(shader==MapShader::Tint)return uint16_t(((r*(ink>>11)/31)<<11)|((g*((ink>>5)&63)/63)<<5)|(b*(ink&31)/31));
    if(shader==MapShader::Normal){
        int dr=int(r*255/31)-180,dg=int(g*255/63)-157,db=int(b*255/31)-137;
        return dr*dr+dg*dg+db*db<16256?blend(paper,rgb(128,128,128),128):source;
    }
    const unsigned rr=mn(31,int(r*(paper>>11)/31+b*(ink>>11)/31+g*31/63));
    const unsigned gg=mn(63,int(r*((paper>>5)&63)/31+b*((ink>>5)&63)/31+g));
    const unsigned bb=mn(31,int(r*(paper&31)/31+b*(ink&31)/31+g*31/63));
    return uint16_t((rr<<11)|(gg<<5)|bb);
}
static int map_sin(int degrees){
    static const int16_t quarter[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
    int phase=((degrees%360+360)%360)*256/360,sign=phase>=128?-1:1;phase%=128;if(phase>64)phase=128-phase;
    const int i=phase/4,j=phase%4;return sign*(i==16?quarter[16]:(quarter[i]*(4-j)+quarter[i+1]*j)/4);
}
static unsigned map_sqrt(unsigned value){
    unsigned bit=1u<<30,result=0;while(bit>value)bit>>=2;
    while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;
}
static void map_picture(Canvas&c,int id,int cx,int cy,int width,int height,MapShader shader=MapShader::Original,
                        uint16_t ink=0,uint16_t paper=0,unsigned opacity=255,int angle=0,int clip_top=WorldMapTop,int clip_bottom=WorldMapBottom){
    if(width<=0||height<=0||!opacity)return;const auto p=c.a.content.image(id);if(!p.pixels)return;
    if(p.format==4){
        // Only the authored, unrotated Boss animations are sparse. Keep their
        // per-pixel mask intact instead of tinting the whole finished sprite.
        const uint8_t*run=p.pixels;const int x=cx-width/2,y=cy-height/2;
        for(unsigned sy=0;sy<p.h;++sy){
            const int y0=mx(clip_top,y+(int(sy)*height+p.h-1)/p.h),y1=mn(clip_bottom,y+(int(sy+1)*height+p.h-1)/p.h);
            unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
            for(unsigned span=0;span<spans;++span){
                unsigned sx=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,mode=run[4];run+=5;unsigned stride=mode?3:2;
                if(y0<y1)for(unsigned j=0;j<n;++j){
                    const int x0=mx(0,x+(int(sx+j)*width+p.w-1)/p.w),x1=mn(320,x+(int(sx+j+1)*width+p.w-1)/p.w);if(x0>=x1)continue;
                    const uint8_t*src=run+j*stride;unsigned alpha=mode?src[2]:255;alpha=div255(alpha*opacity);
                    const uint16_t color=map_shade(uint16_t(src[0]|uint16_t(src[1])<<8),shader,ink,paper);
                    for(int yy=y0;yy<y1;++yy)for(int xx=x0;xx<x1;++xx)c.paint(yy*c.pitch+xx,color,alpha);
                }
                run+=n*stride;
            }
        }
        return;
    }
    const int sine=map_sin(angle),cosine=map_sin(angle+90),as=sine<0?-sine:sine,ac=cosine<0?-cosine:cosine;
    const int half_w=(width*ac+height*as+2047)/2048,half_h=(height*ac+width*as+2047)/2048;
    const int left=mx(0,cx-half_w),right=mn(320,cx+half_w),top=mx(clip_top,cy-half_h),bottom=mn(clip_bottom,cy+half_h);
    for(int yy=top;yy<bottom;++yy)for(int xx=left;xx<right;++xx){
        const int dx=2*(xx-cx)+1,dy=2*(yy-cy)+1;
        const int u=dx*cosine+dy*sine+width*1024,v=dy*cosine-dx*sine+height*1024;
        if(u<0||v<0||u>=width*2048||v>=height*2048)continue;
        const unsigned at=unsigned(v*p.h/(height*2048))*p.w+unsigned(u*p.w/(width*2048));
        unsigned alpha=p.alpha?p.alpha[at]:255;alpha=div255(alpha*opacity);if(!alpha)continue;
        const uint8_t*src=p.palette?p.palette+p.pixels[at]*2:p.pixels+at*2;
        const uint16_t color=map_shade(uint16_t(src[0]|uint16_t(src[1])<<8),shader,ink,paper);c.paint(yy*c.pitch+xx,color,alpha);
    }
}
static int map_reticle_project(int coordinate,unsigned scale,bool smaller){
    // Authored source coordinates at the map's 0.22 scale. A10's Boss root
    // adds a 0.75 parent scale; the reticle does not inherit icon hover scale.
    const int denominator=409600*(smaller?4:1),value=coordinate*int(scale)*22*(smaller?3:1);
    return value<0?-((-value+denominator/2)/denominator):(value+denominator/2)/denominator;
}
static void map_reticle_tile(Canvas&c,const Picture&p,int left,int top,int right,int bottom,
                             bool flip_x,bool flip_y,unsigned opacity,bool shadow){
    if(right<=left||bottom<=top)return;
    for(int y=mx(WorldMapTop,top);y<mn(WorldMapBottom,bottom);++y){
        unsigned sy=unsigned((2*(y-top)+1)*p.h/(2*(bottom-top)));if(flip_y)sy=p.h-1-sy;
        for(int x=mx(0,left);x<mn(320,right);++x){
            unsigned sx=unsigned((2*(x-left)+1)*p.w/(2*(right-left)));if(flip_x)sx=p.w-1-sx;
            const unsigned at=sy*p.w+sx,source_alpha=p.alpha?p.alpha[at]:255;
            // Map instances override the shared reticle's shadow to 0.35.
            // Keep the texture alpha hole and round the full product once.
            const unsigned alpha=shadow?(source_alpha*opacity*35u+12750u)/25500u:div255(source_alpha*opacity);
            if(!alpha)continue;
            const uint8_t*source=p.palette?p.palette+p.pixels[at]*2:p.pixels+at*2;
            const uint16_t color=shadow?0:uint16_t(source[0]|uint16_t(source[1])<<8);
            c.paint(y*c.pitch+x,color,alpha);
        }
    }
}
static void map_reticle(Canvas&c,unsigned point,int x,int y,int width,int height,bool smaller=false){
    const unsigned opacity=c.a.map_reticle_opacity(point);if(!opacity)return;
    const auto p=c.a.content.image(c.a.content.world_image("map","SELECTION_RETICLE"));
    // This profile entry is a static original texture: the texture encoder
    // produces dense or palette pixels, never the sparse animation format.
    if(!p.pixels||!p.w||!p.h||p.format==4)return;
    const unsigned scale=c.a.map_reticle_scale(point);
    for(unsigned corner=0;corner<4;++corner){
        const bool flip_x=corner&2,flip_y=corner&1;
        const int left=flip_x?width/2-20:-width/2-10,top=flip_y?height/2-20:-height/2-10;
        // Each shadow is behind its own corner. The authored negative child
        // offsets cancel the corner mirror, leaving a (+3,+2) world offset.
        for(unsigned pass=0;pass<2;++pass){const bool shadow=pass==0;const int dx=shadow?3:0,dy=shadow?2:0;
            map_reticle_tile(c,p,x+map_reticle_project(left+dx,scale,smaller),y+map_reticle_project(top+dy,scale,smaller),
                x+map_reticle_project(left+30+dx,scale,smaller),y+map_reticle_project(top+30+dy,scale,smaller),
                flip_x,flip_y,opacity,shadow);
        }
    }
}
