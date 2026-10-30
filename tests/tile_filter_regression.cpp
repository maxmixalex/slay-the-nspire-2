#include "app.hpp"
#include "tile_filter.hpp"
#include "resource_transfer.hpp"
#include "index_prediction.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
#include "fixtures/large_tile_blocks.inc"
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
static void in_place_boundaries(){
    for(unsigned count:{1u,128u,129u,130u,256u,257u,258u,259u,387u,388u,516u,517u,
        645u,646u,679u,680u,681u,774u,775u,903u,904u,1032u,1033u,1161u,1162u,1290u,1291u,1359u,1360u}){
        Bytes original(count*770);
        for(unsigned t=0;t<count;++t){original[t*770]=uint8_t(t%16+1);original[t*770+1]=uint8_t(t/16%16+1);
            for(unsigned p=0;p<256;++p){put(original,t*770+2+p*2,t*256+p,2);original[t*770+514+p]=uint8_t((t*256+p)*13);}}
        for(unsigned mode:{1u,2u})for(bool stepped:{false,true}){
            const Bytes encoded=filtered(original,mode);Bytes guarded(encoded.size()+32,0x91),planes(count*258+32,0xa3);
            std::memcpy(guarded.data()+16,encoded.data(),encoded.size());
            if(stepped){std::memcpy(planes.data()+16,encoded.data(),count*2);
                std::memcpy(planes.data()+16+count*2,encoded.data()+count*514,count*256);
                for(unsigned step=0;step<count;++step){CHECK(restore_tile_filter_in_place_step(guarded.data()+16,planes.data()+16,count,mode,step));
                    for(unsigned j=0;j<16;++j)CHECK(guarded[j]==0x91&&guarded[encoded.size()+16+j]==0x91&&planes[j]==0xa3&&planes[count*258+16+j]==0xa3);}}
            else CHECK(restore_tile_filter_in_place(guarded.data()+16,unsigned(encoded.size()),count,mode,planes.data()+16,count*258));
            CHECK(!std::memcmp(guarded.data()+16,original.data(),original.size()));
            for(unsigned j=0;j<16;++j)CHECK(guarded[j]==0x91&&guarded[encoded.size()+16+j]==0x91&&planes[j]==0xa3&&planes[count*258+16+j]==0xa3);
        }
    }
    Bytes one(770),scratch(258);one[0]=one[1]=16;
    CHECK(!restore_tile_filter_in_place(one.data(),770,1,1,scratch.data(),257));
    CHECK(!restore_tile_filter_in_place(one.data(),770,1,1,one.data()+1,258));
    CHECK(!restore_tile_filter_in_place(one.data()+1,770,1,1,one.data(),258));
    CHECK(!restore_tile_filter_in_place_step(one.data(),scratch.data(),1,1,1));
    CHECK(!restore_tile_filter_in_place_step(one.data(),scratch.data(),1361,1,0));
}
struct Package {
    static constexpr unsigned Images=66,Strings=Ui7StringCount,Clips=CharacterDeathAnimationKinds;
    Bytes bytes,expected;unsigned base{},tile_group{},index_group{},frame{},frame_ids[4]{};
    unsigned append(const Bytes&raw){if(bytes.size()&1)bytes.push_back(0);const unsigned at=unsigned(bytes.size());bytes.insert(bytes.end(),raw.begin(),raw.end());return at;}
    Package(unsigned mode,bool large=false,unsigned mixed=0){
        CHECK(!mixed||large);CHECK(mixed<=2);const unsigned pixel_groups=mixed?3u:1u;
        const unsigned counts[3]={mixed==1?680u:large?LargeTileItems:4u,mixed==1?LargeTileItems:680u,mixed==1?680u:LargeTileItems};
        const unsigned clip_count=large?BestiaryKaiserAnimationKinds:Clips;
        const unsigned strings=40+Images*16+16,clips=strings+Strings*8,cards=clips+clip_count*16,aux=cards+CardKinds*16;
        base=aux+(large?unsigned(Ui12AuxiliaryCount):unsigned(GameOverTransitionAuxiliaryCount))*4;const unsigned table=base+64,costs=table+(pixel_groups+1)*8;frame=costs+Images;bytes.resize(frame+(large?32u:16u));
        put(bytes,0,0x3250534e);put(bytes,4,large?LargeTileContentVersion:TileFilterContentVersion);put(bytes,8,0x41cef1ea);put(bytes,16,Images);put(bytes,20,1);put(bytes,24,Strings);put(bytes,28,clip_count);put(bytes,32,CardKinds);
        for(unsigned i=0;i<Images;++i){put(bytes,40+i*16,frame);put(bytes,44+i*16,17,2);put(bytes,46+i*16,17,2);put(bytes,48+i*16,6);put(bytes,52+i*16,16);bytes[costs+i]=1;}
        const unsigned glyph=append({255});put(bytes,40+Images*16,'?');put(bytes,44+Images*16,glyph);put(bytes,48+Images*16,5,2);bytes[50+Images*16]=bytes[51+Images*16]=1;put(bytes,52+Images*16,1,2);
        const unsigned marker=append({'e','n','g',0});for(unsigned i=0;i<Strings;++i){put(bytes,strings+i*8,i==6110?marker:marker+3);put(bytes,strings+i*8+4,i==6110?3:0);}
        for(unsigned i=0;i<clip_count;++i){put(bytes,clips+i*16+4,1,2);put(bytes,clips+i*16+6,33,2);}
        Bytes records(4*770),geometry;
        for(unsigned ty=0;ty<2;++ty)for(unsigned tx=0;tx<2;++tx){const unsigned tile=ty*2+tx;records[tile*770]=tx?1:16;records[tile*770+1]=ty?1:16;}
        for(unsigned y=0;y<17;++y){add(expected,1,2);add(expected,0,2);add(expected,17,2);add(expected,1,1);add(geometry,1,2);add(geometry,0,2);add(geometry,17,2);add(geometry,1,1);
            for(unsigned x=0;x<17;++x){const unsigned color=(x*991+y*771)&65535,alpha=(x+y*13)%256,tile=(y/16)*2+x/16,pos=(y%16)*16+x%16;
                add(expected,color,2);add(expected,alpha,1);put(records,tile*770+2+pos*2,color,2);records[tile*770+514+pos]=uint8_t(alpha);}}
        if(large){const Bytes first=records;records.resize(LargeTileItems*770);for(unsigned i=0;i<LargeTileItems;++i)std::memcpy(records.data()+i*770,first.data()+(i%4)*770,770);}
        const unsigned ids[]={0,681,1022,1359};
        for(unsigned i=0;i<4;++i)frame_ids[i]=large?ids[i]:i;
        if(mixed){frame_ids[1]=1360+(mixed==1?681:1);frame_ids[2]=2720+(mixed==1?2:682);frame_ids[3]=2720+(mixed==1?679:1359);}
        Bytes index;add(index,unsigned(expected.size()));add(index,16);add(index,unsigned(geometry.size()));for(unsigned i=0;i<4;++i)add(index,frame_ids[i]);index.insert(index.end(),geometry.begin(),geometry.end());
        Bytes encoded_index=index;if(mixed)for(unsigned i=0,previous=0;i<4;++i){put(encoded_index,12+i*4,(frame_ids[i]-previous)*4);previous=frame_ids[i];}
        for(unsigned i=0;i<=pixel_groups;++i){const bool is_index=i==pixel_groups;
            if(large)bytes.resize(((bytes.size()+32767u)&~size_t(32767u))+32768u);
            const Bytes raw=is_index?encoded_index:Bytes(records.begin(),records.begin()+counts[i]*770);
            Bytes encoded=stored(is_index?raw:mode?filtered(raw,mode):raw),block(32);
            if(large&&!is_index&&counts[i]==LargeTileItems){if(mode==1)encoded.assign(large_tiles_mode1,large_tiles_mode1+sizeof large_tiles_mode1);
                else if(mode==2)encoded.assign(large_tiles_mode2,large_tiles_mode2+sizeof large_tiles_mode2);
                else encoded.assign(large_tiles_mode0,large_tiles_mode0+sizeof large_tiles_mode0);}
            put(block,0,is_index?(mixed?PredictiveIndexGroupMagic:0x31524754):mode?FilteredTileGroupMagic:0x31524754);put(block,4,is_index?2:1);put(block,8,i);put(block,12,unsigned(raw.size()));put(block,16,unsigned(encoded.size()));put(block,24,is_index?1:counts[i]);put(block,28,is_index?(mixed?1:0):mode);
            block.insert(block.end(),encoded.begin(),encoded.end());const unsigned at=append(block);if(is_index)index_group=at;else if(!i)tile_group=at;put(bytes,table+i*8,at);put(bytes,table+i*8+4,unsigned(block.size()));}
        const unsigned slots=mixed?(pixel_groups-1)*LargeTileItems+counts[pixel_groups-1]:large?LargeTileItems:4u;
        const unsigned header[]={0x314c4954,1,16,large?16u:14u,slots,pixel_groups,1,large?2u:1u,frame,table,770,large?LargeTileItems:4u,large?1048576u:524288u,ResourceTransferCapacity,costs,Images};for(unsigned i=0;i<16;++i)put(bytes,base+i*4,header[i]);
        put(bytes,frame,base);put(bytes,frame+12,unsigned(index.size()));
        if(large){std::memcpy(bytes.data()+frame+16,bytes.data()+frame,16);put(bytes,56,frame+16);}
        put(bytes,12,unsigned(bytes.size()));
    }
};
struct Provider {const Bytes&bytes;unsigned reads=0;bool fail=false;uint32_t bad_byte=0xffffffffu;
    static bool read(void*ctx,uint32_t off,void*out,uint32_t count){auto&p=*static_cast<Provider*>(ctx);++p.reads;if(p.fail||off>p.bytes.size()||count>p.bytes.size()-off)return false;std::memcpy(out,p.bytes.data()+off,count);if(p.bad_byte>=off&&p.bad_byte-off<count)static_cast<uint8_t*>(out)[p.bad_byte-off]=3;return true;}
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
static void large_reader(){
    auto c=std::make_unique<Content>();
    for(unsigned mode:{0u,1u,2u})for(bool room:{false,true}){
        const Package p(mode,true);Provider provider{p.bytes};CHECK(c->open(provider.source()));c->set_image_cache_profile(room);
        auto verify=[&](){const auto image=c->image(0);CHECK(image.pixels&&image.format==4&&image.w==17&&image.h==17);
            CHECK(!std::memcmp(image.pixels,p.expected.data(),p.expected.size()));CHECK(c->healthy()&&!resource_transfer_uses);};
        verify();CHECK(c->open(provider.source()));c->set_image_cache_profile(room);
        std::vector<unsigned> io_steps;auto finish=[&](bool trace=false){PreparationResult result=PreparationResult::Pending;unsigned steps=0;
            while(result==PreparationResult::Pending){const auto before=provider.reads;result=c->preload_image_step(0);
                CHECK(++steps<3000&&provider.reads-before<=2&&!resource_transfer_uses&&c->healthy());if(trace&&provider.reads>before)io_steps.push_back(steps);
                if(result==PreparationResult::Pending)CHECK(!c->image_cached(0));}
            CHECK(result==PreparationResult::Ready);verify();return steps;};
        const unsigned total=finish(true);CHECK(total>250&&!io_steps.empty());
        for(unsigned cutoff:{1u,8u,250u,300u,400u,900u,1600u,total-32u,total-8u,total-1u}){if(cutoff>=total)continue;
            CHECK(c->open(provider.source()));c->set_image_cache_profile(room);
            for(unsigned step=0;step<cutoff;++step)CHECK(c->preload_image_step(0)==PreparationResult::Pending);
            c->cancel_preload();CHECK(c->healthy()&&!resource_transfer_uses);finish();
        }
        // A real provider failure after geometry/IDs are retained releases every pin.
        CHECK(c->open(provider.source()));c->set_image_cache_profile(room);
        for(unsigned step=1;step<io_steps.back();++step)CHECK(c->preload_image_step(0)==PreparationResult::Pending);
        provider.fail=true;CHECK(c->preload_image_step(0)==PreparationResult::Failed&&!c->healthy()&&!resource_transfer_uses);
        provider.fail=false;c->cancel_preload();CHECK(c->open(provider.source()));c->set_image_cache_profile(room);finish();
        // Corrupt only the end marker in the provider's copied input, after
        // an otherwise complete1MiB decode. Source bytes remain unchanged.
        unsigned encoded=0;for(unsigned i=0;i<4;++i)encoded|=unsigned(p.bytes[p.tile_group+16+i])<<(i*8);
        CHECK(c->open(provider.source()));c->set_image_cache_profile(room);provider.bad_byte=p.tile_group+32+encoded-1;
        PreparationResult result=PreparationResult::Pending;unsigned late=0;
        while(result==PreparationResult::Pending){result=c->preload_image_step(0);CHECK(++late<3000&&!resource_transfer_uses);}
        CHECK(late>200&&result==PreparationResult::Failed&&!c->healthy()&&!c->image_cached(0));
        provider.bad_byte=0xffffffffu;c->cancel_preload();CHECK(c->open(provider.source()));c->set_image_cache_profile(room);finish();
        Bytes old=p.bytes;put(old,4,43);CHECK(!c->open(old.data(),old.size()));
        old=p.bytes;put(old,p.base+44,1361);CHECK(!c->open(old.data(),old.size()));
        old=p.bytes;put(old,4,45);CHECK(!c->open(old.data(),old.size()));
    }
    c->set_image_cache_profile(false);
}
static void mixed_group_reader(){
    // A1360-slot divisor does not require every physical group to store1360
    // records. Cover short first/interior groups, not just a partial tail,
    // with TGI2 IDs beyond both omitted slot ranges and a full1MiB neighbor.
    for(unsigned layout:{1u,2u})for(unsigned mode:{0u,1u,2u})for(bool room:{false,true}){
        const Package p(mode,true,layout);Provider provider{p.bytes};auto c=std::make_unique<Content>();
        const auto open=[&](bool direct=false){CHECK(c->open(direct?ResourceSource::memory(p.bytes.data(),p.bytes.size()):provider.source()));c->set_image_cache_profile(room);};
        const auto verify=[&](){const auto image=c->image(0);CHECK(image.pixels&&image.format==4&&image.w==17&&image.h==17);
            CHECK(!std::memcmp(image.pixels,p.expected.data(),p.expected.size())&&c->healthy()&&!resource_transfer_uses);};
        for(bool direct:{false,true}){open(direct);verify();}
        const auto finish=[&](){PreparationResult result=PreparationResult::Pending;unsigned steps=0;
            while(result==PreparationResult::Pending){const auto before=provider.reads;result=c->preload_image_step(0);
                CHECK(++steps<10000&&provider.reads-before<=2&&c->healthy()&&!resource_transfer_uses);
                if(result==PreparationResult::Pending)CHECK(!c->image_cached(0));}
            CHECK(result==PreparationResult::Ready);verify();return steps;};
        open();const auto total=finish();CHECK(total>500);
        for(unsigned cutoff:{total/2,total-1}){open();for(unsigned i=0;i<cutoff;++i)CHECK(c->preload_image_step(0)==PreparationResult::Pending);
            c->cancel_preload();CHECK(c->healthy()&&!resource_transfer_uses);finish();}
        // A directory-valid global ID that points into an omitted interior
        // slot must still fail before publication. Rewrite all spatial deltas
        // so the prediction itself is valid and only the physical slot is bad.
        Bytes bad=p.bytes;unsigned ids[4];for(unsigned i=0;i<4;++i)ids[i]=p.frame_ids[i];ids[0]=layout==1?680:2040;
        for(unsigned i=0,previous=0;i<4;++i){const int delta=int(ids[i])-int(previous);const unsigned zigzag=delta<0?unsigned(-delta*2-1):unsigned(delta*2);
            put(bad,p.index_group+32+3+12+i*4,zigzag*2);previous=ids[i];}
        Provider broken{bad};CHECK(c->open(broken.source()));c->set_image_cache_profile(room);CHECK(!c->image(0).pixels&&!c->healthy());
        CHECK(c->open(broken.source()));c->set_image_cache_profile(room);PreparationResult failed=PreparationResult::Pending;unsigned steps=0;
        while(failed==PreparationResult::Pending){failed=c->preload_image_step(0);CHECK(++steps<10000&&!resource_transfer_uses);}
        CHECK(failed==PreparationResult::Failed&&!c->healthy()&&!c->image_cached(0));
        open();finish();c->set_image_cache_profile(false);
    }
}
static void foreground_shared_index(){
    for(unsigned mode:{1u,2u})for(unsigned cutoff:{20u,400u}){
        Package p(mode,true);Bytes raw(524288);std::memcpy(raw.data(),p.expected.data(),p.expected.size());
        const Bytes encoded=stored(raw);Bytes group(32);put(group,0,0x31524752);put(group,4,unsigned(raw.size()));
        put(group,8,unsigned(encoded.size()));put(group,12,14);put(group,16,1);group.insert(group.end(),encoded.begin(),encoded.end());
        const unsigned at=p.append(group),descriptor_at=(unsigned(p.bytes.size())+1)&~1u;Bytes descriptor(16);
        put(descriptor,0,uint32_t(int64_t(at)-descriptor_at));put(descriptor,4,unsigned(group.size()));put(descriptor,12,(4u<<24)|unsigned(p.expected.size()));
        CHECK(p.append(descriptor)==descriptor_at);put(p.bytes,72,descriptor_at);put(p.bytes,80,7);put(p.bytes,84,16);put(p.bytes,12,unsigned(p.bytes.size()));
        auto c=std::make_unique<Content>();CHECK(c->open(p.bytes.data(),p.bytes.size()));c->set_image_cache_profile(true);
        for(unsigned step=0;step<cutoff;++step)CHECK(c->preload_image_step(0)==PreparationResult::Pending);
        // A different frame shares the same index. Its foreground lease
        // must not clear a still-live background pin. A full retained group
        // then forces real eviction instead of relying on spare pool pages.
        for(int id:{1,2}){const auto image=c->image(id);CHECK(image.pixels&&c->healthy());
            CHECK(!std::memcmp(image.pixels,p.expected.data(),p.expected.size()));}
        PreparationResult result=PreparationResult::Pending;unsigned steps=0;
        while(result==PreparationResult::Pending){result=c->preload_image_step(0);CHECK(++steps<3000&&c->healthy()&&!resource_transfer_uses);}
        CHECK(result==PreparationResult::Ready);const auto image=c->image(0);CHECK(image.pixels&&!std::memcmp(image.pixels,p.expected.data(),p.expected.size()));
        c->set_image_cache_profile(false);
    }
}
int main(){all_bits();in_place_boundaries();reader();large_reader();mixed_group_reader();foreground_shared_index();std::printf("tile filter: %u checks; exact alias boundaries, mixed512KiB/1MiB interior groups, late cancellation/faults, shared-index pressure and legacy bounds passed\n",checks);}
