// The two source screen-distortion materials are sampled as signed LCD
// displacements. Keep source rows in a20KiB rolling window before changing
// any destination, rather than allocating another320x240 framebuffer.
static void architect_source_displacement(Canvas&c,Picture p,int left,int top){
 if(!p.pixels||p.w>320||p.h>240)return;
 const int first=mx(22,top),last=mn(c.clip_h,top+p.h);if(first>=last)return;
 constexpr unsigned Rows=32;ForegroundScratch<uint16_t[320],Rows> storage;if(!storage)return;
 auto&saved=storage.array();
 int loaded=mx(22,first-15)-1;
 const auto preload=[&](int y){
  const int end=mn(c.clip_h-1,y+15);
  while(loaded<end){++loaded;for(int x=0;x<c.clip_w;++x)saved[unsigned(loaded)%Rows][x]=c.pixels[loaded*c.pitch+x];}
 };
 const auto displace=[&](int x,int y,uint16_t encoded,unsigned alpha){
  if(!alpha||x<0||x>=c.clip_w||y<first||y>=last)return;
  const int dx=int(encoded>>11)-16,dy=int((encoded>>5)&63)-32;
  // The converter verifies |dy|<=15 for these pinned source shaders.
  // An unsupported optional asset cannot read outside the rolling window.
  if(dy<-15||dy>15)return;
  const int sx=health_bar::bounded(x+dx,0,c.clip_w-1),sy=health_bar::bounded(y+dy,22,c.clip_h-1);
  const int at=y*c.pitch+x;c.pixels[at]=saved[unsigned(sy)%Rows][sx];if(c.coverage)c.coverage[at]=255;
 };
 if(p.format==4){const uint8_t*run=p.pixels;
  for(int sy=0;sy<p.h;++sy){const int y=top+sy;if(y>=first&&y<last)preload(y);
   const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
   for(unsigned span=0;span<spans;++span){const unsigned x=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,stride=run[4]?3u:2u;run+=5;
    if(y>=first&&y<last)for(unsigned i=0;i<n;++i){const auto*pixel=run+i*stride;
     displace(left+int(x+i),y,uint16_t(pixel[0]|uint16_t(pixel[1])<<8),stride==3?pixel[2]:255);}
    run+=n*stride;
   }
  }return;
 }
 for(int y=first;y<last;++y){preload(y);const unsigned row=unsigned(y-top)*p.w;
  for(int x=mx(0,-left),end=mn(p.w,c.clip_w-left);x<end;++x){const unsigned at=row+unsigned(x);
   const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
   displace(left+x,y,uint16_t(pixel[0]|uint16_t(pixel[1])<<8),p.alpha?p.alpha[at]:255);
  }
 }
}
static void architect_source_attack(Canvas&c,unsigned kind,unsigned age,int x,int y,bool mirror=false){
 if(kind>ArchitectFireBurst)return;const auto&asset=ArchitectAttackAssets[kind];
 if(age>=asset.lifetime||c.a.content.ui_image(asset.first)<0)return;
 const unsigned frame=asset.frame(age);const int left=x-asset.pivot_x,top=y-asset.pivot_y;
 if(asset.planes&ArchitectAttackAsset::Warp)
  architect_source_displacement(c,c.a.content.image(c.a.content.ui_image(asset.first+asset.frames+frame)),left,top);
 const auto paint=[&](unsigned id,unsigned blend){
  const auto p=c.a.content.image(c.a.content.ui_image(id));if(!p.pixels)return;
  if(mirror)c.picture_region(p,x+asset.pivot_x-int(p.w)+1,top,0,0,p.w,p.h,true,255,blend);
  else c.picture(p,left,top,0,0,255,blend);
 };
 paint(asset.first+frame,presentation::Normal);
 if(asset.planes&ArchitectAttackAsset::Add)paint(asset.first+asset.frames+frame,presentation::Add);
}
