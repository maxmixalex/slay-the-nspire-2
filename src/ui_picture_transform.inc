// Small original UI textures use inverse affine sampling. All arithmetic in
// the pixel loop is integer; no temporary image or per-frame allocation is
// needed. Resources used here are deliberately ordinary/indexed RGBA, not
// sparse animation rows (which cannot be randomly addressed).
static void ui_picture_transform_aligned(Canvas&c,Picture p,int cx,int cy,int width,int height,int sine,int cosine,unsigned opacity){
    if(width<=0||height<=0)return;
    if(!p.pixels||p.format>3)return;
    const int sn=CombatCardMotion::magnitude(sine),cs=CombatCardMotion::magnitude(cosine);
    const int half_w=(cs*width+sn*height)/2048+2,half_h=(sn*width+cs*height)/2048+2;
    const int left=mx(0,cx-half_w),right=mn(c.clip_w,cx+half_w),top=mx(0,cy-half_h),bottom=mn(c.clip_h,cy+half_h);
    int du=cosine*int(p.w)/width,dv=-sine*int(p.h)/height;
    const int ud=2*width,vd=2*height;int du_remainder=0,dv_remainder=0;
    du_remainder=2*cosine*int(p.w)%ud;dv_remainder=-2*sine*int(p.h)%vd;
    if(du_remainder<0){--du;du_remainder+=ud;}if(dv_remainder<0){--dv;dv_remainder+=vd;}
    for(int y=top;y<bottom;++y){
        const int rx=(left-cx)*cosine+(y-cy)*sine,ry=-(left-cx)*sine+(y-cy)*cosine;
        // History's odd9/13px boxes retain their unrotated nearest samples.
        // Existing callers retain the original integer-center convention.
        const int un=(width*1024+2*rx-(width&1)*cosine-(height&1)*sine)*int(p.w),vn=(height*1024+2*ry+(width&1)*sine-(height&1)*cosine)*int(p.h);
        int u=un/ud,ur=un%ud,v=vn/vd,vr=vn%vd;if(ur<0){--u;ur+=ud;}if(vr<0){--v;vr+=vd;}
        for(int x=left;x<right;++x){
            const int sample_u=u,sample_v=v;u+=du;v+=dv;
            // Exact rational stepping avoids an extra edge pixel when a
            // small odd source and destination have incommensurate widths.
            ur+=du_remainder;vr+=dv_remainder;if(ur>=ud){++u;ur-=ud;}if(vr>=vd){++v;vr-=vd;}
            if(sample_u<0||sample_v<0||sample_u>=int(p.w)*1024||sample_v>=int(p.h)*1024)continue;
            const unsigned at=unsigned(sample_v/1024)*p.w+unsigned(sample_u/1024);unsigned alpha=p.alpha?p.alpha[at]:255;
            if(opacity!=255)alpha=div255(alpha*opacity);
            if(!alpha)continue;const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
            c.paint(y*c.pitch+x,uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha);
        }
    }
}
static void ui_picture_transform(Canvas&c,Picture p,int cx,int cy,int width,int height,int sine=0,int cosine=1024,bool align_pixel_box=false,unsigned opacity=255){
    if(align_pixel_box){ui_picture_transform_aligned(c,p,cx,cy,width,height,sine,cosine,opacity);return;}
    // The legacy hot loop remains unchanged. Only History opts into the
    // exact odd-box sampling and per-texture opacity above.
    if(width<=0||height<=0)return;
    if(!p.pixels||p.format>3)return;
    const int sn=CombatCardMotion::magnitude(sine),cs=CombatCardMotion::magnitude(cosine);
    const int half_w=(cs*width+sn*height)/2048+2,half_h=(sn*width+cs*height)/2048+2;
    const int left=mx(0,cx-half_w),right=mn(c.clip_w,cx+half_w),top=mx(0,cy-half_h),bottom=mn(c.clip_h,cy+half_h);
    const int du=cosine*int(p.w)/width,dv=-sine*int(p.h)/height;
    for(int y=top;y<bottom;++y){
        int u=int(p.w)*512+((left-cx)*cosine+(y-cy)*sine)*int(p.w)/width;
        int v=int(p.h)*512+(-(left-cx)*sine+(y-cy)*cosine)*int(p.h)/height;
        for(int x=left;x<right;++x,u+=du,v+=dv){
            if(u<0||v<0||u>=int(p.w)*1024||v>=int(p.h)*1024)continue;
            const unsigned at=unsigned(v/1024)*p.w+unsigned(u/1024),alpha=p.alpha?p.alpha[at]:255;
            if(!alpha)continue;const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
            c.paint(y*c.pitch+x,uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha);
        }
    }
}
static void ui_picture_transform(Canvas&c,int image,int cx,int cy,int width,int height,int sine=0,int cosine=1024){ui_picture_transform(c,c.a.content.image(image),cx,cy,width,height,sine,cosine);}
static unsigned ui_integer_sqrt(unsigned n){
    unsigned root=0,bit=1u<<30;while(bit>n)bit>>=2;
    while(bit){if(n>=root+bit){n-=root+bit;root=(root>>1)+bit;}else root>>=1;bit>>=2;}return root;
}
static void ui_direction(int dx,int dy,int&sine,int&cosine){
    const unsigned length=ui_integer_sqrt(unsigned(dx*dx+dy*dy));
    if(!length){sine=0;cosine=1024;return;}
    // The authored arrow and segment point up. angle(direction)+PI/2 maps
    // that local axis to the actual tangent without an atan2/sin roundtrip.
    sine=dx*1024/int(length);cosine=-dy*1024/int(length);
}
