#include "app.hpp"
#include "resource_transfer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"grouped string line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t n,unsigned count=4){for(unsigned i=0;i<count;++i)b[at+i]=uint8_t(n>>(8*i));}
static uint32_t word(const Bytes&b,unsigned at){return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
static Bytes stored(const Bytes&raw){Bytes out;for(unsigned at=0;at<raw.size();at+=65536){const unsigned n=unsigned(raw.size()-at)>65536?65536:unsigned(raw.size()-at);out.push_back(at?2:1);out.push_back(uint8_t((n-1)>>8));out.push_back(uint8_t(n-1));out.insert(out.end(),raw.begin()+at,raw.begin()+at+n);}out.push_back(0);return out;}
struct Package {
 static constexpr unsigned Images=66,Glyphs=1,Strings=Ui7StringCount,Clips=CharacterDeathAnimationKinds,Aux=GameOverTransitionAuxiliaryCount;
 Bytes bytes;unsigned string_at=40+Images*16+Glyphs*16,base=0,group_at=0,group_size=0,descriptors[5]{};
 std::string text[5];
 unsigned append(const Bytes&data){if(bytes.size()&1)bytes.push_back(0);const unsigned at=unsigned(bytes.size());bytes.insert(bytes.end(),data.begin(),data.end());return at;}
 unsigned group(const Bytes&raw){Bytes encoded=stored(raw),g(32);put(g,0,0x31524752);put(g,4,unsigned(raw.size()));put(g,8,unsigned(encoded.size()));put(g,12,14);put(g,16,4);g.insert(g.end(),encoded.begin(),encoded.end());return append(g);}
 explicit Package(bool grouped=true,bool pressure=false,unsigned extra=0,bool missing_nul=false,unsigned auxiliary=Aux){
  const unsigned clips=string_at+Strings*8,cards=clips+Clips*16,aux=cards+CardKinds*16;
  base=aux+auxiliary*4;const unsigned table=base+64,cost=table+16,frame=cost+Images;
  bytes.resize(frame+16);put(bytes,0,0x3250534e);put(bytes,4,grouped?GroupedStringContentVersion:GroupedGlyphContentVersion);put(bytes,8,0x41cef1ea);
  put(bytes,16,Images);put(bytes,20,Glyphs);put(bytes,24,Strings);put(bytes,28,Clips);put(bytes,32,CardKinds);
  put(bytes,base,0x314c4954);put(bytes,base+4,1);put(bytes,base+8,16);put(bytes,base+12,14);
  for(unsigned off:{16u,20u,24u,28u})put(bytes,base+off,1);
  put(bytes,base+32,frame);put(bytes,base+36,table);put(bytes,base+40,770);put(bytes,base+44,1);put(bytes,base+48,524288);put(bytes,base+52,ResourceTransferCapacity);put(bytes,base+56,cost);put(bytes,base+60,Images);
  const unsigned image=append({0x34,0x12});for(unsigned i=0;i<Images;++i){put(bytes,40+i*16,image);put(bytes,44+i*16,1,2);put(bytes,46+i*16,1,2);put(bytes,52+i*16,2);bytes[cost+i]=1;}
  if(pressure)for(unsigned i=0;i<11;++i){Bytes raw(524288,uint8_t(i+1));raw[0]=uint8_t(i);raw[1]=0x42;const unsigned g=group(raw),n=unsigned(bytes.size())-g;Bytes d(16);const unsigned at=(unsigned(bytes.size())+1)&~1u;put(d,0,uint32_t(g-at));put(d,4,n);put(d,12,2);const unsigned descriptor=append(d);put(bytes,40+i*16,descriptor);put(bytes,48+i*16,7);put(bytes,52+i*16,16);}
  Bytes tile(770);tile[0]=tile[1]=1;tile[2]=0x34;tile[3]=0x12;tile[514]=255;
  Bytes index(23);put(index,0,9);put(index,4,4);put(index,8,7);put(index,16,1,2);put(index,20,1,2);
  for(unsigned i=0;i<2;++i){const Bytes&raw=i?index:tile;Bytes encoded=stored(raw),g(32);put(g,0,0x31524754);put(g,4,i+1);put(g,8,i);put(g,12,unsigned(raw.size()));put(g,16,unsigned(encoded.size()));put(g,24,1);g.insert(g.end(),encoded.begin(),encoded.end());const unsigned at=append(g);put(bytes,table+i*8,at);put(bytes,table+i*8+4,unsigned(g.size()));}
  put(bytes,40+65*16,frame);put(bytes,48+65*16,6);put(bytes,52+65*16,16);put(bytes,frame,base);put(bytes,frame+12,unsigned(index.size()));
  for(unsigned i=0;i<Clips;++i){put(bytes,clips+i*16+4,1,2);put(bytes,clips+i*16+6,33,2);}
  const unsigned glyph=append({255}),glyph_at=40+Images*16;put(bytes,glyph_at,'?');put(bytes,glyph_at+4,glyph);put(bytes,glyph_at+8,5,2);bytes[glyph_at+10]=bytes[glyph_at+11]=1;put(bytes,glyph_at+12,1,2);
  text[0]=u8"Hello 中文 [gold]text[/gold]\n";text[2]=text[0];text[3].resize(16383);for(unsigned i=0;i<text[3].size();++i)text[3][i]=char('A'+i%23);text[4]=std::string(8000,'z');
  Bytes raw;unsigned offsets[5]{};for(unsigned i=0;i<5;++i){if(i==2){offsets[i]=offsets[0];continue;}offsets[i]=unsigned(raw.size());raw.insert(raw.end(),text[i].begin(),text[i].end());raw.push_back(missing_nul&&i==0?'!':0);}
  if(extra)raw.resize(extra);if(grouped){group_at=group(raw);group_size=unsigned(bytes.size())-group_at;}
  for(unsigned i=0;i<5;++i){if(i==2){descriptors[i]=descriptors[0];continue;}if(grouped){Bytes d(16);const unsigned at=(unsigned(bytes.size())+1)&~1u;put(d,0,uint32_t(group_at-at));put(d,4,group_size);put(d,8,offsets[i]);put(d,12,unsigned(text[i].size())+1);descriptors[i]=append(d);}else {Bytes literal(text[i].begin(),text[i].end());literal.push_back(0);descriptors[i]=append(literal);}}
  // Keep the locale marker in its own reader page so the opening contract
  // cannot accidentally warm a test's first compressed text group.
  bytes.resize((bytes.size()+4095)&~size_t(4095));const unsigned marker=append({'e','n','g',0});
  for(unsigned i=0;i<Strings;++i){const unsigned item=i<5?i:1;put(bytes,string_at+i*8,i==6110?marker:descriptors[item]);put(bytes,string_at+i*8+4,i==6110?3:unsigned(text[item].size())|(grouped?GroupedStringFlag:0));}
  put(bytes,12,unsigned(bytes.size()));
 }
};
struct Provider {const Bytes*bytes;bool fail=false;unsigned calls=0;uint64_t byte_count=0;
 static bool read(void*context,uint32_t off,void*out,uint32_t size){auto&s=*static_cast<Provider*>(context);++s.calls;s.byte_count+=size;if(s.fail||off>s.bytes->size()||size>s.bytes->size()-off)return false;std::memcpy(out,s.bytes->data()+off,size);return true;}
 ResourceSource source(){return {this,unsigned(bytes->size()),read,nullptr};}
};
static void exact_and_lease(){
 Package raw(false),packed;Provider provider{&packed.bytes};auto before=std::make_unique<Content>(),direct=std::make_unique<Content>(),streamed=std::make_unique<Content>();
 CHECK(before->open(raw.bytes.data(),raw.bytes.size())&&direct->open(packed.bytes.data(),packed.bytes.size())&&streamed->open(provider.source()));CHECK(streamed->current_profile());
 const unsigned opened=provider.calls;CHECK(!std::strcmp(streamed->string(LocaleMarkerString),"eng"));CHECK(provider.calls==opened);
 const char*hello=streamed->string(0);CHECK(!std::strcmp(hello,packed.text[0].c_str()));const unsigned calls=provider.calls;
 {ResourceTransferLease save(ResourceTransferUse::Save);CHECK(save);save.save()->bytes[0]=0xa7;CHECK(streamed->string(0)==hello);CHECK(!*streamed->string(4));CHECK(streamed->healthy()&&provider.calls==calls&&save.save()->bytes[0]==0xa7);}
 for(unsigned i=0;i<5;++i){const char*a=before->string(i),*b=direct->string(i),*c=streamed->string(i);CHECK(std::strlen(c)==packed.text[i].size());CHECK(!std::memcmp(a,b,packed.text[i].size()+1)&&!std::memcmp(a,c,packed.text[i].size()+1));}
 CHECK(streamed->string(2)==hello);const unsigned warm=provider.calls;provider.fail=true;for(unsigned frame=0;frame<64;++frame)for(unsigned i=0;i<5;++i)CHECK(!std::strcmp(streamed->string(i),packed.text[i].c_str()));CHECK(provider.calls==warm&&streamed->healthy());
}
static void lifetime(){Package p(true,true);Provider provider{&p.bytes};auto content=std::make_unique<Content>();CHECK(content->open(provider.source()));const char*small=content->string(0),*maximum=content->string(3);CHECK(!std::strcmp(small,p.text[0].c_str())&&std::strlen(maximum)==16383);for(unsigned i=0;i<11;++i){const auto picture=content->image(i);CHECK(picture.pixels&&picture.pixels[0]==i&&picture.pixels[1]==0x42);}const unsigned calls=provider.calls;provider.fail=true;CHECK(content->string(0)==small&&content->string(3)==maximum);CHECK(!std::strcmp(small,p.text[0].c_str())&&!std::memcmp(maximum,p.text[3].c_str(),16384));CHECK(content->healthy()&&provider.calls==calls);}
static void optional_catalogs(){
 for(unsigned count:{unsigned(GameOverTransitionAuxiliaryCount),unsigned(CardResourceAuxiliaryCount),unsigned(FtueAuxiliaryCount)}){
  Package p(true,false,0,false,count);Provider provider{&p.bytes};auto content=std::make_unique<Content>();
  CHECK(content->open(provider.source())&&content->current_profile());
  CHECK(content->has_ftue_assets()==(count==FtueAuxiliaryCount));
  const unsigned opened=provider.calls;CHECK(!std::strcmp(content->string(LocaleMarkerString),"eng"));CHECK(provider.calls==opened);
  CHECK(!std::strcmp(content->string(0),p.text[0].c_str())&&content->healthy());
 }
 for(unsigned count:{unsigned(CardResourceAuxiliaryCount+1),unsigned(FtueAuxiliaryCount-1),unsigned(FtueAuxiliaryCount+1)}){
  Package p(true,false,0,false,count);auto content=std::make_unique<Content>();CHECK(!content->open(p.bytes.data(),p.bytes.size()));
 }
 Package old(false,false,0,false,FtueAuxiliaryCount);auto content=std::make_unique<Content>();CHECK(!content->open(old.bytes.data(),old.bytes.size()));
 Package invalid(true,false,0,false,FtueAuxiliaryCount);put(invalid.bytes,invalid.base-4,Package::Images);CHECK(!content->open(invalid.bytes.data(),invalid.bytes.size()));
}
static void malformed(){
 for(unsigned mode=0;mode<11;++mode){Package p;const unsigned d=p.descriptors[0];switch(mode){case 0:put(p.bytes,d,0x80000000u);break;case 1:put(p.bytes,d+4,32);break;case 2:put(p.bytes,d+8,32768);break;case 3:put(p.bytes,d+12,1);break;case 4:put(p.bytes,p.group_at,0);break;case 5:put(p.bytes,p.group_at+12,13);break;case 6:put(p.bytes,p.group_at+8,0);break;case 7:put(p.bytes,d+4,ResourceTransferCapacity+1);break;case 8:put(p.bytes,p.string_at+4,GroupedStringFlag|16384);break;case 9:put(p.bytes,4,GroupedGlyphContentVersion);break;default:put(p.bytes,d,0);break;}
  auto content=std::make_unique<Content>();CHECK(content->open(p.bytes.data(),p.bytes.size()));CHECK(!*content->string(0)&&!content->healthy());
 }
 for(unsigned mode=0;mode<2;++mode){Package p(true,false,mode?0:GroupedStringRawLimit+1,mode);auto content=std::make_unique<Content>();CHECK(content->open(p.bytes.data(),p.bytes.size()));CHECK(!*content->string(0)&&!content->healthy());}
 Package p;Provider provider{&p.bytes};auto content=std::make_unique<Content>();CHECK(content->open(provider.source()));provider.fail=true;CHECK(!*content->string(0)&&!content->healthy());provider.fail=false;CHECK(content->open(provider.source()));CHECK(!std::strcmp(content->string(0),p.text[0].c_str())&&content->healthy());
 put(p.bytes,p.string_at+LocaleMarkerString*8+4,GroupedStringFlag|3);CHECK(!content->open(p.bytes.data(),p.bytes.size()));
}
static Bytes read_file(const char*path){FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>0);CHECK(!std::fseek(f,0,SEEK_SET));Bytes b(size);CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));return b;}
static void actual(const char*old_path,const char*new_path){Bytes before=read_file(old_path),after=read_file(new_path);Provider input{&after};auto old=std::make_unique<Content>(),direct=std::make_unique<Content>(),streamed=std::make_unique<Content>();CHECK(old->open(before.data(),before.size())&&direct->open(after.data(),after.size())&&streamed->open(input.source()));CHECK(direct->version()==GroupedStringContentVersion&&direct->current_profile());const unsigned count=word(before,24);CHECK(count==word(after,24));const uint64_t open_bytes=input.byte_count;for(unsigned i=0;i<count;++i){const char*a=old->string(i),*b=direct->string(i),*c=streamed->string(i);CHECK(!std::strcmp(a,b)&&!std::strcmp(a,c));CHECK(old->healthy()&&direct->healthy()&&streamed->healthy());}for(unsigned i=0;i<128;++i)CHECK(!std::strcmp(streamed->string(i),old->string(i)));const unsigned cold=input.calls;input.fail=true;for(unsigned frame=0;frame<64;++frame)for(unsigned i=0;i<128;++i)CHECK(!std::strcmp(streamed->string(i),old->string(i)));CHECK(streamed->healthy()&&input.calls==cold);std::printf("actual string rows %u exact; open read %llu bytes; 128 strings x64 warm frames add zero provider calls\n",count,(unsigned long long)open_bytes);}
int main(int argc,char**argv){CHECK(argc==1||argc==3);exact_and_lease();lifetime();optional_catalogs();malformed();if(argc==3)actual(argv[1],argv[2]);std::printf("grouped strings: %u checks; raw/grouped/direct/ranged/max-length/aliases/lease/evicted-group pointers/bad bounds\n",checks);}
