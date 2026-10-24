// Small original UI textures use inverse affine sampling. All arithmetic in
// the pixel loop is integer; no temporary image or per-frame allocation is
// needed. Resources used here are deliberately ordinary/indexed RGBA, not
// sparse animation rows (which cannot be randomly addressed).
static void ui_picture_transform(Canvas&c,Picture p,int cx,int cy,int width,int height,int sine=0,int cosine=1024){
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
