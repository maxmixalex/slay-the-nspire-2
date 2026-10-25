#include "app.hpp"
#include "resource_transfer.hpp"
#include "reference_act.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
void put(Bytes&b,size_t at,uint32_t value,unsigned width=4){
    CHECK(at<=b.size()&&width<=b.size()-at);
    for(unsigned i=0;i<width;++i)b[at+i]=uint8_t(value>>(8*i));
}
unsigned append(Bytes&b,const Bytes&data){
    if(b.size()&1)b.push_back(0);
    const unsigned at=unsigned(b.size());b.insert(b.end(),data.begin(),data.end());return at;
}
// Independent legal raw LZMA2 stored chunks, with no encoder oracle.
Bytes stored(const Bytes&raw){
    CHECK(!raw.empty()&&raw.size()<=524288);Bytes out;
    for(size_t at=0;at<raw.size();){
        const unsigned count=unsigned(std::min<size_t>(65536,raw.size()-at));
        out.push_back(at?2:1);out.push_back(uint8_t((count-1)>>8));out.push_back(uint8_t(count-1));
        out.insert(out.end(),raw.begin()+at,raw.begin()+at+count);at+=count;
    }
    out.push_back(0);return out;
}
struct Fixture {
    static constexpr unsigned Images=A_Count+1;
    Bytes bytes;
    std::vector<Bytes> raw;
    unsigned base=0,costs=0,group=0,group_size=0;
    std::vector<unsigned> descriptors;
    explicit Fixture(unsigned version=20){
        const unsigned strings=version>=34?Ui7StringCount:LocaleStringCount;
        const unsigned clips=version>=34?BestiaryKaiserAnimationKinds:MerchantEffectsAnimationKinds;
        const unsigned aux=version>=HistoryBodyContentVersion?unsigned(SettingsUi10AuxiliaryCount):version>=HistoryHeaderContentVersion?unsigned(HistoryHeaderAuxiliaryCount):version>=SpeechBubbleContentVersion?unsigned(SpeechBubbleAuxiliaryCount):version>=34?unsigned(GameOverTransitionAuxiliaryCount):
            version>=21?unsigned(TitleEffectsAuxiliaryCount):unsigned(MerchantEffectsAuxiliaryCount);
        base=40+Images*16+16+strings*8+clips*16+CardKinds*16+aux*4;
        const unsigned table=base+64;costs=table+16;
        const unsigned tile_descriptor=costs+((Images+1)&~1u);
        bytes.resize(tile_descriptor+16);
        put(bytes,0,0x3250534e);put(bytes,4,version);put(bytes,8,0x41cef1ea);
        put(bytes,16,Images);put(bytes,20,1);put(bytes,24,strings);put(bytes,28,clips);put(bytes,32,CardKinds);
        const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,tile_descriptor,table,770,1,524288,617323,costs,Images};
        for(unsigned i=0;i<16;++i)put(bytes,base+i*4,fields[i]);
        std::fill(bytes.begin()+costs,bytes.begin()+costs+Images,1);
        raw={{0x34,0x12,0xcd,0xab},{0x56,0x34,0x12,0x78,0,255},Bytes(514),Bytes(516),Bytes(480)};
        for(unsigned i=0;i<512;++i){raw[2][i]=uint8_t(i*29);raw[3][i]=uint8_t(i*31);}
        raw[2][512]=1;raw[2][513]=9;raw[3][512]=2;raw[3][513]=7;raw[3][514]=0;raw[3][515]=173;
        Bytes joined;std::vector<unsigned> offsets;
        for(const Bytes&b:raw){offsets.push_back(unsigned(joined.size()));joined.insert(joined.end(),b.begin(),b.end());}
        joined.resize(8192);const Bytes encoded=stored(joined);Bytes wrapped(32);
        put(wrapped,0,0x31524752);put(wrapped,4,unsigned(joined.size()));put(wrapped,8,unsigned(encoded.size()));
        put(wrapped,12,14);put(wrapped,16,unsigned(raw.size()));
        wrapped.insert(wrapped.end(),encoded.begin(),encoded.end());group=append(bytes,wrapped);group_size=unsigned(wrapped.size());
        for(unsigned i=0;i<raw.size();++i){
            Bytes descriptor(16);const unsigned at=(unsigned(bytes.size())+1)&~1u;
            put(descriptor,0,uint32_t(int64_t(group)-at));put(descriptor,4,group_size);
            put(descriptor,8,offsets[i]);put(descriptor,12,(i<<24)|unsigned(raw[i].size()));
            descriptors.push_back(append(bytes,descriptor));
        }
        for(unsigned i=0;i<Images;++i){
            const unsigned which=i<5?i:0,at=40+i*16;
            put(bytes,at,descriptors[which]);put(bytes,at+4,which==4?320:2,2);put(bytes,at+6,which==4?240:1,2);
            put(bytes,at+8,7);put(bytes,at+12,16);
        }
        put(bytes,40+(Images-1)*16,tile_descriptor);put(bytes,44+(Images-1)*16,1,2);
        put(bytes,46+(Images-1)*16,1,2);put(bytes,48+(Images-1)*16,6);
        Bytes tile(770);tile[0]=tile[1]=1;
        Bytes index(18);put(index,0,2);put(index,4,4);put(index,8,2);
        // A cold tiled image must actually reach the provider after retained
        // image 0 is warm. Sharing the preceding descriptor cache page would
        // skip after_read entirely and never exercise reentrant transfer I/O.
        bytes.resize((bytes.size()+ResourceReader::PageSize-1)&~size_t(ResourceReader::PageSize-1));
        for(unsigned i=0;i<2;++i){
            const Bytes&data=i?index:tile;const Bytes payload=stored(data);Bytes header(32);
            put(header,0,0x31524754);put(header,4,i+1);put(header,8,i);put(header,12,unsigned(data.size()));
            put(header,16,unsigned(payload.size()));put(header,24,1);header.insert(header.end(),payload.begin(),payload.end());
            const unsigned at=append(bytes,header);put(bytes,table+i*8,at);put(bytes,table+i*8+4,unsigned(header.size()));
        }
        put(bytes,tile_descriptor,base);put(bytes,tile_descriptor+12,unsigned(index.size()));
        const unsigned glyph=append(bytes,{0xab});unsigned at=40+Images*16;
        put(bytes,at,65);put(bytes,at+4,glyph);put(bytes,at+8,8,2);put(bytes,at+10,1,1);
        put(bytes,at+11,1,1);put(bytes,at+12,1,2);at+=16;
        // Keep startup locale reads away from retained descriptors and payload.
        bytes.resize((bytes.size()+4095)&~size_t(4095));
        const unsigned text=append(bytes,{'e','n','g',0});
        for(unsigned i=0;i<strings;++i){put(bytes,at+i*8,text);put(bytes,at+i*8+4,3);}
        at+=strings*8;
        for(unsigned i=0;i<clips;++i){put(bytes,at+i*16+4,1,2);put(bytes,at+i*16+6,100,2);}
        if(version>=21){
            const unsigned aux_at=at+clips*16+CardKinds*16;
            put(bytes,aux_at+U_TitleBackdrop*4,2);put(bytes,aux_at+U_TitleForeground*4,3);
            put(bytes,aux_at+U_TitleParticles*4,4);
        }
        put(bytes,12,unsigned(bytes.size()));
    }
};
// Independent scalar encoder: production only contains the inverse. Keep
// dense alpha in a separate plane, and sparse alpha in its source spans.
unsigned color_word(const uint8_t*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
void filter_colors(Bytes&bytes,unsigned offset,unsigned count,unsigned stride,unsigned mode){
    for(unsigned i=0;i<count;++i){
        const unsigned at=offset+i*stride,color=color_word(bytes.data()+at),green=(color>>5)&63;
        unsigned red=((color>>11)+32-green/2)%32,blue=((color&31)+32-green/2)%32;
        if(mode==2){red=red<16?2*red:2*(32-red)-1;blue=blue<16?2*blue:2*(32-blue)-1;}
        put(bytes,at,green+64*red+2048*blue,2);
    }
}
Bytes filtered(const Bytes&raw,unsigned format,unsigned mode){
    Bytes bytes=raw;
    if(format<2)filter_colors(bytes,0,unsigned(bytes.size())/(format==1?3:2),2,mode);
    else if(format<4)filter_colors(bytes,0,256,2,mode);
    else for(unsigned at=0;at<bytes.size();){
        const unsigned spans=color_word(bytes.data()+at);at+=2;
        for(unsigned span=0;span<spans;++span){
            const unsigned count=color_word(bytes.data()+at+2),stride=2+bytes[at+4];at+=5;
            filter_colors(bytes,at,count,stride,mode);at+=count*stride;
        }
    }
    return bytes;
}
void filter_fixture(Fixture&f,unsigned mode){
    Bytes joined,table;
    for(unsigned format=0;format<f.raw.size();++format){
        const unsigned at=unsigned(joined.size()),size=unsigned(f.raw[format].size());
        const unsigned end=unsigned(table.size());table.resize(end+8);
        put(table,end,at);put(table,end+4,(format<<24)|size);
        const Bytes bytes=filtered(f.raw[format],format,mode);joined.insert(joined.end(),bytes.begin(),bytes.end());
    }
    const unsigned end=unsigned(table.size()),at=unsigned(joined.size());table.resize(end+8);
    put(table,end,at);put(table,end+4,8192-at);joined.resize(8192);
    const Bytes encoded=stored(joined);Bytes block(32);
    put(block,0,FilteredRetainedGroupMagic);put(block,4,unsigned(joined.size()));put(block,8,unsigned(encoded.size()));
    put(block,12,14);put(block,16,unsigned(table.size()/8));put(block,20,mode);put(block,24,unsigned(table.size()));
    block.insert(block.end(),table.begin(),table.end());block.insert(block.end(),encoded.begin(),encoded.end());
    f.group=append(f.bytes,block);f.group_size=unsigned(block.size());
    for(unsigned descriptor:f.descriptors){put(f.bytes,descriptor,uint32_t(int64_t(f.group)-descriptor));put(f.bytes,descriptor+4,f.group_size);}
    put(f.bytes,12,unsigned(f.bytes.size()));
}
void filter_all_bits(){
    for(unsigned mode:{1u,2u})for(unsigned format:{0u,1u}){
        Bytes original(65536*(format==1?3:2));
        for(unsigned color=0;color<65536;++color){put(original,color*2,color,2);if(format)original[131072+color]=uint8_t(color*37);}
        Bytes restored=filtered(original,format,mode);
        if(format)CHECK(!std::memcmp(restored.data()+131072,original.data()+131072,65536));
        CHECK(restore_retained_colors(restored.data(),unsigned(restored.size()),format,mode));CHECK(restored==original);
    }
    Bytes sparse={0,0,2,0,0,0,2,0,0,0xff,0xff,0x1f,0x78,3,0,2,0,1,0x34,0x12,0,0x1f,0xf8,137};
    for(unsigned mode:{1u,2u}){
        Bytes actual=filtered(sparse,4,mode);CHECK(actual[20]==0&&actual[23]==137);
        CHECK(restore_retained_colors(actual.data(),unsigned(actual.size()),4,mode));CHECK(actual==sparse);
    }
    for(unsigned which=0;which<10;++which){
        Bytes bad=sparse;unsigned size=unsigned(bad.size()),format=4,mode=1;
        switch(which){case 0:size=0;break;case 1:mode=3;break;case 2:format=5;break;
        case 3:format=1;size=5;break;case 4:format=3;size=513;bad.resize(size);break;
        case 5:bad[17]=2;break;case 6:bad[15]=0;break;case 7:bad[13]=1;break;
        case 8:bad.resize(482);std::fill(bad.begin(),bad.end(),0);size=482;break;
        default:size=unsigned(bad.size())-1;break;}
        CHECK(!restore_retained_colors(bad.data(),size,format,mode));
    }
}
struct Provider {
    const Bytes*bytes;uint64_t read_bytes=0;unsigned calls=0;
    void(*after_read)(void*)=nullptr;void*hook=nullptr;bool fail_next=false;
    static bool read(void*context,uint32_t offset,void*out,uint32_t count){
        auto&p=*static_cast<Provider*>(context);++p.calls;p.read_bytes+=count;
        if(offset>p.bytes->size()||count>p.bytes->size()-offset)return false;
        std::memcpy(out,p.bytes->data()+offset,count);
        if(p.after_read)p.after_read(p.hook);
        if(p.fail_next){p.fail_next=false;return false;}return true;
    }
    ResourceSource source(){return {this,uint32_t(bytes->size()),read,nullptr};}
};
Picture picture(const Content&content,unsigned image,const Fixture&fixture,unsigned which){
    const Picture p=content.image(image);CHECK(p.pixels&&p.format==which);
    CHECK(p.w==(which==4?320:2)&&p.h==(which==4?240:1));
    const uint8_t*data=which==2||which==3?p.palette:p.pixels;
    CHECK(data&&!std::memcmp(data,fixture.raw[which].data(),fixture.raw[which].size()));CHECK(content.healthy());return p;
}
void roundtrip(bool streamed,unsigned version,unsigned mode=0){
    Fixture f(version);if(mode)filter_fixture(f,mode);
    Provider source{&f.bytes};auto content=std::make_unique<Content>();
    CHECK(streamed?content->open(source.source()):content->open(f.bytes.data(),f.bytes.size()));
    CHECK(content->current_profile());const auto initial=source.read_bytes;
    const auto ordinary=content->image_dimensions(0),sparse=content->image_dimensions(4);
    CHECK(ordinary.w==2&&ordinary.h==1&&sparse.w==320&&sparse.h==240&&source.read_bytes==initial);
    CHECK(content->version()==version);
    CHECK(content->has_speech_bubble_assets()==(version>=SpeechBubbleContentVersion));
    CHECK(content->has_history_header_assets()==(version>=HistoryHeaderContentVersion));
    CHECK(content->has_history_body_assets()==(version>=HistoryBodyContentVersion));
    CHECK(content->has_settings_ui10_assets()==(version>=HistoryBodyContentVersion));
    if(version>=HistoryHeaderContentVersion)CHECK(content->ui_image(I10_HistoryReminderLock)==0);
    if(version>=SpeechBubbleContentVersion)CHECK(content->has_ftue_assets()&&content->has_selection_assets()&&content->has_inspection_assets()&&content->ui_image(I10_FakeMerchantWelcome)==0);
    CHECK(content->ui_image(U_TitleBackground)==0);
    for(unsigned i=0;i<3;++i)CHECK(content->ui_image(U_TitleBackdrop+i)==(version>=21?int(i+2):-1));
    CHECK(content->image_cache_pages(4)==1);CHECK(source.read_bytes==initial);
    const Picture first=picture(*content,0,f,0);const auto after_first=source.read_bytes;
    for(unsigned i=1;i<5;++i)picture(*content,i,f,i);
    CHECK(source.read_bytes-after_first<=4096);
    CHECK(picture(*content,5,f,0).pixels==first.pixels);
    const auto warm=source.read_bytes;for(unsigned i=0;i<5;++i)picture(*content,i,f,i);
    CHECK(source.read_bytes==warm);
    const Picture tile=content->image(Fixture::Images-1);CHECK(tile.pixels&&tile.w==1&&tile.h==1&&tile.format==4);
    CHECK(tile.pixels[0]==0&&tile.pixels[1]==0);picture(*content,0,f,0);
    CHECK(content->open(source.source()));picture(*content,3,f,3);
    if(version>=21){
        Fixture historical;Provider old{&historical.bytes};CHECK(content->open(old.source())&&content->current_profile());
        CHECK(content->version()==20&&content->ui_image(U_TitleBackground)==0);
        for(unsigned i=0;i<3;++i)CHECK(content->ui_image(U_TitleBackdrop+i)==-1);
        picture(*content,0,historical,0);
    }
}
void filtered_bounds(){
    auto content=std::make_unique<Content>();
    for(unsigned mode:{1u,2u})for(unsigned test=0;test<15;++test){
        Fixture f(RetainedFilterContentVersion);filter_fixture(f,mode);
        switch(test){
        case 0:put(f.bytes,4,SceneFontContentVersion);break;
        case 1:put(f.bytes,f.group+20,0);break;case 2:put(f.bytes,f.group+20,3);break;
        case 3:put(f.bytes,f.group+16,0);break;case 4:put(f.bytes,f.group+16,0x20000000);break;
        case 5:put(f.bytes,f.group+24,40);break;case 6:put(f.bytes,f.group+28,1);break;
        case 7:put(f.bytes,f.group+32,1);break;case 8:put(f.bytes,f.group+40,2);break;
        case 9:put(f.bytes,f.group+36,0);break;case 10:put(f.bytes,f.group+36,(5u<<24)|4);break;
        case 11:put(f.bytes,f.group+76,6670);break;case 12:put(f.bytes,f.group+36,(1u<<24)|4);break;
        case 13:put(f.bytes,f.group+8,f.group_size);break;
        default:f.bytes[f.group+80]=0;break;
        }
        Provider provider{&f.bytes};CHECK(content->open(provider.source()));
        CHECK(!content->image(0).pixels&&!content->healthy());CHECK(!resource_transfer_uses);
        Fixture good(RetainedFilterContentVersion);filter_fixture(good,mode);provider.bytes=&good.bytes;
        CHECK(content->open(provider.source()));picture(*content,0,good,0);
    }
}
void requested_bounds(){
    Fixture f;auto content=std::make_unique<Content>();Bytes changed=f.bytes;
    put(changed,36,0xdeadbeef);put(changed,f.group+20,0xffffffff);
    CHECK(content->open(changed.data(),changed.size()));picture(*content,0,f,0);
    changed=f.bytes;put(changed,40+6*16,0xffffffff);
    CHECK(content->open(changed.data(),changed.size()));picture(*content,1,f,1);
    CHECK(!content->image(6).pixels&&!content->healthy());
    for(unsigned test=0;test<3;++test){
        changed=f.bytes;
        if(test==0)put(changed,f.descriptors[0],0x80000000);
        if(test==1)put(changed,f.descriptors[0]+8,8192);
        if(test==2)put(changed,f.group+4,524289);
        CHECK(content->open(changed.data(),changed.size()));CHECK(!content->image(0).pixels&&!content->healthy());
    }
    changed=f.bytes;put(changed,f.base+60,Fixture::Images+1);CHECK(!content->open(changed.data(),changed.size()));
    changed=f.bytes;put(changed,f.base+56,0xffffffff);CHECK(!content->open(changed.data(),changed.size()));
}
void foreground_workspace_reuse(unsigned mode=0){
    Fixture f(mode?RetainedFilterContentVersion:20);if(mode)filter_fixture(f,mode);
    Provider source{&f.bytes};auto content=std::make_unique<Content>();
    CHECK(content->open(source.source()));const Picture retained=picture(*content,0,f,0);
    const Picture tile=content->image(Fixture::Images-1);CHECK(tile.pixels&&tile.format==4);
    const auto warm=source.read_bytes;
    // Rendering publishes only stable decoded views. File save/history I/O
    // can overwrite every encoded byte without evicting or corrupting them.
    {ResourceTransferLease transfer(ResourceTransferUse::Bytes);CHECK(transfer);
     std::memset(transfer.bytes(),0xa5,ResourceTransferCapacity);}
    CHECK(!std::memcmp(retained.pixels,f.raw[0].data(),f.raw[0].size()));
    CHECK(tile.pixels[0]==0&&tile.pixels[1]==0);picture(*content,0,f,0);
    CHECK(source.read_bytes==warm);
    static State original,restored;static SaveData expected,actual;
    new_run(original,0x71727,true);CHECK(save(original,expected));
    Bytes disk;
    {ResourceTransferLease transfer(ResourceTransferUse::Save);CHECK(transfer);
     auto&scratch=*transfer.save();std::memset(scratch.bytes,0x6d,sizeof scratch.bytes);CHECK(save(original,scratch));
     CHECK(scratch.size==expected.size&&!std::memcmp(scratch.bytes,expected.bytes,expected.size));
     // State construction must use the aligned suffix, not overwrite unused
     // file-buffer bytes, the live encoded prefix or a retained image view.
     for(size_t i=scratch.size;i<sizeof scratch.bytes;++i)CHECK(scratch.bytes[i]==0x6d);
     disk.assign(scratch.bytes,scratch.bytes+scratch.size);
     ResourceTransferLease nested_bytes(ResourceTransferUse::Bytes),nested_save(ResourceTransferUse::Save);
     CHECK(!nested_bytes&&!nested_save);picture(*content,0,f,0);
     CHECK(!content->image(3).pixels&&content->healthy());
     CHECK(save(original,actual)&&actual.size==expected.size&&!std::memcmp(actual.bytes,expected.bytes,actual.size));}
    CHECK(content->open(source.source()));picture(*content,3,f,3); // Cold retained decode borrows the same storage.
    {ResourceTransferLease transfer(ResourceTransferUse::Save);CHECK(transfer);
     auto&readback=*transfer.save();std::memcpy(readback.bytes,disk.data(),disk.size());
     CHECK(load(restored,readback.bytes,disk.size()));CHECK(save(restored,actual));
     CHECK(!std::memcmp(readback.bytes,disk.data(),disk.size()));
     CHECK(!load(restored,readback.bytes,disk.size()-4));
     CHECK(!std::memcmp(readback.bytes,disk.data(),disk.size()));
     CHECK(save(restored,actual));}
    CHECK(expected.size==actual.size&&!std::memcmp(expected.bytes,actual.bytes,expected.size));
    auto progress=std::make_unique<UiProgress>();clear_ui_progress(*progress);
    progress->characters[2].play_seconds=903;progress->card_wins[0]=17;progress->last_saved_at=1792838400;
    Bytes history;
    {ResourceTransferLease transfer(ResourceTransferUse::Bytes);CHECK(transfer);
     const size_t size=encode_ui_progress(*progress,transfer.bytes(),UiProgressCapacity);CHECK(size);
     history.assign(transfer.bytes(),transfer.bytes()+size);}
    picture(*content,4,f,4);
    {ResourceTransferLease transfer(ResourceTransferUse::Bytes);CHECK(transfer);
     std::memcpy(transfer.bytes(),history.data(),history.size());
     clear_ui_progress(*progress);CHECK(decode_ui_progress(transfer.bytes(),history.size(),*progress));}
    CHECK(progress->characters[2].play_seconds==903&&progress->card_wins[0]==17&&progress->last_saved_at==1792838400);
    picture(*content,3,f,3);picture(*content,4,f,4);
    CHECK(!resource_transfer_uses);
}
struct ReentrantRead {
    Content*content;const Fixture*fixture;const State*original;State*target;
    const SaveData*saved;SaveData*output;unsigned calls=0;
    static void attempt(void*context){
        auto&r=*static_cast<ReentrantRead*>(context);++r.calls;
        reference::ReferenceMap map;reference::QuotaTrace trace;
        CHECK(!reference::generate_act_map(map,0x73706163,reference::ActSettings{},trace));
        CHECK(map.error==reference::MapError::Capacity);
        // Exercise an actual ResourceSource callback after it copied encoded
        // input. Nested serialization must fail before either source or target
        // is touched, and a cold image must remain retryable after the lease.
        CHECK(!save(*r.original,*r.output));CHECK(!load(*r.target,r.saved->bytes,r.saved->size));
        CHECK(!r.content->image(4).pixels&&r.content->healthy());
        const auto retained=r.content->image(0);CHECK(retained.pixels);
        CHECK(!std::memcmp(retained.pixels,r.fixture->raw[0].data(),r.fixture->raw[0].size()));
    }
};
void reentrant_transfer_and_failure(unsigned mode=0){
    Fixture f(mode?RetainedFilterContentVersion:20);if(mode)filter_fixture(f,mode);
    Provider source{&f.bytes};auto content=std::make_unique<Content>();
    CHECK(content->open(source.source()));picture(*content,0,f,0);
    static State original,target;static SaveData saved,output;
    new_run(original,0x41434,true);new_run(target,0x19811,true,Character::Defect);CHECK(save(original,saved));
    std::memset(&output,0xa6,sizeof output);
    Bytes before(reinterpret_cast<const uint8_t*>(&target),reinterpret_cast<const uint8_t*>(&target)+sizeof target);
    ReentrantRead reentry{content.get(),&f,&original,&target,&saved,&output};
    source.after_read=ReentrantRead::attempt;source.hook=&reentry;
    const auto tiled=content->image(Fixture::Images-1);CHECK(tiled.pixels&&tiled.format==4);
    CHECK(tiled.pixels[0]==0&&tiled.pixels[1]==0);CHECK(reentry.calls>0);
    CHECK(!std::memcmp(&target,before.data(),before.size()));
    const auto*bytes=reinterpret_cast<const uint8_t*>(&output);
    for(size_t i=0;i<sizeof output;++i)CHECK(bytes[i]==0xa6);
    source.after_read=nullptr;picture(*content,4,f,4);CHECK(!resource_transfer_uses);
    {
        ResourceTransferLease codec(ResourceTransferUse::Codec);CHECK(codec);
        State&staged=*::new(detail::InPlaceAddress{codec.state_storage()}) State(original);
        CHECK(!save(original,output)&&!load(target,saved.bytes,saved.size));
        CHECK(!content->image(3).pixels&&content->healthy());picture(*content,0,f,0);
        ResourceTransferLease nested(ResourceTransferUse::Save);CHECK(!nested);
        CHECK(!std::memcmp(&staged,&original,sizeof staged));
        CHECK(!std::memcmp(&target,before.data(),before.size()));
    }
    CHECK(load(target,saved.bytes,saved.size));CHECK(save(target,output));
    CHECK(saved.size==output.size&&!std::memcmp(saved.bytes,output.bytes,saved.size));picture(*content,3,f,3);
    // Failed streamed I/O releases the lease as well as the image reservation.
    CHECK(content->open(source.source()));source.fail_next=true;
    CHECK(!content->image(0).pixels&&!content->healthy());CHECK(!resource_transfer_uses);
    CHECK(save(original,output));CHECK(content->open(source.source()));picture(*content,0,f,0);
    CHECK(!resource_transfer_uses);
}
}
int main(){
    for(unsigned version:{20u,21u,34u,35u,36u,37u,38u,39u})for(bool streamed:{false,true})roundtrip(streamed,version);
    for(unsigned mode:{1u,2u})for(bool streamed:{false,true})roundtrip(streamed,RetainedFilterContentVersion,mode);
    for(unsigned mode:{1u,2u})for(bool streamed:{false,true})roundtrip(streamed,SpeechBubbleContentVersion,mode);
    filter_all_bits();filtered_bounds();requested_bounds();
    for(unsigned mode:{0u,1u,2u}){foreground_workspace_reuse(mode);reentrant_transfer_and_failure(mode);}
    std::printf("%u retained-content checks passed\n",checks);
}
