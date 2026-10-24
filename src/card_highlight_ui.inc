// NCardHighlight is behind Portrait/Frame, above Shadow. Its packed source
// scalar/color pairs use the existing image page cache and no staging image.
static void card_selection_highlight(Canvas&c,int x,int y,int w,int h,unsigned width){
    if(!width||!c.a.content.has_selection_assets())return;
    const Picture image=c.a.content.image(c.a.content.ui_image(I9_CardHighlightSdf));
    if(!image.pixels||image.format!=0||image.w!=290||image.h!=181)return;
    const int left=x+w/2-card_face::scaled(381,w,300),top=y+h/2-card_face::scaled(475,h,422);
    const int wide=card_face::scaled(759,w,300),high=card_face::scaled(951,h,422);
    if(wide<=0||high<=0)return;
    const int x0=mx(0,-left),x1=mn(wide,c.clip_w-left),y0=mx(0,-top),y1=mn(high,c.clip_h-top);
    if(x0>=x1||y0>=y1)return;
    const card_highlight::Context shader(width,c.a.map_clock_ms);
    uint16_t columns[320];
    const unsigned xstep=145u/unsigned(wide),xrest=145u%unsigned(wide);
    unsigned column=unsigned(x0)*145u/unsigned(wide),remainder=unsigned(x0)*145u%unsigned(wide);
    for(int xx=x0;xx<x1;++xx){columns[xx-x0]=uint16_t(column*4);column+=xstep;remainder+=xrest;if(remainder>=unsigned(wide)){remainder-=unsigned(wide);++column;}}
    const unsigned ystep=181u/unsigned(high),yrest=181u%unsigned(high);
    unsigned row_index=unsigned(y0)*181u/unsigned(high),yrem=unsigned(y0)*181u%unsigned(high);
    for(int yy=y0;yy<y1;++yy){const unsigned row=row_index*580;
        for(int xx=x0;xx<x1;++xx){const unsigned at=row+columns[xx-x0];
            const uint16_t scalar=uint16_t(image.pixels[at]|unsigned(image.pixels[at+1])<<8);
            const unsigned alpha=shader.alpha(scalar);if(!alpha)continue;
            const uint16_t original=uint16_t(image.pixels[at+2]|unsigned(image.pixels[at+3])<<8);
            const uint16_t color=uint16_t(((((original>>5)&63)*957+500)/1000<<5)|(((original&31)*988+500)/1000));
            c.paint((top+yy)*c.pitch+left+xx,color,alpha,presentation::Add);
        }
        row_index+=ystep;yrem+=yrest;if(yrem>=unsigned(high)){yrem-=unsigned(high);++row_index;}
    }
}
