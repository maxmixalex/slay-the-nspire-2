#include "app.hpp"
#include "tile_filter.hpp"
#include "resource_transfer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"tile filter line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t value,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=uint8_t(value>>(i*8));}
static void add(Bytes&b,uint32_t value,unsigned n=4){const unsigned at=unsigned(b.size());b.resize(at+n);put(b,at,value,n);}
static Bytes stored(const Bytes&raw){Bytes out;for(unsigned at=0;at<raw.size();at+=65536){const unsigned left=unsigned(raw.size())-at,n=left<65536?left:65536;out.push_back(at?2:1);out.push_back(uint8_t((n-1)>>8));out.push_back(uint8_t(n-1));out.insert(out.end(),raw.begin()+at,raw.begin()+at+n);}out.push_back(0);return out;}
// Independent scalar forward oracle. Runtime only contains the inverse.
static Bytes filtered(const Bytes&raw,unsigned mode){
    const unsigned count=unsigned(raw.size())/770;Bytes out(raw.size());
    for(unsigned tile=0;tile<count;++tile){const unsigned at=tile*770;out[tile*2]=raw[at];out[tile*2+1]=raw[at+1];
        for(unsigned i=0;i<256;++i){const unsigned rgb=unsigned(raw[at+2+i*2])|(unsigned(raw[at+3+i*2])<<8),g=(rgb>>5)&63;
            unsigned r=((rgb>>11)+32-(g>>1))&31,b=((rgb&31)+32-(g>>1))&31;
            if(mode==2){r=r<16?r*2:(32-r)*2-1;b=b<16?b*2:(32-b)*2-1;}
            put(out,count*2+tile*512+i*2,g|(r<<6)|(b<<11),2);out[count*514+tile*256+i]=raw[at+514+i];
        }
    }return out;
}
static void all_bits(){
    for(unsigned count:{1u,256u,680u}){Bytes original(count*770),output(original.size());
        for(unsigned t=0;t<count;++t){original[t*770]=uint8_t(t%16+1);original[t*770+1]=uint8_t(t/16%16+1);
            for(unsigned p=0;p<256;++p){put(original,t*770+2+p*2,t*256+p,2);original[t*770+514+p]=uint8_t((t*256+p)*13);}}
        for(unsigned mode:{1u,2u}){const Bytes encoded=filtered(original,mode);CHECK(restore_tile_filter(output.data(),encoded.data(),unsigned(encoded.size()),count,mode));
            for(unsigned i=0;i<original.size();++i)CHECK(output[i]==original[i]);}
    }
    Bytes raw(770),out(770);raw[0]=raw[1]=16;
    CHECK(!restore_tile_filter(out.data(),raw.data(),770,1,0));CHECK(!restore_tile_filter(out.data(),raw.data(),770,1,3));
    CHECK(!restore_tile_filter(out.data(),raw.data(),769,1,1));CHECK(!restore_tile_filter(out.data(),raw.data(),770,681,1));
    raw[0]=0;CHECK(!restore_tile_filter(out.data(),raw.data(),770,1,1));
}
struct Package {
    static constexpr unsigned Images=66,Strings=Ui7StringCount,Clips=CharacterDeathAnimationKinds;
    Bytes bytes,expected;unsigned base{},tile_group{},index_group{},frame{};
    unsigned append(const Bytes&raw){if(bytes.size()&1)bytes.push_back(0);const unsigned at=unsigned(bytes.size());bytes.insert(bytes.end(),raw.begin(),raw.end());return at;}
    Package(unsigned mode){
        const unsigned strings=40+Images*16+16,clips=strings+Strings*8,cards=clips+Clips*16,aux=cards+CardKinds*16;
        base=aux+GameOverTransitionAuxiliaryCount*4;const unsigned table=base+64,costs=table+16;frame=costs+Images;bytes.resize(frame+16);
        put(bytes,0,0x3250534e);put(bytes,4,TileFilterContentVersion);put(bytes,8,0x41cef1ea);put(bytes,16,Images);put(bytes,20,1);put(bytes,24,Strings);put(bytes,28,Clips);put(bytes,32,CardKinds);
        for(unsigned i=0;i<Images;++i){put(bytes,40+i*16,frame);put(bytes,44+i*16,17,2);put(bytes,46+i*16,17,2);put(bytes,48+i*16,6);put(bytes,52+i*16,16);bytes[costs+i]=1;}
        const unsigned glyph=append({255});put(bytes,40+Images*16,'?');put(bytes,44+Images*16,glyph);put(bytes,48+Images*16,5,2);bytes[50+Images*16]=bytes[51+Images*16]=1;put(bytes,52+Images*16,1,2);
        const unsigned marker=append({'e','n','g',0});for(unsigned i=0;i<Strings;++i){put(bytes,strings+i*8,i==6110?marker:marker+3);put(bytes,strings+i*8+4,i==6110?3:0);}
        for(unsigned i=0;i<Clips;++i){put(bytes,clips+i*16+4,1,2);put(bytes,clips+i*16+6,33,2);}
        Bytes records(4*770),geometry;
        for(unsigned ty=0;ty<2;++ty)for(unsigned tx=0;tx<2;++tx){const unsigned tile=ty*2+tx;records[tile*770]=tx?1:16;records[tile*770+1]=ty?1:16;}
        for(unsigned y=0;y<17;++y){add(expected,1,2);add(expected,0,2);add(expected,17,2);add(expected,1,1);add(geometry,1,2);add(geometry,0,2);add(geometry,17,2);add(geometry,1,1);
            for(unsigned x=0;x<17;++x){const unsigned color=(x*991+y*771)&65535,alpha=(x+y*13)%256,tile=(y/16)*2+x/16,pos=(y%16)*16+x%16;
                add(expected,color,2);add(expected,alpha,1);put(records,tile*770+2+pos*2,color,2);records[tile*770+514+pos]=uint8_t(alpha);}}
        Bytes index;add(index,unsigned(expected.size()));add(index,16);add(index,unsigned(geometry.size()));for(unsigned i=0;i<4;++i)add(index,i);index.insert(index.end(),geometry.begin(),geometry.end());
        for(unsigned i=0;i<2;++i){Bytes encoded=stored(i?index:mode?filtered(records,mode):records),block(32);
            put(block,0,!i&&mode?FilteredTileGroupMagic:0x31524754);put(block,4,i+1);put(block,8,i);put(block,12,unsigned(i?index.size():records.size()));put(block,16,unsigned(encoded.size()));put(block,24,i?1:4);put(block,28,i?0:mode);
            block.insert(block.end(),encoded.begin(),encoded.end());const unsigned at=append(block);(i?index_group:tile_group)=at;put(bytes,table+i*8,at);put(bytes,table+i*8+4,unsigned(block.size()));}
        const unsigned header[]={0x314c4954,1,16,14,4,1,1,1,frame,table,770,4,524288,ResourceTransferCapacity,costs,Images};for(unsigned i=0;i<16;++i)put(bytes,base+i*4,header[i]);
        put(bytes,frame,base);put(bytes,frame+12,unsigned(index.size()));put(bytes,12,unsigned(bytes.size()));
    }
};
struct Provider {const Bytes&bytes;unsigned reads=0;bool fail=false;
    static bool read(void*ctx,uint32_t off,void*out,uint32_t count){auto&p=*static_cast<Provider*>(ctx);++p.reads;if(p.fail||off>p.bytes.size()||count>p.bytes.size()-off)return false;std::memcpy(out,p.bytes.data()+off,count);return true;}
    ResourceSource source(){return {this,uint32_t(bytes.size()),read,nullptr};}
};
static void reader(){
    auto memory=std::make_unique<Content>(),ranged=std::make_unique<Content>();
    for(unsigned mode:{0u,1u,2u}){Package p(mode);Provider provider{p.bytes};CHECK(memory->open(p.bytes.data(),p.bytes.size()));CHECK(ranged->open(provider.source()));
        for(Content*content:{memory.get(),ranged.get()}){const Picture image=content->image(0);CHECK(image.pixels&&image.format==4&&image.w==17&&image.h==17);CHECK(!std::memcmp(image.pixels,p.expected.data(),p.expected.size()));CHECK(content->healthy());}
        const unsigned reads=provider.reads;provider.fail=true;CHECK(ranged->image(65).pixels&&ranged->healthy());CHECK(provider.reads==reads);
    }
    for(unsigned mode=0;mode<5;++mode){Package p(1);switch(mode){case 0:put(p.bytes,4,GroupedGlyphContentVersion);break;case 1:put(p.bytes,p.tile_group+28,3);break;case 2:put(p.bytes,p.index_group,FilteredTileGroupMagic);put(p.bytes,p.index_group+28,1);break;case 3:put(p.bytes,p.tile_group+24,680);break;default:p.bytes[p.tile_group+35]=0;break;}
        CHECK(memory->open(p.bytes.data(),p.bytes.size()));CHECK(!memory->image(0).pixels&&!memory->healthy());}
}
int main(){all_bits();reader();std::printf("tile filter: %u checks; both transforms, all RGB565 values, alpha, largest group, old/new readers and malformed bounds passed\n",checks);}
