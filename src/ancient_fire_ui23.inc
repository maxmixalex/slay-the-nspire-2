namespace ancient_fire {
using namespace ancient_fire_ui23;
static void node(Canvas&c,const Table&table,unsigned index,unsigned frame,const Shader*shaders){
 const auto*r=table.node(index);const int cx=table.coordinate(index,0,frame),cy=table.coordinate(index,1,frame);
 const int ax=table.coordinate(index,2,frame),ay=table.coordinate(index,3,frame),bx=table.coordinate(index,4,frame),by=table.coordinate(index,5,frame);
 if(CombatCardMotion::magnitude(cx)>=1048576||CombatCardMotion::magnitude(cy)>=1048576||CombatCardMotion::magnitude(ax)>=16384||CombatCardMotion::magnitude(ay)>=16384||CombatCardMotion::magnitude(bx)>=16384||CombatCardMotion::magnitude(by)>=16384)return;
 const int64_t determinant=int64_t(ax)*by-int64_t(bx)*ay;if(determinant>-65536&&determinant<65536)return;
 const int hw=(CombatCardMotion::magnitude(ax)+CombatCardMotion::magnitude(bx)+1)/2;
 const int hh=(CombatCardMotion::magnitude(ay)+CombatCardMotion::magnitude(by)+1)/2;
 const int left=mx(0,(cx-hw)/256-1),right=mn(c.clip_w,(cx+hw)/256+2);
 const int top=mx(0,(cy-hh)/256-1),bottom=mn(c.clip_h,(cy+hh)/256+2);if(left>=right||top>=bottom)return;
 // Q24 derivatives avoid accumulating a Q16 rounding error across a small
 // transformed flame. All four divisions occur once per source node.
 const int du=int(int64_t(by)*4294967296LL/determinant),dv=int(-int64_t(ay)*4294967296LL/determinant);
 const int uy=int(-int64_t(bx)*4294967296LL/determinant),vy=int(int64_t(ax)*4294967296LL/determinant);
 const int dx=left*256+128-cx,dy=top*256+128-cy;
 const int64_t firstu=8388608+(int64_t(dx)*by-int64_t(dy)*bx)*16777216/determinant;
 const int64_t firstv=8388608+(-int64_t(dx)*ay+int64_t(dy)*ax)*16777216/determinant;
 // Reject malformed ill-conditioned quads before the signed32 inner loop.
 // Checking its four corners also bounds every affine increment, including
 // the one-past row/column positions computed by the loop statements.
 for(unsigned corner=0;corner<4;++corner){const int64_t u=firstu+int64_t(corner&1?right-left:0)*du+int64_t(corner&2?bottom-top:0)*uy;
  const int64_t v=firstv+int64_t(corner&1?right-left:0)*dv+int64_t(corner&2?bottom-top:0)*vy;
  if(u<-2147483647||u>2147483647||v<-2147483647||v>2147483647)return;
 }
 int startu=int(firstu),startv=int(firstv);
 const auto&shader=shaders[r[36]];const unsigned blend=r[37]?presentation::Add:0;
 for(int y=top;y<bottom;++y,startu+=uy,startv+=vy){int u=startu,v=startv;
  for(int x=left;x<right;++x,u+=du,v+=dv){if(unsigned(u)>=16777216||unsigned(v)>=16777216)continue;
   const auto pixel=shader.sample(u>>4,v>>4);const unsigned alpha=(pixel.alpha*255+524288)>>20;if(!alpha)continue;
   unsigned color[3];for(unsigned channel=0;channel<3;++channel){
    const unsigned value=word(r+24+channel*2)+unsigned(uint64_t(pixel.inner)*word(r+30+channel*2)>>20);
    color[channel]=mn(255u,(value*255+32768)>>16);
   }
   c.paint(y*c.pitch+x,rgb(int(color[0]),int(color[1]),int(color[2])),alpha,blend);
  }
 }
}
static bool draw(Canvas&c,uint32_t elapsed,uint32_t seed){
 if(c.a.content.ui_image(Auxiliary)!=int(ParameterImage))return false;
 static constexpr unsigned dimensions[][3]={{1024,1024,1},{500,500,3},{500,500,1},{50,500,1},{20,500,1}};
 Field fields[5];unsigned page=0;
 for(unsigned f=0;f<5;++f){auto&field=fields[f];field.width=dimensions[f][0];field.height=dimensions[f][1];field.channels=dimensions[f][2];
  const unsigned bytes=field.width*field.height*field.channels;
  for(unsigned offset=0;offset<bytes;offset+=PageBytes){const auto image=c.a.content.image(int(FirstImage+page++));const unsigned used=mn(PageBytes,bytes-offset);
   if(!image.pixels||image.format||image.w!=320||image.h!=(used+639)/640)return false;
   field.pages[offset>>17]=image.pixels;
  }
 }
 const auto parameters=c.a.content.image(ParameterImage);
 const int ids[]={int(tez_cinders_ui23::FirstImage+1),int(tez_cinders_ui23::FirstImage),c.a.content.ui_image(I13_RandomLight),c.a.content.ui_image(ancient_lights_ui23::AuxiliaryFirst+1)};
 const auto cinder_parameters=c.a.content.image(ids[0]),sprite=c.a.content.image(ids[1]);
 const auto light=c.a.content.image(ids[2]),light_parameters=c.a.content.image(ids[3]);
 // No Content call occurs in this borrowed window. All byte pages and typed
 // parameters must still be resident after the last request; no larger pool.
 for(unsigned i=FirstImage;i<Images;++i)if(!c.a.content.image_cached(int(i)))return false;
 for(int id:ids)if(!c.a.content.image_cached(id))return false;
 Table table;tez_cinders_ui23::Table cinder_table;ancient_lights_ui23::Table light_table;
 const water_reflection::Texture texture(sprite,24,24),light_texture(light,256,256);
 if(!table.read(parameters)||!cinder_table.read(cinder_parameters)||!light_table.read(light_parameters,1)||!texture.valid()||!light_texture.valid())return false;
 const bool reduced=c.a.content.reduced_animations();const uint32_t clock=reduced?0:elapsed,shader_clock=reduced?0:c.a.map_clock_ms;
 Model models[3];Shader shaders[3];for(unsigned i=0;i<3;++i){models[i].read(table.data+ModelsAt+i*128);shaders[i].bind(fields,models[i],shader_clock);}
 auto&state=tez_cinder_room(c.a).state;
 state.advance(clock,seed,cinder_table.source_curve(0),[&](){c.poll();},[&](unsigned emitter,int32_t born){return cinder_table.inherit(emitter,born);});
 const unsigned frame=table.frame(clock);
 for(unsigned i=0;i<Commands;++i){const unsigned command=table.command(i),kind=command>>14,index=command&16383;
  if(kind==0){if(!ancient_lights::nodes(c,light_texture,light_table,1,clock,seed,index,1))return false;}
  else if(kind==1){for(unsigned j=0;j<24;++j)tez_cinders::particle(c,texture,state.particles[index*24+j],cinder_table,index,frame);}
  else node(c,table,index,frame,shaders);
  if(!(i&31u))c.poll();
 }
 return true;
}
}
