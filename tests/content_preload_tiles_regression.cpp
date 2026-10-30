// Reuse the independent public synthetic pack writer and expected byte images.
// The synchronous implementation is not the output oracle for this test.
#define main existing_content_tiles_suite
#include "content_tiles_regression.cpp"
#undef main
#include "resource_transfer.hpp"
#include "tile_filter.hpp"

namespace {
unsigned finish_tile(Content& c, Provider& provider, unsigned id){
    unsigned steps=0;PreparationResult result=PreparationResult::Pending;
    while(result==PreparationResult::Pending){
        const auto before=c.read_stats();const auto calls=provider.calls;
        result=c.preload_image_step(int(id));const auto after=c.read_stats();
        CHECK(++steps<20000);CHECK(c.healthy());CHECK(!resource_transfer_uses);
        // One group-input chunk or at most two cache pages for a short read.
        CHECK(provider.calls-calls<=2);CHECK(after.callbacks-before.callbacks<=2);
        CHECK(after.callback_bytes-before.callback_bytes<=32768);
        if(result==PreparationResult::Pending)CHECK(!c.image_cached(int(id)));
    }
    CHECK(result==PreparationResult::Ready&&c.image_cached(int(id)));
    return steps;
}
void incremental_frames(){
    for(unsigned stride:{1u,85u,680u})for(bool direct:{false,true}){
        label="incremental tiled byte output, aliases, bounded input";
        Package p=basic(stride);Provider provider{&p.bytes};auto c=std::make_unique<Content>();
        CHECK(c->open(direct?ResourceSource::memory(p.bytes.data(),p.bytes.size()):provider.source()));
        for(unsigned i=0;i<p.frames.size();++i){
            CHECK(finish_tile(*c,provider,p.image_id(i))>1);picture(*c,p.image_id(i),p.frames[i]);
        }
        const auto before=c->read_stats();CHECK(c->preload_image_step(p.alias_id())==PreparationResult::Ready);
        picture(*c,p.alias_id(),p.frames[0]);CHECK(same_reads(before,c->read_stats()));CHECK(!provider.bad_range);
    }
    for(Pattern pattern:{Pattern::Alpha,Pattern::ManySpans,Pattern::Empty}){
        label="maximum dimensions and geometry are bounded across steps";
        Package p;p.per_group=680;p.frames.push_back(frame(320,240,pattern));p.collect();p.make_groups({{0}});p.encode();
        Provider provider{&p.bytes};auto c=std::make_unique<Content>();CHECK(c->open(provider.source()));
        CHECK(finish_tile(*c,provider,1)>240);picture(*c,1,p.frames[0]);CHECK(!provider.bad_range);
    }
}
Bytes filtered_fixture(const Group& group,unsigned mode){
    Bytes encoded(group.raw.size());
    const auto signed5=[](unsigned value){return value<16?int(value):int(value)-32;};
    const auto zigzag=[](int value){return unsigned(value<0?-2*value-1:2*value);};
    for(unsigned t=0;t<group.items;++t){
        const auto*raw=group.raw.data()+t*TileBytes;encoded[t*2]=raw[0];encoded[t*2+1]=raw[1];
        for(unsigned i=0;i<256;++i){
            const unsigned rgb=unsigned(raw[2+i*2])|(unsigned(raw[3+i*2])<<8),g=(rgb>>5)&63;
            unsigned r=((rgb>>11)-(g>>1))&31,b=((rgb&31)-(g>>1))&31;
            if(mode==2){r=zigzag(signed5(r));b=zigzag(signed5(b));}
            put(encoded,group.items*2+t*512+i*2,g|(r<<6)|(b<<11),2);
            encoded[group.items*514+t*256+i]=raw[514+i];
        }
    }
    return encoded;
}
Package predictive_filtered(unsigned mode){
    Package p=basic(2);
    for(unsigned i=0;i<p.groups.size();++i){auto&g=p.groups[i];
        g.encoded=stored_lzma2(i<p.tile_groups()?filtered_fixture(g,mode):predicted_index_fixture(g.raw,g.items));g.custom=true;
    }
    p.encode(false,InspectionAuxiliaryCount,PredictiveIndexContentVersion,Ui7StringCount,BestiaryKaiserAnimationKinds);
    const unsigned strings=40+p.images*16+p.glyphs*16;
    put(p.bytes,strings+LocaleMarkerString*8,unsigned(p.bytes.size()));put(p.bytes,strings+LocaleMarkerString*8+4,3);
    p.bytes.insert(p.bytes.end(),{'e','n','g',0});seal(p.bytes);
    for(unsigned i=0;i<p.groups.size();++i){put(p.bytes,p.group_offsets[i],i<p.tile_groups()?FilteredTileGroupMagic:PredictiveIndexGroupMagic);
        put(p.bytes,p.group_offsets[i]+28,i<p.tile_groups()?mode:1);
    }
    return p;
}
void transformed_frames(){
    for(unsigned mode:{1u,2u}){
        label="TGI2 prediction and both TGR2 filters match independent original records";
        const Package p=predictive_filtered(mode);Provider provider{&p.bytes};auto c=std::make_unique<Content>();
        CHECK(c->open(provider.source()));
        for(unsigned i=0;i<p.frames.size();++i){finish_tile(*c,provider,p.image_id(i));picture(*c,p.image_id(i),p.frames[i]);}
    }
}
void cancellation_and_demand(){
    label="cancel/reopen/profile changes and visible demand release unpublished reservations";
    Package p;p.per_group=680;p.frames.push_back(frame(320,240,Pattern::Alpha));p.frames.push_back(frame(32,32,Pattern::Mixed));
    p.collect();p.make_groups({{0,1}});p.encode();Provider provider{&p.bytes};auto c=std::make_unique<Content>();
    for(unsigned cutoff:{1u,2u,3u,8u,20u,70u,180u,350u}){
        CHECK(c->open(provider.source()));
        for(unsigned n=0;n<cutoff;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
        c->cancel_preload();CHECK(c->healthy());finish_tile(*c,provider,1);picture(*c,1,p.frames[0]);
        CHECK(c->open(provider.source()));
        for(unsigned n=0;n<cutoff;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
        picture(*c,1,p.frames[0]);CHECK(c->preload_image_step(1)==PreparationResult::Ready);
    }
    CHECK(c->open(provider.source()));
    for(unsigned n=0;n<20;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
    c->set_image_cache_profile(true);CHECK(c->preload_image_step(1)==PreparationResult::Failed&&c->healthy());
    c->set_image_cache_profile(false);finish_tile(*c,provider,1);picture(*c,1,p.frames[0]);
    CHECK(c->open(provider.source()));
    for(unsigned n=0;n<20;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
    picture(*c,2,p.frames[1]);finish_tile(*c,provider,1);picture(*c,1,p.frames[0]);CHECK(!provider.bad_range);
}
void faults_and_retry(){
    label="tiled incremental faults never publish partial output and reopen clears state";
    Package p=predictive_filtered(2);Provider provider{&p.bytes};auto c=std::make_unique<Content>();
    for(unsigned fault:{p.group_offsets[0],p.group_offsets[p.tile_groups()]+32,p.group_offsets[1]+32}){
        CHECK(c->open(provider.source()));provider.fail_first=fault;provider.fail_last=fault+1;
        PreparationResult result=PreparationResult::Pending;unsigned steps=0;
        while(result==PreparationResult::Pending){result=c->preload_image_step(1);CHECK(++steps<2000);}
        CHECK(result==PreparationResult::Failed&&!c->healthy()&&!c->image_cached(1));CHECK(!resource_transfer_uses);
        provider.fail_first=provider.fail_last=0;CHECK(c->open(provider.source()));finish_tile(*c,provider,1);picture(*c,1,p.frames[0]);
    }
    for(unsigned mutation:{0u,1u,2u,3u}){
        Bytes bad=p.bytes;
        if(mutation==0)put(bad,p.group_offsets[0]+24,681);
        if(mutation==1)put(bad,p.group_offsets[0]+28,3);
        if(mutation==2)put(bad,p.group_offsets[p.tile_groups()]+32+3+12,unsigned(p.tiles.size())*4);
        if(mutation==3)put(bad,p.group_offsets[p.tile_groups()]+24,65);
        provider.bytes=&bad;CHECK(c->open(provider.source()));PreparationResult result=PreparationResult::Pending;unsigned steps=0;
        while(result==PreparationResult::Pending){result=c->preload_image_step(1);CHECK(++steps<2000);}
        CHECK(result==PreparationResult::Failed&&!c->healthy());provider.bytes=&p.bytes;
    }
    CHECK(c->open(provider.source()));finish_tile(*c,provider,1);picture(*c,1,p.frames[0]);CHECK(!provider.bad_range);
}
}
int main(){
    incremental_frames();transformed_frames();cancellation_and_demand();faults_and_retry();
    std::printf("incremental tiled preparation: %u checks; exact bytes, bounded reads, transforms, cancellation and faults\n",checks);
}
