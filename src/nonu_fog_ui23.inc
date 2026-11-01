namespace nonu_fog {
using namespace nonu_fog_ui23;
// Source parent anisotropic/negative scales are applied AFTER particle
// rotation. Keeping two affine basis vectors avoids silently exchanging them.
static void plane(Canvas c,const uint8_t*alpha,int cx,int cy,int ax,int ay,int bx,int by,unsigned gray,unsigned source_opacity=97){
 if(!gray||!alpha||c.clip_w<=0||c.clip_h<=0)return;
 const int64_t determinant=int64_t(ax)*by-int64_t(ay)*bx;if(!determinant)return;
 const int hw=(CombatCardMotion::magnitude(ax)+CombatCardMotion::magnitude(bx))/2;
 const int hh=(CombatCardMotion::magnitude(ay)+CombatCardMotion::magnitude(by))/2;
 const int left=mx(0,(cx-hw)/256-1),right=mn(c.clip_w,(cx+hw)/256+2);
 const int top=mx(0,(cy-hh)/256-1),bottom=mn(c.clip_h,(cy+hh)/256+2);
 if(left>=right||top>=bottom)return;
 const int du=int(int64_t(by)*256*16777216/determinant);
 const int dv=int(-int64_t(ay)*256*16777216/determinant);
 const unsigned r=div255(145*gray),g=div255(185*gray),b=div255(194*gray);
 const uint16_t color=uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));if(!color&&!c.coverage)return;
 c.opacity=uint8_t(div255(c.opacity*source_opacity));
 const bool direct=!c.coverage&&!c.occlusion&&!c.color_matrix&&c.brightness==255&&
   c.tint_red==255&&c.tint_green==255&&c.tint_blue==255;
 // Constant particle color: compute the 256 possible source contributions
 // once, instead of repeating three multiplications/divisions per texel.
 // This 512-byte local table is not a persistent scene cache.
 uint16_t contributions[256];
 if(direct)for(unsigned i=0;i<256;++i)
  contributions[i]=presentation::add_opaque(0,color,div255(i*c.opacity));
 const int dx=left*256+128-cx,dy=top*256+128-cy;
 AffineRows row_u((int64_t(dx)*by-int64_t(dy)*bx)*16777216,-int64_t(bx)*4294967296ll,determinant);
 AffineRows row_v((int64_t(dy)*ax-int64_t(dx)*ay)*16777216,int64_t(ax)*4294967296ll,determinant);
 for(int y=top;y<bottom;++y,row_u.next(),row_v.next()){
  if(!(y&15))c.poll();
  int u=8388608+row_u.value(),v=8388608+row_v.value();
  for(int x=left;x<right;++x,u+=du,v+=dv){
   if(unsigned(u)>=16777216u||unsigned(v)>=16777216u)continue;
   const unsigned coverage=nonu_fog_ui23::alpha(alpha,u,v);
   if(coverage){const int at=y*c.pitch+x;
    if(direct)c.pixels[at]=nonu_fog_ui23::add_weighted(c.pixels[at],contributions[coverage]);
    else c.paint(at,color,coverage,presentation::Add);
   }
  }
 }
}
static void draw(Canvas&c,uint32_t elapsed,uint32_t seed){
 using namespace fake_merchant;
 struct Node {int x,y;int16_t sx,sy;uint8_t count;};
 static constexpr Node nodes[]={
  {source(1540),source(1246.0001),fixed(-1.595),fixed(.96),5},
  {source(756),source(1263),fixed(1.595),fixed(.96),5},
  {source(1630.0001),source(913),fixed(1.595),fixed(.595),7},
  {source(306),source(908),fixed(-1.595),fixed(.595),7},
  {source(2783),source(1081),fixed(-1.595),fixed(.96),10},
  {source(-939.00006),source(1081),fixed(1.595),fixed(.96),10}};
 for(unsigned node=0;node<6;++node){const auto&n=nodes[node];Parameters parameters;
  if(!parameters.read(c.a.content.image(c.a.content.ui_image(Auxiliary))))return;
  Particle particles[10];
  // Compute before fetching any atlas plane: Content loads may evict the
  // borrowed parameter view. The bounded10-particle stack is not a cache.
  for(unsigned i=0;i<n.count;++i)particles[i]=particle(parameters,elapsed,seed,node,i);
  for(unsigned i=0;i<n.count;++i){const auto&p=particles[i];if(!p.gray)continue;
   const auto image=c.a.content.image(FirstImage+p.atlas);
   if(!image.pixels||image.w!=100||image.h!=200||image.format||image.alpha||image.palette)return;
   const int sn=CombatCardMotion::sine(p.angle),cs=CombatCardMotion::sine(p.angle+1024);
   const int side=ancient_size(200)*p.scale/Q;
   const int ax=int(int64_t(side)*cs*n.sx/(1024*Q)),ay=int(int64_t(side)*sn*n.sy/(1024*Q));
   const int bx=int(-int64_t(side)*sn*n.sx/(1024*Q)),by=int(int64_t(side)*cs*n.sy/(1024*Q));
   plane(c,image.pixels,ancient_x(n.x+int(int64_t(p.x)*n.sx/Q)),ancient_y(n.y),ax,ay,bx,by,p.gray);
  }
 }
}
}
