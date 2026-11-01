namespace neow_fire {
using namespace neow_fire_ui23;
struct Contribution {uint16_t green,blue;};
struct RenderWorkspace {
 Contribution sum[MaskBytes];
 uint16_t grid[161*121];
};
static_assert(sizeof(RenderWorkspace)<=ForegroundScratchCapacity,"Fire grid reuses the existing foreground scratch capacity");
struct Grid {
 uint16_t*values;const Shader&shader;
 int u0,v0,du,dv,uy,vy;
 unsigned at(unsigned x,unsigned y){
  auto&value=values[(y>>1)*161+(x>>1)];if(value!=65535u)return unsigned(value)<<8;
  const int u=u0+int(y)*uy+int(x)*du,v=v0+int(y)*vy+int(x)*dv;
  value=uint16_t(unsigned(u)<268435456u&&unsigned(v)<268435456u?(shader.alpha(u>>8,v>>8)+128u)>>8:0);return unsigned(value)<<8;
 }
 unsigned sample(unsigned x,unsigned y,int u,int v){
  const unsigned margin_u=CombatCardMotion::magnitude(du)+CombatCardMotion::magnitude(uy);
  const unsigned margin_v=CombatCardMotion::magnitude(dv)+CombatCardMotion::magnitude(vy);
  if(unsigned(u)<margin_u||unsigned(u)>268435455u-margin_u||
     unsigned(v)<margin_v||unsigned(v)>268435455u-margin_v)return shader.alpha(u>>8,v>>8);
  const unsigned xx=x&~1u,yy=y&~1u,a=at(xx,yy);
  const unsigned b=x&1u?at(xx+2,yy):a;
  const unsigned c=y&1u?at(xx,yy+2):a;
  const unsigned d=(x&1u)&&(y&1u)?at(xx+2,yy+2):(x&1u?b:c);
  const unsigned low=mn(mn(a,b),mn(c,d)),high=mx(mx(a,b),mx(c,d));
  // Keep sharp transitions fully sampled. Only the separate soft fire light
  // is interpolated; the source body, full-size foreground mask and clocks
  // are unchanged. Grid values keep Q12 alpha, below one final alpha8 step.
  if(((x|y)&1u)&&high-low>32768u)return shader.alpha(u>>8,v>>8);
  return (a+b+c+d+2u)>>2;
 }
};

static bool render(Canvas&c,const Table&table,const Mask&mask,const ancient_fire_ui23::Field*fields,uint32_t clock){
 // Compact support, fixed Q28 projection and the source alpha equation share
 // the original fields. No body image is rescaled or drawn a second time.
 ForegroundScratch<RenderWorkspace> temporary;if(!temporary)return false;
 auto&sum=temporary[0].sum;__builtin_memset(sum,0,sizeof sum);
 Shader shaders[2];for(unsigned i=0;i<2;++i)shaders[i].bind(fields,table.data+ModelsAt+i*128,clock);
 for(unsigned node=0;node<4;++node){const auto*r=table.node(node);
  const int u0=ancient_fire_ui23::integer(r),v0=ancient_fire_ui23::integer(r+4);
  const int du=ancient_fire_ui23::integer(r+8),dv=ancient_fire_ui23::integer(r+12);
  const int uy=ancient_fire_ui23::integer(r+16),vy=ancient_fire_ui23::integer(r+20);
  const unsigned green=word(r+24),blue=word(r+26);const auto&shader=shaders[r[28]];
  const bool spatial=shader.lcd_sampling&&(shader.inverse&0x80000000u);
  Grid grid{temporary[0].grid,shader,u0,v0,du,dv,uy,vy};
  if(spatial)__builtin_memset(grid.values,255,sizeof temporary[0].grid);
  for(unsigned y=0;y<Height;++y){if(!(y&15u))c.poll();unsigned pixel=table.row_pixel(y);
   for(unsigned span=table.row_span(y);span<table.row_span(y+1);++span){
    const unsigned x=table.x(span),count=table.count(span);
    int u=u0+int(y)*uy+int(x)*du,v=v0+int(y)*vy+int(x)*dv;
    for(unsigned i=0;i<count;++i,++pixel,u+=du,v+=dv){
     if(!mask.pixels[pixel]||!table.active(node,pixel)||unsigned(u)>=268435456u||unsigned(v)>=268435456u)continue;
     const unsigned sampled=spatial?grid.sample(x+i,y,u,v):shader.alpha(u>>8,v>>8);
     const unsigned alpha=(sampled*255u+524288u)>>20;
     sum[pixel].green=uint16_t(sum[pixel].green+green*alpha);
     sum[pixel].blue=uint16_t(sum[pixel].blue+blue*alpha);
    }
   }
  }
 }
 // All-time source bounds prove saturation can occur only where the complete
 // source foreground is opaque. Its affine transmission therefore recovers
 // the correct behind-body placement without retaining a second body raster.
 constexpr unsigned denominator=255u*255u*255u;
 for(unsigned y=0;y<Height;++y){if(!(y&15u))c.poll();unsigned pixel=table.row_pixel(y);
  for(unsigned span=table.row_span(y);span<table.row_span(y+1);++span){
   const unsigned start=table.x(span),count=table.count(span);
   for(unsigned i=0;i<count;++i,++pixel){const unsigned transmission=mask.pixels[pixel];if(!transmission)continue;
    const unsigned green=(unsigned(sum[pixel].green)*transmission*63u+denominator/2)/denominator;
    const unsigned blue=(unsigned(sum[pixel].blue)*transmission*31u+denominator/2)/denominator;
    const unsigned at=y*c.pitch+start+i,old=c.pixels[at];
    const unsigned g=((old>>5)&63u)+green,b=(old&31u)+blue;
    c.pixels[at]=uint16_t((old&0xf800u)|((g>63u?63u:g)<<5)|(b>31u?31u:b));
   }
  }
 }
 return true;
}

static bool draw(Canvas&c,uint32_t elapsed){
 if(!scene_opaque_canvas(c)||c.a.content.ui_image(Auxiliary)!=int(ParameterImage))return false;
 const bool reduced=c.a.content.reduced_animations();const auto generation=c.a.content.generation();
 auto&mask=neow_fire_mask(c.a);
 if(!mask.restore(c.a.content,FirstMask,reduced?0:(elapsed%Duration)/33,c.a.callbacks.foreground_poll))return false;
 ancient_fire_ui23::Field fields[4];
 // Source water, shared reviewed triangle, original light RGB and original
 // distortion R/G. Only their actual channels are retained, at source size.
 const unsigned dimensions[][3]={{600,636,1},{250,250,3},{256,256,3},{100,100,2}};
 const unsigned first[]={FirstImage,ancient_fire_ui23::FirstImage+2,FirstImage+3,FirstImage+5};
 for(unsigned field=0;field<4;++field){auto&f=fields[field];f.width=dimensions[field][0];f.height=dimensions[field][1];f.channels=dimensions[field][2];
  const unsigned bytes=f.width*f.height*f.channels;
  for(unsigned offset=0;offset<bytes;offset+=131072){const unsigned used=mn(131072u,bytes-offset);
   const auto image=c.a.content.image(int(first[field]+offset/131072));
   if(!image.pixels||image.format||image.alpha||image.palette||image.w!=320||image.h!=(used+639)/640)return false;
   f.pages[offset/131072]=image.pixels;
  }
 }
 Table table;if(!table.read(c.a.content.image(ParameterImage),reduced))return false;
 for(unsigned id=FirstImage;id<FirstMask;++id)if(!c.a.content.image_cached(int(id)))return false;
 for(unsigned id=ancient_fire_ui23::FirstImage+2;id<ancient_fire_ui23::FirstImage+4;++id)if(!c.a.content.image_cached(int(id)))return false;
 if(!c.a.content.image_cached(ParameterImage)||!c.a.content.healthy()||c.a.content.generation()!=generation)return false;
 return render(c,table,mask,fields,reduced?0:c.a.map_clock_ms);
}
}
