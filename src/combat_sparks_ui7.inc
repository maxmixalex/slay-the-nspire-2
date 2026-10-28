// Visible particle systems from hit_spark_vfx/block_spark_vfx.tscn. Original
// textures remain private; these fixed-point samplers preserve the authored
// grayscale, polar-ring and flipbook shader contracts at calculator scale.
namespace {
struct SparkTexture {
 Picture source{};uint32_t rows[128];
 bool bind(Picture picture){
  source=picture;if(!source.pixels||!source.w||!source.h||source.w>128||source.h>128)return false;
  if(source.format!=4)return source.format<4;
  const uint8_t*p=source.pixels;
  for(unsigned y=0;y<source.h;++y){
   rows[y]=uint32_t(p-source.pixels);const unsigned spans=p[0]|unsigned(p[1])<<8;p+=2;
   for(unsigned n=0;n<spans;++n){const unsigned count=p[2]|unsigned(p[3])<<8,mode=p[4];p+=5+count*(mode?3:2);}
  }
  return true;
 }
 bool read(const Content&content,unsigned id){
  return bind(content.image(content.ui_image(id)));
 }
 void sample(unsigned u,unsigned v,unsigned&r,unsigned&a)const{
  const unsigned x=u*source.w/1024u,y=v*source.h/1024u,at=y*source.w+x;
  if(source.format==4){
   const auto*row=source.pixels+rows[y];const unsigned spans=row[0]|unsigned(row[1])<<8;row+=2;
   for(unsigned n=0;n<spans;++n){
    const unsigned left=row[0]|unsigned(row[1])<<8,count=row[2]|unsigned(row[3])<<8,mode=row[4];row+=5;
    if(x<left)break;
    if(x-left<count){const auto*p=row+(x-left)*(mode?3:2);const unsigned red=p[1]>>3;
     r=(red<<3)|(red>>2);a=mode?p[2]:255;return;}
    row+=count*(mode?3:2);
   }
   r=a=0;return;
  }
  const uint8_t*p=source.palette?source.palette+2*source.pixels[at]:source.pixels+2*at;const unsigned red=p[1]>>3;
  r=(red<<3)|(red>>2);a=source.alpha?source.alpha[at]:255;
 }
};
static_assert(sizeof(SparkTexture)<=576,"Particle texture sampling must not reserve a 32 KiB stack raster");
static unsigned spark_smooth(unsigned value,unsigned lower,unsigned width){
 if(value<=lower)return 0;if(value>=lower+width)return 255;
 const unsigned t=(value-lower)*1024u/width;return t*t/1024u*(3072u-2u*t)/1024u*255u/1024u;
}
// mode0 is the original grayscale LUT shader, mode1 polar ring, mode2
// outward-streak flipbook. Width/height/center retainQ10 subpixel precision.
static void spark_quad(Canvas&c,const SparkTexture&p,int cx,int cy,int width,int height,int angle,uint16_t color,unsigned opacity,unsigned mode=0,unsigned progress=0,unsigned frame=0){
 if(width<128||height<128||!opacity)return;
 const int sn=CombatCardMotion::sine(angle),cs=CombatCardMotion::sine(angle+1024);
 if(mode==2){cx-=height/4*sn/1024;cy+=height/4*cs/1024;}
 const int ex=(CombatCardMotion::magnitude(cs)*width+CombatCardMotion::magnitude(sn)*height)/2048+1024;
 const int ey=(CombatCardMotion::magnitude(sn)*width+CombatCardMotion::magnitude(cs)*height)/2048+1024;
 const int x0=mx(0,(cx-ex)/1024),x1=mn(c.clip_w,(cx+ex+1023)/1024),y0=mx(22,(cy-ey)/1024),y1=mn(c.clip_h,(cy+ey+1023)/1024);
 const unsigned erosion=mode==1?unsigned(CombatSparkMotion::bounded(CombatSparkMotion::curve(int(progress),512,1024,1184,0)))*255u/1024u:
  mode==2?unsigned(CombatSparkMotion::bounded(CombatSparkMotion::curve(int(progress),0,1024,2378,237)))*255u/1024u:0u;
 for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){const int rx=x*1024+512-cx,ry=y*1024+512-cy;
  int u=(rx*cs+ry*sn)/width+512,v=(-rx*sn+ry*cs)/height+512;if(u<0||u>=1024||v<0||v>=1024)continue;
  if(mode==1){const int xx=u-512,yy=v-512;u=int(ui_integer_sqrt(unsigned(xx*xx+yy*yy))*2u)&1023;v=(CombatCardMotion::direction(yy,xx)&4095)/4;}
  else if(mode==2){u=(u+int(frame&1u)*1024)/2;v=(v+int((frame>>1)&1u)*1024)/2;}
  unsigned red=0,alpha=0;p.sample(unsigned(u),unsigned(v),red,alpha);uint16_t pixel=color;
  if(mode==1)alpha=spark_smooth(red,erosion,26);
  else if(mode==2){alpha=spark_smooth(alpha,erosion,64);const unsigned brightness=red>=144?255u:89u+red*166u/144u;pixel=CombatCardMotion::modulate(color,brightness);}
  else alpha=red;
  c.paint(y*c.pitch+x,pixel,div255(alpha*opacity));
 }
}
static int spark_glow_scale(unsigned q){
 return q<208?CombatSparkMotion::segment(int(q),0,208,0,863,11819,0):CombatSparkMotion::segment(int(q),208,1024,863,1024,216,202);
}
static int spark_ring_scale(unsigned q){
 return q<153?CombatSparkMotion::segment(int(q),0,153,251,871,9579,188):CombatSparkMotion::segment(int(q),153,1024,871,1024,188,165);
}
static uint16_t spark_color(bool blocked,bool bright,unsigned variation){
 if(!bright)return blocked?rgb(94,205,255):rgb(255,157,128);
 const unsigned end=blocked?1869u:1610u;
 const unsigned r=(blocked?768u:1024u)+(end-(blocked?768u:1024u))*variation/255u;
 const unsigned g=(blocked?870u:768u)+(end-(blocked?870u:768u))*variation/255u;
 const unsigned b=(blocked?1024u:768u)+(end-(blocked?1024u:768u))*variation/255u;
 return rgb(mn(255,int(r*(blocked?42u:255u)/1024u)),mn(255,int(g*(blocked?169u:122u)/1024u)),mn(255,int(b*(blocked?255u:47u)/1024u)));
}
static void combat_hit_sparks(Canvas&c,uint32_t clock=UINT32_MAX){
 auto&a=c.a;auto&m=a.combat_sparks;const uint32_t now=clock==UINT32_MAX?a.animation*33u:clock;
 m.step(now);if(!m.count||a.content.version()<Ui7ContentVersion)return;
 bool live_hit=false;for(unsigned i=0;i<m.count;++i)if(m.bursts[i].kind==CombatSparkMotion::Hit&&int32_t(now-m.bursts[i].at)>=0)live_hit=true;
 if(!live_hit)return; // Orb/power bursts share metadata, not five unnecessary hit-texture reads.
 // One resource is used by every live burst before the next decode. The
 // sparse fallback indexes the original rows with512 bytes, preserving every
 // texel without allocating a second raster on the calculator's small stack.
 SparkTexture texture;
 static constexpr unsigned images[]={I7_GlowSpeck,I7_Speck,I7_Glow,I7_BlockRing,I7_HitStreak};
 for(unsigned system=0;system<5;++system){if(!texture.read(a.content,images[system]))continue;
  for(unsigned n=0;n<m.count;++n){const auto&b=m.bursts[n];if(b.kind!=CombatSparkMotion::Hit)continue;const int elapsed=int32_t(now-b.at);if(elapsed<0)continue;const unsigned age=unsigned(elapsed);
   if(system<2){for(unsigned i=system?6u:0u;i<(system?10u:6u);++i){const auto&p=b.particles[i];if(age>=p.lifetime)continue;
     const unsigned q=CombatSparkMotion::progress(age,p.lifetime),velocity=CombatSparkMotion::velocity(p,age),vq=mn(1024,int(velocity*102400u/67071u));
     const int scale=int(p.scale)*CombatSparkMotion::bounded(CombatSparkMotion::curve(int(q),1024,0,0,-2656))/1024;
     const int base=128*scale/6,width=base*CombatSparkMotion::curve(int(vq),1024,619,0,-1483)/1024,height=base*CombatSparkMotion::curve(int(vq),1022,3331,0,6180)/1024;
     const int radius=CombatSparkMotion::radius(b,p,age),cx=b.x*1024+radius*CombatCardMotion::sine(p.angle+1024)/1024,cy=b.y*1024+radius*CombatCardMotion::sine(p.angle)/1024;
     spark_quad(c,texture,cx,cy,width,height,int(p.angle)-1024,spark_color(b.blocked,system==0,p.variation),system?unsigned(128+p.variation/2)*507u/1024u:255u);
    }}
   else if(system==2&&age<350){const unsigned q=age*1024u/350u;const int size=128*5*spark_glow_scale(q)/6;
    spark_quad(c,texture,b.x*1024,b.y*1024,size,size,0,b.blocked?rgb(42,165,255):rgb(255,144,116),unsigned(CombatSparkMotion::bounded(CombatSparkMotion::curve(int(q),158,0)))*255u/1024u);
   }else if(system==3&&b.blocked&&age<450){const unsigned q=age*1024u/450u;const int size=256*2*spark_ring_scale(q)/6;
    spark_quad(c,texture,b.x*1024,b.y*1024,size,size,int(b.seed&4095u),rgb(89,194,255),126,1,q);
   }else if(system==4&&!b.blocked&&age<300){uint32_t rng=b.seed^0x337acfe1u;
    for(unsigned i=0;i<2;++i){const unsigned life=225+CombatSparkMotion::random(rng)%76u,angle=CombatSparkMotion::random(rng)&4095u,frame=CombatSparkMotion::random(rng)%4u;
     const unsigned scale=768+CombatSparkMotion::random(rng)%257u,initial=25+CombatSparkMotion::random(rng)%76u,speed=500+CombatSparkMotion::random(rng)%701u;
     if(age>=life)continue;const unsigned q=age*1024u/life;
     int radius=int(initial*1024u/6u);unsigned t=0;
     while(t<age){const unsigned dt=mn(17,int(age-t));t+=dt;const unsigned v=unsigned(CombatSparkMotion::bounded(CombatSparkMotion::curve(int(t*1024u/life),1024,0,-2298,0)))*speed/1024u;radius+=int(v*dt*1024u/6000u);}
     const int cx=b.x*1024+radius*CombatCardMotion::sine(int(angle)+1024)/1024,cy=b.y*1024+radius*CombatCardMotion::sine(int(angle))/1024,base=int(256u*scale/6u);
     spark_quad(c,texture,cx,cy,base*CombatSparkMotion::curve(int(q),514,1024,1591,0)/1024,base*CombatSparkMotion::curve(int(q),1024,824,-660,0)/1024,int(angle)-1024,rgb(255,164,89),255,2,q,frame);
    }
   }
  }
 }
}
}
