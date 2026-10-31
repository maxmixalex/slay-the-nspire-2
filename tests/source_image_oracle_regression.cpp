// Compare the real memory/range/preload decoder with an independent Python
// byte spool. The spool contains private images; the repository contains none.
#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
using namespace spire;
static uint64_t checks=0;
static unsigned current_image=0,current_backend=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"source image oracle backend=%u image=%u line=%u: %s\n",current_backend,current_image,unsigned(__LINE__),#x);std::exit(1);}}while(0)
static uint32_t word(const uint8_t*p){return p[0]|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
static unsigned half(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
struct Provider {const std::vector<uint8_t>*bytes;};
static bool read_at(void*context,uint32_t offset,void*out,uint32_t count){
 const auto&bytes=*static_cast<Provider*>(context)->bytes;
 if(offset>bytes.size()||count>bytes.size()-offset)return false;
 std::memcpy(out,bytes.data()+offset,count);return true;
}
static std::vector<uint8_t> load(const char*path){
 FILE*f=std::fopen(path,"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long length=std::ftell(f);
 CHECK(length>40&&uint64_t(length)<=MaxContentFile&&!std::fseek(f,0,SEEK_SET));
 std::vector<uint8_t>data(size_t(length),0);CHECK(std::fread(data.data(),1,data.size(),f)==data.size()&&!std::fclose(f));return data;
}
static std::vector<uint8_t> picture_bytes(Picture p){
 CHECK(p.pixels&&p.w&&p.h&&p.w<=320&&p.h<=240&&p.format<=4);
 const uint8_t*start=p.format>=2&&p.format<4?p.palette:p.pixels;CHECK(start);unsigned size=0;
 if(p.format<4)size=p.format<2?p.w*p.h*(p.format?3u:2u):512+p.w*p.h*(p.format==3?2u:1u);
 else{constexpr unsigned limit=320u*240u*8u+2u*240u;
  for(unsigned y=0;y<p.h;++y){CHECK(size<=limit-2);const unsigned spans=half(start+size);size+=2;CHECK(spans<=p.w);
   unsigned last=0;for(unsigned i=0;i<spans;++i){CHECK(size<=limit-5);
    const unsigned x=half(start+size),count=half(start+size+2),mode=start[size+4];size+=5;
    CHECK(count&&mode<=1&&x>=last&&x+count<=p.w);last=x+count;
    const unsigned bytes=count*(mode?3u:2u);CHECK(bytes<=limit-size);size+=bytes;
   }
  }
 }
 return {start,start+size};
}
static bool matches_original(unsigned width,unsigned height,unsigned actual_format,unsigned expected_format,
                             const std::vector<uint8_t>&actual,const std::vector<uint8_t>&expected,bool&normalized){
 normalized=false;
 if(actual_format==expected_format)return actual==expected;
 // ImageReservation has always normalized an entirely opaque alpha plane.
 // Reconstitute that implicit 255 plane logically, without changing the old
 // reference. Every color/palette/index byte and every original alpha byte
 // must still compare exactly; no other format substitution is permitted.
 if(!width||!height||width>320||height>240||
    !((expected_format==1&&actual_format==0)||(expected_format==3&&actual_format==2)))return false;
 const size_t area=size_t(width)*height,prefix=expected_format==1?area*2:512+area;
 if(actual.size()!=prefix||expected.size()!=prefix+area||
    std::memcmp(actual.data(),expected.data(),prefix))return false;
 for(size_t at=prefix;at<expected.size();++at)if(expected[at]!=255)return false;
 normalized=true;return true;
}
static void opaque_reference_tests(){
 for(unsigned format:{1u,3u}){
  const unsigned area=4,prefix=format==1?area*2:512+area;
  std::vector<uint8_t>actual(prefix),expected(prefix+area,255);bool normalized=false;
  for(unsigned i=0;i<prefix;++i)actual[i]=expected[i]=uint8_t(i*37+19);
  CHECK(matches_original(2,2,format-1,format,actual,expected,normalized)&&normalized);
  CHECK(matches_original(2,2,format,format,expected,expected,normalized)&&!normalized);
  for(unsigned i=0;i<area;++i){expected[prefix+i]=254;
   CHECK(!matches_original(2,2,format-1,format,actual,expected,normalized));expected[prefix+i]=255;}
  actual[prefix/2]^=1;CHECK(!matches_original(2,2,format-1,format,actual,expected,normalized));actual[prefix/2]^=1;
  actual.push_back(255);CHECK(!matches_original(2,2,format-1,format,actual,expected,normalized));actual.pop_back();
  expected.pop_back();CHECK(!matches_original(2,2,format-1,format,actual,expected,normalized));expected.push_back(255);
  CHECK(!matches_original(2,2,format==1?2:0,format,actual,expected,normalized));
  CHECK(!matches_original(0,2,format-1,format,actual,expected,normalized));
 }
 bool normalized=false;const std::vector<uint8_t>same(8,255);
 CHECK(!matches_original(2,2,0,4,same,same,normalized));
 CHECK(!matches_original(2,2,1,0,same,same,normalized));
}
int main(int argc,char**argv){
 opaque_reference_tests();
 if(argc==2&&!std::strcmp(argv[1],"--self-test")){
  std::printf("source image oracle: %llu opaque representation checks passed\n",(unsigned long long)checks);return 0;
 }
 CHECK(argc==3);const auto data=load(argv[1]);Provider provider{&data};
 const unsigned catalog=word(data.data()+16);uint64_t total_bytes=0,steps_total=0;unsigned maximum_steps=0,measured=0,first=0,opaque_normalizations=0;
 for(current_backend=0;current_backend<3;++current_backend){
  auto content=std::make_unique<Content>();
  CHECK(current_backend?content->open({&provider,uint32_t(data.size()),read_at,nullptr}):content->open(data.data(),data.size()));
  if(current_backend==2)content->set_image_cache_profile(false);
  FILE*oracle=std::fopen(argv[2],"rb");CHECK(oracle);uint8_t header[28];CHECK(std::fread(header,1,sizeof header,oracle)==sizeof header);
  CHECK(!std::memcmp(header,"IMGR0022",8)&&word(header+8)==catalog&&word(header+20)==data.size()&&word(header+24)==word(data.data()+36));
  first=word(header+12);measured=word(header+16);CHECK(first<catalog&&measured&&measured<=catalog-first);uint64_t decoded=0;unsigned normalized_images=0;
  for(current_image=first;current_image<first+measured;++current_image){
   uint8_t row[20];CHECK(std::fread(row,1,sizeof row,oracle)==sizeof row&&word(row)==current_image);
   const unsigned width=word(row+4),height=word(row+8),format=word(row+12),size=word(row+16);
   CHECK(width&&width<=320&&height&&height<=240&&format<=4&&size&&size<=614880);
   std::vector<uint8_t>expected(size);CHECK(std::fread(expected.data(),1,size,oracle)==size);
   if(current_backend==2){
    unsigned steps=0;auto result=PreparationResult::Pending;
    while(result==PreparationResult::Pending){result=content->preload_image_step(int(current_image));CHECK(++steps<=1000000);}
    CHECK(result==PreparationResult::Ready&&content->image_cached(int(current_image)));steps_total+=steps;if(steps>maximum_steps)maximum_steps=steps;
   }
   const auto before=content->read_stats();const auto picture=content->image(int(current_image));const auto actual=picture_bytes(picture);
   bool normalized=false;
   CHECK(picture.w==width&&picture.h==height&&
         matches_original(width,height,picture.format,format,actual,expected,normalized)&&content->healthy());
   normalized_images+=normalized;
   if(current_backend==2){const auto after=content->read_stats();CHECK(before.callbacks==after.callbacks&&before.callback_bytes==after.callback_bytes);}
   decoded+=size;
   if((current_image-first+1)%2000==0){std::printf("source image oracle backend %u: %u/%u images, %llu exact bytes\n",current_backend,current_image-first+1,measured,(unsigned long long)decoded);std::fflush(stdout);}
  }
  CHECK(std::fgetc(oracle)==EOF&&!std::ferror(oracle)&&!std::fclose(oracle));
  if(current_backend)CHECK(decoded==total_bytes&&normalized_images==opaque_normalizations);
  else{total_bytes=decoded;opaque_normalizations=normalized_images;}
 }
 std::printf("{\"complete_catalog\":%s,\"images\":%u,\"catalog_images\":%u,\"first_image\":%u,\"backends\":3,\"decoded_bytes_per_backend\":%llu,\"checks\":%llu,\"preload_steps\":%llu,\"max_preload_steps\":%u,\"after_ready_source_reads\":0,\"lossless_opaque_normalizations_per_backend\":%u,\"independent_python_reference\":true}\n",first==0&&measured==catalog?"true":"false",measured,catalog,first,(unsigned long long)total_bytes,(unsigned long long)checks,(unsigned long long)steps_total,maximum_steps,opaque_normalizations);
}
