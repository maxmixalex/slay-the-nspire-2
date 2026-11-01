// Original Ancient camera/bone projections sampled every33ms. Only Neow's
// one intervening vignette is deferred; ordinary scene frame IDs stay stable.
namespace ancient_lights {
using namespace ancient_lights_ui23;
static bool layer(Canvas&c,unsigned scene,unsigned layer,uint32_t elapsed,uint32_t seed){
 const water_reflection::Texture texture(c.a.content.image(c.a.content.ui_image(I13_RandomLight)),256,256);
 const auto image=c.a.content.image(c.a.content.ui_image(AuxiliaryFirst+scene));
 Table table;if(!texture.valid()||!table.read(image,scene))return false;
 const bool reduced=c.a.content.reduced_animations();const uint32_t clock=reduced?0:elapsed;
 const unsigned frame=table.frame(clock,reduced);
 static constexpr fake_merchant::Key original[]={fake_merchant::key(0,.321019),fake_merchant::key(.502521,.507939),fake_merchant::key(1,.36242)};
 static constexpr auto scale=fake_merchant::light_curve(original);
 static constexpr fake_merchant::ColorKey ramp[]={fake_merchant::color_key(0,.34902,.34902,.34902),
  fake_merchant::color_key(.411546,1,1,1),fake_merchant::color_key(.994413,.34902,.34902,.34902)};
 for(unsigned i=0;i<table.nodes;++i){const auto*r=table.node(i);if(r[30]!=layer)continue;
  const int x=table.coordinate(r,0,frame),y=table.coordinate(r,1,frame);
  const int w=table.coordinate(r,2,frame),h=table.coordinate(r,3,frame);
  const int angle=table.coordinate(r,4,frame),shear=table.coordinate(r,5,frame);
  const unsigned preprocess=fake_merchant::hash(seed+i)%6;
  for(unsigned particle=0;particle<r[28];++particle){
   // The amount1 node uses phase0; amount2 uses the original half-cycle.
   const unsigned age=fake_merchant::preprocessed_light_age(clock,particle,preprocess);
   if(!age)continue;const unsigned q=age*fake_merchant::Q/3000;
   const int amount=fake_merchant::curve(r[31]?fake_merchant::CandleLightScale:scale,q);
   const int width=int(int64_t(w)*amount/fake_merchant::Q),height=int(int64_t(h)*amount/fake_merchant::Q);
   const int64_t skew=int64_t(CombatCardMotion::magnitude(shear))*height;
   if(width<=0||height<=0||int64_t(width)+height+(skew+65535)/65536>=2097152)return false;
   const unsigned shade=fake_merchant::gradient(ramp,q).r;
   fake_merchant::quad(c,texture,x,y,width,height,angle,
    div255(r[24]*shade),div255(r[25]*shade),div255(r[26]*shade),r[27],presentation::Add,shear);
  }
 }
 return true;
}
static bool draw(Canvas&c,unsigned scene,uint32_t elapsed){
 if(c.a.content.ui_image(AuxiliaryFirst+scene)!=int(FirstImage+1+scene))return false;
 const uint32_t seed=fake_merchant::hash(uint32_t(c.a.game.seed)^unsigned(c.a.game.floor)*0x9e3779b9u^
  c.a.ancient_dialogue.entered^(scene?0x54455a43u:0x4e454f57u));
 if(!scene){
  if(!layer(c,scene,0,elapsed,seed))return false;
  const auto vignette=c.a.content.image(FirstImage);
  if(!vignette.pixels||vignette.w!=320||vignette.h!=240||vignette.format!=3||!vignette.alpha||!vignette.palette)return false;
  c.picture(vignette,0,0,320,240);
 }
 // Reacquire texture/table views after the vignette request. No image load
 // occurs inside either borrowed-pointer drawing window.
 return layer(c,scene,1,elapsed,seed);
}
}
