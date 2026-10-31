#include "../src/app.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks,frames;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"scene rasters:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static RandomGradientCache& gradient_cache(){
#ifdef SPIRE_TEST_ORIGINAL_RASTERS
 return random_gradient_cache;
#else
 return random_gradient_cache();
#endif
}
static void clear(){std::fill(std::begin(a.pixels),std::end(a.pixels),rgb(17,27,39));}
static std::vector<uint16_t> pixels(){return {std::begin(a.pixels),std::end(a.pixels)};}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
#ifndef SPIRE_TEST_ORIGINAL_RASTERS
static std::vector<uint8_t> lifecycle_pack(uint16_t color){
 constexpr unsigned images=A_Count,strings=40+images*16,clips=strings+ContentStrings*8;
 constexpr unsigned cards=clips+AnimationKinds*16,ui=cards+CardKinds*16,payload=ui+U_Count*4;
 std::vector<uint8_t>data(payload+1+ScreenW*ScreenH*2);
 const auto put=[&](unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)data[at+i]=uint8_t(value>>(8*i));};
 put(0,0x3250534e);put(4,ContentVersion);put(8,0x41cef1ea);put(12,unsigned(data.size()));put(16,images);put(24,ContentStrings);put(28,AnimationKinds);put(32,CardKinds);
 for(unsigned i=0;i<images;++i){put(40+i*16,payload+1);put(44+i*16,i?1:ScreenW,2);put(46+i*16,i?1:ScreenH,2);put(52+i*16,i?2:ScreenW*ScreenH*2);}
 for(unsigned i=0;i<ContentStrings;++i)put(strings+i*8,payload);
 for(unsigned i=0;i<AnimationKinds;++i){put(clips+i*16+4,1,2);put(clips+i*16+6,33,2);}
 for(unsigned i=0;i<ScreenW*ScreenH;++i)put(payload+1+i*2,color,2);
 return data;
}
static void reused_app_address(){
 alignas(App) static uint8_t storage[sizeof(App)];
 for(bool ranged:{false,true})for(unsigned pass=0;pass<2;++pass){
  const uint16_t color=pass?0xd361:0x27bd;auto data=lifecycle_pack(color);
  auto*instance=::new(detail::InPlaceAddress{storage}) App;
  if(ranged){ResourceSource source;source.context=&data;source.length=uint32_t(data.size());
   source.read_at=[](void*context,uint32_t at,void*target,uint32_t count){const auto&bytes=*static_cast<const std::vector<uint8_t>*>(context);
    if(at>bytes.size()||count>bytes.size()-at)return false;std::memcpy(target,bytes.data()+at,count);return true;};
   CHECK(instance->init(source,{}));
  }else CHECK(instance->init(data.data(),data.size(),{}));
  // A fresh object really reuses both its Content address and generation 1.
  // App initialization must invalidate the old static scene owner itself.
  CHECK(instance->content.generation()==1&&scene_raster_owner==SceneRasterOwner::None);
  auto&raster=character_selection_raster();CHECK(!raster.valid);
  CHECK(raster.restore(instance->content,0,0));for(auto pixel:raster.pixels)CHECK(pixel==color);
  instance->~App();
 }
}
#endif
static void synthetic(){
 static uint16_t gradient[64*64],noise_pixels[50],light_pixels[256*256];static uint8_t noise_alpha[50],dot_alpha[256*256];
 for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)gradient[y*64+x]=rgb(x*4,x*4,x*4);
 for(unsigned i=0;i<50;++i)noise_alpha[i]=uint8_t(i*47);
 for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){light_pixels[y*256+x]=rgb(x/2,y/2,0);dot_alpha[y*256+x]=uint8_t((x+y)%256);}
 const water_reflection::Texture g({reinterpret_cast<uint8_t*>(gradient),nullptr,nullptr,64,64,0});
 const water_reflection::Texture n({reinterpret_cast<uint8_t*>(noise_pixels),noise_alpha,nullptr,10,5,1});
 const water_reflection::Texture light({reinterpret_cast<uint8_t*>(light_pixels),nullptr,nullptr,256,256,0});
 const water_reflection::Texture dot({reinterpret_cast<uint8_t*>(light_pixels),dot_alpha,nullptr,256,256,1});
 Canvas c{a};new_run(a.game,0x52415354,false);CHECK(save(a.game,before));
 for(unsigned clock:{0u,7500u,16000u}){
  random_character_water(c,g,n,clock);const auto expected=pixels();
  random_character_water(c,g,n,clock,&gradient_cache());CHECK(gradient_cache().valid&&pixels()==expected);
  // An ineligible off-screen/covered canvas must not evict the live cache.
  Canvas faded=c;faded.opacity=128;CHECK(!merchant_cached_rug(faded));
#ifndef SPIRE_TEST_ORIGINAL_RASTERS
  CHECK(scene_raster_owner==SceneRasterOwner::Random);
#endif
  CHECK(!merchant_cached_rug(c)); // No resource pack: still switches the cache owner.
  random_character_water(c,g,n,clock,&gradient_cache());CHECK(gradient_cache().valid&&pixels()==expected);
  clear();random_character_particle(c,light,130*65536,100*65536,170*65536,clock%4096,102,171,113,0,presentation::Add,RandomTextureSource::Color);const auto full_light=pixels();
  clear();random_character_particle(c,light,130*65536,100*65536,170*65536,clock%4096,102,171,113,0,presentation::Add,RandomTextureSource::OpaqueColor);CHECK(pixels()==full_light);
  CHECK(!merchant_cached_rug(c));
  // A white particle uses no light lookup; drawing it cannot steal the rug.
  random_character_particle(c,dot,130*65536,100*65536,70*65536,clock%4096,110,128,179,69,presentation::Normal,RandomTextureSource::WhiteAlpha);
#ifndef SPIRE_TEST_ORIGINAL_RASTERS
  CHECK(scene_raster_owner==SceneRasterOwner::Merchant);
#endif
  clear();random_character_particle(c,light,130*65536,100*65536,170*65536,clock%4096,102,171,113,0,presentation::Add,RandomTextureSource::OpaqueColor);CHECK(pixels()==full_light);
  unchanged();
 }
}
static std::vector<uint8_t> file(const char*path){FILE*f=std::fopen(path,"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>40&&!std::fseek(f,0,SEEK_SET));std::vector<uint8_t> data(size_t(size),0);CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));return data;}
static void record(const char*directory,const char*scene,unsigned pass,unsigned clock){
 uint64_t hash=1469598103934665603ull;for(auto p:a.pixels){hash=(hash^uint8_t(p))*1099511628211ull;hash=(hash^uint8_t(p>>8))*1099511628211ull;}
 std::printf("frame %u %s %u %u %016llx\n",pass,scene,a.content.language(),clock,static_cast<unsigned long long>(hash));++frames;
 if(directory){char path[768];std::snprintf(path,sizeof path,"%s/%u-%s-%u.rgb565",directory,pass,scene,clock);FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fwrite(a.pixels,1,sizeof a.pixels,f)==sizeof a.pixels);CHECK(!std::fclose(f));}
}
static void actual(const char*english,const char*chinese,const char*captures){
 const auto eng=file(english),zhs=file(chinese);
 for(unsigned pass=0;pass<3;++pass){const auto&data=pass==1?zhs:eng;CHECK(a.init(data.data(),data.size(),{}));
  new_run(a.game,73,false);a.game.map[0].room=Room::Shop;CHECK(choose_node(a.game,0));
  a.title=a.library=a.paused=a.help=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.progress.tutorial_flags=0;a.reset_presentation();a.pointer.visible=false;
  a.map_clock_ms=1400;a.shop_open=true;a.room_presentation.merchant.set_open(true,400);a.merchant_preparation.planned=true;
  CHECK(save(a.game,before));Canvas c{a};
  for(unsigned clock:{1400u,7500u,16000u}){
   a.map_clock_ms=clock;clear();CHECK(random_character_background(c,clock,clock,91));const auto random=pixels();record(captures,"random",pass,clock);unchanged();
   clear();shop_screen(c,false);const auto shop=pixels();
   clear();shop_screen(c,true);CHECK(pixels()==shop);record(captures,"shop",pass,clock);unchanged();
   clear();CHECK(random_character_background(c,clock,clock,91));CHECK(pixels()==random);unchanged();
   clear();shop_screen(c,true);CHECK(pixels()==shop);unchanged();
  }
 }
}
int main(int argc,char**argv){synthetic();
#ifndef SPIRE_TEST_ORIGINAL_RASTERS
 reused_app_address();
#endif
 CHECK(argc==1||argc==3||argc==4);if(argc>1)actual(argv[1],argv[2],argc>3?argv[3]:nullptr);std::printf("scene rasters: %u checks, %u actual frames\n",checks,frames);}
