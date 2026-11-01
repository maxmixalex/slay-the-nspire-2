// These two Ancient option assets are white source RGB with an alpha shape.
// Validate every visible texel on every draw; alternate/old assets and Canvas
// effects keep the ordinary path. No proof or image survives this call.
// Constant tint and the two separately rounded opacity stages are shared by
// all destination pixels. The ordinary nearest projection remains unchanged.
static void ancient_option_image(Canvas&c,int image,int x,int y,int width,int height,unsigned opacity,unsigned mode=presentation::Normal){
 c.poll();const auto p=c.a.content.image(image);if(!width)width=p.w;if(!height)height=p.h;
 const int left=mx(0,-x),right=mn(width,c.clip_w-x),top=mx(0,-y),bottom=mn(height,c.clip_h-y);
 if(width<=0||height<=0||left>=right||top>=bottom||!opacity)return;
 if(!p.pixels||!p.alpha||p.palette||p.format!=1||p.w>320||p.h>240||c.coverage||c.occlusion||c.color_matrix||right-left>320||(mode!=presentation::Normal&&mode!=presentation::Add)){c.picture(p,x,y,width,height,opacity,mode);return;}
 for(unsigned i=0;i<unsigned(p.w*p.h);++i){if(!(i&4095u))c.poll();if(p.alpha[i]&&(p.pixels[i*2]!=255||p.pixels[i*2+1]!=255)){c.picture(p,x,y,width,height,opacity,mode);return;}}
 uint16_t color=presentation::modulate_rgb(65535,c.brightness);color=presentation::tint_rgb(color,c.tint_red,c.tint_green,c.tint_blue);
 uint8_t alphas[256];for(unsigned i=0;i<256;++i)alphas[i]=uint8_t(div255(div255(i*opacity)*c.opacity));
 if(!alphas[255])return;
 uint16_t contribution[256];
 if(mode==presentation::Add){for(unsigned i=0;i<256;++i)contribution[i]=presentation::add_opaque(0,color,alphas[i]);}
 else{for(unsigned i=0;i<32;++i){contribution[i]=presentation::source_over_rgb565(uint16_t(i<<11),color,alphas[255])&0xf800;contribution[96+i]=presentation::source_over_rgb565(uint16_t(i),color,alphas[255])&31;}
  for(unsigned i=0;i<64;++i)contribution[32+i]=presentation::source_over_rgb565(uint16_t(i<<5),color,alphas[255])&0x7e0;}
 uint16_t columns[320];const unsigned step=p.w/unsigned(width),advance=p.w%unsigned(width),first=unsigned(left)*p.w;
 unsigned column=first/unsigned(width),rem=first%unsigned(width);for(int i=left;i<right;++i){columns[i-left]=uint16_t(column);column+=step;rem+=advance;if(rem>=unsigned(width)){rem-=unsigned(width);++column;}}
 const unsigned row_step=p.h/unsigned(height),row_advance=p.h%unsigned(height),first_row=unsigned(top)*p.h;
 unsigned row_index=first_row/unsigned(height),row_rem=first_row%unsigned(height);
 for(int yy=top;yy<bottom;++yy){c.poll_row(unsigned(yy-top));const auto*source=p.alpha+row_index*p.w;auto*out=c.pixels+(y+yy)*c.pitch+x+left;
  for(int xx=left;xx<right;++xx,++out){const unsigned alpha=source[columns[xx-left]];if(mode==presentation::Add){const uint16_t add=contribution[alpha];if(add)*out=presentation::add_preweighted(*out,add);}
   else if(alpha==255){const unsigned old=*out;*out=uint16_t(contribution[old>>11]|contribution[32+((old>>5)&63)]|contribution[96+(old&31)]);}
   else if(alphas[alpha])*out=presentation::source_over_rgb565(*out,color,alphas[alpha]);}
  row_index+=row_step;row_rem+=row_advance;if(row_rem>=unsigned(height)){row_rem-=unsigned(height);++row_index;}
 }
}
