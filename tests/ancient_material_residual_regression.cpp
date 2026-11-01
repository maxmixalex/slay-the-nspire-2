// Compile the exact cache implementation; dead sections remove unrelated APIs.
#include "../src/content.cpp"
#include <array>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"material residual:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 using namespace ancient_material_residual;
 static Content owner_object;const auto*owner=&owner_object;
 std::vector<uint8_t> metadata((First+Count+2)*16);
 const ImageDirectoryView directory{metadata.data(),16};
 auto put=[&](unsigned at,unsigned n){for(unsigned b=0;b<4;++b)metadata[at+b]=uint8_t(n>>(b*8));};
 for(unsigned id=0;id<First+Count+2;++id){put(id*16,1234);put(id*16+4,240u<<16|320);put(id*16+12,153600);}
 CHECK(same_identity(0,1)&&!same_identity(First,First+1)&&!same_identity(0,First)&&same_identity(First,First));
 // Identical descriptors across every typed leaf must still allocate distinct
 // entries: zero residuals predict different coordinates, despite equal bytes.
 std::array<ImageCache*,Count> entries{};
 for(unsigned i=0;i<Count;++i){ImageReservation r(153600);CHECK(r.bytes());r.publish(owner,directory,First+i,0);entries[i]=find_image(owner,directory,First+i);CHECK(entries[i]);
  entries[i]->packed_format|=Reconstructed;for(unsigned j=0;j<i;++j)CHECK(entries[i]!=entries[j]);}
 for(unsigned i=0;i<Count;++i){CHECK(entries[i]->format()==0);CHECK(entries[i]->packed_format&Reconstructed);CHECK(find_image(owner,directory,First+i)==entries[i]);}
 // Both chain bits below and above1023 survive the reconstruction bit.
 for(unsigned link=0;link<2048;++link){ImageCache a{};a.packed_format=uint8_t(Reconstructed|3);a.link(link);CHECK(a.next()==link&&a.format()==3&&(a.packed_format&Reconstructed));}
 {ImageReservation a(153600);a.publish(owner,directory,0,0);CHECK(find_image(owner,directory,1)==find_image(owner,directory,0));}
 for(unsigned i:{0u,8u,Count-1}){discard_image(*entries[i]);CHECK(!find_image(owner,directory,First+i));CHECK(!entries[i]->packed_format);}
 for(unsigned i=0;i<Count;++i)if(i!=0&&i!=8&&i!=Count-1)CHECK(find_image(owner,directory,First+i)==entries[i]);
 clear_caches(owner);for(const auto&e:image_cache)CHECK(!e.owner&&!e.packed_format);for(unsigned b:image_buckets)CHECK(!b);
 // Reusing a discarded entry publishes no stale marker, including owner reuse.
 {ImageReservation a(153600);a.publish(owner,directory,First,0);CHECK(!(find_image(owner,directory,First)->packed_format&Reconstructed));}
 clear_caches(owner);
 std::vector<uint8_t> base(320*240*2),residual(320*240*2),expected(residual.size());
 for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){const unsigned c=((x%32)<<11)|((y%64)<<5)|((x+y)%32);const unsigned at=(y*320+x)*2;base[at]=uint8_t(c);base[at+1]=uint8_t(c>>8);}
 const water_reflection::Texture texture({base.data(),nullptr,nullptr,320,240,0});
 Mapping mapping{-5*65536,-2*65536,21845,21845};CHECK(mapping.valid());
 // Independent reference uses double weights with the specified8-bit fraction
 // and separate per-channel rounding; values span negative/clamped borders.
 auto scalar=[&](unsigned x,unsigned y){const int64_t xx=mapping.x+int64_t(x)*mapping.dx,yy=mapping.y+int64_t(y)*mapping.dy;
  const int l=int(xx>>16),t=int(yy>>16);const unsigned fx=unsigned(xx&65535)>>8,fy=unsigned(yy&65535)>>8;
  auto color=[&](int px,int py){px=px<0?0:px>319?319:px;py=py<0?0:py>239?239:py;const unsigned at=(unsigned(py)*320+unsigned(px))*2;return unsigned(base[at])|(unsigned(base[at+1])<<8);};
  const unsigned a=color(l,t),b=color(l+1,t),c=color(l,t+1),d=color(l+1,t+1);unsigned out=0;
  for(unsigned shift:{0u,5u,11u}){const unsigned mask=shift==5?63:31;auto lerp=[](unsigned u,unsigned v,unsigned f){return unsigned((double(u)*(256-f)+double(v)*f+128)/256);};
   const unsigned top=lerp((a>>shift)&mask,(b>>shift)&mask,fx),bottom=lerp((c>>shift)&mask,(d>>shift)&mask,fx);out|=lerp(top,bottom,fy)<<shift;}
  return out;};
 for(unsigned index=0;index<9;++index){
  for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){const unsigned gx=index%3*320+x,gy=index/3*240+y,at=(y*320+x)*2,p=scalar(gx,gy);CHECK(mapping.predict(texture,gx,gy)==p);
   const unsigned rr=(x+y)&31,gg=(x*3+y)&63,bb=(x+y*2)&31,r=(rr<<11)|(gg<<5)|bb;residual[at]=uint8_t(r);residual[at+1]=uint8_t(r>>8);
   const unsigned wanted=((((p>>11)+rr)%32)<<11)|(((((p>>5)&63)+gg)%64)<<5)|(((p&31)+bb)%32);expected[at]=uint8_t(wanted);expected[at+1]=uint8_t(wanted>>8);}
  CHECK(mapping.restore(residual.data(),First+index,texture));CHECK(residual==expected);
 }
 auto start=std::chrono::steady_clock::now();for(unsigned repeat=0;repeat<20;++repeat)CHECK(mapping.restore(residual.data(),First,texture));
 const auto cold=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/20;
 Mapping bad=mapping;bad.dx=0;CHECK(!bad.restore(residual.data(),First,texture));CHECK(!mapping.restore(residual.data(),First+Count,texture));
 CHECK(sizeof(ImageCache)==(sizeof(void*)==4?16:24));CHECK(sizeof picture_pixels==13u*1024u*1024u-256u*1024u);
 std::printf("material residual: %u checks; cold page reconstruction %.3f ms host; no new cache bytes\n",checks,cold);
}
