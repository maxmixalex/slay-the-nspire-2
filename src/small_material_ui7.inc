// The source hsv.gdshader keeps hue at 1 for these controls. Its YIQ chroma
// saturation/value operations are independent from alpha and the backdrop.
// A bounded thumbnail is enough for both rest choices and character portraits.
static void small_material_icon(Canvas&c,int image,int x,int y,int w,int h,unsigned saturation,unsigned value,bool source_modulation=false){
 if(saturation==1000&&value==1000){c.picture(image,x,y,w,h);return;}
 if(w<=0||h<=0||c.clip_w<=0||c.clip_h<=0||!c.opacity||x>=c.clip_w||y>=c.clip_h||x<=-w||y<=-h)return;
 // UI11 enlarged controls beyond the old thumbnail buffer. Raster bounded
 // tiles of the complete scaled image; never drop the authored artwork.
 const auto picture=c.a.content.image(image);if(!picture.pixels)return;
 const int first_x=mx(0,-x),last_x=mn(w,c.clip_w-x),first_y=mx(0,-y),last_y=mn(h,c.clip_h-y);
 const auto material=[&](uint16_t color){
  if(source_modulation)return presentation::yiq_material(color,saturation,value,c.brightness,c.tint_red,c.tint_green,c.tint_blue);
  const int r=int(color>>11)*255/31,g=int((color>>5)&63)*255/63,b=int(color&31)*255/31;
  const int luminance=(2989*r+5870*g+1140*b+5000)/10000;
  const auto channel=[&](int v){return mn(255,mx(0,(luminance+(v-luminance)*int(saturation)/1000)*int(value)/1000));};
  return rgb(channel(r),channel(g),channel(b));
 };
 Canvas output=c;if(source_modulation){output.brightness=255;output.tint_red=output.tint_green=output.tint_blue=255;}
 const bool direct=output.opacity==255&&output.brightness==255&&output.tint_red==255&&output.tint_green==255&&output.tint_blue==255&&
  !output.color_matrix&&!output.coverage&&!output.occlusion;
 if(picture.format<4&&last_x-first_x<=320){
  // A dense/palette source already has one color and alpha per texel.
  // Sampling it directly preserves the existing nearest rule while avoiding
  // a temporary RGBA raster and a second general paint call for each pixel.
  uint16_t columns[320],palette[256];
  const unsigned divisor=unsigned(w),step=picture.w/divisor,advance=picture.w%divisor;
  unsigned column=unsigned(first_x)*picture.w/divisor,remainder=unsigned(first_x)*picture.w%divisor;
  for(int xx=first_x;xx<last_x;++xx){columns[xx-first_x]=uint16_t(column);column+=step;remainder+=advance;if(remainder>=divisor){remainder-=divisor;++column;}}
  if(picture.palette)for(unsigned i=0;i<256;++i)palette[i]=material(uint16_t(picture.palette[2*i]|unsigned(picture.palette[2*i+1])<<8));
  unsigned previous=65536;uint16_t transformed=0;
  for(int yy=first_y;yy<last_y;++yy){
   const unsigned row=unsigned(yy)*picture.h/unsigned(h)*picture.w;
   auto*out=c.pixels+(y+yy)*c.pitch+x+first_x;
   for(int xx=first_x;xx<last_x;++xx,++out){
    const unsigned at=row+columns[xx-first_x],alpha=picture.alpha?picture.alpha[at]:255;if(!alpha)continue;
    uint16_t color;
    if(picture.palette)color=palette[picture.pixels[at]];
    else{const auto*source=picture.pixels+2*at;const unsigned raw=source[0]|unsigned(source[1])<<8;
     if(raw!=previous){previous=raw;transformed=material(uint16_t(raw));}color=transformed;}
    if(direct)*out=alpha==255?color:blend(*out,color,alpha);
    else output.paint((y+yy)*c.pitch+x+xx,color,alpha);
   }
  }
  return;
 }
 for(int by=first_y;by<last_y;by+=16)for(int bx=first_x;bx<last_x;bx+=64){
 const int tile_w=mn(64,last_x-bx),tile_h=mn(16,last_y-by);
 uint16_t pixels[64*16];uint8_t coverage[64*16]{};
 Canvas tile{c.a,pixels,64,tile_w,tile_h,coverage};tile.picture(picture,-bx,-by,w,h);
 for(int yy=0;yy<tile_h;++yy)for(int xx=0;xx<tile_w;++xx){const unsigned at=unsigned(yy*64+xx),alpha=coverage[at];
  if(!alpha||x+bx+xx<0||x+bx+xx>=c.clip_w||y+by+yy<0||y+by+yy>=c.clip_h)continue;
  const auto color=material(pixels[at]);const int destination=(y+by+yy)*c.pitch+x+bx+xx;
  if(direct)c.pixels[destination]=alpha==255?color:blend(c.pixels[destination],color,alpha);
  else output.paint(destination,color,alpha);
 }
 }
}
