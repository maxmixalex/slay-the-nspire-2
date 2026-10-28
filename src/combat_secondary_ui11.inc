// CPU mappings for the remaining source card/orb particle systems. All
// bursts use the existing fixed hit pool; trails retain six actual poses.
// Their authored triggers and lifetimes survive, while GPU turbulence and
// large particle counts are adapted to the 320x240 software framebuffer.
static uint16_t combat_orb_color(unsigned kind){
 switch(DOrb(kind)){case DOrb::Dark:return rgb(193,100,255);case DOrb::Lightning:return rgb(255,234,100);
 case DOrb::Frost:return rgb(151,222,255);case DOrb::Plasma:return rgb(255,184,71);case DOrb::Glass:return rgb(106,255,204);default:return White;}
}
static uint16_t combat_trail_color(Character character){
 switch(character){case Character::Ironclad:return rgb(255,66,51);case Character::Silent:return rgb(0,177,33);
 case Character::Defect:return rgb(0,154,201);case Character::Regent:return rgb(233,100,0);case Character::Necrobinder:return rgb(239,85,99);}return White;
}
static unsigned combat_exhaust_progress(unsigned age){
 if(age>=400)return 1024;const int q=int(age*1024u/400u);
 // vfx_card_exhaust's four authored curve points/tangents.
 const int value=q<256?CombatSparkMotion::segment(q,0,256,0,149,1247,0):q<610?
  CombatSparkMotion::segment(q,256,610,149,513,0,3130):CombatSparkMotion::segment(q,610,1024,513,1024,3130,0);
 return unsigned(CombatSparkMotion::bounded(value));
}
static void combat_secondary_sparks(Canvas&c){
 auto&a=c.a;auto&m=a.combat_sparks;const uint32_t now=a.animation*33u;m.step(now);
 bool active=false;for(unsigned n=0;n<m.count;++n)if(m.bursts[n].kind!=CombatSparkMotion::Hit&&int32_t(now-m.bursts[n].at)>=0)active=true;
 if(!active||a.content.version()<Ui7ContentVersion)return;
 SparkTexture texture;if(!texture.read(a.content,I7_GlowSpeck))return;
 for(unsigned n=0;n<m.count;++n){const auto&b=m.bursts[n];const int elapsed=int32_t(now-b.at);
  if(elapsed<0||b.kind==CombatSparkMotion::Hit)continue;const unsigned age=unsigned(elapsed);
  const uint16_t color=b.kind==CombatSparkMotion::PowerUp?rgb(255,213,48):combat_orb_color(b.variant);
  if(b.kind==CombatSparkMotion::OrbBeam){
   if(age>=300)continue;
   // NOrbVfx receives the exact creature positions from EvokeActivated.
   // Lightning branches use short jagged segments; other orbs carry a
   // handful of motes along the same target vector.
   const int dx=b.target_x-b.x,dy=b.target_y-b.y;
   if(b.variant==unsigned(DOrb::Lightning)){Canvas ray=c;ray.opacity=uint8_t((300-age)*230/300);int x=b.x,y=b.y;
    for(int i=1;i<=7;++i){const int jitter=i==7?0:int((b.seed+uint32_t(i*73)+(age/50u)*29u)%9u)-4;
     const int xx=b.x+dx*i/7,yy=b.y+dy*i/7+jitter;ray.line(x,y,xx,yy,color);x=xx;y=yy;}
   }else for(unsigned i=0;i<5;++i){const int q=int(age*1024u/300u)-int(i)*90;if(q<0)continue;
    spark_quad(c,texture,(b.x*1024+dx*q),(b.y*1024+dy*q),5*1024,5*1024,0,color,(300-age)*190u/300u);}
   continue;
  }
  const bool power=b.kind==CombatSparkMotion::PowerUp,evoke=b.kind==CombatSparkMotion::OrbEvoke;
  const unsigned life=power?1000u:evoke?650u:400u;if(age>=life)continue;
  const unsigned alpha=power?(age<100?age*2u:age>900?(1000-age)*2u:200u):(life-age)*230u/life;
  const unsigned particles=power?6u:evoke?8u:4u;
  for(unsigned i=0;i<particles;++i){const unsigned phase=(b.seed+i*1597u)&4095u;
   const int radius=power?9+int((b.seed+i*7u)%12u):3+int(age*(evoke?20u:9u)/life);
   const int x=b.x*1024+radius*CombatCardMotion::sine(int(phase)+1024);
   const int y=power?b.y*1024+14*1024-int(((age+i*83u)%life)*38u*1024u/life):b.y*1024+radius*CombatCardMotion::sine(int(phase));
   spark_quad(c,texture,x,y,(power?4:3)*1024,(power?11:3)*1024,power?0:int(phase),power&&(i&1u)?rgb(68,199,255):color,alpha);
  }
 }
}
static void combat_card_secondary(Canvas&c){
 auto&a=c.a;if(!ui_cards_bound(a)||a.content.version()<Ui7ContentVersion)return;
 bool active=false;for(const auto&v:a.combat_cards.sprites)if(v.mode!=CombatCardMotion::Empty&&
  (v.trail_count||v.mode==CombatCardMotion::Exhaust||(v.generated&&(a.animation-v.at)*33u<450u)))active=true;
 if(!active)return;
 SparkTexture texture;if(!texture.read(a.content,I7_GlowSpeck))return;
 const uint16_t trail=combat_trail_color(a.game.character);
 for(const auto&v:a.combat_cards.sprites){if(v.mode==CombatCardMotion::Empty)continue;
  const unsigned age=(a.animation-v.at)*33u;
  if(v.trail_count&&(v.mode==CombatCardMotion::Fly||v.mode==CombatCardMotion::Power||v.mode==CombatCardMotion::Shuffle||v.mode==CombatCardMotion::Tail)){
   const unsigned fade=v.mode==CombatCardMotion::Tail?(age>=500u?0u:(500-age)*255u/500u):255u;
   for(unsigned i=0;i<v.trail_count;++i){const int size=8-int(i);const unsigned alpha=fade*(180u-i*23u)/255u;
    spark_quad(c,texture,v.trail[i].x*1024,v.trail[i].y*1024,size*1024,size*1024,0,trail,alpha);
   }
  }
  if(v.mode==CombatCardMotion::Exhaust&&age<1000u){
   const unsigned progress=combat_exhaust_progress(age);const auto p=CombatCardMotion::pose(v);
   for(unsigned i=0;i<8;++i){const unsigned born=i*37u;if(age<born||age-born>=600u)continue;const unsigned particle_age=age-born;
    const int dx=(int((i*41u+v.id*7u)%73u)-36)*p.scale/1024;
    const int y=p.cy-(HandFaceH/2)*p.scale/1024+int(progress)*HandFaceH*p.scale/(1024*1024)-int(particle_age*25u/600u);
    spark_quad(c,texture,(p.cx+dx)*1024,y*1024,3*1024,6*1024,0,rgb(230,99,130),(600u-particle_age)*210u/600u);
   }
  }else if(v.generated&&v.mode==CombatCardMotion::Hand&&age<450u){
   const auto p=CombatCardMotion::pose(v);for(unsigned i=0;i<4;++i){const int phase=int(i)*1024+512,radius=12+int(age*22u/450u);
    spark_quad(c,texture,p.cx*1024+radius*CombatCardMotion::sine(phase+1024),p.cy*1024+radius*CombatCardMotion::sine(phase),4*1024,4*1024,phase,trail,(450u-age)*190u/450u);
   }
  }
 }
}
