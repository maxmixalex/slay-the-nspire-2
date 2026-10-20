// Bounded RGB565 canvas; optional coverage is used only by cached card tiles.
static int mn(int a,int b){return a<b?a:b;}static int mx(int a,int b){return a>b?a:b;}
static uint16_t rgb(int r,int g,int b){return (uint16_t)((r>>3)<<11|(g>>2)<<5|(b>>3));}
static const uint16_t Ink=rgb(16,20,22),Panel=rgb(27,29,31),Gold=rgb(223,186,112),White=rgb(239,229,206),Muted=rgb(153,156,150),Red=rgb(168,57,45),Blue=rgb(88,157,188),Green=rgb(118,180,112);
static unsigned div255(unsigned v){v+=128;return (v+(v>>8))>>8;}
static uint16_t blend(uint16_t a,uint16_t b,unsigned alpha){
    unsigned r=div255((a>>11)*(255-alpha)+(b>>11)*alpha);
    unsigned ag=(a>>5)&63,bg=(b>>5)&63;
    unsigned g=div255(ag*(255-alpha)+bg*alpha);
    unsigned bl=div255((a&31)*(255-alpha)+(b&31)*alpha);
    return (uint16_t)((r<<11)|(g<<5)|bl);
}
struct TextBuffer{
    char*p;size_t cap,n;
    void ch(char v){if(n+1<cap){p[n++]=v;p[n]=0;}}
    void text(const char*s){while(*s)ch(*s++);}
    void number(int v){if(v<0){ch('-');v=-v;}char b[12];int n=0;do{b[n++]=(char)('0'+v%10);v/=10;}while(v&&n<12);while(n)ch(b[--n]);}
};
static uint32_t utf8(const char*&p){
    uint32_t x=(uint8_t)*p++;if(x<128)return x;int n=x<224?1:(x<240?2:3);x&=(1u<<(6-n))-1;
    while(n--){uint8_t b=(uint8_t)*p;if(!b)return '?';++p;if((b&192)!=128)return '?';x=(x<<6)|(b&63);}return x;
}
struct Canvas{
    App&a;
    uint16_t* pixels=a.pixels;
    int pitch=320, clip_w=320, clip_h=240;
    uint8_t* coverage=nullptr;
    void paint(int at,uint16_t color,unsigned alpha=255){
        if(!alpha)return;
        if(!coverage){pixels[at]=alpha==255?color:blend(pixels[at],color,alpha);return;}
        unsigned previous=coverage[at];
        if(alpha==255||!previous){pixels[at]=color;coverage[at]=(uint8_t)alpha;return;}
        unsigned back=div255(previous*(255-alpha)),total=alpha+back;
        unsigned r=((color>>11)*alpha+(pixels[at]>>11)*back+total/2)/total;
        unsigned g=(((color>>5)&63)*alpha+((pixels[at]>>5)&63)*back+total/2)/total;
        unsigned b=((color&31)*alpha+(pixels[at]&31)*back+total/2)/total;
        pixels[at]=(uint16_t)((r<<11)|(g<<5)|b);coverage[at]=(uint8_t)total;
    }
    void ui(int id,int x,int y,int w=0,int h=0,unsigned alpha=255){picture(a.content.ui_image(id),x,y,w,h,alpha);}
    void action(int x,int y,int w,int h,UiAction act,bool enabled=true){hit(x,y,w,h,HitKind::UiAction,(int)act,enabled);}
    void hit(int x,int y,int w,int h,HitKind k,int value=0,bool enabled=true){a.add_hit(x,y,w,h,k,value,enabled);}
    void close_button(int x=292,int y=26){box(x,y,21,20,Gold);centered("×",x,y,21,12,Gold);hit(x,y,21,20,HitKind::Close);}

