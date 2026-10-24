// The source hsv.gdshader keeps hue at 1 for these controls. Its YIQ chroma
// saturation/value operations are independent from alpha and the backdrop.
// A bounded thumbnail is enough for both rest choices and character portraits.
static void small_material_icon(Canvas&c,int image,int x,int y,int w,int h,unsigned saturation,unsigned value){
 if(saturation==1000&&value==1000){c.picture(image,x,y,w,h);return;}
 if(w<=0||h<=0||w>56||h>48)return;
 uint16_t pixels[56*48];uint8_t coverage[56*48]{};
 Canvas tile{c.a,pixels,56,w,h,coverage};tile.picture(image,0,0,w,h);
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){const unsigned at=unsigned(yy*56+xx),alpha=coverage[at];
  if(!alpha||x+xx<0||x+xx>=c.clip_w||y+yy<0||y+yy>=c.clip_h)continue;
  const unsigned color=pixels[at];const int r=int(color>>11)*255/31,g=int((color>>5)&63)*255/63,b=int(color&31)*255/31;
  const int luminance=(2989*r+5870*g+1140*b+5000)/10000;
  auto channel=[&](int v){return mn(255,mx(0,(luminance+(v-luminance)*int(saturation)/1000)*int(value)/1000));};
  c.paint((y+yy)*c.pitch+x+xx,rgb(channel(r),channel(g),channel(b)),alpha);
 }
}
