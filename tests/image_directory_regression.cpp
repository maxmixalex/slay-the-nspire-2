#include "app.hpp"
#include "resource_transfer.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"image directory:%d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Bytes=std::vector<uint8_t>;
static void put(Bytes&b,unsigned at,uint32_t value,unsigned n=4){CHECK(at+n<=b.size());for(unsigned i=0;i<n;++i)b[at+i]=uint8_t(value>>(8*i));}
static uint16_t sample(unsigned id){return uint16_t((id*4793u+0x185fu)^(id*17u));}
static void records(){
 uint8_t before[18],after[14];uint32_t random=0x68913b2u;
 for(unsigned i=0;i<100000;++i){
  std::fill(std::begin(before),std::end(before),0x19);std::fill(std::begin(after),std::end(after),0x71);
  random=random*1664525u+1013904223u;const uint32_t offset=random;
  random=random*1664525u+1013904223u;const uint32_t shape=random;
  random=random*1664525u+1013904223u;const uint32_t length=random&0x1fffffffu,format=i%8;
  // Deliberately odd source/destination addresses also cover byte-safe ARM
  // access; decoded image pointers do not have a host-alignment guarantee.
  ImageDirectoryView::word(before+1,offset);ImageDirectoryView::word(before+5,shape);
  ImageDirectoryView::word(before+9,format);ImageDirectoryView::word(before+13,length);
  ImageDirectoryView::compact(after+1,before+1);
  const auto a=ImageDirectoryView{before+1,16}[0],b=ImageDirectoryView{after+1,12}[0];
  CHECK(a.offset==b.offset&&a.shape==b.shape&&a.format==b.format&&a.length==b.length);
  CHECK(a.width()==b.width()&&a.height()==b.height());CHECK(after[0]==0x71&&after[13]==0x71);
 }
 for(uint32_t format:{8u,255u,0xffffffffu}){
  ImageDirectoryView::word(before+9,format);ImageDirectoryView::compact(after+1,before+1);
  CHECK((ImageDirectoryView{after+1,12}[0].length>ResourceTransferCapacity));
 }
 ImageDirectoryView::word(before+9,0);ImageDirectoryView::word(before+13,0x20000000u);
 ImageDirectoryView::compact(after+1,before+1);CHECK((ImageDirectoryView{after+1,12}[0].length>ResourceTransferCapacity));
}
static Bytes fixture(){
 constexpr unsigned images=8192,strings=66,clips=11;
 constexpr unsigned string_at=40+images*16,clip_at=string_at+strings*8,payload=clip_at+clips*16;
 Bytes b(payload+images*2+1);put(b,0,0x3250534eu);put(b,4,3);put(b,8,0x41cef1eau);put(b,12,unsigned(b.size()));
 put(b,16,images);put(b,24,strings);put(b,28,clips);
 for(unsigned i=0;i<images;++i){
  put(b,40+i*16,payload+(i==images-1?0:i)*2);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);
  put(b,payload+i*2,sample(i),2);
 }
 for(unsigned i=0;i<strings;++i)put(b,string_at+i*8,payload+images*2);
 for(unsigned i=0;i<clips;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
 return b;
}
struct Source {
 const Bytes* bytes;unsigned fail_at=~0u,large_reads=0;
 static bool read(void*context,uint32_t at,void*out,uint32_t count){
  auto&s=*static_cast<Source*>(context);if(at>=s.fail_at||at>s.bytes->size()||count>s.bytes->size()-at)return false;
  if(count==65536)++s.large_reads;
  std::memcpy(out,s.bytes->data()+at,count);return true;
 }
 ResourceSource view(){return {this,uint32_t(bytes->size()),read,nullptr};}
};
static void content(){
 const auto bytes=fixture(),unchanged=bytes;
 auto direct=std::make_unique<Content>(),streamed=std::make_unique<Content>();Source source{&bytes};
 CHECK(direct->open(bytes.data(),bytes.size()));CHECK(streamed->open(source.view()));
 CHECK(source.large_reads==2);CHECK(!foreground_scratch_top&&!resource_transfer_uses);
 for(unsigned id=0;id<8192;++id){
  CHECK(streamed->image_dimensions(int(id)).w==1&&streamed->image_dimensions(int(id)).h==1);
  const auto a=direct->image(int(id)),b=streamed->image(int(id));
  CHECK(a.pixels&&b.pixels&&a.format==b.format&&b.w==1&&b.h==1);
  CHECK(a.pixels[0]==b.pixels[0]&&a.pixels[1]==b.pixels[1]);
  CHECK(uint16_t(b.pixels[0]|unsigned(b.pixels[1])<<8)==sample(id==8191?0:id));
 }
 CHECK(direct->image_alias(0,8191)&&streamed->image_alias(0,8191));
 CHECK(!streamed->image_alias(0,1));CHECK(bytes==unchanged);CHECK(streamed->healthy()&&direct->healthy());
 CHECK(streamed->clip(10).count==1&&streamed->clip(10).frame_ms==33);CHECK(!*streamed->string(65));
 // A failed later chunk publishes no partial resident view; reopening after
 // fixing the provider restores both images and the moved non-image tables.
 source.fail_at=65536;CHECK(!streamed->open(source.view()));CHECK(!streamed->healthy());
 CHECK(!foreground_scratch_top&&!resource_transfer_uses);
 source.fail_at=~0u;CHECK(streamed->open(source.view()));CHECK(streamed->image(4093).pixels);
 CHECK(!foreground_scratch_top&&!resource_transfer_uses);
 // Pre-existing foreground scratch can coexist within the fixed160KiB arena.
 {ForegroundScratch<uint8_t,32768> outer;CHECK(bool(outer));std::fill_n(outer.array(),32768,0xa5);
  CHECK(streamed->open(source.view()));for(unsigned i=0;i<32768;++i)CHECK(outer[i]==0xa5);}
 CHECK(!foreground_scratch_top&&!resource_transfer_uses);
}
int main(){records();content();std::printf("resident image directory: %u checks; descriptor bounds, odd addresses, 64 KiB carries, complete legacy provider images, immutable input, reopen and shared scratch\n",checks);}
