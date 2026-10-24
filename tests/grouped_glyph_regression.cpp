#include "app.hpp"
#include "resource_transfer.hpp"
#include "inline_icons.hpp"
#include "unicode_text.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"grouped glyph line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t n,unsigned count=4){for(unsigned i=0;i<count;++i)b[at+i]=uint8_t(n>>(8*i));}
static uint32_t word(const Bytes&b,unsigned at){return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
static Bytes stored(const Bytes&raw){Bytes out;for(unsigned at=0;at<raw.size();at+=65536){const unsigned n=unsigned(raw.size()-at)>65536?65536:unsigned(raw.size()-at);out.push_back(at?2:1);out.push_back(uint8_t((n-1)>>8));out.push_back(uint8_t(n-1));out.insert(out.end(),raw.begin()+at,raw.begin()+at+n);}out.push_back(0);return out;}

struct Package {
    static constexpr unsigned Images=66,Glyphs=5,Strings=Ui7StringCount,Clips=CharacterDeathAnimationKinds,Aux=GameOverTransitionAuxiliaryCount;
    Bytes bytes;unsigned glyph_at=40+Images*16,base=0,group_at=0,descriptors[Glyphs]{};
    Bytes raw[Glyphs];
    unsigned append(const Bytes&data){if(bytes.size()&1)bytes.push_back(0);const unsigned at=unsigned(bytes.size());bytes.insert(bytes.end(),data.begin(),data.end());return at;}
    unsigned group(const Bytes&data){Bytes encoded=stored(data),g(32);put(g,0,0x31524752);put(g,4,unsigned(data.size()));put(g,8,unsigned(encoded.size()));put(g,12,14);put(g,16,Glyphs);g.insert(g.end(),encoded.begin(),encoded.end());return append(g);}
    explicit Package(bool grouped=true,bool pressure=false,unsigned extra_group_bytes=0){
        const unsigned strings=glyph_at+Glyphs*16,clips=strings+Strings*8,cards=clips+Clips*16,aux=cards+CardKinds*16;
        base=aux+Aux*4;const unsigned table=base+64,cost=table+16,frame=cost+Images;
        bytes.resize(frame+16);put(bytes,0,0x3250534e);put(bytes,4,grouped?GroupedGlyphContentVersion:GameOverTransitionContentVersion);put(bytes,8,0x41cef1ea);
        put(bytes,16,Images);put(bytes,20,Glyphs);put(bytes,24,Strings);put(bytes,28,Clips);put(bytes,32,CardKinds);
        put(bytes,base,0x314c4954);put(bytes,base+4,1);put(bytes,base+8,16);put(bytes,base+12,14);
        for(unsigned off:{16u,20u,24u,28u})put(bytes,base+off,1);
        put(bytes,base+32,frame);put(bytes,base+36,table);put(bytes,base+40,770);put(bytes,base+44,1);put(bytes,base+48,524288);put(bytes,base+52,ResourceTransferCapacity);put(bytes,base+56,cost);put(bytes,base+60,Images);
        const unsigned image=append({0x34,0x12});
        for(unsigned i=0;i<Images;++i){put(bytes,40+i*16,image);put(bytes,44+i*16,1,2);put(bytes,46+i*16,1,2);put(bytes,52+i*16,2);bytes[cost+i]=1;}
        if(pressure)for(unsigned i=0;i<11;++i){Bytes raw(524288,uint8_t(i+1));raw[0]=uint8_t(i);raw[1]=0x42;const unsigned g=group(raw),n=unsigned(bytes.size())-g;Bytes d(16);const unsigned at=(unsigned(bytes.size())+1)&~1u;put(d,0,uint32_t(g-at));put(d,4,n);put(d,12,2);const unsigned descriptor=append(d);put(bytes,40+i*16,descriptor);put(bytes,48+i*16,7);put(bytes,52+i*16,16);}
        Bytes tile(770);tile[0]=tile[1]=1;tile[2]=0x34;tile[3]=0x12;tile[514]=255;
        Bytes index(23);put(index,0,9);put(index,4,4);put(index,8,7);put(index,16,1,2);put(index,20,1,2);
        for(unsigned i=0;i<2;++i){const Bytes&raw=i?index:tile;Bytes encoded=stored(raw),g(32);put(g,0,0x31524754);put(g,4,i+1);put(g,8,i);put(g,12,unsigned(raw.size()));put(g,16,unsigned(encoded.size()));put(g,24,1);g.insert(g.end(),encoded.begin(),encoded.end());const unsigned at=append(g);put(bytes,table+i*8,at);put(bytes,table+i*8+4,unsigned(g.size()));}
        put(bytes,40+65*16,frame);put(bytes,48+65*16,6);put(bytes,52+65*16,16);
        put(bytes,frame,base);put(bytes,frame+12,unsigned(index.size()));
        for(unsigned i=0;i<Clips;++i){put(bytes,clips+i*16+4,1,2);put(bytes,clips+i*16+6,33,2);}
        raw[0]={91};raw[1]={0,7,255,255,13,0};raw[2]={1,0,4,255,1,0};raw[3]=raw[1];raw[4].resize(3200);for(unsigned i=0;i<1600;++i){raw[4][2*i]=1;raw[4][2*i+1]=uint8_t(i);}
        Bytes glyph_data;unsigned offsets[Glyphs]{};
        for(unsigned i=0;i<Glyphs;++i){if(i==3){offsets[i]=offsets[1];continue;}offsets[i]=unsigned(glyph_data.size());glyph_data.insert(glyph_data.end(),raw[i].begin(),raw[i].end());}
        if(extra_group_bytes)glyph_data.resize(extra_group_bytes);
        unsigned group_size=0;
        if(grouped){group_at=group(glyph_data);group_size=unsigned(bytes.size())-group_at;}
        for(unsigned i=0;i<Glyphs;++i){
            if(i==3)descriptors[i]=descriptors[1];
            else if(grouped){const unsigned at=(unsigned(bytes.size())+1)&~1u;Bytes d(16);put(d,0,uint32_t(group_at-at));put(d,4,group_size);put(d,8,offsets[i]);put(d,12,unsigned(raw[i].size()));descriptors[i]=append(d);}
            else descriptors[i]=append(raw[i]);
            const unsigned row=glyph_at+i*16;put(bytes,row,i==0?'?':i==4?0x4e00:64+i);put(bytes,row+4,descriptors[i]);put(bytes,row+8,i==4?20:5,2);bytes[row+10]=i==0?1:i==4?40:i==3?2:3;bytes[row+11]=i==0?1:i==4?40:i==3?3:2;put(bytes,row+12,i==4?40:i==3?5:4,2);put(bytes,row+14,(grouped?GroupedGlyphFlag:0)|(i==2?6:i==4?3200:0),2);
        }
        const unsigned marker=append({'e','n','g',0});for(unsigned i=0;i<Strings;++i){put(bytes,strings+i*8,i==6110?marker:marker+3);put(bytes,strings+i*8+4,i==6110?3:0);}
        put(bytes,12,unsigned(bytes.size()));
    }
};
struct Provider {const Bytes*bytes;bool fail=false;unsigned calls=0;
    static bool read(void*context,uint32_t off,void*out,uint32_t size){auto&s=*static_cast<Provider*>(context);++s.calls;if(s.fail||off>s.bytes->size()||size>s.bytes->size()-off)return false;std::memcpy(out,s.bytes->data()+off,size);return true;}
    ResourceSource source(){return {this,unsigned(bytes->size()),read,nullptr};}
};

static void exact_and_lease(){
    Package raw(false),packed;Provider input{&packed.bytes};auto old=std::make_unique<Content>(),direct=std::make_unique<Content>(),streamed=std::make_unique<Content>();
    CHECK(old->open(raw.bytes.data(),raw.bytes.size()));CHECK(direct->open(packed.bytes.data(),packed.bytes.size()));CHECK(streamed->open(input.source()));
    const Glyph a=streamed->glyph('A',5);CHECK(a.pixels&&!std::memcmp(a.pixels,packed.raw[1].data(),6));
    const unsigned before=input.calls;
    {ResourceTransferLease save(ResourceTransferUse::Save);CHECK(save);save.save()->bytes[0]=0xa7;
     CHECK(streamed->glyph('A',5).pixels==a.pixels);CHECK(!streamed->glyph('B',5).pixels);CHECK(streamed->healthy());CHECK(input.calls==before);CHECK(save.save()->bytes[0]==0xa7);}
    for(unsigned i=0;i<Package::Glyphs;++i){const unsigned cp=i==0?'?':i==4?0x4e00:64+i,size=i==4?20:5;
        const Glyph expected=old->glyph(cp,size),memory=direct->glyph(cp,size),ranged=streamed->glyph(cp,size),metric=streamed->glyph_metrics(cp,size);
        for(const Glyph g:{memory,ranged}){CHECK(g.pixels&&g.w==expected.w&&g.h==expected.h&&g.advance==expected.advance&&g.encoded_size==expected.encoded_size);CHECK(!std::memcmp(g.pixels,expected.pixels,packed.raw[i].size()));}
        CHECK(!metric.pixels&&metric.w==expected.w&&metric.h==expected.h&&metric.advance==expected.advance&&metric.encoded_size==expected.encoded_size);
    }
    CHECK(streamed->glyph('C',5).pixels==a.pixels);input.fail=true;CHECK(streamed->glyph('B',5).pixels&&streamed->healthy());
}
static void lifetime(){Package p(true,true);Provider provider{&p.bytes};auto content=std::make_unique<Content>();CHECK(content->open(provider.source()));const Glyph glyph=content->glyph('A',5);CHECK(glyph.pixels);for(unsigned i=0;i<11;++i){auto picture=content->image(i);CHECK(picture.pixels&&picture.pixels[0]==i&&picture.pixels[1]==0x42);}const unsigned calls=provider.calls;provider.fail=true;CHECK(content->glyph('A',5).pixels==glyph.pixels);CHECK(!std::memcmp(glyph.pixels,p.raw[1].data(),6));CHECK(content->healthy()&&provider.calls==calls);}
static void malformed(){
    for(unsigned mode=0;mode<12;++mode){Package p;const unsigned descriptor=p.descriptors[1];
        switch(mode){case 0:put(p.bytes,descriptor,0x80000000u);break;case 1:put(p.bytes,descriptor+4,32);break;case 2:put(p.bytes,descriptor+4,ResourceTransferCapacity+1);break;case 3:put(p.bytes,descriptor+8,65536);break;case 4:put(p.bytes,descriptor+12,7);break;case 5:put(p.bytes,p.group_at,0);break;case 6:put(p.bytes,p.group_at+12,13);break;case 7:put(p.bytes,p.group_at+8,0);break;case 8:put(p.bytes,descriptor+4,unsigned(p.bytes.size()));break;case 9:put(p.bytes,p.glyph_at+16+14,GroupedGlyphFlag|1,2);break;case 10:put(p.bytes,4,GameOverTransitionContentVersion);break;default:put(p.bytes,descriptor,0);break;}
        auto content=std::make_unique<Content>();CHECK(content->open(p.bytes.data(),p.bytes.size()));CHECK(!content->glyph('A',5).pixels&&!content->healthy());
    }
    Package oversized(true,false,GroupedGlyphRawLimit+1);auto content=std::make_unique<Content>();CHECK(content->open(oversized.bytes.data(),oversized.bytes.size()));CHECK(!content->glyph('A',5).pixels&&!content->healthy());
    // The encoded group exceeds a reader page: metadata prefetch must not
    // accidentally satisfy the deliberately refused provider request.
    Package p(true,false,8192);Provider provider{&p.bytes};CHECK(content->open(provider.source()));provider.fail=true;CHECK(!content->glyph('A',5).pixels&&!content->healthy());provider.fail=false;CHECK(content->open(provider.source()));CHECK(content->glyph('A',5).pixels&&content->healthy());
}
static Bytes read_file(const char*path){FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>0);CHECK(!std::fseek(f,0,SEEK_SET));Bytes result(size);CHECK(std::fread(result.data(),1,result.size(),f)==result.size());CHECK(!std::fclose(f));return result;}
static void actual(const char*before_path,const char*after_path){
    Bytes before=read_file(before_path),after=read_file(after_path);auto original=std::make_unique<Content>(),candidate=std::make_unique<Content>();CHECK(original->open(before.data(),before.size()));CHECK(candidate->open(after.data(),after.size()));CHECK(candidate->version()==GroupedGlyphContentVersion);
    const unsigned count=word(before,20);CHECK(count==word(after,20));const unsigned rows=40+word(before,16)*16;unsigned skipped=0;
    for(unsigned i=0;i<count;++i){const unsigned at=rows+i*16,cp=word(before,at),packed=unsigned(before[at+8])|(unsigned(before[at+9])<<8),size=packed&31,style=packed>>5;
        if(text_format_control(cp)||inline_icon(cp)){++skipped;continue;}
        const Glyph a=original->glyph(cp,size,style),b=candidate->glyph(cp,size,style);CHECK(a.pixels&&b.pixels&&a.w==b.w&&a.h==b.h&&a.advance==b.advance&&a.encoded_size==b.encoded_size);const unsigned length=a.encoded_size?a.encoded_size:unsigned(a.w)*a.h;CHECK(!std::memcmp(a.pixels,b.pixels,length));CHECK(original->healthy()&&candidate->healthy());
    }
    std::printf("actual glyph rows: %u, structural inline/control entries: %u; exact raw/RLE bytes and metrics\n",count,skipped);
}
int main(int argc,char**argv){CHECK(argc==1||argc==3);exact_and_lease();lifetime();malformed();if(argc==3)actual(argv[1],argv[2]);std::printf("grouped glyph: %u checks; old/new, direct/ranged, aliases, raw/RLE, lease, image eviction and bad bounds passed\n",checks);}
