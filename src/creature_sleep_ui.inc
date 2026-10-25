static void creature_sleep_particles(Canvas&c,int slot){
 auto&a=c.a;const auto&s=a.game;if(!s.enemy_world.active||slot<0||slot>=s.enemy_count)return;
 const auto&m=a.enemy_presentation[slot];const auto&sleep=m.sleep;const uint32_t now=a.animation*33u;if(!sleep.alive(now))return;
 const bool beetle=m.model==unsigned(EnemyId::SlumberingBeetle);if(!beetle&&m.model!=unsigned(EnemyId::LagavulinMatriarch))return;
 const auto camera=encounter_camera(s.enemy_world.encounter);const auto layout=a.creature_layout.right?a.creature_layout:combat_layout_for(s);
 const auto root=enemy_formation_point(s,slot,camera.zoom);
 // SleepVfxPos is a child of the .3-scaled Visuals. NCreature resets the
 // Visuals node's position, so its editor preview translation is excluded.
 const int anchor_x=beetle?-211:-172,anchor_y=beetle?-125:-149;
 const auto px=[&](int x){return combat_project_x(camera,layout,root.x+(anchor_x+x)*camera.zoom,true);};
 const auto py=[&](int y){return combat_project_y(camera,layout,root.y+(anchor_y+y)*camera.zoom);};
 const unsigned age=now-sleep.start;const uint32_t seed=s.seed^m.identity*0x9e3779b9u;
 if(age<350u){const int side=mx(5,combat_project_extent(camera,layout,220));room_vfx_stamp(c,I7_Glow,px(0)-side/2,py(0)-side/2,side,side,(350u-age)*120u/350u,0xe075ff);}
 for(unsigned i=0;i<4;++i){const auto z=sleep.sample(i,now,seed);if(!z.visible)continue;
  const int size=mx(4,combat_project_extent(camera,layout,128*z.size*3/1000)),x=px(z.x*3/10),y=py(z.y*3/10);
  Canvas letter=c;letter.opacity=uint8_t(div255(letter.opacity*z.alpha));
  // The original localized texture has Japanese and English variants;
  // existing UI40 packs keep a readable original-font fallback until the
  // optional tiny masks have been installed alongside the startup catalog.
  const int asset=a.content.has_sleeping_vfx_assets()?a.content.ui_image(a.content.language()==9?I12_SleepingZJapanese:I12_SleepingZEnglish):-1;
  if(asset>=0){const auto picture=a.content.image(asset);Canvas outline=letter;outline.tint_red=112;outline.tint_green=0;outline.tint_blue=97;
   ui_picture_transform(outline,picture,x-1,y,size,size);ui_picture_transform(outline,picture,x+1,y,size,size);
   ui_picture_transform(outline,picture,x,y-1,size,size);ui_picture_transform(outline,picture,x,y+1,size,size);
   letter.tint_red=255;letter.tint_green=169;letter.tint_blue=250;ui_picture_transform(letter,picture,x,y,size,size);
  }
  else letter.text_fitted("Z",x-size/2,y-size/2,size,mx(5,size),rgb(255,169,250),5,true,true,rgb(112,0,97));
 }
 // Three continuous little stars belong to the same source emitter. An
 // awake actor keeps only stars emitted before Stop, then sheds the tail.
 for(unsigned i=0;i<3;++i){const uint32_t delay=i*333u;if(age<delay)continue;const uint32_t t=(age-delay)%1000u,born=age-t;
  if(!sleep.active&&born>uint32_t(sleep.stopped-sleep.start))continue;
  const uint32_t hash=room_vfx_hash(seed+i*0x74d13ab1u+born);const int x=px((int(hash%181u)-90)*3/10),y=py((int((hash>>8)%181u)-90)*3/10);
  const unsigned alpha=t<400?255u:(1000u-t)*255u/600u;room_vfx_stamp(c,I7_Speck,x-1,y-1,2,2,alpha,0xfdbeff);
 }
}
