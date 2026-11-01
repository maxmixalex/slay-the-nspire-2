namespace neow_fire {
using namespace neow_fire_ui23;
struct Contribution {uint16_t green,blue;};
static bool build(Canvas&c,const Table&table,const ancient_fire_ui23::Field*fields,Illumination&light){
 // Build one actual source pose. Cache only illumination: the full-resolution
 // animated transmission is composed afterwards on every displayed frame.
 ForegroundScratch<Contribution,MaskBytes> temporary;if(!temporary)return false;
 auto*sum=temporary.array();__builtin_memset(sum,0,MaskBytes*sizeof(Contribution));
 Shader shaders[2];for(unsigned i=0;i<2;++i)shaders[i].bind(fields,table.data+ModelsAt+i*128,0);
 for(unsigned node=0;node<4;++node){const auto*r=table.node(node);
  const int u0=ancient_fire_ui23::integer(r),v0=ancient_fire_ui23::integer(r+4);
  const int du=ancient_fire_ui23::integer(r+8),dv=ancient_fire_ui23::integer(r+12);
  const int uy=ancient_fire_ui23::integer(r+16),vy=ancient_fire_ui23::integer(r+20);
  const unsigned green=word(r+24),blue=word(r+26);const auto&shader=shaders[r[28]];
  for(unsigned y=0;y<Height;++y){if(!(y&15u))c.poll();unsigned pixel=table.row_pixel(y);
   for(unsigned span=table.row_span(y);span<table.row_span(y+1);++span){
    const unsigned x=table.x(span),count=table.count(span);
    int u=u0+int(y)*uy+int(x)*du,v=v0+int(y)*vy+int(x)*dv;
    for(unsigned i=0;i<count;++i,++pixel,u+=du,v+=dv){
     if(!table.active(node,pixel)||unsigned(u)>=268435456u||unsigned(v)>=268435456u)continue;
     const unsigned alpha=(shader.alpha(u>>8,v>>8)*255u+524288u)>>20;
     sum[pixel].green=uint16_t(sum[pixel].green+green*alpha);
     sum[pixel].blue=uint16_t(sum[pixel].blue+blue*alpha);
    }
   }
  }
 }
 for(unsigned i=0;i<MaskBytes;++i)light.pixels[i]=uint16_t(
  div255(unsigned(sum[i].green))<<8|div255(unsigned(sum[i].blue)));
 light.owner=&c.a.content;light.generation=c.a.content.generation();light.valid=true;
 return true;
}
static void apply(Canvas&c,const Table&table,const Mask&mask,const Illumination&light){
 constexpr unsigned denominator=255u*255u;
 for(unsigned y=0;y<Height;++y){if(!(y&15u))c.poll();unsigned pixel=table.row_pixel(y);
  for(unsigned span=table.row_span(y);span<table.row_span(y+1);++span){
   const unsigned start=table.x(span),count=table.count(span);
   for(unsigned i=0;i<count;++i,++pixel){const unsigned transmission=mask.pixels[pixel];if(!transmission)continue;
    const unsigned radiance=light.pixels[pixel];
    const unsigned green=((radiance>>8)*transmission*63u+denominator/2)/denominator;
    const unsigned blue=((radiance&255u)*transmission*31u+denominator/2)/denominator;
    const unsigned at=y*c.pitch+start+i,old=c.pixels[at];
    const unsigned g=((old>>5)&63u)+green,b=(old&31u)+blue;
    c.pixels[at]=uint16_t((old&0xf800u)|((g>63u?63u:g)<<5)|(b>31u?31u:b));
   }
  }
 }
}

static bool draw(Canvas&c,uint32_t elapsed){
 if(!scene_opaque_canvas(c)||c.a.content.ui_image(Auxiliary)!=int(ParameterImage))return false;
 const bool reduced=c.a.content.reduced_animations();const auto generation=c.a.content.generation();
 auto&room=neow_room();auto&mask=room.transmission;auto&light=room.illumination;
 if(!mask.restore(c.a.content,FirstMask,reduced?0:(elapsed%Duration)/33,c.a.callbacks.foreground_poll))return false;
 Table table;if(!table.read(c.a.content.image(ParameterImage),reduced))return false;
 if(!light.matches(&c.a.content,generation)){
 if(table.baked){
  const auto p=c.a.content.image(FirstImage);
  if(!p.pixels||p.format||p.alpha||p.palette||p.w!=256||p.h!=(MaskBytes*2+511)/512)return false;
  for(unsigned i=0;i<MaskBytes;++i){if(!(i&2047u))c.poll();light.pixels[i]=uint16_t(word(p.pixels+2*i));}
  if(!table.read(c.a.content.image(ParameterImage),reduced)||!table.baked||c.a.content.generation()!=generation)return false;
  light.owner=&c.a.content;light.generation=generation;light.valid=true;
 }else{
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
 if(!table.read(c.a.content.image(ParameterImage),reduced))return false;
 for(unsigned id=FirstImage;id<FirstMask;++id)if(!c.a.content.image_cached(int(id)))return false;
 for(unsigned id=ancient_fire_ui23::FirstImage+2;id<ancient_fire_ui23::FirstImage+4;++id)if(!c.a.content.image_cached(int(id)))return false;
 if(!c.a.content.image_cached(ParameterImage)||!c.a.content.healthy()||c.a.content.generation()!=generation)return false;
 if(!build(c,table,fields,light))return false;
 }
 }
 apply(c,table,mask,light);return true;
}
}