    void rect(int x,int y,int w,int h,uint16_t color){for(int yy=mx(0,y);yy<mn(clip_h,y+h);yy++)for(int xx=mx(0,x);xx<mn(clip_w,x+w);xx++)paint(yy*pitch+xx,color);}
    void box(int x,int y,int w,int h,uint16_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
    void dim(int x,int y,int w,int h,unsigned alpha){for(int yy=mx(0,y);yy<mn(clip_h,y+h);yy++)for(int xx=mx(0,x);xx<mn(clip_w,x+w);xx++){paint(yy*pitch+xx,Ink,alpha);}}
    void line(int x,int y,int xx,int yy,uint16_t color,bool dots=false){
        int dx=xx>x?xx-x:x-xx,dy=yy>y?y-yy:yy-y,sx=x<xx?1:-1,sy=y<yy?1:-1,err=dx+dy,step=0;
        for(;;){if(!dots||(step/3)%2==0)rect(x,y,1,1,color);if(x==xx&&y==yy)break;int e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}step++;}
    }
    void picture(int id,int x,int y,int width=0,int height=0,unsigned opacity=255){
        auto p=a.content.image(id);if(!p.pixels||!opacity)return;if(!width)width=p.w;if(!height)height=p.h;
        if(width<=0||height<=0)return;int left=mx(0,-x),right=mn(width,clip_w-x),top=mx(0,-y),bottom=mn(height,clip_h-y);if(left>=right||top>=bottom)return;
        if(p.format==4){
            if(width!=p.w||height!=p.h){
                const uint8_t*run=p.pixels;
                for(unsigned sy=0;sy<p.h;sy++){
                    int y0=(int(sy)*height+p.h-1)/p.h,y1=(int(sy+1)*height+p.h-1)/p.h;
                    unsigned spans=run[0]|(unsigned)run[1]<<8;run+=2;
                    for(unsigned s=0;s<spans;s++){
                        unsigned sx=run[0]|(unsigned)run[1]<<8,n=run[2]|(unsigned)run[3]<<8,mode=run[4];run+=5;
                        unsigned stride=mode?3:2;
                        if(y1>top&&y0<bottom)for(unsigned j=0;j<n;j++){
                            int x0=(int(sx+j)*width+p.w-1)/p.w,x1=(int(sx+j+1)*width+p.w-1)/p.w;
                            if(x1<=left||x0>=right)continue;const uint8_t*src=run+j*stride;
                            unsigned alpha=mode?src[2]:255;if(opacity!=255)alpha=div255(alpha*opacity);
                            uint16_t color=src[0]|uint16_t(src[1])<<8;
                            for(int yy=mx(top,y0);yy<mn(bottom,y1);yy++)for(int xx=mx(left,x0);xx<mn(right,x1);xx++)paint((y+yy)*pitch+x+xx,color,alpha);
                        }
                        run+=n*stride;
                    }
                }
                return;
            }
            // Animation frames are prebaked at native size. Skip transparent runs entirely.
            const uint8_t* run=p.pixels;
            for(int yy=0;yy<p.h;yy++){
                unsigned spans=run[0]|(unsigned)run[1]<<8;run+=2;
                for(unsigned s=0;s<spans;s++){
                    unsigned sx=run[0]|(unsigned)run[1]<<8,n=run[2]|(unsigned)run[3]<<8,mode=run[4];run+=5;
                    unsigned stride=mode?3:2;
                    if(yy>=top&&yy<bottom){int l=mx(left,(int)sx),h=mn(right,(int)(sx+n));
                        if(l>=h){run+=n*stride;continue;}
                        int at=(y+yy)*pitch+x+l;const uint8_t*src=run+(l-(int)sx)*stride;
                        for(int xx=l;xx<h;xx++,at++,src+=stride){unsigned alpha=mode?src[2]:255;if(opacity!=255)alpha=div255(alpha*opacity);uint16_t v=src[0]|(uint16_t)src[1]<<8;paint(at,v,alpha);}}
                    run+=n*stride;
                }
            }
            return;
        }
        if(width==p.w&&height==p.h&&!p.alpha&&!p.palette&&opacity==255){
            for(int yy=top;yy<bottom;yy++){int at=(y+yy)*pitch+x+left;const uint8_t*src=p.pixels+(yy*p.w+left)*2;for(int xx=left;xx<right;xx++,at++,src+=2)paint(at,src[0]|(uint16_t)src[1]<<8);}
            return;
        }
        uint16_t source[320];for(int xx=left;xx<right;xx++)source[xx-left]=(uint16_t)(xx*p.w/width);
        for(int yy=top;yy<bottom;yy++){
            int row=(yy*p.h/height)*p.w;int at=(y+yy)*pitch+x+left;
            for(int xx=left;xx<right;xx++,at++){
                int i=row+source[xx-left];unsigned alpha=p.alpha?p.alpha[i]:255;if(opacity!=255)alpha=div255(alpha*opacity);if(!alpha)continue;
                const uint8_t*pixel=p.palette?p.palette+p.pixels[i]*2:p.pixels+i*2;uint16_t color=pixel[0]|(uint16_t)pixel[1]<<8;
                paint(at,color,alpha);
            }
        }
    }
    void clip(int id,int x,int y,uint32_t milliseconds,bool anchored=true,bool loop=true){
        auto c=a.content.clip(id);if(!c.count)return;unsigned index=milliseconds/c.frame_ms;
        index=loop?index%c.count:(unsigned)mn((int)index,c.count-1);
        picture((int)c.first+(int)index,x-(anchored?c.pivot_x:0),y-(anchored?c.pivot_y:0));
    }
    void actor(int actor,int x,int y,int scale=1024){
        if(actor<0||actor>MaxEnemies)return;
        int id=actor?AnimEnemyIdle:AnimHeroIdle;uint32_t elapsed=a.animation*33;
        const auto&m=a.motion[actor];if(m.clip>=0&&(int32_t)(a.animation-m.start)>=0){int actual=actor?m.clip:character_animation(a.game.character,m.clip);auto c=a.content.clip(actual);uint32_t t=(a.animation-m.start)*33;if(t<(uint32_t)c.count*c.frame_ms){id=m.clip;elapsed=t;}}
        if(actor==0)id=character_animation(a.game.character,id);
        elapsed+=((id==AnimEnemyIdle)?actor*347:0);
        if(scale==1024){clip(id,x,y,elapsed);return;}
        auto cl=a.content.clip(id);if(!cl.count)return;
        int image=int(cl.first)+(elapsed/cl.frame_ms)%cl.count;auto p=a.content.image(image);
        picture(image,x-cl.pivot_x*scale/1024,y-cl.pivot_y*scale/1024,mx(1,p.w*scale/1024),mx(1,p.h*scale/1024));
    }
    int width(const char*p,int size){int w=0;while(*p&&*p!='\n')w+=a.content.glyph(utf8(p),size).advance;return w;}
    void glyph(Glyph g,int x,int y,uint16_t color){
        if(g.pixels&&g.encoded_size){
            unsigned position=0;
            for(unsigned at=0;at<g.encoded_size;at+=2){
                unsigned count=g.pixels[at],alpha=g.pixels[at+1];
                if(alpha)for(unsigned j=0;j<count;j++){
                    int xx=x+(int)((position+j)%g.w),yy=y+(int)((position+j)/g.w);
                    if(xx>=0&&xx<clip_w&&yy>=0&&yy<clip_h){paint(yy*pitch+xx,color,alpha);}
                }
                position+=count;
            }return;
        }
        if(g.pixels)for(int yy=mx(0,-y);yy<mn(g.h,clip_h-y);yy++)for(int xx=mx(0,-x);xx<mn(g.w,clip_w-x);xx++){
            unsigned alpha=g.pixels[yy*g.w+xx];paint((y+yy)*pitch+x+xx,color,alpha);
        }
    }
    int text(const char*p,int x,int y,int size=12,uint16_t color=White,int maxwidth=320,int maxlines=1){
        int start=x,line=0;while(*p){uint32_t cp=utf8(p);auto g=a.content.glyph(cp,size);
            if(cp=='\n'||(x>start&&x+g.advance>start+maxwidth)){x=start;y+=size+3;if(++line>=maxlines)break;if(cp=='\n')continue;}
            glyph(g,x,y,color);x+=g.advance;
        }return x;
    }
    int paragraph(const char*p,int x,int y,int w,int lines,int skip=0,int size=12,uint16_t color=White){
        const uint16_t colors[]={color,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
        RichReader reader(p);RichChar ch{};int at=x,row=0;
        static const int wave[16]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
        while(reader.next(ch)){auto g=a.content.glyph(ch.codepoint,size);int advance=g.advance+(ch.style.bold?1:0);
            if(ch.codepoint=='\n'||(at>x&&at+advance>x+w)){at=x;row++;if(ch.codepoint=='\n')continue;}
            if(row>=skip&&row<skip+lines){int dy=ch.style.wave?wave[(a.animation/2+ch.index*2)%16]:0;
                int dx=ch.style.jitter?(int)((a.animation/2+ch.index*7)%3)-1:0;
                static const uint8_t rainbow[]={2,6,1,4,7,3,5};
                auto ink=colors[ch.style.rainbow?rainbow[(a.animation/5+ch.index/2)%7]:ch.style.color];int yy=y+(row-skip)*(size+3)+dy;
                glyph(g,at+dx,yy,ink);if(ch.style.bold)glyph(g,at+dx+1,yy,ink);}
            at+=advance;
        }return row+1;
    }
    void centered(const char*p,int x,int y,int w,int size=12,uint16_t color=White){text(p,x+mx(0,(w-width(p,size))/2),y,size,color,w);}
    void num(int v,int x,int y,int size=12,uint16_t color=White){char b[16]={};TextBuffer t{b,sizeof b,0};t.number(v);text(b,x,y,size,color);}
    void energy_label(int value,int x,int y,int size=12){
        if(value==-2)return; // Unplayable models have no energy label.
        char text[16]={};TextBuffer t{text,sizeof text,0};if(value==-1)t.text("X");else t.number(value);
        for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++)if(i||j)this->text(text,x+i,y+j,size,Ink);
        this->text(text,x,y,size,White);
    }
    void star_label(int value,int x,int y,int size=12){
        ui(U_Star,x,y+1,size+1,size+1);
        char label[16]={};TextBuffer t{label,sizeof label,0};if(value<0)t.ch('X');else t.number(value);
        for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++)if(i||j)text(label,x+size+2+i,y+j,size,Ink);
        text(label,x+size+2,y,size,Gold);
    }
    void hp(int value,int total,int x,int y,int w){
        if(w<1)return;total=mx(1,total);value=mn(mx(0,value),total);int filled=0;
        // Compare integer pixel thresholds without multiplying billion-HP
        // bosses by a screen width in signed 32-bit arithmetic.
        while(filled<w&&total/w*(filled+1)+(total%w*(filled+1)+w-1)/w<=value)++filled;
        rect(x,y,w,5,Ink);rect(x,y,filled,5,Red);char b[32]={};TextBuffer t{b,sizeof b,0};
        auto amount=[&](int v){if(v>=1000000){t.number(v/1000000);t.ch('M');}else if(v>=10000){t.number(v/1000);t.ch('k');}else t.number(v);};
        amount(value);t.ch('/');amount(total);int size=width(b,10)>w?8:10;centered(b,x,y+3,w,size);
    }
    void button(const char*p,int x,int y,int w,bool selected,int h=25){
        picture(A_WideButton,x,y,w,h,selected?255:195);
        if(selected){rect(x+8,y+h-2,w-16,1,Gold);text("›",x+5,y+3,12,Gold);}centered(p,x+10,y+(h-18)/2,w-20,12,selected?Gold:White);
    }
};
