// Small room particles share the already-installed UI7 masks. Their clocks
// and hashes are cosmetic; sampling never touches the saved run or its RNG.
static uint32_t room_vfx_hash(uint32_t value){
 value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;return value^(value>>16);
}
static void room_vfx_stamp(Canvas c,int asset,int x,int y,int w,int h,unsigned alpha,
                           uint32_t tint,unsigned blend_mode=presentation::Add,bool below_top_bar=true){
 if(!alpha||w<=0||h<=0||c.a.content.version()<Ui7ContentVersion)return;
 const int image=c.a.content.ui_image(asset);if(image<0)return;
 if(below_top_bar){
  if(c.clip_h<=22)return;c.pixels+=22*c.pitch;if(c.coverage)c.coverage+=22*c.pitch;
  c.clip_h-=22;y-=22;
 }
 c.tint_red=uint8_t(div255(c.tint_red*(tint>>16&255)));
 c.tint_green=uint8_t(div255(c.tint_green*(tint>>8&255)));
 c.tint_blue=uint8_t(div255(c.tint_blue*(tint&255)));
 c.picture(image,x,y,w,h,alpha,blend_mode);
}
static void room_vfx_cinders(Canvas&c,int x,int y,unsigned count,uint32_t now,uint32_t seed,unsigned alpha=255){
 // Original camp and candle emitters rise slowly and expire independently.
 // At LCD scale a fixed maximum of eight visible specks keeps both geometry
 // and work bounded, including a room left open for hours.
 for(unsigned i=0;i<mn(int(count),8);++i){
  const uint32_t hash=room_vfx_hash(seed+i*0x9e3779b9u),life=2500+hash%1801;
  const uint32_t age=(now+(hash>>8)%life)%life;
  const unsigned fade=age<350?age*255/350:age>life-900?(life-age)*255/900:255;
  const int drift=(int(hash>>24)-128)*int(age)/16000;
  const int px=x+int(hash%9)-4+drift,py=y-int(age*(9+(hash>>16)%7)/1000);
  room_vfx_stamp(c,I7_Speck,px,py,1+(hash&1),2,div255(alpha*fade),0xff9b45);
 }
}
static void room_vfx_smoke(Canvas&c,int x,int y,int spread,uint32_t age,unsigned count,uint32_t seed){
 if(age>=4000)return;
 // RestSmoke emits over its first 0.6s, with 2.25--3s particle lifetimes.
 // Reusing three overlapping soft masks per cloud supplies the rising cloud
 // silhouette without adding a smoke atlas or retaining a particle heap.
 for(unsigned i=0;i<mn(int(count),12);++i){
  const uint32_t hash=room_vfx_hash(seed+i*0x9e3779b9u),delay=hash%601,life=2250+(hash>>9)%751;
  if(age<delay||age-delay>=life)continue;const uint32_t t=age-delay;
  const unsigned q=t*4096/life;
  const unsigned alpha=q<512?q*112/512:q<2144?112-(q-512)*56/1632:(4096-q)*56/1952;
  const int side=24+int(hash%17)+int(q*36/4096);
  const int px=x-spread/2+int((hash>>16)%unsigned(mx(1,spread)))+(int((hash>>5)&31)-15)*int(t)/2500;
  const int py=y+int((hash>>24)%35)-int(t*18/1000)-int(uint64_t(t)*t*23/1000000);
  room_vfx_stamp(c,I7_Glow,px-side/2,py-side/2,side,side,alpha,0x666666,0);
  room_vfx_stamp(c,I7_Glow,px-side/2-side/5,py-side/3,side*2/3,side*2/3,alpha,0x555555,0);
  room_vfx_stamp(c,I7_Glow,px+side/8,py-side/3,side*2/3,side*2/3,alpha,0x555555,0);
 }
}
static void room_vfx_rest_tone(Canvas&c,uint32_t age){
 if(age>=3000||!age)return;
 // NDesaturateTransitionVfx: one second to brightness .1 / saturation .25
 // / contrast .7, one second to contrast .8, then one second back to normal.
 // Apply only to the room, preserving the LCD's legible top-bar labels.
 const unsigned strength=age<1000?age*256/1000:age<2000?256:(3000-age)*256/1000;
 const unsigned saturation=256-192*strength/256,brightness=256-230*strength/256;
 const unsigned contrast=age<1000?256-age*77/1000:age<2000?179+(age-1000)*26/1000:205+(age-2000)*51/1000;
 for(int y=22;y<c.clip_h;++y)for(int x=0;x<c.clip_w;++x){
  uint16_t&pixel=c.pixels[y*c.pitch+x];const unsigned source=pixel;
  const int r=int(source>>11)*2,g=int(source>>5&63),b=int(source&31)*2;
  const int gray=(77*r+150*g+29*b+128)>>8;
  const auto channel=[&](int value){
   const int saturated=gray+(value-gray)*int(saturation)/256;
   const int contrasted=32+(saturated-32)*int(contrast)/256;
   return mx(0,mn(63,contrasted*int(brightness)/256));
  };
  pixel=uint16_t((channel(r)/2<<11)|(channel(g)<<5)|(channel(b)/2));
 }
}
