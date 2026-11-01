namespace neow_particles {
using namespace neow_particles_ui23;
static bool draw(Canvas&c,uint32_t elapsed,uint32_t seed){
 using namespace fake_merchant;
 const int root=c.a.content.ui_image(Auxiliary);if(root!=int(FirstImage))return false;
 // Compute the small per-node array before loading another image; all borrowed
 // source-curve pointers expire before the cloud mask request can evict them.
 static constexpr int origin[][2]={{1088*256,1302*256},{36*256,1311*256},{1088*256,1302*256}};
 for(unsigned node=0;node<3;++node){Parameters parameters;if(!parameters.read(c.a.content.image(root)))return false;
  Fog particles[5];for(unsigned i=0;i<5;++i)particles[i]=fog(parameters,elapsed,seed,node,i);
  for(unsigned i=0;i<5;++i){const auto&p=particles[i];if(!p.gray)continue;
   const auto image=c.a.content.image(AtlasFirst+p.atlas);
   if(!image.pixels||image.w!=100||image.h!=200||image.format||image.alpha||image.palette)return false;
   const int sn=CombatCardMotion::sine(p.angle),cs=CombatCardMotion::sine(p.angle+1024),side=ancient_size(200)*p.scale/Q;
   nonu_fog::plane(c,image.pixels,ancient_x(origin[node][0]+p.x),ancient_y(origin[node][1]),
       side*cs/1024,side*sn/1024,-side*sn/1024,side*cs/1024,p.gray,255);
  }
 }
 Parameters parameters;if(!parameters.read(c.a.content.image(root)))return false;
 const auto*alpha=parameters.data+3422;
 const water_reflection::Texture texture(Picture{alpha,alpha,parameters.data+2910,32,32,3});
 for(unsigned i=0;i<10;++i){const auto p=star(elapsed,seed^0x4e535452u,i);
  unsigned color[4];for(unsigned channel=0;channel<4;++channel)color[channel]=(parameters.star_color(channel,p.phase)*parameters.data[52+channel]+32767u)/65535u;
  const int size=ancient_size(32)*p.scale/Q;
  quad(c,texture,ancient_x(p.x),ancient_y(p.y),size,size,0,color[0],color[1],color[2],color[3],presentation::Add);
 }
 return true;
}
}
