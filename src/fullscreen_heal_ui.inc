// PlayerFullscreenHealVfx: source textures, emission count, curves and timing.
// A deterministic cosmetic hash never consumes a gameplay random stream.
static unsigned fullscreen_heal_curve(const uint16_t*values,unsigned age,unsigned lifetime){
 if(age>=lifetime)return values[32];const unsigned q=age*32u,i=q/lifetime,f=q%lifetime;
 return (unsigned(values[i])*(lifetime-f)+unsigned(values[i+1])*f+lifetime/2u)/lifetime;
}
static uint32_t fullscreen_heal_color(unsigned age){
 static constexpr uint16_t time[]={0,134,228,311,384,482,607,853,1000};
 static constexpr uint32_t color[]={0x27987a,0xe2ed30,0xfff6bb,0xfcf495,0xf3ee31,0xc5ea2f,0xc9eb3c,0x2da478,0x167781};
 unsigned i=1;while(i<8&&age>time[i])++i;const unsigned q=mn(age-unsigned(time[i-1]),unsigned(time[i]-time[i-1])),span=time[i]-time[i-1];
 const auto channel=[&](unsigned shift){return (((color[i-1]>>shift)&255u)*(span-q)+((color[i]>>shift)&255u)*q)/span;};
 return channel(16)<<16|channel(8)<<8|channel(0);
}
static constexpr uint16_t heal_scale[]={0,638,1207,1712,2156,2542,2875,3158,3395,3590,3747,3870,3961,4026,4067,4089,4096,4089,4064,4019,3949,3853,3725,3563,3364,3123,2838,2505,2120,1681,1183,624,0};
static constexpr uint16_t heal_alpha[]={0,570,1199,1848,2480,3057,3541,3894,4078,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4048,3927,3738,3490,3190,2846,2466,2056};
static void fullscreen_heal_screen(Canvas&c){
 auto&a=c.a;const auto&v=a.reward_presentation.heal;if(!v.alive(a.map_clock_ms))return;
 const unsigned elapsed=a.map_clock_ms-v.start;
 if(elapsed<1100){
  const unsigned target=mx(102u,unsigned(v.ratio)*204u/4096u);
  const unsigned tail=elapsed>100?elapsed-100u:0u,q=tail*4096u/1000u;
  const unsigned alpha=elapsed<100?target*elapsed/100u:target*(4096u-(q*q/4096u)*q/4096u)/4096u;
  room_vfx_stamp(c,I15_HealVignette,0,0,320,240,alpha,0x7fff00,presentation::Normal,false);
  Canvas reflected=c;reflected.pixels+=(reflected.clip_h-1)*reflected.pitch;reflected.pitch=-reflected.pitch;
  const unsigned highlight=elapsed<100?84u*elapsed/100u:84u*(1000u-tail)/1000u;
  room_vfx_stamp(reflected,I15_HealVignette,0,0,320,240,div255(alpha*highlight),0x00ff00,presentation::Add,false);
 }
 const unsigned count=10u+(unsigned(v.ratio)*40u+2048u)/4096u;
 for(unsigned i=0;i<count;++i){
  // A non-explosive one-shot emitter distributes births across one lifetime.
  // Its1.2 speed scales emission and motion together, so the last particle
  // dies at1.667s and its final half-second subemitter ends near2.1s.
  const unsigned born=i*1000u/count;if(elapsed*6u/5u<born)continue;
  const unsigned age=elapsed*6u/5u-born;
  const uint32_t hash=room_vfx_hash(v.start+i*0x9e3779b9u);
  const int x=int(hash%320u),base_y=int((hash>>9)%240u);
  const auto py=[&](unsigned time){return base_y-int(time*time)/210000;};
  for(unsigned trail=0;trail<4;++trail){const unsigned emitted=trail*250u;if(age<emitted||age-emitted>=600u||emitted>=1000u)continue;
   const unsigned t=(age-emitted)*5u/6u,scale=fullscreen_heal_curve(heal_scale,t,500);
   const unsigned gradient=t<67u?t*255u/67u:t>426u?(500u-t)*255u/74u:255u;
   const int size=mx(1,int(12u*scale/4096u));
   room_vfx_stamp(c,I15_HealFalloff,x-size/2,py(emitted)-size/2,size,size,gradient*124u/255u,fullscreen_heal_color(t*2u),presentation::Normal,false);
  }
  if(age>=1000u)continue;
  const unsigned scale=fullscreen_heal_curve(heal_scale,age,1000u);
  const unsigned gradient=age<134u?age*255u/134u:age>853u?(1000u-age)*255u/147u:255u;
  const unsigned alpha=fullscreen_heal_curve(heal_alpha,age,1000u)*gradient/4096u;
  const int size=mx(1,int((12u+(hash>>22)%10u)*scale/4096u));
  room_vfx_stamp(c,I15_HealCross,x-size/2,py(age)-size/2,size,size,alpha,fullscreen_heal_color(age),presentation::Normal,false);
 }
}
