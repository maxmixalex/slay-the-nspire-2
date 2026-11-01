// Exercise the private production quad against its unchanged general path.
#include "../src/app.cpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace spire;
static App app;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"light lookup:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 std::vector<uint16_t> colors;for(unsigned g=0;g<64;++g)for(unsigned r=0;r<32;++r)
  if(int(r)*2-int(g)>=-3&&int(r)*2-int(g)<=1)colors.push_back(uint16_t((r<<11)|(g<<5)|r));
 bool used[256]{};for(auto color:colors){unsigned key=((color>>3)&252)|(color&3);CHECK(!used[key]);used[key]=true;}
 // Logical256² texture is stored in a320-wide payload; padding is excluded.
 std::vector<uint8_t> raw(320*205*2+1),alpha(320*205,0);
 for(unsigned i=0;i<65536;++i){auto color=colors[(i*137+i/256)%colors.size()];raw[1+i*2]=uint8_t(color);raw[2+i*2]=uint8_t(color>>8);alpha[i]=255;}
 Picture p{raw.data()+1,alpha.data(),nullptr,320,205,1};water_reflection::Texture texture(p,256,256);
 RandomLightQuadCache cache;CHECK(cache.bind(app.content,texture));CHECK(cache.native_lookup);
 std::vector<uint16_t> first(320*240),second(first.size());
 struct Pose{int x,y,w,h,angle,shear;};const Pose poses[]={{40960,30720,65536,65536,0,0},{-1000,20000,40000,18000,273,-12738},{50000,40000,90000,80000,731,22000},{40960,30720,128,256,0,0},{20000,20000,30000,40000,-271,-14000}};
 for(auto pose:poses)for(unsigned opacity:{0u,1u,91u,255u})for(unsigned mod:{0u,127u,255u}){
  for(unsigned i=0;i<first.size();++i)first[i]=second[i]=uint16_t(i*4793+0x187f);
  Canvas a{app,first.data()},b{app,second.data()};a.brightness=b.brightness=mod;
  fake_merchant::quad(a,texture,pose.x,pose.y,pose.w,pose.h,pose.angle,234,181,91,opacity,presentation::Add,pose.shear);
  fake_merchant::quad(b,texture,pose.x,pose.y,pose.w,pose.h,pose.angle,234,181,91,opacity,presentation::Add,pose.shear,&cache);
  CHECK(first==second);
 }
 // Reject every proof premise independently; never infer it from dimensions.
 const uint8_t saved=alpha[65535];alpha[65535]=254;cache.observed=false;CHECK(cache.bind(app.content,texture));CHECK(!cache.native_lookup);alpha[65535]=saved;
 const auto lo=raw[1],hi=raw[2];raw[1]=1;raw[2]=0;cache.observed=false;CHECK(cache.bind(app.content,texture));CHECK(!cache.native_lookup);
 raw[1]=0xe0;raw[2]=7;cache.observed=false;CHECK(cache.bind(app.content,texture));CHECK(!cache.native_lookup);raw[1]=lo;raw[2]=hi;
 cache.observed=false;CHECK(cache.bind(app.content,texture));CHECK(cache.native_lookup);
 // The Character owner's background/effect lifetimes survive the new view.
 auto&r=character_selection_raster();r.pixels[17]=0x5a71;r.index=37;r.valid=true;
 auto&room=scene_raster_storage.character;::new(detail::InPlaceAddress{&room.effects.cinders}) TezCinderRoom;
 room.effect_owner=CharacterEffectOwner::Cinders;room.effects.cinders.state.valid=true;
 auto&live=character_light_quads();CHECK(live.bind(app.content,texture));CHECK(live.native_lookup);
 CHECK(scene_raster_owner==SceneRasterOwner::Character&&r.pixels[17]==0x5a71&&r.index==37&&r.valid);
 CHECK(room.effect_owner==CharacterEffectOwner::Cinders&&room.effects.cinders.state.valid);
 random_scene_rasters();auto&fresh=character_light_quads();CHECK(!fresh.observed&&!fresh.valid);
 CHECK(sizeof(SceneRasterStorage)==sizeof(MerchantRugRaster));
 std::printf("Ancient light lookup: %u checks, %zu exact colors, union %zu bytes, Character %zu bytes.\n",checks,colors.size(),sizeof(SceneRasterStorage),sizeof(CharacterSceneRasters));
}
