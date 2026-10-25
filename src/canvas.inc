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
    const Content*locale=nullptr;
    void ch(char v){if(n+1<cap){p[n++]=v;p[n]=0;}}
    void text(const char*s){if(locale)s=locale->ui_text(s);while(*s)ch(*s++);}
    void number(int v){uint32_t u=v<0?uint32_t(-(v+1))+1:uint32_t(v);if(v<0)ch('-');char b[12];int n=0;do{b[n++]=(char)('0'+u%10);u/=10;}while(u&&n<12);while(n)ch(b[--n]);}
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
    // Local UI layers can modulate images and text together. Appending this
    // field preserves cached-card aggregate initializers and opaque drawing.
    uint8_t opacity=255;
    uint8_t brightness=255;
    uint8_t tint_red=255,tint_green=255,tint_blue=255;
    // Godot visible_ratio reveals glyphs after laying out the complete text.
    // Keep fitting, word wrapping and centering stable during room dialogue.
    unsigned text_visible_characters=~0u;
    uint8_t font_face=0;
    uint8_t bold_face=0; // A scene can explicitly override its bold family.
    bool wrap_latin_words=false; // Original article fallback keeps Latin word boundaries in every locale.
    bool sparse_scene_sizes=false; // Opt in only when this label's full alphabet is baked.
    unsigned font_style(const RichChar&ch)const{const unsigned style=rich_font_style(ch);return (style==1&&bold_face?unsigned(bold_face):style)|font_face;}
    void paint(int at,uint16_t color,unsigned alpha=255,unsigned mode=0){
        if(opacity!=255)alpha=div255(alpha*opacity);
        if(!alpha)return;
        if(brightness!=255)color=presentation::modulate_rgb(color,brightness);
        if(tint_red!=255||tint_green!=255||tint_blue!=255)
            color=presentation::tint_rgb(color,tint_red,tint_green,tint_blue);
        if(mode==presentation::Add){
            if(!coverage||coverage[at]==255)pixels[at]=presentation::add_opaque(pixels[at],color,alpha);
            else{
                const unsigned previous=coverage[at];
                pixels[at]=presentation::add_covered(pixels[at],color,alpha,previous);
                coverage[at]=(uint8_t)(alpha+div255(previous*(255-alpha)));
            }
            return;
        }
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
    bool nine_image(int id,int x,int y,int w,int h,int l,int t,int r,int b,unsigned alpha=255){
        if(w<=0||h<=0||!alpha)return false;
        const auto p=a.content.image(id);if(!p.pixels||p.format>3)return false;
        const int left=mx(0,-x),right=mn(w,clip_w-x),top=mx(0,-y),bottom=mn(h,clip_h-y);
        if(left>=right||top>=bottom)return true;
        uint16_t columns[320];
        for(int xx=left;xx<right;++xx)columns[xx-left]=uint16_t(nine_patch::coordinate(xx,w,p.w,l,r));
        for(int yy=top;yy<bottom;++yy){
            const int row=nine_patch::coordinate(yy,h,p.h,t,b)*p.w;
            for(int xx=left;xx<right;++xx){
                const int at=row+columns[xx-left];unsigned coverage=p.alpha?p.alpha[at]:255;
                if(alpha!=255)coverage=div255(coverage*alpha);if(!coverage)continue;
                const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
                paint((y+yy)*pitch+x+xx,uint16_t(pixel[0]|unsigned(pixel[1])<<8),coverage);
            }
        }return true;
    }
    bool nine_ui(int id,int x,int y,int w,int h,int l,int t,int r,int b,unsigned alpha=255){
        return nine_image(a.content.ui_image(id),x,y,w,h,l,t,r,b,alpha);
    }
    ButtonVisual control_visual(const char*label,int x,int y,int w,int h,ButtonFamily family,bool selected,bool enabled,int shift_x=0,bool neutral_initial=false){
        const auto&pointer=a.pointer;
        const bool inside=pointer.visible&&pointer.x>=x+shift_x&&pointer.x<x+shift_x+w&&pointer.y>=y&&pointer.y<y+h;
        const auto&captured=pointer.captured;
        const bool pressed=(family==ButtonFamily::EndTurn&&a.end_turn_hold.pressed)||(inside&&pointer.held&&captured.enabled&&captured.kind!=HitKind::None&&
            captured.x<=x+shift_x+w/2&&captured.x+captured.w>x+shift_x+w/2&&captured.y<=y+h/2&&captured.y+captured.h>y+h/2&&
            pointer.context==a.input_context());
        const ButtonState mode=!enabled?ButtonState::Disabled:pressed?ButtonState::Press:inside||selected?ButtonState::Focus:ButtonState::Rest;
        uint32_t key=2166136261u;for(const char*p=label;*p;++p)key=(key^uint8_t(*p))*16777619u;
        const int values[]={x,y,w,h,int(family),int(a.input_context())};for(int v:values)key=(key^uint32_t(v))*16777619u;if(!key)key=1;
        ButtonMotion*motion=nullptr;ButtonMotion*oldest=&a.button_motions[0];
        for(auto&candidate:a.button_motions){if(candidate.identity==key){motion=&candidate;break;}if(!candidate.identity)oldest=&candidate;else if(oldest->identity&&uint32_t(a.map_clock_ms-candidate.seen)>uint32_t(a.map_clock_ms-oldest->seen))oldest=&candidate;}
        if(!motion)motion=oldest;return motion->observe(key,family,mode,a.map_clock_ms,neutral_initial);
    }
    void action(int x,int y,int w,int h,UiAction act,bool enabled=true){hit(x,y,w,h,HitKind::UiAction,(int)act,enabled);}
    void hit(int x,int y,int w,int h,HitKind k,int value=0,bool enabled=true){a.add_hit(x,y,w,h,k,value,enabled);}
    void close_button(int x=292,int y=26){
        const auto visual=control_visual("CLOSE",x,y,21,20,ButtonFamily::Action,false,true);
        const int cross=a.content.version()>=Ui6ContentVersion?a.content.ui_image(I6_CloseX):-1;
        if(cross>=0){Canvas layer=*this;layer.brightness=uint8_t(div255(unsigned(brightness)*visual.shade));
            const int size=14*visual.scale/4096;layer.tint_green=246;layer.tint_blue=226;
            layer.picture(cross,x+(21-size)/2,y+(20-size)/2,size,size);
        }else centered("×",x,y,21,16,Gold);
        hit(x,y,21,20,HitKind::Close);
    }

    void rect(int x,int y,int w,int h,uint16_t color){
        const int left=mx(0,x),right=mn(clip_w,x+w),top=mx(0,y),bottom=mn(clip_h,y+h);
        // Main-surface clears and opaque plaques need no per-pixel blending
        // state. Cached coverage and every layer modulation keep paint().
        if(!coverage&&opacity==255&&brightness==255&&tint_red==255&&tint_green==255&&tint_blue==255){
            for(int yy=top;yy<bottom;++yy)for(int xx=left;xx<right;++xx)pixels[yy*pitch+xx]=color;
            return;
        }
        for(int yy=top;yy<bottom;++yy)for(int xx=left;xx<right;++xx)paint(yy*pitch+xx,color);
    }
    void box(int x,int y,int w,int h,uint16_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
    void dim(int x,int y,int w,int h,unsigned alpha){for(int yy=mx(0,y);yy<mn(clip_h,y+h);yy++)for(int xx=mx(0,x);xx<mn(clip_w,x+w);xx++){paint(yy*pitch+xx,Ink,alpha);}}
    void line(int x,int y,int xx,int yy,uint16_t color,bool dots=false){
        int dx=xx>x?xx-x:x-xx,dy=yy>y?y-yy:yy-y,sx=x<xx?1:-1,sy=y<yy?1:-1,err=dx+dy,step=0;
        for(;;){if(!dots||(step/3)%2==0)rect(x,y,1,1,color);if(x==xx&&y==yy)break;int e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}step++;}
    }
    void picture(int id,int x,int y,int width=0,int height=0,unsigned opacity=255,unsigned blend_mode=0){
        picture(a.content.image(id),x,y,width,height,opacity,blend_mode);
    }
    // Conservative occlusion for a later normal-blended image. Every visible
    // destination sample must have exactly full alpha after nearest sampling;
    // one hole or antialiased edge keeps the original lower layer. Sparse
    // covers deliberately take that fallback rather than guessing a bound.
    bool opaque_picture_covers(Picture cover,int x,int y,int width,int height,
        int target_x,int target_y,int target_w,int target_h)const{
        if(!cover.pixels||cover.format>3||!cover.w||!cover.h||opacity!=255)return false;
        if(!width)width=cover.w;if(!height)height=cover.h;if(width<=0||height<=0)return false;
        const int left=mx(0,target_x),right=mn(clip_w,target_x+target_w);
        const int top=mx(0,target_y),bottom=mn(clip_h,target_y+target_h);
        if(left>=right||top>=bottom)return true;
        if(left<x||right>x+width||top<y||bottom>y+height)return false;
        if(!cover.alpha)return true;
        const int columns=right-left;if(columns>320)return false;
        uint16_t source[320];
        const unsigned divisor=unsigned(width),step=cover.w/divisor,advance=cover.w%divisor;
        const unsigned first=unsigned(left-x)*cover.w;
        unsigned column=first/divisor,remainder=first%divisor;
        for(int xx=0;xx<columns;++xx){
            source[xx]=uint16_t(column);column+=step;remainder+=advance;
            if(remainder>=divisor){remainder-=divisor;++column;}
        }
        const unsigned rows=unsigned(height),row_step=cover.h/rows,row_advance=cover.h%rows;
        const unsigned initial=unsigned(top-y)*cover.h;
        unsigned row=initial/rows,row_remainder=initial%rows,previous=~0u;
        for(int yy=top;yy<bottom;++yy){
            if(row!=previous){const auto*alpha=cover.alpha+row*cover.w;
                for(int xx=0;xx<columns;++xx)if(alpha[source[xx]]!=255)return false;
                previous=row;
            }
            row+=row_step;row_remainder+=row_advance;
            if(row_remainder>=rows){row_remainder-=rows;++row;}
        }
        return true;
    }
    // A full-width atlas band shares its owner's decoded storage, including
    // sparse rows. It can use the existing scaled/additive picture path.
    static Picture picture_rows(Picture p,int first,int count){
        if(!p.pixels||first<0||count<=0||first>=p.h)return {};
        count=mn(count,int(p.h)-first);
        if(p.format==4){
            const uint8_t*run=p.pixels;
            for(int y=0;y<first;++y){
                unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
                for(unsigned span=0;span<spans;++span){
                    const unsigned n=run[2]|unsigned(run[3])<<8,stride=run[4]?3u:2u;
                    run+=5+n*stride;
                }
            }
            p.pixels=run;
        }else{
            const unsigned offset=unsigned(first)*p.w;
            p.pixels+=offset*(p.palette?1u:2u);if(p.alpha)p.alpha+=offset;
        }
        p.h=uint16_t(count);return p;
    }
    void picture(Picture p,int x,int y,int width=0,int height=0,unsigned opacity=255,unsigned blend_mode=0){
        if(!p.pixels||!opacity)return;if(!width)width=p.w;if(!height)height=p.h;
        if(width<=0||height<=0)return;int left=mx(0,-x),right=mn(width,clip_w-x),top=mx(0,-y),bottom=mn(height,clip_h-y);if(left>=right||top>=bottom)return;
        if(p.format==4){
            if(width!=p.w||height!=p.h){
                // Inverse nearest sampling visits only visible destination
                // columns. Full-width atlas bands can hide most source
                // pixels; do not divide twice for every discarded pixel.
                uint16_t source[320];const int columns=right-left;
                const unsigned divisor=unsigned(width),step=p.w/divisor,advance=p.w%divisor;
                const unsigned first=unsigned(left)*p.w;
                unsigned column=first/divisor,remainder=first%divisor;
                for(int xx=0;xx<columns;++xx){
                    source[xx]=uint16_t(column);column+=step;remainder+=advance;
                    if(remainder>=divisor){remainder-=divisor;++column;}
                }
                const auto lower_column=[&](unsigned value){
                    int lo=0,hi=columns;
                    while(lo<hi){const int mid=lo+(hi-lo)/2;if(source[mid]<value)lo=mid+1;else hi=mid;}
                    return lo;
                };
                // Start at ceil(0 * height / p.h). Adding the quotient and
                // carrying the remainder preserves every original ceil edge.
                const unsigned row_step=unsigned(height)/p.h,row_advance=unsigned(height)%p.h;
                unsigned row_remainder=p.h-1;int y0=0;
                const uint8_t*run=p.pixels;
                for(unsigned sy=0;sy<p.h;sy++){
                    int y1=y0+int(row_step);row_remainder+=row_advance;
                    if(row_remainder>=p.h){row_remainder-=p.h;++y1;}
                    const int row_top=mx(top,y0),row_bottom=mn(bottom,y1);
                    unsigned spans=run[0]|(unsigned)run[1]<<8;run+=2;
                    for(unsigned s=0;s<spans;s++){
                        unsigned sx=run[0]|(unsigned)run[1]<<8,n=run[2]|(unsigned)run[3]<<8,mode=run[4];run+=5;
                        unsigned stride=mode?3:2;
                        if(row_top<row_bottom&&source[0]<sx+n&&source[columns-1]>=sx){
                            const int begin=lower_column(sx),end=lower_column(sx+n);
                            for(int yy=row_top;yy<row_bottom;++yy){
                                int at=(y+yy)*pitch+x+left+begin;
                                for(int xx=begin;xx<end;++xx,++at){
                                    const uint8_t*src=run+(unsigned(source[xx])-sx)*stride;
                                    unsigned alpha=mode?src[2]:255;if(opacity!=255)alpha=div255(alpha*opacity);
                                    if(alpha)paint(at,uint16_t(src[0]|uint16_t(src[1])<<8),alpha,blend_mode);
                                }
                            }
                        }
                        run+=n*stride;
                    }
                    y0=y1;
                }
                return;
            }
            // Animation frames are prebaked at native size. Skip transparent runs entirely.
            const bool direct=!coverage&&this->opacity==255&&opacity==255&&brightness==255&&
                tint_red==255&&tint_green==255&&tint_blue==255&&blend_mode==presentation::Normal;
            const uint8_t* run=p.pixels;
            for(int yy=0;yy<p.h;yy++){
                unsigned spans=run[0]|(unsigned)run[1]<<8;run+=2;
                for(unsigned s=0;s<spans;s++){
                    unsigned sx=run[0]|(unsigned)run[1]<<8,n=run[2]|(unsigned)run[3]<<8,mode=run[4];run+=5;
                    unsigned stride=mode?3:2;
                    if(yy>=top&&yy<bottom){int l=mx(left,(int)sx),h=mn(right,(int)(sx+n));
                        if(l>=h){run+=n*stride;continue;}
                        int at=(y+yy)*pitch+x+l;const uint8_t*src=run+(l-(int)sx)*stride;
                        // Opaque native-size spans need no per-pixel paint
                        // modulation. Keep explicit loads: sparse row headers
                        // can place RGB565 bytes at an odd address.
                        if(direct&&!mode){for(int xx=l;xx<h;++xx,++at,src+=2)pixels[at]=uint16_t(src[0]|(uint16_t(src[1])<<8));}
                        else for(int xx=l;xx<h;xx++,at++,src+=stride){unsigned alpha=mode?src[2]:255;if(opacity!=255)alpha=div255(alpha*opacity);uint16_t v=src[0]|(uint16_t)src[1]<<8;paint(at,v,alpha,blend_mode);}}
                    run+=n*stride;
                }
            }
            return;
        }
        if(width==p.w&&height==p.h&&!p.alpha&&!p.palette&&opacity==255){
            if(!coverage&&this->opacity==255&&brightness==255&&tint_red==255&&tint_green==255&&tint_blue==255&&blend_mode==presentation::Normal){
                // Decoded RGB565 bytes may be unaligned. Preserve the explicit
                // little-endian load instead of casting the resource pointer.
                for(int yy=top;yy<bottom;++yy){int at=(y+yy)*pitch+x+left;const uint8_t*src=p.pixels+(yy*p.w+left)*2;
                    for(int xx=left;xx<right;++xx,++at,src+=2)pixels[at]=uint16_t(src[0]|(uint16_t(src[1])<<8));}
                return;
            }
            for(int yy=top;yy<bottom;yy++){int at=(y+yy)*pitch+x+left;const uint8_t*src=p.pixels+(yy*p.w+left)*2;for(int xx=left;xx<right;xx++,at++,src+=2)paint(at,src[0]|(uint16_t)src[1]<<8,255,blend_mode);}
            return;
        }
        // Exact nearest-neighbour sampling needs a division only at the
        // clipped start and for the step, rather than for each destination
        // column/row. This matters on the calculator's integer-only ARMv5.
        uint16_t source[320];
        const unsigned divisor=unsigned(width),step=p.w/divisor,advance=p.w%divisor;
        const unsigned first=unsigned(left)*p.w;
        unsigned column=first/divisor,remainder=first%divisor;
        for(int xx=left;xx<right;++xx){
            source[xx-left]=uint16_t(column);column+=step;remainder+=advance;
            if(remainder>=divisor){remainder-=divisor;++column;}
        }
        const unsigned row_divisor=unsigned(height),row_step=p.h/row_divisor,row_advance=p.h%row_divisor;
        const unsigned first_row=unsigned(top)*p.h;
        unsigned row_index=first_row/row_divisor,row_remainder=first_row%row_divisor;
        for(int yy=top;yy<bottom;yy++){
            int row=int(row_index*p.w);int at=(y+yy)*pitch+x+left;
            for(int xx=left;xx<right;xx++,at++){
                int i=row+source[xx-left];unsigned alpha=p.alpha?p.alpha[i]:255;if(opacity!=255)alpha=div255(alpha*opacity);if(!alpha)continue;
                const uint8_t*pixel=p.palette?p.palette+p.pixels[i]*2:p.pixels+i*2;uint16_t color=pixel[0]|(uint16_t)pixel[1]<<8;
                paint(at,color,alpha,blend_mode);
            }
            row_index+=row_step;row_remainder+=row_advance;
            if(row_remainder>=row_divisor){row_remainder-=row_divisor;++row_index;}
        }
    }
    // Atlas regions stay at their prebaked size. A mirrored region shares its
    // original image/cache entry; no second resource or scratch bitmap is needed.
    void picture_region(Picture p,int x,int y,int source_x,int source_y,int width,int height,bool mirror=false,unsigned opacity=255,unsigned mode=0){
        if(!p.pixels||!opacity||width<=0||height<=0)return;
        const int left=mx(0,-x),right=mn(width,clip_w-x),top=mx(0,-y),bottom=mn(height,clip_h-y);
        if(left>=right||top>=bottom)return;
        const int sx0=mx(0,source_x+(mirror?width-right:left)),sx1=mn(p.w,source_x+(mirror?width-left:right));
        const int sy0=mx(0,source_y+top),sy1=mn(p.h,source_y+bottom);
        if(sx0>=sx1||sy0>=sy1)return;
        if(p.format==4){
            const uint8_t*run=p.pixels;
            for(int sy=0;sy<sy1;++sy){
                unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
                for(unsigned span=0;span<spans;++span){
                    const int sx=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8;
                    const unsigned stride=run[4]?3u:2u;run+=5;
                    if(sy>=sy0)for(int col=mx(sx,sx0),end=mn(sx+n,sx1);col<end;++col){
                        const uint8_t*pixel=run+(col-sx)*stride;
                        const unsigned alpha=div255((stride==3?pixel[2]:255)*opacity);
                        const int dx=mirror?width-1-(col-source_x):col-source_x;
                        paint((y+sy-source_y)*pitch+x+dx,uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha,mode);
                    }
                    run+=n*stride;
                }
            }
            return;
        }
        for(int sy=sy0;sy<sy1;++sy)for(int sx=sx0;sx<sx1;++sx){
            const unsigned at=unsigned(sy)*p.w+unsigned(sx),alpha=div255((p.alpha?p.alpha[at]:255)*opacity);
            if(!alpha)continue;
            const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
            const int dx=mirror?width-1-(sx-source_x):sx-source_x;
            paint((y+sy-source_y)*pitch+x+dx,uint16_t(pixel[0]|unsigned(pixel[1])<<8),alpha,mode);
        }
    }
    void clip(int id,int x,int y,uint32_t milliseconds,bool anchored=true,bool loop=true){
        auto c=a.content.clip(id);if(!c.count)return;unsigned index=milliseconds/c.frame_ms;
        index=loop?index%c.count:(unsigned)mn((int)index,c.count-1);
        picture((int)c.first+(int)index,x-(anchored?c.pivot_x:0),y-(anchored?c.pivot_y:0));
    }
    void scaled_clip(int id,int x,int y,uint32_t milliseconds,int scale,bool loop=true){
        if(scale==1024){clip(id,x,y,milliseconds,true,loop);return;}
        auto cl=a.content.clip(id);if(!cl.count||scale<=0)return;
        unsigned frame=milliseconds/cl.frame_ms;frame=loop?frame%cl.count:unsigned(mn(int(frame),cl.count-1));
        int image=int(cl.first)+int(frame);auto p=a.content.image(image);
        picture(p,x-cl.pivot_x*scale/1024,y-cl.pivot_y*scale/1024,mx(1,p.w*scale/1024),mx(1,p.h*scale/1024));
    }
    void actor(int actor,int x,int y,int scale=1024){
        if(actor<0||actor>MaxEnemies)return;
        int id=actor?AnimEnemyIdle:AnimHeroIdle;uint32_t elapsed=a.animation*33;bool loop=true;
        if(actor&&a.game.enemy_world.active){if(!a.enemy_frame(actor-1,id,elapsed,loop))return;}else{
            const auto&m=a.motion[actor];if(m.clip>=0&&(int32_t)(a.animation-m.start)>=0){int actual=actor?m.clip:character_animation(a.game.character,m.clip);auto c=a.content.clip(actual);uint32_t t=(a.animation-m.start)*33;if(t<(uint32_t)c.count*c.frame_ms){id=m.clip;elapsed=t;loop=false;}}
            if(actor==0)id=character_animation(a.game.character,id);
            elapsed+=((id==AnimEnemyIdle)?actor*347:0);
        }
        scaled_clip(id,x,y,elapsed,scale,loop);
    }
    int width(const char*p,int size){
        size=a.content.font_size(size,font_face,false,sparse_scene_sizes);
        char localized[1024];p=a.ui_text(p,localized,sizeof localized);
        RichReader reader(p);RichChar ch{};int w=0;
        while(reader.next(ch)&&ch.codepoint!='\n')w+=a.content.glyph_metrics(ch.codepoint,size,font_style(ch)).advance;
        return w;
    }
    void glyph(Glyph g,int x,int y,uint16_t color){
        if(g.pixels&&g.encoded_size){
            unsigned position=0,column=0,row=0;const unsigned pixels=unsigned(g.w)*g.h;
            for(unsigned at=0;at+1<g.encoded_size&&position<pixels;at+=2){
                unsigned count=mn(unsigned(g.pixels[at]),pixels-position);const unsigned alpha=g.pixels[at+1];
                position+=count;
                // RLE spans may cross several rows. Advance their row/column
                // cursor once per span instead of doing ARM software division
                // and remainder for every visible antialiased glyph pixel.
                while(count){
                    const unsigned span=mn(count,unsigned(g.w)-column);
                    const int yy=y+int(row),left=x+int(column),right=left+int(span);
                    if(alpha&&yy>=0&&yy<clip_h){
                        const int start=mx(0,left),end=mn(clip_w,right);
                        for(int index=yy*pitch+start,stop=yy*pitch+end;index<stop;++index)paint(index,color,alpha);
                    }
                    column+=span;count-=span;
                    if(column==g.w){column=0;++row;}
                }
            }return;
        }
        if(g.pixels)for(int yy=mx(0,-y);yy<mn(g.h,clip_h-y);yy++)for(int xx=mx(0,-x);xx<mn(g.w,clip_w-x);xx++){
            unsigned alpha=g.pixels[yy*g.w+xx];paint((y+yy)*pitch+x+xx,color,alpha);
        }
    }
    void character(uint32_t codepoint,Glyph g,int x,int y,uint16_t color,bool shadow=false){
        if(codepoint==InlineStarCodepoint){
            // Sprite-font images keep their authored colors and do not gain
            // a second copy from font bold/outline passes.
            if(!shadow)ui(U_Star,x,y,g.w,g.h);return;
        }
        glyph(g,x,y,color);
    }
    int text(const char*p,int x,int y,int size=12,uint16_t color=White,int maxwidth=320,int maxlines=1){
        size=a.content.font_size(size,font_face,false,sparse_scene_sizes);
        char localized[1024];p=a.ui_text(p,localized,sizeof localized);
        const uint16_t colors[]={color,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
        static const int wave[16]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
        RichReader reader(p);RichChar ch{};int start=x,line=0;
        while(reader.next(ch)){auto metric=a.content.glyph_metrics(ch.codepoint,size,font_style(ch));int advance=metric.advance;
            if(ch.codepoint=='\n'||(x>start&&x+advance>start+maxwidth)){x=start;y+=size+3;if(++line>=maxlines)break;if(ch.codepoint=='\n')continue;}
            if(ch.index>=text_visible_characters){x+=advance;continue;}
            const auto g=a.content.glyph(ch.codepoint,size,font_style(ch));
            int dy=a.text_effects&&ch.style.wave?wave[(a.animation/2+ch.index*2)%16]:0;
            if(a.text_effects&&ch.style.thinky_dots)dy+=rich_thinky_offset(a.map_clock_ms,ch.index-ch.style.thinky_start);
            int dx=a.text_effects&&ch.style.jitter?int((a.animation/2+ch.index*7)%3)-1:0;
            const auto ink=a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,a.map_clock_ms,(x-start)*26000/mx(1,size)):colors[ch.style.color];
            character(ch.codepoint,g,x+dx,y+dy,ink);x+=advance;
        }return x;
    }
    int paragraph(const char*p,int x,int y,int w,int lines,int skip=0,int size=12,uint16_t color=White){
        size=a.content.font_size(size,font_face,false,sparse_scene_sizes);
        char localized[1024];p=a.ui_text(p,localized,sizeof localized);
        const uint16_t colors[]={color,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
        RichReader reader(p);RichChar ch{};int at=x,row=0;bool boundary=true;
        const bool words=a.content.english()||wrap_latin_words;
        auto metrics=[&](const RichChar&value,int px){const auto g=a.content.glyph_metrics(value.codepoint,px,font_style(value));const int advance=g.advance;return fitted_text::Metric{advance,advance};};
        static const int wave[16]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
        while(reader.next(ch)){
            // English paragraphs and source article readers reserve whole words. The
            // copied rich reader preserves styles across tags and the live
            // reader's character indices. Oversized words retain glyph wrap.
            const int word=words&&boundary&&at>x&&fitted_text::word_character(ch.codepoint)?fitted_text::word_width(reader,ch,size,metrics):0;
            const auto metric=a.content.glyph_metrics(ch.codepoint,size,font_style(ch));int advance=metric.advance;
            const bool space=ch.codepoint==' '||ch.codepoint=='\t';
            if(words&&space&&(at==x||at+advance>x+w)){boundary=true;continue;}
            if(ch.codepoint=='\n'||(at>x&&(at+advance>x+w||(word&&word<=w&&at+word>x+w)))){at=x;row++;if(ch.codepoint=='\n'){boundary=true;continue;}}
            if(words){boundary=space;if(at==x&&space)continue;}
            if(row>=skip&&row<skip+lines&&ch.index<text_visible_characters){const auto g=a.content.glyph(ch.codepoint,size,font_style(ch));int dy=a.text_effects&&ch.style.wave?wave[(a.animation/2+ch.index*2)%16]:0;
                if(a.text_effects&&ch.style.thinky_dots)dy+=rich_thinky_offset(a.map_clock_ms,ch.index-ch.style.thinky_start);
                int dx=a.text_effects&&ch.style.jitter?(int)((a.animation/2+ch.index*7)%3)-1:0;
                // Original event body text is 26px. Normalize native glyph
                // advances to that scale, retaining label-local x after wrap.
                auto ink=a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,a.map_clock_ms,(at-x)*26000/mx(1,size)):colors[ch.style.color];int yy=y+(row-skip)*(size+3)+dy;
                character(ch.codepoint,g,at+dx,yy,ink);}
            at+=advance;
        }return row+1;
    }
    void centered(const char*p,int x,int y,int w,int size=12,uint16_t color=White){text(p,x+mx(0,(w-width(p,size))/2),y,size,color,w);}
    // Card labels have a real rectangle, unlike scrollable detail text. Use
    // actual available glyph bands, then ellipsize within the final visible line.
    fitted_text::Plan fitted(const char*p,int x,int y,int w,int h,int preferred,int minimum,uint16_t color,bool centered=false,bool single_line=false,bool outline=false,bool drop_shadow=false,uint16_t outline_color=Ink){
        char localized[1024];p=a.ui_text(p,localized,sizeof localized);
        auto metrics=[&](const RichChar&ch,int size){const auto g=a.content.glyph_metrics(ch.codepoint,size,font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
        minimum=a.content.font_size(minimum,font_face,false,sparse_scene_sizes);preferred=mx(minimum,a.content.font_size(preferred,font_face,false,sparse_scene_sizes));
        fitted_text::Plan plan;
        for(;;){
            plan=fitted_text::plan(p,w,h,preferred,preferred,metrics);
            if(single_line){plan.rows=1;plan.step=plan.size+1;plan.truncated=plan.total>1;}
            if(!plan.truncated||preferred<=minimum)break;
            const int next=a.content.font_size(preferred-1,font_face,true,sparse_scene_sizes);
            if(next>=preferred)break;preferred=mx(minimum,next);
        }
        int widths[32]{};if(plan.rows>32)plan.rows=32;
        if(centered)fitted_text::visit(p,w,plan,metrics,[&](const RichChar&ch,int at,int row,int size){widths[row]=at+metrics(ch,size).width;});
        const uint16_t colors[]={color,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
        const int top=y+(centered&&!single_line?mx(0,(h-(plan.rows*plan.step+2))/2):0);
        const auto draw=[&](int offset_x,int offset_y,bool shadow){
        Canvas clipped=*this;clipped.clip_w=mn(clip_w,x+w+offset_x);clipped.clip_h=mn(clip_h,y+h+offset_y);
        if(shadow&&drop_shadow&&!outline)clipped.opacity=uint8_t(div255(clipped.opacity*128));
        fitted_text::visit(p,w,plan,metrics,[&](const RichChar&ch,int at,int row,int size){
            if(ch.index>=text_visible_characters)return;
            const auto g=a.content.glyph(ch.codepoint,size,font_style(ch));int xx=x+at+(centered?mx(0,(w-widths[row])/2):0),yy=top+row*plan.step;
            // Keep animated rich text within its allocated card rectangle.
            static const int wave[]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
            int dy=a.text_effects&&ch.style.wave?wave[(a.animation/2+ch.index*2)%16]:0;
            if(a.text_effects&&ch.style.thinky_dots)dy+=rich_thinky_offset(a.map_clock_ms,ch.index-ch.style.thinky_start);
            int dx=a.text_effects&&ch.style.jitter?int((a.animation/2+ch.index*7)%3)-1:0;
            xx=mx(x,mn(xx+dx,x+w-int(g.w)));yy=mx(y,yy+dy);
            auto ink=shadow?outline_color:a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,a.map_clock_ms,at*26000/mx(1,size)):colors[ch.style.color];
            clipped.character(ch.codepoint,g,xx+offset_x,yy+offset_y,ink,shadow);
        });};
        // The eight outline passes reuse one fit/centering decision. Their
        // paint order stays unchanged, including overlapping antialiased glyphs.
        if(outline){for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)draw(dx,dy,true);}
        // The native event labels use a half-alpha offset shadow. One pass
        // keeps that authored shadow instead of paying for an eight-pass rim.
        else if(drop_shadow)draw(1,1,true);
        draw(0,0,false);return plan;
    }
    void text_fitted(const char*p,int x,int y,int w,int preferred=10,uint16_t color=White,int minimum=8,bool centered=false,bool outline=false,uint16_t outline_color=Ink){fitted(p,x,y,w,preferred+3,preferred,minimum,color,centered,true,outline,false,outline_color);}
    fitted_text::Plan paragraph_fitted(const char*p,int x,int y,int w,int h,int preferred=10,uint16_t color=White,int minimum=8,bool centered=false){return fitted(p,x,y,w,h,preferred,minimum,color,centered);}
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
    void hp(const HealthBarValues&,const HealthBarMotion&,int x,int y,int width);
    void button(const char*p,int x,int y,int w,bool selected,int h=25,bool enabled=true){
        const auto visual=control_visual(p,x,y,w,h,ButtonFamily::Pause,selected,enabled);
        const int width=mx(1,w*visual.scale/4096),height=mx(1,h*visual.scale/4096);
        const int left=x+(w-width)/2,top=y+(h-height)/2+(visual.y+2048)/4096;
        Canvas skin=*this;skin.brightness=uint8_t(div255(unsigned(brightness)*visual.shade));
        // Small auxiliary controls share the original pause/reward plaque.
        // Preserve its corner artwork when a readable label needs more width.
        const int normal=a.content.ui_image(U_PauseButton),hover=a.content.ui_image(U_PauseButtonHover);
        if(normal>=0){skin.nine_image(normal,left,top,width,height,14,5,14,5);
            if(visual.hover&&hover>=0)skin.nine_image(hover,left,top,width,height,14,5,14,5,unsigned(visual.hover)*255/4096);
        }else skin.ui(U_RewardButton,left,top,width,height);
        const uint16_t cream=rgb(255,246,226),gold=rgb(239,200,81);
        const uint16_t color=enabled?blend(cream,gold,unsigned(visual.hover)*255/4096):Muted;
        const int label_y=y+(h-12)/2+(visual.y+2048)/4096;
        skin.centered(p,x+7,label_y+1,w-12,12,rgb(37,84,92));
        skin.centered(p,x+6,label_y,w-12,12,color);
    }
};
#include "health_bar_ui.inc"
