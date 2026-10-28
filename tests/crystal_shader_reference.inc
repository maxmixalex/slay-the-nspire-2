// Frozen UI8 stage-four software-renderer oracle. Keep its original runtime
// coordinate arithmetic independent of the candidate constant lookup tables.
static void crystal_sphere_reference(Canvas&c,bool fog){
 auto&a=c.a;const auto&motion=a.crystal_presentation;const uint32_t now=a.map_clock_ms;
 const int x=fog?crystal_layout::FogX:crystal_layout::SphereX,y=fog?crystal_layout::FogY:crystal_layout::SphereY;
 const int w=fog?crystal_layout::FogW:crystal_layout::SphereW,h=fog?crystal_layout::FogH:crystal_layout::SphereH;
 const auto polar=a.content.image(a.content.ui_image(I8_CrystalPolar));
 const auto n1=a.content.image(a.content.ui_image(I8_CrystalNoiseOne)),n2=a.content.image(a.content.ui_image(I8_CrystalNoiseTwo));
 const auto inside=a.content.image(a.content.ui_image(fog?I8_CrystalInner:I8_CrystalOuterInside));
 const auto outside=fog?inside:a.content.image(a.content.ui_image(I8_CrystalOuterOutside));
 if(!polar.pixels||!n1.pixels||!n2.pixels||!inside.pixels||!outside.pixels)return;
 if(polar.format>3||n1.format>3||n2.format>3||inside.format>3||outside.format>3||polar.w!=160||polar.h!=160||n1.w!=256||n1.h!=240||n2.w!=256||n2.h!=240||inside.w!=256||inside.h!=240||outside.w!=256||outside.h!=240)return;
 const unsigned phase1=unsigned(uint64_t(now)*425984/1000000),phase2=unsigned(uint64_t(now)*720896/10000000);
 uint8_t alpha[CrystalCells];for(unsigned i=0;i<CrystalCells;++i)alpha[i]=uint8_t(fog?motion.cell_alpha(i,now):255);
 int8_t gx[160],gy[160];uint8_t sx[160],sy[160],lx[160],rx[160],ty[160],by[160];
 const auto coordinates=[](int size,int8_t*grid,uint8_t*sample,uint8_t*low,uint8_t*high){
  for(int i=0;i<size;++i){
   sample[i]=uint8_t((2*i+1)*160/(2*size));
   const int value=(2*i+1)*1419-348*size,denom=200*size;
   const int cell=value>=0?value/denom:-((-value+denom-1)/denom),fraction=(value-cell*denom)*1000/denom;
   grid[i]=int8_t(cell);low[i]=uint8_t(fraction>=50?0:mn(255,(50-fraction)*255/30));
   high[i]=uint8_t(fraction<=950?0:mn(255,(fraction-950)*255/30));
  }
 };
 coordinates(w,gx,sx,lx,rx);coordinates(h,gy,sy,ty,by);
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
  const int cx=gx[xx],cy=gy[yy];const bool valid=crystal_layout::exists(cx,cy);
  if(fog&&!valid)continue;
  const unsigned packed=crystal_word(polar,unsigned(sy[yy])*160+sx[xx]),radius=packed>>8,angle=packed&255;
  const unsigned one=crystal_noise(n1,radius,(angle+(phase1>>8))&255,phase1&255);
  const unsigned two=crystal_noise(n2,radius,(angle+(phase2>>8))&255,phase2&255);
  const unsigned at=radius*256+((one*two)>>24);const Picture&table=valid?inside:outside;
  unsigned opacity=table.alpha?table.alpha[at]:255;
  uint16_t color=crystal_word(table,at);
  if(fog){
   const unsigned cell=unsigned(cy*11+cx),coverage=alpha[cell];unsigned border=0;
   const auto edge=[&](int nx,int ny,unsigned weight){if(nx<0||nx>=11||ny<0||ny>=11||!weight)return;
    const int difference=int(coverage)-int(alpha[ny*11+nx]);border=unsigned(mx(int(border),int(div255(unsigned(difference<0?-difference:difference)*weight))));};
   edge(cx-1,cy,lx[xx]);edge(cx+1,cy,rx[xx]);edge(cx,cy-1,ty[yy]);edge(cx,cy+1,by[yy]);
   opacity=div255(opacity*unsigned(mx(coverage,border)));if(!opacity)continue;
   color=uint16_t((mn(31,(color>>11)+int(border*28/255))<<11)|(mn(63,((color>>5)&63)+int(border*56/255))<<5)|mn(31,(color&31)+int(border*31/255)));
  }
  c.paint((y+yy)*c.pitch+x+xx,color,opacity);
 }
}
