#include "app.hpp"
#include "doom_assets.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
constexpr unsigned TileSide=16,TileBytes=770,GroupBound=524288,EncodedBound=617323;
constexpr unsigned ImageBound=614880,DirectoryBound=2u*1024u*1024u;
constexpr unsigned ImageEntryCapacity=Content::image_capacity_entries(),TilePoolBytes=6u*1024u*1024u;
unsigned checks=0;
std::string label;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL [%s] line %d: %s\n",label.c_str(),__LINE__,#x);std::exit(1);}}while(0)

void put(Bytes&b,size_t at,uint32_t value,unsigned width=4){
    CHECK(at<=b.size()&&width<=b.size()-at);
    for(unsigned i=0;i<width;++i)b[at+i]=uint8_t(value>>(8*i));
}
void append(Bytes&b,uint32_t value,unsigned width=4){
    const size_t at=b.size();b.resize(at+width);put(b,at,value,width);
}
void seal(Bytes&b){put(b,12,uint32_t(b.size()));}
// Legal independent raw LZMA2 streams: reset-dictionary uncompressed chunk,
// optional continuation chunks, then the explicit end marker. This fixture
// writer uses no compressor, original assets, SDK internals or decoder oracle.
Bytes stored_lzma2(const Bytes&raw){
    CHECK(!raw.empty()&&raw.size()<=GroupBound);Bytes encoded;
    for(size_t at=0;at<raw.size();){
        const unsigned count=unsigned(std::min<size_t>(65536,raw.size()-at));
        encoded.push_back(at?2:1);encoded.push_back(uint8_t((count-1)>>8));encoded.push_back(uint8_t(count-1));
        encoded.insert(encoded.end(),raw.begin()+at,raw.begin()+at+count);at+=count;
    }
    encoded.push_back(0);return encoded;
}
struct Span {unsigned x,count,mode;};
struct Pixel {uint16_t color=0;uint8_t alpha=0;};
enum class Pattern {Mixed,Alpha,Opaque,Empty,ManySpans,AlphaSpans};
struct Frame {
    unsigned w,h;
    Bytes expected,geometry;
    std::vector<size_t> span_offsets;
    std::vector<Bytes> tiles;
    std::vector<unsigned> ids;
    Bytes record()const{
        Bytes b;append(b,uint32_t(expected.size()));append(b,uint32_t(ids.size()*4));append(b,uint32_t(geometry.size()));
        for(unsigned id:ids)append(b,id);
        b.insert(b.end(),geometry.begin(),geometry.end());return b;
    }
};
Frame frame(unsigned w,unsigned h,Pattern pattern,unsigned salt=0){
    CHECK(w&&w<=320&&h&&h<=240);Frame f{w,h,{},{},{},{},{}};
    std::vector<Pixel> pixels(w*h);
    for(unsigned y=0;y<h;++y){
        std::vector<Span> row;
        if(pattern==Pattern::Alpha||pattern==Pattern::Opaque)row.push_back({0,w,pattern==Pattern::Alpha?1u:0u});
        if(pattern==Pattern::Mixed){
            CHECK(w>=5);
            if(y%3==1){row.push_back({0,2,0});row.push_back({w-2,2,1});}
            if(y%3==2)row.push_back({1,w-2,1});
        }
        if(pattern==Pattern::ManySpans||pattern==Pattern::AlphaSpans)
            for(unsigned x=0;x<w;++x)row.push_back({x,1,pattern==Pattern::AlphaSpans?1u:x&1});
        append(f.expected,uint32_t(row.size()),2);append(f.geometry,uint32_t(row.size()),2);
        for(const Span&s:row){
            f.span_offsets.push_back(f.geometry.size());
            append(f.geometry,s.x,2);append(f.geometry,s.count,2);append(f.geometry,s.mode,1);
            append(f.expected,s.x,2);append(f.expected,s.count,2);append(f.expected,s.mode,1);
            for(unsigned x=s.x;x<s.x+s.count;++x){
                Pixel p{uint16_t(0x1234u+salt*911u+x*317u+y*191u),
                        s.mode?uint8_t(((x+2*y+salt)%3)*127u+(x+y+salt)%2):uint8_t(255)};
                // Preserve nonzero RGB under alpha zero as part of format-4
                // bytes, not merely a visible-pixel comparison.
                if(s.mode&&(x+y)%7==0)p.alpha=0;
                pixels[y*w+x]=p;append(f.expected,p.color,2);if(s.mode)append(f.expected,p.alpha,1);
            }
        }
    }
    for(unsigned ty=0;ty<h;ty+=TileSide)for(unsigned tx=0;tx<w;tx+=TileSide){
        Bytes tile(TileBytes);tile[0]=uint8_t(std::min(TileSide,w-tx));tile[1]=uint8_t(std::min(TileSide,h-ty));
        for(unsigned y=0;y<tile[1];++y)for(unsigned x=0;x<tile[0];++x){
            const Pixel p=pixels[(ty+y)*w+tx+x];const unsigned at=y*TileSide+x;
            put(tile,2+at*2,p.color,2);tile[514+at]=p.alpha;
        }
        f.tiles.push_back(std::move(tile));
    }
    return f;
}
Bytes blank_tile(){Bytes b(TileBytes);b[0]=b[1]=16;return b;}
struct Group {unsigned type,items;Bytes raw,encoded;bool custom=false;};
struct Descriptor {unsigned group=~0u,offset=0,length=0;};
struct Package {
    unsigned per_group=85,alignment=2,images=160,glyphs=1,reserved_descriptors=0;
    unsigned base=0,prefix=0,descriptor_base=0,table=0;
    std::vector<Frame> frames;
    std::vector<Bytes> tiles;
    std::vector<Group> groups;
    std::vector<Descriptor> descriptors;
    std::vector<unsigned> group_offsets,group_lengths;
    Bytes bytes;
    unsigned tile_groups()const{return (unsigned(tiles.size())+per_group-1)/per_group;}
    unsigned image_id(unsigned i)const{return i+1;}
    unsigned alias_id()const{return unsigned(frames.size())+1;}
    size_t image_entry(unsigned id)const{return 40+size_t(id)*16;}
    size_t descriptor(unsigned i)const{return descriptor_base+size_t(i)*16;}
    void collect(){
        tiles.clear();for(Frame&f:frames){f.ids.clear();for(const Bytes&t:f.tiles){f.ids.push_back(unsigned(tiles.size()));tiles.push_back(t);}}
    }
    void scatter(unsigned count){
        CHECK(count&&tiles.size()<=size_t(count)*per_group);
        std::vector<Bytes> spread(size_t(count)*per_group,blank_tile());
        for(unsigned i=0;i<tiles.size();++i)spread[(i%count)*per_group+i/count]=tiles[i];
        for(Frame&f:frames)for(unsigned&id:f.ids)id=(id%count)*per_group+id/count;
        tiles=std::move(spread);
    }
    void make_groups(const std::vector<std::vector<unsigned>>&plans){
        CHECK(per_group>=1&&per_group<=680&&!frames.empty()&&!tiles.empty());groups.clear();
        for(unsigned at=0;at<tiles.size();at+=per_group){
            const unsigned count=std::min(per_group,unsigned(tiles.size())-at);Group g{1,count,{},{},false};
            for(unsigned i=0;i<count;++i)g.raw.insert(g.raw.end(),tiles[at+i].begin(),tiles[at+i].end());
            groups.push_back(std::move(g));
        }
        descriptors.assign(frames.size(),{});
        for(unsigned i=0;i<plans.size();++i){
            CHECK(!plans[i].empty()&&plans[i].size()<=64);Group g{2,unsigned(plans[i].size()),{},{},false};
            for(unsigned id:plans[i]){
                CHECK(id<frames.size());const Bytes r=frames[id].record();
                descriptors[id]={i,unsigned(g.raw.size()),unsigned(r.size())};g.raw.insert(g.raw.end(),r.begin(),r.end());
            }
            CHECK(g.raw.size()<=GroupBound);groups.push_back(std::move(g));
        }
        for(const Descriptor&d:descriptors)CHECK(d.group<plans.size());
    }
    void encode(bool legacy=false,unsigned auxiliary=U_Count,unsigned version=0,unsigned strings=ContentStrings,unsigned animations=AnimationKinds){
        CHECK(legacy||(!groups.empty()&&groups.size()>tile_groups()));
        images=std::max(images,unsigned(frames.size())+2);
        const unsigned descriptor_count=std::max(unsigned(frames.size()),reserved_descriptors);
        base=40+images*16+glyphs*16+strings*8+animations*16+CardKinds*16+auxiliary*4;
        table=base+64;prefix=legacy?base:table+unsigned(groups.size())*8;
        const unsigned costs=!legacy&&version>=RetainedContentVersion?prefix:0;
        if(costs)prefix+=(images+1u)&~1u;
        descriptor_base=prefix;bytes.assign(prefix+(legacy?0:descriptor_count*16),0);
        const unsigned card=unsigned(bytes.size());bytes.resize(bytes.size()+9840);
        const unsigned glyph=unsigned(bytes.size());bytes.push_back(0xab);
        const unsigned empty=unsigned(bytes.size());bytes.push_back(0);
        group_offsets.clear();group_lengths.clear();
        if(!legacy)for(unsigned i=0;i<groups.size();++i){
            const Group&g=groups[i];const Bytes encoded=g.custom?g.encoded:stored_lzma2(g.raw);
            const unsigned pad=(alignment-unsigned(bytes.size())%alignment)%alignment;bytes.resize(bytes.size()+pad);
            const unsigned off=unsigned(bytes.size()),length=32+unsigned(encoded.size());CHECK(length<=EncodedBound);
            group_offsets.push_back(off);group_lengths.push_back(length);bytes.resize(bytes.size()+32);
            put(bytes,off,0x31524754);put(bytes,off+4,g.type);put(bytes,off+8,i);put(bytes,off+12,unsigned(g.raw.size()));
            put(bytes,off+16,unsigned(encoded.size()));put(bytes,off+20,0);put(bytes,off+24,g.items);
            bytes.insert(bytes.end(),encoded.begin(),encoded.end());
        }
        put(bytes,0,0x3250534e);put(bytes,4,version?version:legacy?14:15);put(bytes,8,0x41cef1ea);
        put(bytes,16,images);put(bytes,20,glyphs);put(bytes,24,strings);put(bytes,28,animations);put(bytes,32,CardKinds);
        for(unsigned id=0;id<images;++id){
            const unsigned which=id==alias_id()?0:id-1;const bool tiled=!legacy&&id&&which<frames.size();const size_t at=image_entry(id);
            put(bytes,at,tiled?unsigned(descriptor(which)):card);put(bytes,at+4,tiled?frames[which].w:82,2);put(bytes,at+6,tiled?frames[which].h:60,2);
            put(bytes,at+8,tiled?6:0);put(bytes,at+12,tiled?16:9840);
        }
        unsigned at=40+images*16;
        for(unsigned i=0;i<glyphs;++i){
            const unsigned row=at+i*16;put(bytes,row,'?'+i);put(bytes,row+4,glyph);put(bytes,row+8,10,2);
            put(bytes,row+10,1,1);put(bytes,row+11,1,1);put(bytes,row+12,1,2);
        }
        at+=glyphs*16;
        for(unsigned i=0;i<strings;++i)put(bytes,at+i*8,empty);
        at+=strings*8;
        for(unsigned i=0;i<animations;++i){put(bytes,at+i*16+4,1,2);put(bytes,at+i*16+6,33,2);}at+=animations*16;
        // All card records and auxiliary references use the shared historical
        // card image zero and empty string zero. Catalog counts remain exact.
        CHECK(at+CardKinds*16+auxiliary*4==base);
        if(!legacy){
            const unsigned fields[]={0x314c4954,1,16,14,unsigned(tiles.size()),tile_groups(),unsigned(groups.size())-tile_groups(),descriptor_count,descriptor_base,table,770,per_group,GroupBound,EncodedBound,costs,costs?images:0};
            for(unsigned i=0;i<16;++i)put(bytes,base+i*4,fields[i]);
            for(unsigned i=0;i<groups.size();++i){put(bytes,table+i*8,group_offsets[i]);put(bytes,table+i*8+4,group_lengths[i]);}
            for(unsigned i=0;i<descriptor_count;++i){const Descriptor&d=descriptors[i<descriptors.size()?i:0];const size_t p=descriptor(i);
                put(bytes,p,base);put(bytes,p+4,d.group);put(bytes,p+8,d.offset);put(bytes,p+12,d.length);
            }
        }
        seal(bytes);
    }
};
Package basic(unsigned per_group=85,unsigned salt=0){
    Package p;p.per_group=per_group;
    p.frames.push_back(frame(33,17,Pattern::Mixed,salt));p.frames.push_back(frame(17,17,Pattern::Alpha,salt+1));
    p.frames.push_back(frame(16,16,Pattern::Opaque,salt+2));p.frames.push_back(frame(48,2,Pattern::Empty,salt+3));
    p.collect();p.make_groups({{0,1,2,3}});p.encode();return p;
}
struct Provider {
    struct Progress {ResourceStage stage;uint32_t completed,total;};
    const Bytes* bytes=nullptr;
    unsigned calls=0,max_read=0,fail_first=0,fail_last=0,failed=0;
    bool corrupt=false,bad_range=false;
    std::vector<Progress> progress;
    ResourceStage fail_stage=ResourceStage::Count;
    unsigned tracked_first=0,tracked_last=0,tracked_calls=0;
    uint64_t tracked_bytes=0;
    static bool read(void*context,uint32_t offset,void*dst,uint32_t count){
        auto&p=*static_cast<Provider*>(context);++p.calls;p.max_read=std::max(p.max_read,count);
        if(!p.bytes||offset>p.bytes->size()||count>p.bytes->size()-offset){p.bad_range=true;return false;}
        const ResourceStage stage=p.progress.empty()?ResourceStage::Count:p.progress.back().stage;
        if(p.tracked_last>p.tracked_first&&
           offset<p.tracked_last&&offset+count>p.tracked_first){
            ++p.tracked_calls;p.tracked_bytes+=std::min(offset+count,p.tracked_last)-std::max(offset,p.tracked_first);
        }
        const bool fault=(p.fail_stage==ResourceStage::Count||stage==p.fail_stage)&&
            p.fail_last>p.fail_first&&offset<p.fail_last&&count>p.fail_first-std::min(offset,p.fail_first);
        std::memcpy(dst,p.bytes->data()+offset,count);
        if(fault){++p.failed;if(p.corrupt){
                const unsigned at=p.fail_first>offset?p.fail_first-offset:0;static_cast<uint8_t*>(dst)[at]^=0x80;
            }else{if(count)std::memset(dst,0xa5,count/2+1);return false;}}
        return true;
    }
    static void report(void*context,ResourceStage stage,uint32_t completed,uint32_t total){
        static_cast<Provider*>(context)->progress.push_back({stage,completed,total});
    }
    ResourceSource source(bool reporting=false){return {this,uint32_t(bytes->size()),read,nullptr,reporting?report:nullptr};}
};
void picture(const Content&c,unsigned id,const Frame&f){
    const Picture p=c.image(int(id));CHECK(p.pixels&&p.w==f.w&&p.h==f.h&&p.format==4);
    CHECK(!p.alpha&&!p.palette);CHECK(!std::memcmp(p.pixels,f.expected.data(),f.expected.size()));CHECK(c.healthy());
}
bool same_reads(ResourceReadStats a,ResourceReadStats b){
    return a.callbacks==b.callbacks&&a.cache_hits==b.cache_hits&&a.cache_misses==b.cache_misses&&
        a.bypasses==b.bypasses&&a.direct_reads==b.direct_reads&&a.callback_bytes==b.callback_bytes;
}
void required_metadata_prefix(){
    label="compressed source prefix must fit the retained metadata directory";
    const Package p=basic();auto content=std::make_unique<Content>();
    for(bool direct:{false,true}){
        Provider provider{&p.bytes};
        ResourceSource source=direct?ResourceSource::memory(p.bytes.data(),p.bytes.size()):provider.source(true);
        source.required_cached_prefix=p.descriptor_base;
        CHECK(content->open(source));picture(*content,1,p.frames[0]);
        source.required_cached_prefix=p.descriptor_base+1;
        provider.progress.clear();CHECK(!content->open(source));
        for(const auto& event:provider.progress)CHECK(event.stage!=ResourceStage::Directory&&event.stage!=ResourceStage::Ready);
        source.required_cached_prefix=0;CHECK(content->open(source));picture(*content,1,p.frames[0]);
    }
}
void progress_contract(){
    label="startup reads the resident directory and defers image payloads";
    const Package p=basic();Provider provider{&p.bytes};auto content=std::make_unique<Content>();
    provider.tracked_first=p.group_offsets[0];provider.tracked_last=unsigned(p.bytes.size());
    CHECK(content->open(provider.source(true))&&!provider.progress.empty());
    ResourceStage previous=ResourceStage::Header;uint32_t completed=0,total=1;unsigned phases=1;
    for(const auto& event:provider.progress){
        CHECK(event.stage==ResourceStage::Header||event.stage==ResourceStage::Directory||event.stage==ResourceStage::Ready);
        CHECK(unsigned(event.stage)>=unsigned(previous)&&event.completed<=event.total);
        if(event.stage==previous){CHECK(event.total==total&&event.completed>=completed);}
        else {CHECK(completed==total);++phases;}
        previous=event.stage;completed=event.completed;total=event.total;
    }
    CHECK(phases==3&&previous==ResourceStage::Ready&&completed==1&&total==1);
    CHECK(provider.tracked_calls==0&&provider.tracked_bytes==0);
    CHECK(content->image_cache_usage().entries==0&&!content->image_cached(1));
    provider.progress.clear();provider.fail_first=p.group_offsets[0]+32;provider.fail_last=provider.fail_first+1;
    CHECK(content->open(provider.source(true))&&content->healthy());CHECK(provider.failed==0);
    CHECK(!content->image(1).pixels&&!content->healthy());CHECK(provider.failed>0);
    provider.fail_first=0;provider.fail_last=1;provider.progress.clear();
    CHECK(!content->open(provider.source(true))&&!content->healthy());
    for(const auto& event:provider.progress)CHECK(event.stage!=ResourceStage::Ready);
}
void empty_image_pool(){
    // Drive the real public API beyond the distinct one-page key capacity,
    // then clear that owner. This gives allocation tests a known empty pool
    // without exposing cache internals or relying on earlier test ordering.
    Package p;p.per_group=680;
    const unsigned count=ImageEntryCapacity+44;
    std::vector<std::vector<unsigned>> plans((count+49)/50);
    for(unsigned i=0;i<count;++i){p.frames.push_back(frame(1,1,Pattern::Alpha,900+i));plans[i/50].push_back(i);}
    p.collect();p.make_groups(plans);p.encode();Provider provider{&p.bytes};auto content=std::make_unique<Content>();
    CHECK(content->open(provider.source()));
    for(unsigned i=0;i<p.frames.size();++i)picture(*content,p.image_id(i),p.frames[i]);
    CHECK(!content->open(nullptr,0));CHECK(!content->healthy());
}
void image_pool_cycles_and_fragmentation(){
    label="small images share the pixel pool without premature key eviction";
    empty_image_pool();Package small;small.per_group=680;std::vector<std::vector<unsigned>> small_plans((ImageEntryCapacity+63)/64);
    for(unsigned i=0;i<ImageEntryCapacity;++i){small.frames.push_back(frame(1,1,Pattern::Alpha,1200+i));small_plans[i/64].push_back(i);}
    small.collect();small.make_groups(small_plans);small.encode();
    auto small_content=std::make_unique<Content>();Provider small_source{&small.bytes};
    CHECK(small_content->open(small_source.source()));
    std::vector<Picture> small_kept;
    // Explicitly load the shared legacy card before filling the key limit.
    // Directory-only opening does not populate the decoded picture cache.
    CHECK(small_content->image(0).pixels);
    for(unsigned i=0;i<ImageEntryCapacity-1;++i){picture(*small_content,small.image_id(i),small.frames[i]);small_kept.push_back(small_content->image(small.image_id(i)));}
    const auto small_warm=small_content->read_stats();
    for(unsigned i=0;i<small_kept.size();++i)CHECK(small_content->image(small.image_id(i)).pixels==small_kept[i].pixels);
    CHECK(same_reads(small_warm,small_content->read_stats()));
    // Refresh the card and first frame. The next key must evict frame one,
    // preserve all other views, and still serve an exact-descriptor alias.
    CHECK(small_content->image(0).pixels);CHECK(small_content->image(small.image_id(0)).pixels==small_kept[0].pixels);
    CHECK(small_content->image_cached(small.image_id(1)));
    CHECK(small_content->image_cache_pages(small.image_id(1))==1);
    CHECK(small_content->image_alias(small.image_id(1),small.image_id(1)));
    // Planning queries must not refresh the oldest entry's LRU age.
    picture(*small_content,small.image_id(ImageEntryCapacity-1),small.frames.back());
    const auto small_survivors=small_content->read_stats();
    for(unsigned i:{0u,2u,127u,ImageEntryCapacity-2}){
        CHECK(small_content->image(small.image_id(i)).pixels==small_kept[i].pixels);
        CHECK(!std::memcmp(small_kept[i].pixels,small.frames[i].expected.data(),small.frames[i].expected.size()));
    }
    CHECK(small_content->image(small.alias_id()).pixels==small_kept[0].pixels);
    CHECK(same_reads(small_survivors,small_content->read_stats()));
    picture(*small_content,small.image_id(1),small.frames[1]);
    CHECK(!same_reads(small_survivors,small_content->read_stats()));CHECK(!small_source.bad_range);

    label="23 medium frames retain a complete loop";
    Package cycle;cycle.per_group=680;std::vector<unsigned> all;
    for(unsigned i=0;i<23;++i){cycle.frames.push_back(frame(320,218,Pattern::Mixed,100+i));all.push_back(i);}
    cycle.collect();cycle.make_groups({all});cycle.encode();
    for(const Frame&f:cycle.frames)CHECK(f.expected.size()>32768&&f.expected.size()<=131072);
    auto content=std::make_unique<Content>();Provider source{&cycle.bytes};
    for(bool ranged:{false,true}){
        empty_image_pool();CHECK(ranged?content->open(source.source()):content->open(cycle.bytes.data(),cycle.bytes.size()));
        std::vector<Picture> kept;
        for(unsigned i=0;i<cycle.frames.size();++i){picture(*content,cycle.image_id(i),cycle.frames[i]);kept.push_back(content->image(cycle.image_id(i)));}
        const auto before=content->read_stats();
        for(unsigned i=0;i<cycle.frames.size();++i){
            const Picture p=content->image(cycle.image_id(i));CHECK(p.pixels==kept[i].pixels);
            CHECK(!std::memcmp(p.pixels,cycle.frames[i].expected.data(),cycle.frames[i].expected.size()));
        }
        CHECK(content->image(cycle.alias_id()).pixels==kept[0].pixels);
        CHECK(same_reads(before,content->read_stats()));CHECK(content->healthy()&&!source.bad_range);
    }

    label="contiguous allocation evicts LRU holes without moving survivors";
    empty_image_pool();Package mixed;mixed.per_group=680;std::vector<std::vector<unsigned>> plans;
    const unsigned pairs=(Content::image_capacity_pages()-9)/128;
    for(unsigned i=0;i<pairs;++i){mixed.frames.push_back(frame(160,218,Pattern::ManySpans,200+i));plans.push_back({i});}
    mixed.frames.push_back(frame(320,240,Pattern::AlphaSpans,777));plans.push_back({pairs});
    CHECK(mixed.frames[0].expected.size()==262036&&mixed.frames[pairs].expected.size()==ImageBound);
    mixed.collect();mixed.make_groups(plans);mixed.encode();
    auto holes=std::make_unique<Content>(),survivors=std::make_unique<Content>(),large=std::make_unique<Content>();
    Provider a{&mixed.bytes},b{&mixed.bytes},c{&mixed.bytes};
    CHECK(holes->open(a.source()));CHECK(survivors->open(b.source()));CHECK(large->open(c.source()));
    CHECK(holes->image(0).pixels&&survivors->image(0).pixels&&large->image(0).pixels);
    std::vector<Picture> kept;
    for(unsigned i=0;i<pairs;++i){picture(*holes,mixed.image_id(i),mixed.frames[i]);picture(*survivors,mixed.image_id(i),mixed.frames[i]);kept.push_back(survivors->image(mixed.image_id(i)));}
    // Interleaved 64-page frames plus three 3-page card images leave a
    // sub-151-page tail. Reopening one owner frees separated 64-page holes.
    // A 151-page maximum image needs eviction/coalescing despite enough total
    // free pages. Refreshing the first survivor makes the second the victim.
    CHECK(holes->open(a.source()));CHECK(holes->image(0).pixels);const auto before_refresh=survivors->read_stats();
    CHECK(survivors->image(mixed.image_id(0)).pixels==kept[0].pixels);
    CHECK(same_reads(before_refresh,survivors->read_stats()));
    picture(*large,mixed.image_id(pairs),mixed.frames[pairs]);const Picture largest=large->image(mixed.image_id(pairs));
    const auto before_survivors=survivors->read_stats();
    for(unsigned i:{0u,2u,3u,4u}){
        CHECK(!std::memcmp(kept[i].pixels,mixed.frames[i].expected.data(),mixed.frames[i].expected.size()));
        CHECK(survivors->image(mixed.image_id(i)).pixels==kept[i].pixels);
    }
    CHECK(same_reads(before_survivors,survivors->read_stats()));
    picture(*survivors,mixed.image_id(1),mixed.frames[1]);
    CHECK(!same_reads(before_survivors,survivors->read_stats()));
    CHECK(!std::memcmp(largest.pixels,mixed.frames[pairs].expected.data(),mixed.frames[pairs].expected.size()));
    CHECK(!a.bad_range&&!b.bad_range&&!c.bad_range);

    label="failed large reservations release pages and never publish a key";
    auto faulty=std::make_unique<Content>();Provider fault{&mixed.bytes};
    // Enough abandoned 151-page reservations exceed the complete image pool
    // if an error path leaks its destination. Corruption is injected
    // only after successful structural opening of the immutable fixture.
    for(unsigned attempt=0;attempt<Content::image_capacity_pages()/151+2;++attempt){
        fault.fail_first=fault.fail_last=0;CHECK(faulty->open(fault.source()));
        const unsigned group=mixed.frames[pairs].ids[0]/mixed.per_group;
        fault.fail_first=mixed.group_offsets[group]+32;fault.fail_last=fault.fail_first+1;
        const unsigned before=fault.failed;CHECK(!faulty->image(mixed.image_id(pairs)).pixels);
        CHECK(fault.failed>before&&!faulty->healthy());CHECK(!faulty->image(0).pixels&&!faulty->glyph('?',10).pixels);
    }
    fault.fail_first=fault.fail_last=0;CHECK(faulty->open(fault.source()));
    picture(*faulty,mixed.image_id(pairs),mixed.frames[pairs]);CHECK(!fault.bad_range);
}
void valid_packages(){
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();
    for(unsigned per:{1u,85u,170u,680u}){
        label="valid group size "+std::to_string(per);Package p=basic(per);Provider provider{&p.bytes};
        CHECK(direct->open(p.bytes.data(),p.bytes.size()));CHECK(range->open(provider.source()));
        CHECK(direct->current_profile()&&range->current_profile());CHECK(direct->card_count()==CardKinds&&range->card_count()==CardKinds);
        const auto before_direct=direct->read_stats(),before_range=range->read_stats();
        CHECK(direct->image_cached(0)&&!range->image_cached(0));
        CHECK(!direct->image_cached(1)&&!range->image_cached(1));
        CHECK(direct->image_cache_pages(0)==0&&range->image_cache_pages(0)==3);
        for(unsigned i=0;i<p.frames.size();++i){
            const unsigned pages=(unsigned(p.frames[i].expected.size())+4095)/4096;
            CHECK(direct->image_cache_pages(p.image_id(i))>=pages&&range->image_cache_pages(p.image_id(i))>=pages);
        }
        CHECK(direct->image_alias(1,p.alias_id())&&range->image_alias(1,p.alias_id()));
        CHECK(!direct->image_alias(1,2)&&!range->image_alias(-1,1));
        CHECK(!range->image_cached(-1)&&range->image_cache_pages(p.images)==0);
        CHECK(direct->image_cache_usage().entries==0&&range->image_cache_usage().entries==0);
        CHECK(same_reads(before_direct,direct->read_stats())&&same_reads(before_range,range->read_stats()));
        CHECK(p.descriptors[1].offset%2==1); // Deliberately unaligned record fields.
        for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);
            const unsigned pages=(unsigned(p.frames[i].expected.size())+4095)/4096;
            CHECK(direct->image_cache_pages(p.image_id(i))==pages&&range->image_cache_pages(p.image_id(i))==pages);
        }
        const Picture a=direct->image(1),b=range->image(1);const auto da=direct->read_stats(),ra=range->read_stats();
        CHECK(direct->image(int(p.alias_id())).pixels==a.pixels);CHECK(range->image(int(p.alias_id())).pixels==b.pixels);
        CHECK(direct->read_stats().direct_reads==da.direct_reads&&range->read_stats().callbacks==ra.callbacks);
        CHECK(!provider.bad_range&&provider.max_read<=EncodedBound);
        CHECK(range->glyph('?',10).pixels&&range->glyph('?',10).pixels[0]==0xab);
        CHECK(range->card_image(0)==0&&!std::strcmp(range->string(0),""));
    }
    for(unsigned per:{1u,85u,170u,680u}){
        label="full and final partial tile groups "+std::to_string(per);Package p;p.per_group=per;
        p.frames.push_back(frame(16,16,Pattern::Opaque,7));p.collect();const Bytes tile=p.tiles[0];
        p.tiles.resize(per+1,blank_tile());p.tiles.back()=tile;p.frames[0].ids[0]=per;
        p.make_groups({{0}});p.encode();Provider provider{&p.bytes};
        CHECK(direct->open(p.bytes.data(),p.bytes.size()));CHECK(range->open(provider.source()));
        picture(*direct,1,p.frames[0]);picture(*range,1,p.frames[0]);CHECK(p.tile_groups()==2);
    }
    for(unsigned extra:{1u,60u}){
        label="complete unreferenced index records "+std::to_string(extra);Package p=basic();
        Group&g=p.groups[p.tile_groups()];const Bytes unused=p.frames[3].record();
        for(unsigned i=0;i<extra;++i)g.raw.insert(g.raw.end(),unused.begin(),unused.end());
        g.items+=extra;p.encode();
        Provider provider{&p.bytes};CHECK(direct->open(p.bytes.data(),p.bytes.size()));CHECK(range->open(provider.source()));
        for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);}
    }
    label="complete legacy v14 remains current";Package old=basic();old.encode(true);Provider legacy{&old.bytes};
    CHECK(direct->open(old.bytes.data(),old.bytes.size()));CHECK(range->open(legacy.source()));
    CHECK(direct->current_profile()&&range->current_profile());CHECK(direct->image(1).format==0&&range->image(1).format==0);
}
struct Rejector {
    std::unique_ptr<Content> direct=std::make_unique<Content>(),range=std::make_unique<Content>();
    void operator()(const char*name,Bytes bytes){
        label=name;seal(bytes);Provider provider{&bytes};
        CHECK(!direct->open(bytes.data(),bytes.size()));CHECK(!range->open(provider.source()));
        CHECK(!direct->healthy()&&!range->healthy());CHECK(!direct->card_count()&&!range->card_count());
        CHECK(!direct->image(1).pixels&&!range->image(1).pixels);CHECK(!direct->glyph('?',10).pixels&&!range->glyph('?',10).pixels);
        CHECK(!provider.bad_range);
    }
};
struct ImageRejector {
    std::unique_ptr<Content> direct=std::make_unique<Content>(),range=std::make_unique<Content>();
    void operator()(const char*name,Bytes bytes,unsigned image=1,int first=-1){
        label=name;seal(bytes);Provider provider{&bytes};
        // Only the directory is needed at open. A selected descriptor,
        // geometry or decode failure is handled by the actual image request.
        CHECK(direct->open(bytes.data(),bytes.size()));CHECK(range->open(provider.source()));
        CHECK(direct->healthy()&&range->healthy());CHECK(direct->current_profile()&&range->current_profile());
        CHECK(direct->image(0).pixels&&range->image(0).pixels);
        CHECK(direct->glyph('?',10).pixels&&range->glyph('?',10).pixels);
        if(first>=0){CHECK(direct->image(first).pixels&&range->image(first).pixels);CHECK(direct->healthy()&&range->healthy());}
        CHECK(!direct->image(int(image)).pixels&&!range->image(int(image)).pixels);
        CHECK(!direct->healthy()&&!range->healthy());
        CHECK(!direct->image(0).pixels&&!range->image(0).pixels);
        CHECK(!direct->glyph('?',10).pixels&&!range->glyph('?',10).pixels);
        CHECK(!*direct->string(0)&&!*range->string(0));CHECK(!provider.bad_range);
    }
};
void accept_frames(const char*name,Bytes bytes,const Package&p){
    label=name;seal(bytes);Provider provider{&bytes};auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();
    CHECK(direct->open(bytes.data(),bytes.size()));CHECK(range->open(provider.source()));
    for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);}
    CHECK(!provider.bad_range);
}
void native_ui_packages(){
    label="native UI v16 preserves tile and historical catalog access";
    Package p=basic();p.encode(false,NativeUiAuxiliaryCount,NativeUiContentVersion);
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();Provider provider{&p.bytes};
    CHECK(direct->open(p.bytes.data(),p.bytes.size())&&range->open(provider.source()));
    CHECK(direct->current_profile()&&range->current_profile());
    for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);}
    for(int key:{int(U_HealthBarBackground),int(U_HealthBarStroke),int(U_HealthBarMask),int(U_HealthBarFill),
        int(U_HealthBarInfinity),int(U_HealthBarBlock),int(U_HoverTip),int(U_HoverTipDebuff),
        int(U_PauseButton),int(U_PauseButtonHover),int(U_EndTurnNative),int(U_EndTurnHover),int(U_EndTurnGlow)}){
        CHECK(direct->ui_image(key)==0&&range->ui_image(key)==0);
    }
    for(const char*key:{"HEALTH_BAR_BG","HEALTH_BAR_STROKE","HEALTH_BAR_MASK","HEALTH_BAR_FILL",
        "HEALTH_BAR_INFINITY","HEALTH_BAR_BLOCK","HOVER_TIP","HOVER_TIP_DEBUFF","PAUSE_BUTTON",
        "PAUSE_BUTTON_HOVER","END_TURN","END_TURN_HOVER","END_TURN_GLOW"}){
        CHECK(direct->world_image("common",key)==0&&range->world_image("common",key)==0);
    }
    CHECK(direct->ui_image(NativeUiAuxiliaryCount)==-1&&range->ui_image(NativeUiAuxiliaryCount)==-1);
    CHECK(range->glyph('?',10).pixels&&range->glyph('?',10).pixels[0]==0xab);
    CHECK(direct->card_image(0)==0&&range->card_image(0)==0&&!std::strcmp(range->string(0),""));
    CHECK(!provider.bad_range&&provider.max_read<=EncodedBound);

    Rejector reject;
    Package complete=basic();complete.encode(false,NativeUiAuxiliaryCount,NativeUiContentVersion,NativeUiStringCount);
    const unsigned textdir=40+complete.images*16+complete.glyphs*16;
    for(unsigned i=0;i<NativeUiStringCount-WorldStringCount;++i){
        put(complete.bytes,textdir+(WorldStringCount+i)*8,unsigned(complete.bytes.size()));
        put(complete.bytes,textdir+(WorldStringCount+i)*8+4,1);
        complete.bytes.push_back(uint8_t('A'+i));complete.bytes.push_back(0);
    }
    seal(complete.bytes);Provider with_text{&complete.bytes};
    CHECK(direct->open(complete.bytes.data(),complete.bytes.size())&&range->open(with_text.source()));
    CHECK(direct->current_profile()&&range->current_profile());
    const char*orb_keys[]={"DARK_ORB.description","DARK_ORB.title","FROST_ORB.description","FROST_ORB.title",
        "GLASS_ORB.description","GLASS_ORB.title","LIGHTNING_ORB.description","LIGHTNING_ORB.title",
        "PLASMA_ORB.description","PLASMA_ORB.title"};
    for(unsigned i=0;i<10;++i){const char expected[]={char('A'+i),0};
        CHECK(!std::strcmp(direct->world_string("orbs",orb_keys[i]),expected));
        CHECK(!std::strcmp(range->world_string("orbs",orb_keys[i]),expected));
    }
    picture(*direct,1,complete.frames[0]);picture(*range,1,complete.frames[0]);
    CHECK(!with_text.bad_range);
    for(unsigned count:{unsigned(NativeUiStringCount)-1,unsigned(NativeUiStringCount)+1}){
        Package bad=basic();bad.encode(false,NativeUiAuxiliaryCount,NativeUiContentVersion,count);
        reject("v16 rejects a partial or oversized text suffix",std::move(bad.bytes));
    }
    {Package bad=basic();bad.encode(false,U_Count,TileContentVersion,NativeUiStringCount);
        reject("v15 does not accept the optional v16 text suffix",std::move(bad.bytes));}
    for(unsigned count:{unsigned(U_Count),unsigned(NativeUiAuxiliaryCount)-1,unsigned(NativeUiAuxiliaryCount)+1}){
        Package bad=basic();bad.encode(false,count,NativeUiContentVersion);
        reject("v16 rejects a missing, short or long auxiliary suffix",std::move(bad.bytes));
    }
    Bytes bad=p.bytes;put(bad,p.base-4,p.images);Provider invalid_skin{&bad};
    CHECK(direct->open(bad.data(),bad.size())&&range->open(invalid_skin.source()));
    CHECK(direct->ui_image(NativeUiAuxiliaryCount-1)==-1&&range->ui_image(NativeUiAuxiliaryCount-1)==-1);
    bad=p.bytes;put(bad,p.descriptor(0),p.base-(NativeUiAuxiliaryCount-U_Count)*4);
    ImageRejector reject_image;reject_image("v16 selected descriptor must point at relocated global header",std::move(bad));

    // Reusing the same Content instance must remove the optional skins when
    // a historical owner pack is opened, with no stale v16 image binding.
    for(bool legacy:{false,true}){
        label=legacy?"v16 to v14 fallback":"v16 to v15 fallback";Package old=basic();old.encode(legacy);
        Provider historical{&old.bytes};CHECK(direct->open(old.bytes.data(),old.bytes.size())&&range->open(historical.source()));
        CHECK(direct->current_profile()&&range->current_profile());
        CHECK(!*direct->world_string("orbs","LIGHTNING_ORB.title")&&!*range->world_string("orbs","LIGHTNING_ORB.title"));
        CHECK(direct->world_image("common","HEALTH_BAR_BG")==-1&&range->world_image("common","HEALTH_BAR_FILL")==-1);
        CHECK(direct->ui_image(U_HealthBarBackground)==-1&&range->ui_image(U_HealthBarBackground)==-1);
        if(!legacy){picture(*direct,1,old.frames[0]);picture(*range,1,old.frames[0]);}
        else CHECK(direct->image(1).format==0&&range->image(1).format==0);
        CHECK(!historical.bad_range);
    }
}
void locale_packages(){
    label="v17 locale profile preserves exact image and tile behavior";
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();Rejector reject;
    const auto marker=[](Package&p,const char*text,unsigned length){
        const unsigned at=40+p.images*16+p.glyphs*16+LocaleMarkerString*8;
        put(p.bytes,at,unsigned(p.bytes.size()));put(p.bytes,at+4,length);
        p.bytes.insert(p.bytes.end(),text,text+length);p.bytes.push_back(0);seal(p.bytes);
    };
    for(const char*locale:{"zhs","eng"}){
        Package p=basic();p.encode(false,LocaleAuxiliaryCount,LocaleContentVersion,LocaleStringCount);marker(p,locale,3);
        Provider provider{&p.bytes};CHECK(direct->open(p.bytes.data(),p.bytes.size())&&range->open(provider.source()));
        CHECK(direct->current_profile()&&range->current_profile()&&range->version()==LocaleContentVersion);
        CHECK(direct->english()==(locale[0]=='e')&&range->english()==(locale[0]=='e'));
        CHECK(!std::strcmp(range->string(LocaleMarkerString),locale));
        CHECK(direct->world_image("common","HEALTH_BAR_BG")==0&&range->world_image("common","END_TURN")==0);
        CHECK(range->ui_image(LocaleAuxiliaryCount-1)==0&&range->ui_image(LocaleAuxiliaryCount)==-1);
        for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);}
        CHECK(!provider.bad_range);
    }
    for(const char*bad:{"","en","engx","zxx","ENG"}){
        Package p=basic();p.encode(false,LocaleAuxiliaryCount,LocaleContentVersion,LocaleStringCount);marker(p,bad,unsigned(std::strlen(bad)));
        reject("v17 rejects absent, partial, unsupported or oversized marker",std::move(p.bytes));
    }
    {Package p=basic();p.encode(false,LocaleAuxiliaryCount,LocaleContentVersion,LocaleStringCount);
        const char embedded[]={'e',0,'g'};marker(p,embedded,3);reject("v17 rejects embedded NUL locale marker",std::move(p.bytes));}
    for(unsigned count:{LocaleStringCount-1,LocaleStringCount+1}){
        Package p=basic();p.encode(false,LocaleAuxiliaryCount,LocaleContentVersion,count);marker(p,"eng",3);
        reject("v17 rejects noncanonical locale string directory",std::move(p.bytes));
    }
    for(unsigned count:{unsigned(NativeUiAuxiliaryCount),LocaleAuxiliaryCount-1,LocaleAuxiliaryCount+1}){
        Package p=basic();p.encode(false,count,LocaleContentVersion,LocaleStringCount);marker(p,"eng",3);
        reject("v17 rejects noncanonical HUD directory",std::move(p.bytes));
    }
}
void bestiary_supplement_packages(){
    // The extended clip directory shifts cards, UI and tile metadata. Exercise
    // both readers against an independently written synthetic directory and
    // retain the previous v33 catalog as a compatible, smaller profile.
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();Rejector reject;
    for(unsigned version:{unsigned(GroupedStringContentVersion),unsigned(BestiaryKaiserContentVersion),unsigned(SceneFontContentVersion),unsigned(RetainedFilterContentVersion),unsigned(SpeechBubbleContentVersion),unsigned(HistoryHeaderContentVersion),unsigned(HistoryBodyContentVersion)})for(const char*locale:{"eng","zhs"}){
        label=version>=BestiaryKaiserContentVersion?"v34 sparse Bestiary clip append":"v33 historical catalog remains readable";
        const unsigned animations=version>=BestiaryKaiserContentVersion?BestiaryKaiserAnimationKinds:CharacterDeathAnimationKinds;
        Package p=basic();
        if(version>=SceneFontContentVersion)p.glyphs=32769;
        p.encode(false,InspectionAuxiliaryCount,version,Ui7StringCount,animations);
        const unsigned strings=40+p.images*16+p.glyphs*16,clips=strings+Ui7StringCount*8;
        put(p.bytes,strings+LocaleMarkerString*8,unsigned(p.bytes.size()));put(p.bytes,strings+LocaleMarkerString*8+4,3);
        p.bytes.insert(p.bytes.end(),locale,locale+4);seal(p.bytes);
        if(version>=BestiaryKaiserContentVersion)for(unsigned i=0;i<18;++i){
            const unsigned at=clips+(BestiaryKaiserAnimationFirst+i)*16;
            put(p.bytes,at,1);put(p.bytes,at+8,uint16_t(-19),2);put(p.bytes,at+10,uint16_t(-23),2);
            put(p.bytes,at+12,unsigned(bestiary_kaiser::Sources[i])+1);
        }
        Provider source{&p.bytes};CHECK(direct->open(p.bytes.data(),p.bytes.size())&&range->open(source.source()));
        CHECK(direct->current_profile()&&range->current_profile());
        CHECK(direct->has_inspection_assets()&&range->has_inspection_assets());
        CHECK(direct->english()==(locale[0]=='e')&&range->english()==(locale[0]=='e'));
        if(version>=SceneFontContentVersion){
            // Exercise an actual key beyond the old directory bound, through
            // both providers, rather than merely accepting a header count.
            for(Content* content:{direct.get(),range.get()}){
                const Glyph last=content->glyph('?' + p.glyphs-1,10);
                CHECK(last.w==1&&last.h==1&&last.advance==1&&last.pixels&&last.pixels[0]==0xab);
            }
            Bytes old=p.bytes;put(old,4,BestiaryKaiserContentVersion);
            reject("v34 retains its historical glyph bound",std::move(old));
            Bytes too_large=p.bytes;put(too_large,16,65535);put(too_large,20,65535);
            reject("v35 retains the fixed directory byte bound",std::move(too_large));
        }
        for(Content*content:{direct.get(),range.get()}){
            const Clip old=content->clip(CharacterDeathAnimationKinds-1);CHECK(old.count==1&&old.frame_ms==33&&old.sprite_units==0);
            CHECK(content->ui_image(InspectionAuxiliaryCount-1)==0&&content->ui_image(InspectionAuxiliaryCount)==-1);
            if(version>=BestiaryKaiserContentVersion)for(unsigned i=0;i<18;++i){
                const Clip added=content->clip(BestiaryKaiserAnimationFirst+i);
                CHECK(added.first==1&&added.count==1&&added.frame_ms==33);
                CHECK(added.pivot_x==-19&&added.pivot_y==-23&&added.sprite_units==unsigned(bestiary_kaiser::Sources[i])+1);
                picture(*content,added.first,p.frames[0]);
            }
        }
        picture(*direct,1,p.frames[0]);picture(*range,1,p.frames[0]);CHECK(!source.bad_range);
        for(unsigned wrong:{animations-1,animations+1}){
            Bytes bad=p.bytes;put(bad,28,wrong);reject("version requires its exact animation directory",std::move(bad));
        }
        Bytes bad=p.bytes;put(bad,4,Ui12ContentVersion+1);reject("unknown future profile remains rejected",std::move(bad));
        bad=p.bytes;put(bad,20,scene_font_glyph_limit(version)+1);reject("Versioned glyph count bound",std::move(bad));
    }
}
void ui13_source_catalogs(){
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();Rejector reject;
    const auto pack=[](unsigned auxiliary){
        Package p=basic();p.encode(false,auxiliary,Ui12ContentVersion,Ui7StringCount,BestiaryKaiserAnimationKinds);
        const unsigned strings=40+p.images*16+p.glyphs*16;
        put(p.bytes,strings+LocaleMarkerString*8,unsigned(p.bytes.size()));put(p.bytes,strings+LocaleMarkerString*8+4,3);
        p.bytes.insert(p.bytes.end(),{'e','n','g',0});seal(p.bytes);return p;
    };
    for(unsigned end:{unsigned(Ui12AuxiliaryCount),IntroLogoAuxiliaryCount,PhobiaSwarmAuxiliaryCount,
            SleepingVfxAuxiliaryCount,FrontendUi13AuxiliaryCount,RoomsUi13AuxiliaryCount,
            ArchitectUi13AuxiliaryCount,ArchitectAttackUi13AuxiliaryCount,DoomUi13AuxiliaryCount,
            RandomCharacterUi13AuxiliaryCount,FakeMerchantBaseUi13AuxiliaryCount,FakeMerchantUi13AuxiliaryCount,
            unsigned(CombatUi14AuxiliaryCount),MerchantUi14AuxiliaryCount}){
        label="UI40 complete optional source catalog";Package p=pack(end);Provider source{&p.bytes};
        CHECK(direct->open(p.bytes.data(),p.bytes.size())&&range->open(source.source()));
        for(Content*content:{direct.get(),range.get()}){
            // App::init accepts only a complete playable profile. New suffixes
            // retain every earlier capability, rather than only opening as data.
            CHECK(content->current_profile());
            CHECK(content->has_ftue_assets()&&content->has_selection_assets()&&
                  content->has_inspection_assets()&&content->has_speech_bubble_assets()&&
                  content->has_history_header_assets()&&content->has_settings_ui10_assets());
            CHECK(content->has_startup_logo_assets()==(end>=IntroLogoAuxiliaryCount));
            CHECK(content->has_phobia_swarm_assets()==(end>=PhobiaSwarmAuxiliaryCount));
            CHECK(content->has_sleeping_vfx_assets()==(end>=SleepingVfxAuxiliaryCount));
            CHECK(content->has_ui13_assets()==(end>=FrontendUi13AuxiliaryCount));
            CHECK(content->ui_image(end-1)==0&&content->ui_image(end)==-1);
            picture(*content,1,p.frames[0]);CHECK(content->healthy());
        }
        CHECK(!source.bad_range);
    }
    for(unsigned end:{FrontendUi13AuxiliaryCount,RoomsUi13AuxiliaryCount,
            ArchitectUi13AuxiliaryCount,ArchitectAttackUi13AuxiliaryCount,DoomUi13AuxiliaryCount,
            RandomCharacterUi13AuxiliaryCount,FakeMerchantBaseUi13AuxiliaryCount,FakeMerchantUi13AuxiliaryCount,
            unsigned(CombatUi14AuxiliaryCount),MerchantUi14AuxiliaryCount}){
        Package incomplete=pack(end-1);reject("Incomplete source catalog is not accepted",std::move(incomplete.bytes));
        Package wrong=pack(end);put(wrong.bytes,wrong.base-4,wrong.images);
        reject("Source catalog references must stay inside the image directory",std::move(wrong.bytes));
    }
    Package extra=pack(MerchantUi14AuxiliaryCount+1);
    reject("Unknown source catalog suffix is not scanned",std::move(extra.bytes));
}
void malformed_container(){
    label="container setup";const Package p=basic();Rejector reject;ImageRejector reject_image;
    const auto field=[&](const char*name,size_t at,uint32_t value,unsigned width=4){Bytes b=p.bytes;put(b,at,value,width);reject(name,std::move(b));};
    const auto image_field=[&](const char*name,size_t at,uint32_t value,unsigned width=4){Bytes b=p.bytes;put(b,at,value,width);reject_image(name,std::move(b));};
    image_field("v14 cannot decode tile format",4,14);field("v16 requires its extended auxiliary directory",4,16);field("unsupported pack version",4,22);
    struct Mutation {unsigned offset,value;const char*name;};
    const Mutation header[]={
        {0,0,"codec magic"},{4,2,"codec version"},{8,8,"tile side"},{12,16,"dictionary property"},
        {16,0,"zero tile count"},{16,~0u,"tile count overflow"},{20,0,"zero tile groups"},{20,2,"inconsistent tile groups"},
        {24,0,"zero index groups"},{24,5,"index groups exceed frames"},{28,0,"zero descriptors"},{28,161,"descriptor count exceeds images"},
        {32,p.prefix-16,"descriptor overlaps prefix"},{32,p.descriptor_base+1,"unaligned descriptor base"},{32,~0u,"descriptor range overflow"},
        {36,p.table+8,"lookup not immediately after header"},{36,~0u,"lookup range overflow"},{40,769,"tile record length"},
        {44,0,"zero tiles per group"},{44,681,"oversize tiles per group"},{48,GroupBound-1,"raw group bound"},
        {52,EncodedBound+1,"encoded group bound"}
    };
    for(const auto&m:header)field(m.name,p.base+m.offset,m.value);
    Bytes oversized=p.bytes;const unsigned many_groups=(DirectoryBound-p.table)/8+1;
    const unsigned oversized_prefix=p.table+(many_groups+1)*8;oversized.resize(oversized_prefix+4096);
    put(oversized,p.base+16,many_groups);put(oversized,p.base+20,many_groups);
    put(oversized,p.base+32,oversized_prefix);put(oversized,p.base+44,1);
    reject("extended directory exceeds fixed resident budget",std::move(oversized));
    const size_t image=p.image_entry(1),desc=p.descriptor(0);
    image_field("image descriptor short",image+12,15);image_field("image descriptor long",image+12,17);
    image_field("image points into prefix",image,p.base);image_field("image points between descriptors",image,p.descriptor_base+2);
    image_field("image points after descriptors",image,p.descriptor_base+unsigned(p.frames.size())*16);
    image_field("descriptor wrong shared header",desc,p.base+2);image_field("descriptor index group range",desc+4,1);
    image_field("descriptor offset overflow",desc+8,~0u);image_field("descriptor offset inside record",desc+8,1);
    image_field("descriptor empty record",desc+12,0);image_field("descriptor short record",desc+12,p.descriptors[0].length-1);
    image_field("descriptor extra record byte",desc+12,p.descriptors[0].length+1);image_field("descriptor record overflow",desc+12,~0u);
    image_field("lookup odd group",p.table,p.group_offsets[0]+1);image_field("lookup overlaps prefix",p.table,p.base);
    image_field("lookup overlaps descriptors",p.table,p.descriptor_base);image_field("lookup offset overflow",p.table,~0u);
    image_field("lookup short group",p.table+4,32);image_field("lookup oversized group",p.table+4,EncodedBound+1);
    image_field("lookup groups overlap",p.table+8,p.group_offsets[0]+2);
    image_field("lookup groups out of order",p.table,p.group_offsets[1]);
    const unsigned g=p.group_offsets[0],index=p.group_offsets[p.tile_groups()];
    const Mutation group[]={
        {0,0,"group magic"},{4,2,"tile group wrong type"},{8,1,"group identity"},{12,0,"group empty raw"},
        {12,GroupBound+1,"group raw overflow"},{12,unsigned(p.groups[0].raw.size())-1,"tile raw record size"},
        {16,p.group_lengths[0]-33,"encoded length disagrees with lookup"},{24,p.groups[0].items+1,"tile item count"}
    };
    for(const auto&m:group)image_field(m.name,g+m.offset,m.value);
    image_field("index group wrong type",index+4,1);
    for(unsigned at:{36u,p.base+56,p.base+60,g+20,g+28,index+20,index+28}){
        Bytes b=p.bytes;put(b,at,0x81234567u);accept_frames("unused legacy checksum and reserved fields",std::move(b),p);
    }
    for(unsigned items:{0u,3u,5u,65u,~0u}){
        Bytes b=p.bytes;put(b,index+24,items);accept_frames("unused index item count does not scan unrelated records",std::move(b),p);
    }
    Package unused=basic(1);unused.tiles.push_back(blank_tile());unused.make_groups({{0,1,2,3}});unused.encode();
    put(unused.bytes,unused.group_offsets[unused.tile_groups()-1]+4,2);
    accept_frames("unrequested tile group is never parsed",unused.bytes,unused);
    // A decoded alias cannot authorize a selected image with incompatible
    // sparse geometry, dimensions or descriptor length.
    for(bool reverse:{false,true}){
        Bytes b=p.bytes;const unsigned bad=reverse?1:p.alias_id(),good=reverse?p.alias_id():1;
        put(b,p.image_entry(bad)+4,32,2);
        reject_image(reverse?"bad alias first":"bad alias last",b,bad);
        reject_image("cached alias retains the selected geometry bound",std::move(b),bad,int(good));
        b=p.bytes;put(b,p.image_entry(bad)+12,15);reject_image("short selected alias descriptor",std::move(b),bad);
    }
    for(bool trailing:{false,true}){
        Package q=p;q.groups[0].custom=true;q.groups[0].encoded=stored_lzma2(q.groups[0].raw);
        if(trailing)q.groups[0].encoded.push_back(0);else q.groups[0].encoded.pop_back();q.encode();
        reject_image(trailing?"selected decode rejects unused LZMA2 input":"selected decode requires LZMA2 end marker",std::move(q.bytes));
    }
}
void malformed_geometry(){
    label="geometry setup";const Package p=basic();ImageRejector reject_image;const unsigned index=p.tile_groups();
    const auto raw_field=[&](const char*name,unsigned group,size_t at,uint32_t value,unsigned width=4){
        Package q=p;put(q.groups[group].raw,at,value,width);q.encode();reject_image(name,std::move(q.bytes));
    };
    const Frame&f=p.frames[0];const unsigned start=p.descriptors[0].offset,geometry=start+12+unsigned(f.ids.size())*4;
    raw_field("zero original image bytes",index,start,0);raw_field("oversize original image",index,start,ImageBound+1);
    raw_field("geometry output length mismatch",index,start,unsigned(f.expected.size())+1);
    raw_field("wrong tile index byte count",index,start+4,unsigned(f.ids.size())*4-4);
    raw_field("tile index byte overflow",index,start+4,~0u);raw_field("zero geometry byte count",index,start+8,0);
    raw_field("geometry byte overflow",index,start+8,~0u);raw_field("active tile id out of range",index,start+12,unsigned(p.tiles.size()));
    raw_field("active tile id overflow",index,start+12,~0u);raw_field("too many row spans",index,geometry,f.w+1,2);
    const unsigned span=geometry+unsigned(f.span_offsets[0]),second=geometry+unsigned(f.span_offsets[1]);
    raw_field("zero span pixel count",index,span+2,0,2);raw_field("span mode",index,span+4,2,1);
    raw_field("span x overflow",index,span,65535,2);raw_field("span exceeds row",index,span+2,f.w+1,2);
    raw_field("overlapping spans",index,second,1,2);raw_field("unordered spans",index,second,0,2);
    Package tail=p;tail.groups[index].raw.push_back(0);tail.encode();accept_frames("unrequested terminal padding",tail.bytes,tail);
    Package hidden=p;Bytes hidden_record=p.frames[1].record();hidden_record[4]^=4;
    hidden.groups[index].raw.insert(hidden.groups[index].raw.end(),hidden_record.begin(),hidden_record.end());++hidden.groups[index].items;hidden.encode();
    accept_frames("unrequested index record structure",hidden.bytes,hidden);
    Package unused_id=p;Bytes out_of_range=p.frames[3].record();put(out_of_range,12,unsigned(p.tiles.size()));
    unused_id.groups[index].raw.insert(unused_id.groups[index].raw.end(),out_of_range.begin(),out_of_range.end());
    ++unused_id.groups[index].items;unused_id.encode();accept_frames("unrequested index record tile ID",unused_id.bytes,unused_id);
    const auto ignored_tile_field=[&](const char*name,unsigned tile,unsigned at,unsigned value){
        Package q=p;put(q.groups[tile/p.per_group].raw,(tile%p.per_group)*TileBytes+at,value,1);
        q.encode();accept_frames(name,q.bytes,q);
    };
    ignored_tile_field("unused tile width",f.ids[0],0,0);ignored_tile_field("unused tile height",f.ids[0],1,17);
    ignored_tile_field("unused right edge metadata",f.ids[2],0,2);ignored_tile_field("unused bottom edge metadata",f.ids[3],1,2);
    ignored_tile_field("unused RGB right padding",f.ids[2],2+2,1);ignored_tile_field("unused alpha right padding",f.ids[2],514+1,1);
    ignored_tile_field("unused RGB bottom padding",f.ids[3],2+16*2,1);ignored_tile_field("unused alpha bottom padding",f.ids[3],514+16,1);
    ignored_tile_field("opaque spans do not consume alpha",p.frames[2].ids[0],514,254);
    ignored_tile_field("inactive tile metadata",p.frames[3].ids[0],1,3);
    Package inactive=p;put(inactive.groups[index].raw,p.descriptors[3].offset+12,~0u);inactive.encode();
    accept_frames("inactive tile IDs are not addressed",inactive.bytes,inactive);
}
void catalog_metadata_reuse(){
    label="large descriptor catalog reads only selected records";
    Package p;p.images=20000;p.reserved_descriptors=19000;
    std::vector<unsigned> reversed;
    for(unsigned i=0;i<64;++i){p.frames.push_back(frame(20+i%13,4+i%8,Pattern::AlphaSpans,i));reversed.push_back(63-i);}
    p.collect();p.make_groups({reversed});p.encode();
    Provider source{&p.bytes};source.tracked_first=p.descriptor_base;
    source.tracked_last=p.descriptor_base+p.reserved_descriptors*16;
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();
    CHECK(direct->open(p.bytes.data(),p.bytes.size()));CHECK(range->open(source.source(true)));
    // Only a boundary page around the global header may overlap descriptors.
    // The descriptor catalog is not copied or traversed at startup.
    CHECK(source.tracked_bytes<=2*ResourceReader::PageSize&&source.tracked_calls<=2);
    for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);}
    CHECK(!source.bad_range);
    const unsigned unused=p.reserved_descriptors-1;
    for(unsigned field:{0u,8u,12u}){
        Bytes bad=p.bytes;put(bad,p.descriptor(unused)+field,~0u);
        accept_frames("unused descriptor content is not parsed",std::move(bad),p);
    }
    const unsigned alias=p.alias_id()+1;
    const auto alias_shape=[&](unsigned descriptor,unsigned width,unsigned height){
        Bytes bytes=p.bytes;const size_t at=p.image_entry(alias);
        put(bytes,at,unsigned(p.descriptor(descriptor)));put(bytes,at+4,width,2);put(bytes,at+6,height,2);
        put(bytes,at+8,6);put(bytes,at+12,16);return bytes;
    };
    ImageRejector reject;
    reject("selected record keeps width bounds",alias_shape(unused,19,4),alias,1);
    reject("selected record keeps row bounds",alias_shape(unused,20,3),alias,1);
    reject("selected record keeps tile-grid bounds",alias_shape(unused,33,4),alias,1);
    reject("shared descriptors keep independent geometry checks",alias_shape(0,19,4),alias,1);
    Bytes wider=alias_shape(unused,21,4);Provider wide_source{&wider};Frame expected=p.frames[0];expected.w=21;
    CHECK(direct->open(wider.data(),wider.size())&&range->open(wide_source.source()));
    picture(*direct,alias,expected);picture(*range,alias,expected);
    label="failed selected descriptor reads invalidate views and recover on reopen";
    Bytes selected=p.bytes;put(selected,p.image_entry(1),unsigned(p.descriptor(unused)));
    Provider fault{&selected};fault.fail_first=unsigned(p.descriptor(unused));fault.fail_last=fault.fail_first+16;
    CHECK(range->open(fault.source(true))&&range->healthy());CHECK(fault.failed==0);
    CHECK(!range->image(1).pixels&&!range->healthy());CHECK(fault.failed>0);
    CHECK(!range->glyph('?',10).pixels);
    fault.fail_last=0;fault.progress.clear();CHECK(range->open(fault.source(true)));
    picture(*range,1,p.frames[0]);CHECK(!fault.bad_range);
}
void maximum_frame_and_unused_tile(){
    label="full 320 by 240 sparse frame and six-page index";
    Package maximum;maximum.frames.push_back(frame(320,240,Pattern::ManySpans,17));
    maximum.collect();maximum.make_groups({{0}});maximum.encode();
    CHECK(maximum.frames[0].ids.size()==300&&maximum.frames[0].geometry.size()==384480);
    CHECK(maximum.frames[0].expected.size()==576480);
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();Provider full{&maximum.bytes};
    CHECK(direct->open(maximum.bytes.data(),maximum.bytes.size()));CHECK(range->open(full.source()));
    picture(*direct,1,maximum.frames[0]);picture(*range,1,maximum.frames[0]);CHECK(!full.bad_range);

    label="valid unused tile inside a referenced group";Package unused=basic();
    Bytes padding=blank_tile();padding[0]=15;unused.tiles.push_back(padding);
    unused.make_groups({{0,1,2,3}});unused.encode();Provider provider{&unused.bytes};
    CHECK(direct->open(unused.bytes.data(),unused.bytes.size()));CHECK(range->open(provider.source()));
    for(unsigned i=0;i<unused.frames.size();++i){picture(*direct,unused.image_id(i),unused.frames[i]);picture(*range,unused.image_id(i),unused.frames[i]);}
    // No selected geometry addresses this padding. Loading another tile
    // from the same group must not introduce a whole-group validation pass.
    const unsigned tile=unsigned(unused.tiles.size())-1;
    put(unused.groups[tile/unused.per_group].raw,(tile%unused.per_group)*TileBytes+2+15*2,1,1);
    unused.encode();accept_frames("unreferenced tile padding in a loaded group",unused.bytes,unused);
}
Package pressure(bool tiny){
    Package p;p.per_group=680;p.alignment=4096;
    const unsigned count=tiny?ImageEntryCapacity+44:16;
    for(unsigned i=0;i<count;++i)p.frames.push_back(frame(tiny?1:80,tiny?1:200,tiny?Pattern::Alpha:Pattern::ManySpans,31+i));
    p.collect();p.scatter(TilePoolBytes/(680u*TileBytes)+2);std::vector<std::vector<unsigned>> plans;
    if(tiny){for(unsigned begin=0;begin<count;begin+=35){std::vector<unsigned> ids;for(unsigned i=begin;i<std::min(count,begin+35);++i)ids.push_back(i);plans.push_back(std::move(ids));}}
    else for(unsigned i=0;i<count;++i)plans.push_back(std::vector<unsigned>(1+i%6,i));
    p.make_groups(plans);p.encode();return p;
}
void fill_pressure(Content&content,Provider&source,const Package&p){
    CHECK(content.open(source.source()));
    for(unsigned i=0;i<p.frames.size();++i)picture(content,p.image_id(i),p.frames[i]);
    picture(content,p.alias_id(),p.frames[0]);
}
void owners_lifetime_and_fragmentation(const Package&pressure_medium){
    label="owners and cache lifetime";empty_image_pool();Package a=basic(85,1),b=basic(85,123);
    auto first=std::make_unique<Content>(),second=std::make_unique<Content>(),evictor=std::make_unique<Content>();
    Provider pa{&a.bytes},pb{&b.bytes},pe{&pressure_medium.bytes};
    CHECK(first->open(pa.source()));CHECK(second->open(pb.source()));
    std::vector<Picture> kept;
    for(unsigned i=0;i<a.frames.size();++i){picture(*first,a.image_id(i),a.frames[i]);picture(*second,b.image_id(i),b.frames[i]);kept.push_back(first->image(int(a.image_id(i))));}
    // The fixture scatters tiles beyond the current pool. Index groups require 1--4
    // pages, deliberately mixing allocation lengths and forcing evictions.
    // Each medium frame references every tile group while its index group
    // must remain pinned. The 16 decoded medium frames occupy 480 image pages;
    // the fresh tiny views fit beside them and must retain their addresses.
    // This asserts surviving-view stability, not permanent size-tier isolation.
    CHECK(pressure_medium.groups[0].raw.size()==523600);
    CHECK(pressure_medium.tile_groups()*pressure_medium.groups[0].raw.size()>TilePoolBytes);
    CHECK(evictor->open(pe.source()));
    const uint64_t before_reconstruction=evictor->read_stats().callback_bytes;
    for(unsigned pass=0;pass<3;++pass)for(unsigned at=0;at<pressure_medium.frames.size();++at){
        const unsigned i=(at*7+pass*5)%unsigned(pressure_medium.frames.size());
        picture(*evictor,pressure_medium.image_id(i),pressure_medium.frames[i]);
    }
    CHECK(evictor->read_stats().callback_bytes-before_reconstruction>TilePoolBytes);CHECK(pe.max_read<=EncodedBound&&!pe.bad_range);
    for(unsigned i=0;i<kept.size();++i){
        CHECK(!std::memcmp(kept[i].pixels,a.frames[i].expected.data(),a.frames[i].expected.size()));
        picture(*first,a.image_id(i),a.frames[i]);picture(*second,b.image_id(i),b.frames[i]);
    }
    // Same owner, same file offsets and descriptors, different immutable file:
    // reopening must discard the old frame and group cache identities.
    pa.bytes=&b.bytes;CHECK(first->open(pa.source()));
    for(unsigned i=0;i<b.frames.size();++i)picture(*first,b.image_id(i),b.frames[i]);
    CHECK(second->healthy());picture(*second,1,b.frames[0]);
    Bytes invalid=a.bytes;put(invalid,a.base,0);seal(invalid);pa.bytes=&invalid;
    CHECK(!first->open(pa.source()));CHECK(!first->healthy()&&!first->image(1).pixels&&!first->glyph('?',10).pixels);
    pa.bytes=&a.bytes;CHECK(first->open(pa.source()));picture(*first,1,a.frames[0]);picture(*second,1,b.frames[0]);
}
void failures_and_inactive_rebuild(const Package&pressure_tiny){
    auto target=std::make_unique<Content>(),other=std::make_unique<Content>();
    for(bool corrupt:{false,true}){
        label=corrupt?"storage corruption invalidates decoded views":"partial read failure invalidates decoded views";
        Package p=basic();Provider source{&p.bytes},evict{&pressure_tiny.bytes};
        CHECK(target->open(source.source()));CHECK(target->glyph('?',10).pixels);picture(*target,1,p.frames[0]);
        // More than the image key capacity and 5 MiB of groups evict both
        // caches, without resetting the target's ResourceReader page cache.
        fill_pressure(*other,evict,pressure_tiny);
        // After capacity+44 frames and the frame-zero alias, the last
        // capacity-1 frames plus zero fill the keys. Refresh the oldest hit;
        // the following frame is now the
        // next victim. A hit must update recency without any resource reads.
        const unsigned oldest=unsigned(pressure_tiny.frames.size())-ImageEntryCapacity+1;
        const auto before_hit=other->read_stats();const Picture kept=other->image(pressure_tiny.image_id(oldest));
        CHECK(kept.pixels&&same_reads(before_hit,other->read_stats()));
        source.fail_first=p.group_offsets[0]+32;source.fail_last=source.fail_first+1;source.corrupt=corrupt;
        CHECK(!target->image(1).pixels);CHECK(source.failed>0);CHECK(!target->healthy());
        CHECK(!target->image(int(p.alias_id())).pixels&&!target->glyph('?',10).pixels);CHECK(!*target->string(0));
        CHECK(other->healthy());
        // A failed destination reuse must not leave another owner's old key
        // pointing at partially overwritten image or group bytes.
        CHECK(!std::memcmp(kept.pixels,pressure_tiny.frames[oldest].expected.data(),pressure_tiny.frames[oldest].expected.size()));
        const auto before_victim=other->read_stats();
        picture(*other,pressure_tiny.image_id(oldest+1),pressure_tiny.frames[oldest+1]);
        CHECK(!same_reads(before_victim,other->read_stats()));
        for(unsigned i=0;i<pressure_tiny.frames.size();++i)picture(*other,pressure_tiny.image_id(i),pressure_tiny.frames[i]);
        source.fail_first=source.fail_last=0;CHECK(target->open(source.source()));picture(*target,1,p.frames[0]);
        CHECK(!source.bad_range&&!evict.bad_range);
    }
    label="inactive tiles are never cold-read for reconstruction";
    Package empty;empty.per_group=85;empty.alignment=4096;empty.frames.push_back(frame(320,240,Pattern::Empty));
    empty.collect();empty.make_groups({{0}});empty.encode();Provider source{&empty.bytes},evict{&pressure_tiny.bytes};
    CHECK(target->open(source.source()));picture(*target,1,empty.frames[0]);fill_pressure(*other,evict,pressure_tiny);
    source.fail_first=empty.group_offsets[0];const unsigned last=empty.tile_groups()-1;
    source.fail_last=empty.group_offsets[last]+empty.group_lengths[last];
    // Directory and index-group page reads are separated from tile payloads
    // by fixture alignment. All 300 IDs remain present in the index record.
    picture(*target,1,empty.frames[0]);CHECK(source.failed==0);CHECK(!source.bad_range);
}
Package empty_canvas(){
    Package p;p.alignment=4096;p.frames.push_back(frame(320,240,Pattern::Empty));
    p.collect();p.make_groups({{0}});p.encode();return p;
}
void block_tiles(Provider&source,const Package&p){
    source.fail_first=p.group_offsets[0];const unsigned last=p.tile_groups()-1;
    source.fail_last=p.group_offsets[last]+p.group_lengths[last];
}
void inactive_tiles_across_owners(const Package&pressure_tiny){
    Package empty=empty_canvas();Provider source{&empty.bytes},independent{&empty.bytes},evict{&pressure_tiny.bytes};
    auto target=std::make_unique<Content>(),other=std::make_unique<Content>(),evictor=std::make_unique<Content>();
    label="first request ignores all inactive tile payloads";
    CHECK(target->open(source.source()));CHECK(target->healthy());block_tiles(source,empty);
    picture(*target,1,empty.frames[0]);CHECK(source.failed==0);
    const Picture original=target->image(1);const auto reads=target->read_stats();
    const Picture alias=target->image(int(empty.alias_id()));CHECK(alias.pixels==original.pixels);
    CHECK(same_reads(reads,target->read_stats()));
    fill_pressure(*evictor,evict,pressure_tiny);
    picture(*target,empty.alias_id(),empty.frames[0]);CHECK(source.failed==0);
    label="another owner uses the same on-demand inactive-tile behavior";
    CHECK(other->open(independent.source()));block_tiles(independent,empty);
    picture(*other,empty.alias_id(),empty.frames[0]);CHECK(independent.failed==0);
    CHECK(target->healthy());picture(*target,empty.alias_id(),empty.frames[0]);
    for(bool failed_open:{false,true}){
        label=failed_open?"reopen after failure does not scan inactive tiles":"reopen discards caches without scanning inactive tiles";
        source.fail_first=source.fail_last=0;CHECK(target->open(source.source()));
        picture(*target,1,empty.frames[0]);picture(*target,empty.alias_id(),empty.frames[0]);
        Bytes bad;
        if(failed_open){
            bad=empty.bytes;put(bad,empty.base,0);seal(bad);source.bytes=&bad;
            CHECK(!target->open(source.source())&&!target->healthy());source.bytes=&empty.bytes;
        }
        CHECK(target->open(source.source()));block_tiles(source,empty);
        picture(*target,empty.alias_id(),empty.frames[0]);CHECK(source.failed==0);
    }
    CHECK(!source.bad_range&&!independent.bad_range&&!evict.bad_range);
}
struct FullDirectory {
    Bytes bytes;Frame expected;unsigned prefix,first_tile,last_tile;
};
FullDirectory full_directory_package(){
    // Maximal legal image/glyph directories plus a real, ordered lookup use
    // the fixed resident allocation to within seven bytes. Repeated stored
    // LZMA2 tile groups keep this fixture independent of any host compressor.
    // All tile IDs are inactive. A single template avoids 54k duplicate
    // raw buffers; none of these payloads are necessary to draw the frame.
    Package p;p.images=65535;p.glyphs=32768;p.per_group=1;
    p.frames.push_back(frame(320,240,Pattern::Empty));p.collect();p.make_groups({{0}});p.encode();
    const unsigned count=(DirectoryBound-p.table)/8-1,prefix=p.table+(count+1)*8;
    CHECK(count>=300&&prefix<=DirectoryBound&&DirectoryBound-prefix<8);
    CHECK(prefix+(p.images+7)/8>DirectoryBound);
    const unsigned referenced=unsigned(p.frames[0].ids.size());
    CHECK(referenced>ResourceReader::PageCount);
    Bytes bytes(prefix+16,0);bytes.reserve(prefix+16+9842+count*806+referenced*4096+8192);
    std::copy(p.bytes.begin(),p.bytes.begin()+p.base+64,bytes.begin());
    std::copy(p.bytes.begin()+p.descriptor_base,p.bytes.begin()+p.descriptor_base+16,bytes.begin()+prefix);
    put(bytes,p.base+16,count);put(bytes,p.base+20,count);put(bytes,p.base+32,prefix);
    const unsigned card=unsigned(bytes.size());bytes.resize(bytes.size()+9840);
    const unsigned glyph=unsigned(bytes.size());bytes.push_back(0xab);
    const unsigned empty=unsigned(bytes.size());bytes.push_back(0);
    for(unsigned id=0;id<p.images;++id)put(bytes,p.image_entry(id),(id==1||id==p.alias_id())?prefix:card);
    unsigned at=40+p.images*16;
    for(unsigned i=0;i<p.glyphs;++i)put(bytes,at+i*16+4,glyph);
    at+=p.glyphs*16;for(unsigned i=0;i<ContentStrings;++i)put(bytes,at+i*8,empty);
    const auto group_block=[](unsigned kind,unsigned items,const Bytes&raw){
        Bytes b(32);const Bytes encoded=stored_lzma2(raw);
        put(b,0,0x31524754);put(b,4,kind);put(b,12,unsigned(raw.size()));put(b,16,unsigned(encoded.size()));
        put(b,20,0);put(b,24,items);b.insert(b.end(),encoded.begin(),encoded.end());return b;
    };
    const Bytes tile=group_block(1,1,p.groups[0].raw);
    CHECK(tile.size()==806);bytes.resize((bytes.size()+4095)&~size_t(4095));
    const unsigned first=unsigned(bytes.size());
    const auto add_group=[&](unsigned id,const Bytes&block){
        const unsigned off=unsigned(bytes.size());bytes.insert(bytes.end(),block.begin(),block.end());
        put(bytes,off+8,id);put(bytes,p.table+id*8,off);put(bytes,p.table+id*8+4,unsigned(block.size()));
    };
    // These small blocks must also exceed the ranged reader's page cache.
    // Packed together, 300 x 806 B fit in 60 pages and a provider fault could
    // be hidden by valid cached bytes even after the tile pool is evicted.
    // Align only the referenced blocks; the other 54k header-only blocks stay
    // compact so the fixture does not grow into hundreds of megabytes.
    for(unsigned i=0;i<count;++i){
        if(i<referenced)bytes.resize((bytes.size()+4095)&~size_t(4095));
        add_group(i,tile);
    }
    const unsigned last=unsigned(bytes.size());bytes.resize((bytes.size()+4095)&~size_t(4095));
    add_group(count,group_block(2,1,p.groups[p.tile_groups()].raw));seal(bytes);
    return {std::move(bytes),std::move(p.frames[0]),prefix,first,last};
}
void directory_capacity_without_prescan(const Package&pressure_tiny){
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>(),evictor=std::make_unique<Content>();
    Provider evict{&pressure_tiny.bytes};ImageRejector reject;
    for(unsigned descriptors:{18000u,24000u,65530u}){
        label="large descriptor payload needs no startup scratch chain";
        Package p=empty_canvas();p.images=65535;p.glyphs=32768;p.reserved_descriptors=descriptors;p.encode();
        Provider source{&p.bytes};CHECK(direct->open(p.bytes.data(),p.bytes.size()));CHECK(range->open(source.source()));
        picture(*direct,1,p.frames[0]);picture(*range,1,p.frames[0]);
        fill_pressure(*evictor,evict,pressure_tiny);block_tiles(source,p);
        picture(*direct,1,p.frames[0]);picture(*range,1,p.frames[0]);CHECK(source.failed==0);
        Bytes invalid=p.bytes;put(invalid,p.image_entry(1)+6,239,2);
        reject("large catalog keeps selected geometry bounds",std::move(invalid));
        invalid=p.bytes;put(invalid,p.descriptor(p.reserved_descriptors-1)+8,~0u);
        accept_frames("large unused descriptor is not traversed",std::move(invalid),p);
        CHECK(!source.bad_range);
    }
    label="full resident directory retains on-demand reconstruction";
    FullDirectory full=full_directory_package();Provider source{&full.bytes};
    CHECK(direct->open(full.bytes.data(),full.bytes.size()));CHECK(range->open(source.source()));
    picture(*direct,1,full.expected);picture(*range,1,full.expected);
    fill_pressure(*evictor,evict,pressure_tiny);
    const auto before=direct->read_stats();picture(*direct,1,full.expected);
    CHECK(direct->read_stats().direct_reads-before.direct_reads<=3);
    source.fail_first=full.first_tile;source.fail_last=full.last_tile;
    picture(*range,1,full.expected);CHECK(source.failed==0&&range->healthy());
    CHECK(!source.bad_range&&!evict.bad_range);
}
} // namespace
int main(int argc,char**argv){
    if(argc==2&&!std::strcmp(argv[1],"--native-ui")){native_ui_packages();locale_packages();bestiary_supplement_packages();ui13_source_catalogs();std::printf("native UI and locale content: %u checks passed\n",checks);return 0;}
    required_metadata_prefix();progress_contract();valid_packages();malformed_container();malformed_geometry();catalog_metadata_reuse();maximum_frame_and_unused_tile();
    native_ui_packages();locale_packages();bestiary_supplement_packages();ui13_source_catalogs();
    image_pool_cycles_and_fragmentation();
    label="bounded group pressure fixtures";
    const Package medium=pressure(false),tiny=pressure(true);
    owners_lifetime_and_fragmentation(medium);failures_and_inactive_rebuild(tiny);
    inactive_tiles_across_owners(tiny);directory_capacity_without_prescan(tiny);
    std::printf("content tiles: %u checks passed; synthetic NSP2/15 directory-only startup, selected decoding, aliases, owners, failures, capacity and fragmented group reuse\n",checks);
}
