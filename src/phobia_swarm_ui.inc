static void phobia_swarm_body(Canvas&c,unsigned model,int x,int feet,int scale,uint32_t clock,uint32_t seed){
 auto&a=c.a;if(model!=unsigned(EnemyId::Entomancer)||!a.phobia_mode||!a.content.has_phobia_swarm_assets()||scale<=0||!c.opacity)return;
 const auto particle=a.content.image(a.content.ui_image(I12_PhobiaSwarm));if(!particle.pixels)return;
 // The original child transforms compose to(0,-175);0.297513 is the
 // alternate Sprite2D's scale.3906 recovers Entomancer's common bake units.
 // Apply the same creature transform as its body, including Bestiary zoom.
 const int center_y=feet*256-175*256*scale/3906;
 for(unsigned slot=0;slot<phobia_swarm::Count;++slot){
  const auto p=phobia_swarm::sample(seed,slot,clock);
  const int size=(15*p.scale*305/4096)*scale/3906;if(size<=0)continue;
  const int px=x*256+(p.x*305/1024)*scale/3906,py=center_y+(p.y*305/1024)*scale/3906;
  Canvas dot=c;if(size<256)dot.opacity=uint8_t(unsigned(dot.opacity)*unsigned(size*size)/65536u);
  const int extent=mx(1,(size+128)/256);
  // A1px particle must sample the source center, not its transparent corner.
  ui_picture_transform(dot,particle,(px+128)/256,(py+128)/256,extent,extent);
 }
}
