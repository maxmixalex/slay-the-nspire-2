// Exercise the actual cache implementation and the public lazy image accessor.
#include "../src/content.cpp"
#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"fire noise:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<uint8_t> read(const char*path){std::ifstream in(path,std::ios::binary);CHECK(bool(in));return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char**argv){
 using namespace ancient_fire_noise;
 static_assert(Count==8&&First+Count==59740,"Reviewed half-noise endpoint");
 static Content owner;std::vector<uint8_t> metadata((First+Count)*16);ImageDirectoryView directory{metadata.data(),16};
 auto put=[&](unsigned at,unsigned n){for(unsigned b=0;b<4;++b)metadata[at+b]=uint8_t(n>>(b*8));};
 for(unsigned id=0;id<First+Count;++id){put(id*16,1234);put(id*16+4,205u<<16|320);put(id*16+12,131200);}
 CHECK(image_identity(0,1,false)&&image_identity(0,1,true));
 for(bool reduced:{false,true}){CHECK(!image_identity(First,First+1,reduced));CHECK(!image_identity(0,First,reduced));CHECK(image_identity(First,First,reduced));}
 std::array<ImageCache*,Count> entries{};
 for(unsigned i=0;i<Count;++i){ImageReservation r(131200);CHECK(r.bytes());r.publish(&owner,directory,First+i,0);entries[i]=find_image(&owner,directory,First+i);CHECK(entries[i]);
  CHECK(!(entries[i]->packed_format&Reconstructed));entries[i]->packed_format|=Reconstructed;
  for(unsigned j=0;j<i;++j)CHECK(entries[i]!=entries[j]);}
 for(unsigned i=0;i<Count;++i){CHECK(entries[i]->format()==0&&entries[i]->packed_format&Reconstructed);CHECK(find_image(&owner,directory,First+i)==entries[i]);}
 for(unsigned link=0;link<2048;++link){ImageCache e{};e.packed_format=uint8_t(Reconstructed|3);e.link(link);CHECK(e.next()==link&&e.format()==3&&(e.packed_format&Reconstructed));}
 discard_image(*entries[Count-1]);CHECK(!find_image(&owner,directory,First+Count-1));CHECK(!entries[Count-1]->packed_format);
 {ImageReservation r(131200);r.publish(&owner,directory,First+Count-1,0);CHECK(!(find_image(&owner,directory,First+Count-1)->packed_format&Reconstructed));}
 clear_caches(&owner);for(const auto&e:image_cache)CHECK(!e.owner&&!e.packed_format);
 double cold=0;
 for(unsigned id=First;id<First+Count;++id){const auto p=layout(id);CHECK(p.used&&p.stride&&p.stride<p.used);
  std::vector<uint8_t> original((p.used+639)/640*640),encoded(original.size());
  for(unsigned i=0;i<p.used;++i)original[i]=uint8_t(i*73+(i/p.stride)*19);encoded=original;
  for(unsigned i=p.stride;i<p.used;++i)encoded[i]=uint8_t(original[i]-original[i-p.stride]);
  const auto start=std::chrono::steady_clock::now();CHECK(restore(encoded.data(),encoded.size(),id,1));
  cold+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();CHECK(encoded==original);
  CHECK(restore(encoded.data(),encoded.size(),id,0)&&encoded==original);
  CHECK(!restore(encoded.data(),encoded.size(),id,2)&&encoded==original);
  encoded.back()=7;CHECK(!restore(encoded.data(),encoded.size(),id,1));
 }
 CHECK(!layout(First-1).used&&!layout(First+Count).used);
 CHECK(sizeof(ImageCache)==(sizeof(void*)==4?16:24));CHECK(sizeof picture_pixels==13u*1024u*1024u-256u*1024u);
 if(argc==3){auto data=read(argv[1]);CHECK(owner.open(data.data(),data.size()));
  // Preloaded pages carry the stored residual, and are not falsely considered
  // reconstructed. image() must restore even after foreground preparation.
  auto state=PreparationResult::Pending;for(unsigned n=0;n<100000&&state==PreparationResult::Pending;++n)state=owner.preload_image_step(First);
  CHECK(state==PreparationResult::Ready);const ImageCache*before=nullptr;for(const auto&e:image_cache)if(e.owner==&owner&&e.id()==First)before=&e;CHECK(before&&!(before->packed_format&Reconstructed));
  for(unsigned id=First;id<First+Count;++id){char name[4096];std::snprintf(name,sizeof name,"%s/%02u.bin",argv[2],id-First);auto expected=read(name);
   const Picture picture=owner.image(int(id));CHECK(picture.pixels&&picture.format==0);CHECK(unsigned(picture.w)*picture.h*2==expected.size());
   CHECK(std::equal(expected.begin(),expected.end(),picture.pixels));const auto reads=owner.read_stats();const auto*ptr=picture.pixels;
   for(unsigned repeat=0;repeat<100;++repeat){const auto warm=owner.image(int(id));CHECK(warm.pixels==ptr&&warm.format==0);}
   CHECK(std::equal(expected.begin(),expected.end(),ptr));CHECK(owner.read_stats().callbacks==reads.callbacks&&owner.read_stats().callback_bytes==reads.callback_bytes);
  }
  for(auto&e:image_cache)if(e.owner==&owner&&e.id()==First)discard_image(e);
  {const auto restored=owner.image(First);char name[4096];std::snprintf(name,sizeof name,"%s/00.bin",argv[2]);auto expected=read(name);CHECK(restored.pixels&&std::equal(expected.begin(),expected.end(),restored.pixels));}
  const auto generation=owner.generation();CHECK(owner.open(data.data(),data.size()));CHECK(owner.generation()!=generation);
  for(const auto&e:image_cache)CHECK(!e.owner&&!e.packed_format);
  const auto first=owner.image(First);char name[4096];std::snprintf(name,sizeof name,"%s/00.bin",argv[2]);auto expected=read(name);CHECK(first.pixels&&std::equal(expected.begin(),expected.end(),first.pixels));
 }
 std::printf("fire noise: %u checks;%u cold page restores %.3f ms host; unchanged cache bytes\n",checks,Count,cold);
}
