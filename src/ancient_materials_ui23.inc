namespace ancient_materials {
using namespace ancient_materials_ui23;
static bool table(Canvas&c,unsigned scene,Table&value){
 if(c.a.content.ui_image(AuxiliaryFirst+scene)!=int(FirstImage+20+scene))return false;
 return value.read(c.a.content.image(FirstImage+20+scene),scene);
}
static void water(Canvas&c,unsigned scene,uint32_t shader_clock){
 if(scene<1||scene>2)return;
 if(c.a.content.reduced_animations()){
  const auto p=c.a.content.material_image(FirstImage+(scene==1?4:13));
  if(p.pixels)c.picture(p,0,0,320,240);return;
 }
 Water view;
 // All11possiblepageviews plus parameters fit in the unchanged image pool.
 // Once pointers are borrowed, no content/image request is made in this draw.
 for(unsigned i=0;i<(scene==1?9u:5u);++i){const auto p=c.a.content.material_image(FirstImage+(scene==1?4:13)+i);
  if(!p.pixels||p.w!=320||p.h!=240||p.format||p.alpha||p.palette)return;view.pages[i]=p.pixels;}
 if(scene==2)for(unsigned i=0;i<2;++i){const auto p=c.a.content.image(FirstImage+18+i);
  if(!p.pixels||p.w!=125||p.h!=125||p.format||p.alpha||p.palette)return;view.noise[i]=p.pixels;}
 Table t;if(!table(c,scene,t))return;if(scene==1)view.noise[0]=t.data+512;
 view.parameters(t,shader_clock);
 for(int y=0;y<c.clip_h;++y){const int v=view.uv0[1]+y*view.step[1];if(unsigned(v)>=16777216u)continue;
  int u=view.uv0[0];for(int x=0;x<c.clip_w;++x,u+=view.step[0]){
   if(unsigned(u)<16777216u)c.paint(y*c.pitch+x,view.sample(u,v,unsigned(x),unsigned(y)),255,presentation::Normal);
  }}
}
static void lights(Canvas&c,unsigned scene,uint32_t elapsed){
 if(scene>1)return;Table t;if(!table(c,scene,t))return;
 const unsigned opacity=t.opacity(elapsed),count=t.lights;
 // Copy only six bounded words per fixed node before another image lookup.
 // No borrowed table pointer is retained across atlas image requests.
 uint16_t rows[4][7]{};for(unsigned i=0;i<count;++i)for(unsigned j=0;j<7;++j)rows[i][j]=uint16_t(word(t.data+32+i*14+j*2));
 for(unsigned i=0;i<count;++i){const auto*r=rows[i];const unsigned h=r[3];
  for(unsigned part=0;part<2&&part*112<h;++part){
   const auto p=c.a.content.image(FirstImage+r[5]+part);
   if(!p.pixels||p.w!=256||p.h!=112||p.format!=1||!p.alpha||p.palette)return;
   const unsigned height=mn(112u,h-part*112);
   c.picture_region(p,r[0],r[1]+part*112,r[6],0,r[2],height,false,opacity,r[4]?presentation::Add:presentation::Normal);
  }}
}
}
