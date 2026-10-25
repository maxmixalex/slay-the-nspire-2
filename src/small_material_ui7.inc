// The source hsv.gdshader keeps hue at 1 for these controls. Its YIQ chroma
// saturation/value operations are independent from alpha and the backdrop.
// A bounded thumbnail is enough for both rest choices and character portraits.
static void small_material_icon(Canvas&c,int image,int x,int y,int w,int h,unsigned saturation,unsigned value,bool source_modulation=false){
 if(saturation==1000&&value==1000){c.picture(image,x,y,w,h);return;}
 if(w<=0||h<=0)return;
 // UI11 enlarged controls beyond the old thumbnail buffer. Raster bounded
 // tiles of the complete scaled image; never drop the authored artwork.
 const auto picture=c.a.content.image(image);if(!picture.pixels)return;
 const int first_x=mx(0,-x),last_x=mn(w,c.clip_w-x),first_y=mx(0,-y),last_y=mn(h,c.clip_h-y);
 for(int by=first_y;by<last_y;by+=48)for(int bx=first_x;bx<last_x;bx+=64){
 const int tile_w=mn(64,last_x-bx),tile_h=mn(48,last_y-by);
 uint16_t pixels[64*48];uint8_t coverage[64*48]{};
 Canvas tile{c.a,pixels,64,tile_w,tile_h,coverage};tile.picture(picture,-bx,-by,w,h);
 Canvas output=c;if(source_modulation){output.brightness=255;output.tint_red=output.tint_green=output.tint_blue=255;}
 for(int yy=0;yy<tile_h;++yy)for(int xx=0;xx<tile_w;++xx){const unsigned at=unsigned(yy*64+xx),alpha=coverage[at];
  if(!alpha||x+bx+xx<0||x+bx+xx>=c.clip_w||y+by+yy<0||y+by+yy>=c.clip_h)continue;
  if(source_modulation){output.paint((y+by+yy)*c.pitch+x+bx+xx,presentation::yiq_material(pixels[at],saturation,value,c.brightness,c.tint_red,c.tint_green,c.tint_blue),alpha);continue;}
  const unsigned color=pixels[at];const int r=int(color>>11)*255/31,g=int((color>>5)&63)*255/63,b=int(color&31)*255/31;
  const int luminance=(2989*r+5870*g+1140*b+5000)/10000;
  auto channel=[&](int v){return mn(255,mx(0,(luminance+(v-luminance)*int(saturation)/1000)*int(value)/1000));};
  c.paint((y+by+yy)*c.pitch+x+bx+xx,rgb(channel(r),channel(g),channel(b)),alpha);
 }
 }
}
