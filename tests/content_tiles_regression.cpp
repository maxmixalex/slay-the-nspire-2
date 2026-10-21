#include "app.hpp"
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
uint32_t crc32(const Bytes&b){
    uint32_t crc=~uint32_t(0);
    for(uint8_t byte:b){crc^=byte;for(unsigned i=0;i<8;++i)crc=(crc>>1)^(0xedb88320u&uint32_t(-int32_t(crc&1)));}
    return ~crc;
}
void seal(Bytes&b){
    put(b,12,uint32_t(b.size()));uint32_t hash=2166136261u;
    for(size_t i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);
}
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
enum class Pattern {Mixed,Alpha,Opaque,Empty,ManySpans};
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
        if(pattern==Pattern::ManySpans)for(unsigned x=0;x<w;++x)row.push_back({x,1,x&1});
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
    void encode(bool legacy=false){
        CHECK(legacy||(!groups.empty()&&groups.size()>tile_groups()));
        images=std::max(images,unsigned(frames.size())+2);
        const unsigned descriptor_count=std::max(unsigned(frames.size()),reserved_descriptors);
        base=40+images*16+glyphs*16+ContentStrings*8+AnimationKinds*16+CardKinds*16+U_Count*4;
        table=base+64;prefix=legacy?base:table+unsigned(groups.size())*8;
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
            put(bytes,off+16,unsigned(encoded.size()));put(bytes,off+20,crc32(g.raw));put(bytes,off+24,g.items);
            bytes.insert(bytes.end(),encoded.begin(),encoded.end());
        }
        put(bytes,0,0x3250534e);put(bytes,4,legacy?14:15);put(bytes,8,0x41cef1ea);
        put(bytes,16,images);put(bytes,20,glyphs);put(bytes,24,ContentStrings);put(bytes,28,AnimationKinds);put(bytes,32,CardKinds);
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
        for(unsigned i=0;i<ContentStrings;++i)put(bytes,at+i*8,empty);
        at+=ContentStrings*8;
        for(unsigned i=0;i<AnimationKinds;++i){put(bytes,at+i*16+4,1,2);put(bytes,at+i*16+6,33,2);}at+=AnimationKinds*16;
        // All card records and auxiliary references use the shared historical
        // card image zero and empty string zero. Catalog counts remain exact.
        CHECK(at+CardKinds*16+U_Count*4==base);
        if(!legacy){
            const unsigned fields[]={0x314c4954,1,16,14,unsigned(tiles.size()),tile_groups(),unsigned(groups.size())-tile_groups(),descriptor_count,descriptor_base,table,770,per_group,GroupBound,EncodedBound,0,0};
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
    const Bytes* bytes=nullptr;
    unsigned calls=0,max_read=0,fail_first=0,fail_last=0,failed=0;
    bool corrupt=false,bad_range=false;
    static bool read(void*context,uint32_t offset,void*dst,uint32_t count){
        auto&p=*static_cast<Provider*>(context);++p.calls;p.max_read=std::max(p.max_read,count);
        if(!p.bytes||offset>p.bytes->size()||count>p.bytes->size()-offset){p.bad_range=true;return false;}
        const bool fault=p.fail_last>p.fail_first&&offset<p.fail_last&&count>p.fail_first-std::min(offset,p.fail_first);
        std::memcpy(dst,p.bytes->data()+offset,count);
        if(fault){++p.failed;if(p.corrupt){
                const unsigned at=p.fail_first>offset?p.fail_first-offset:0;static_cast<uint8_t*>(dst)[at]^=0x80;
            }else{if(count)std::memset(dst,0xa5,count/2+1);return false;}}
        return true;
    }
    ResourceSource source(){return {this,uint32_t(bytes->size()),read,nullptr};}
};
void picture(const Content&c,unsigned id,const Frame&f){
    const Picture p=c.image(int(id));CHECK(p.pixels&&p.w==f.w&&p.h==f.h&&p.format==4);
    CHECK(!p.alpha&&!p.palette);CHECK(!std::memcmp(p.pixels,f.expected.data(),f.expected.size()));CHECK(c.healthy());
}
void valid_packages(){
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>();
    for(unsigned per:{1u,85u,170u,680u}){
        label="valid group size "+std::to_string(per);Package p=basic(per);Provider provider{&p.bytes};
        CHECK(direct->open(p.bytes.data(),p.bytes.size()));CHECK(range->open(provider.source()));
        CHECK(direct->current_profile()&&range->current_profile());CHECK(direct->card_count()==CardKinds&&range->card_count()==CardKinds);
        CHECK(p.descriptors[1].offset%2==1); // Deliberately unaligned record fields.
        for(unsigned i=0;i<p.frames.size();++i){picture(*direct,p.image_id(i),p.frames[i]);picture(*range,p.image_id(i),p.frames[i]);}
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
        // FNV and every structural field are sound. Pixel payload validation
        // belongs to the first real image request, for either source backend.
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
void malformed_container(){
    label="container setup";const Package p=basic();Rejector reject;ImageRejector reject_image;
    const auto field=[&](const char*name,size_t at,uint32_t value,unsigned width=4){Bytes b=p.bytes;put(b,at,value,width);reject(name,std::move(b));};
    field("v14 cannot carry tile format",4,14);field("unsupported pack version",4,16);
    struct Mutation {unsigned offset,value;const char*name;};
    const Mutation header[]={
        {0,0,"codec magic"},{4,2,"codec version"},{8,8,"tile side"},{12,16,"dictionary property"},
        {16,0,"zero tile count"},{16,~0u,"tile count overflow"},{20,0,"zero tile groups"},{20,2,"inconsistent tile groups"},
        {24,0,"zero index groups"},{24,5,"index groups exceed frames"},{28,0,"zero descriptors"},{28,161,"descriptor count exceeds images"},
        {32,p.prefix-16,"descriptor overlaps prefix"},{32,p.descriptor_base+1,"unaligned descriptor base"},{32,~0u,"descriptor range overflow"},
        {36,p.table+8,"lookup not immediately after header"},{36,~0u,"lookup range overflow"},{40,769,"tile record length"},
        {44,0,"zero tiles per group"},{44,681,"oversize tiles per group"},{48,GroupBound-1,"raw group bound"},
        {52,EncodedBound+1,"encoded group bound"},{56,1,"codec flags"},{60,1,"codec reserved"}
    };
    for(const auto&m:header)field(m.name,p.base+m.offset,m.value);
    // A file can be large enough for its lookup table while the table exceeds
    // the reader's fixed 2 MiB resident-directory allocation.
    Bytes oversized=p.bytes;const unsigned many_groups=(2u*1024u*1024u-p.table)/8+1;
    const unsigned oversized_prefix=p.table+(many_groups+1)*8;oversized.resize(oversized_prefix+4096);
    put(oversized,p.base+16,many_groups);put(oversized,p.base+20,many_groups);
    put(oversized,p.base+32,oversized_prefix);put(oversized,p.base+44,1);
    reject("extended directory exceeds fixed resident budget",std::move(oversized));
    const size_t image=p.image_entry(1),desc=p.descriptor(0);
    field("image descriptor short",image+12,15);field("image descriptor long",image+12,17);
    field("image points into prefix",image,p.base);field("image points between descriptors",image,p.descriptor_base+2);
    field("image points after descriptors",image,p.descriptor_base+unsigned(p.frames.size())*16);
    field("descriptor wrong shared header",desc,p.base+2);field("descriptor index group range",desc+4,1);
    field("descriptor offset overflow",desc+8,~0u);field("descriptor offset inside record",desc+8,1);
    field("descriptor empty record",desc+12,0);field("descriptor short record",desc+12,p.descriptors[0].length-1);
    field("descriptor extra record byte",desc+12,p.descriptors[0].length+1);field("descriptor record overflow",desc+12,~0u);
    field("lookup odd group",p.table,p.group_offsets[0]+1);field("lookup overlaps prefix",p.table,p.base);
    field("lookup overlaps descriptors",p.table,p.descriptor_base);field("lookup offset overflow",p.table,~0u);
    field("lookup short group",p.table+4,32);field("lookup oversized group",p.table+4,EncodedBound+1);
    field("lookup groups overlap",p.table+8,p.group_offsets[0]+2);
    field("lookup groups out of order",p.table,p.group_offsets[1]);
    const unsigned g=p.group_offsets[0],index=p.group_offsets[p.tile_groups()];
    const Mutation group[]={
        {0,0,"group magic"},{4,2,"tile group wrong type"},{8,1,"group identity"},{12,0,"group empty raw"},
        {12,GroupBound+1,"group raw overflow"},{12,unsigned(p.groups[0].raw.size())-1,"tile raw record size"},
        {16,p.group_lengths[0]-33,"encoded length disagrees with lookup"},
        {24,p.groups[0].items+1,"tile item count"},{28,1,"group reserved"}
    };
    for(const auto&m:group)field(m.name,g+m.offset,m.value);
    Bytes bad_crc=p.bytes;put(bad_crc,g+20,crc32(p.groups[0].raw)^1);
    reject_image("tile CRC rejected on first image",std::move(bad_crc));
    field("index CRC rejected during open",index+20,crc32(p.groups[p.tile_groups()].raw)^1);
    field("index group wrong type",index+4,1);field("index zero items",index+24,0);
    field("index excessive items",index+24,65);field("index incomplete record count",index+24,3);
    field("index extra record count",index+24,5);
    label="whole-file FNV still covers deferred tile payload bytes";
    Bytes unsealed=p.bytes;unsealed[g+32+3+2]^=1;Provider checksum{&unsealed};
    CHECK(!reject.direct->open(unsealed.data(),unsealed.size()));CHECK(!reject.range->open(checksum.source()));
    CHECK(!reject.direct->healthy()&&!reject.range->healthy());
    Package unused=basic(1);unused.tiles.push_back(blank_tile());unused.make_groups({{0,1,2,3}});unused.encode();
    put(unused.bytes,unused.group_offsets[unused.tile_groups()-1]+4,2);
    reject("unreferenced tile group header is checked during open",std::move(unused.bytes));
    // Corrupt a resident-directory alias in each enumeration order: a valid
    // decoded frame cannot authorize another shape or descriptor length.
    for(bool reverse:{false,true}){
        Bytes b=p.bytes;const unsigned bad=reverse?1:p.alias_id();
        const unsigned good=reverse?p.alias_id():1;
        put(b,p.image_entry(bad)+4,34,2);
        reject_image(reverse?"bad edge alias first in directory":"bad edge alias last in directory",b,bad);
        reject_image(reverse?"cached good alias cannot authorize earlier bad shape":"cached good alias cannot authorize later bad shape",std::move(b),bad,int(good));
        b=p.bytes;put(b,p.image_entry(bad)+12,15);reject(reverse?"short alias before good":"short alias after good",std::move(b));
    }
    for(bool trailing:{false,true}){
        Package q=p;q.groups[0].custom=true;q.groups[0].encoded=stored_lzma2(q.groups[0].raw);
        if(trailing)q.groups[0].encoded.push_back(0);else q.groups[0].encoded.pop_back();q.encode();
        reject_image(trailing?"first image rejects unused LZMA2 input":"first image requires LZMA2 end marker",std::move(q.bytes));
    }
}
void malformed_geometry(){
    label="geometry setup";const Package p=basic();Rejector reject;ImageRejector reject_image;const unsigned index=p.tile_groups();
    const auto raw_field=[&](const char*name,unsigned group,size_t at,uint32_t value,unsigned width=4){
        Package q=p;put(q.groups[group].raw,at,value,width);q.encode();reject(name,std::move(q.bytes));
    };
    const Frame&f=p.frames[0];const unsigned start=p.descriptors[0].offset,geometry=start+12+unsigned(f.ids.size())*4;
    raw_field("zero original image bytes",index,start,0);raw_field("oversize original image",index,start,ImageBound+1);
    raw_field("geometry output length mismatch",index,start,unsigned(f.expected.size())+1);
    raw_field("wrong tile index byte count",index,start+4,unsigned(f.ids.size())*4-4);
    raw_field("tile index byte overflow",index,start+4,~0u);raw_field("zero geometry byte count",index,start+8,0);
    raw_field("geometry byte overflow",index,start+8,~0u);raw_field("tile id out of range",index,start+12,unsigned(p.tiles.size()));
    raw_field("tile id overflow",index,start+12,~0u);raw_field("too many row spans",index,geometry,f.w+1,2);
    const unsigned span=geometry+unsigned(f.span_offsets[0]),second=geometry+unsigned(f.span_offsets[1]);
    raw_field("zero span pixel count",index,span+2,0,2);raw_field("span mode",index,span+4,2,1);
    raw_field("span x overflow",index,span,65535,2);raw_field("span exceeds row",index,span+2,f.w+1,2);
    raw_field("overlapping spans",index,second,1,2);raw_field("unordered spans",index,second,0,2);
    // A frame record may not be a prefix, suffix or arbitrary interior of a
    // different valid record, even when the descriptor range lies in bounds.
    Package tail=p;tail.groups[index].raw.push_back(0);tail.encode();reject("index terminal padding",std::move(tail.bytes));
    Package hidden=p;Bytes hidden_record=p.frames[1].record();hidden_record[4]^=4;
    hidden.groups[index].raw.insert(hidden.groups[index].raw.end(),hidden_record.begin(),hidden_record.end());++hidden.groups[index].items;hidden.encode();
    reject("unrequested index record structure",std::move(hidden.bytes));
    Package unused_id=p;Bytes out_of_range=p.frames[3].record();put(out_of_range,12,unsigned(p.tiles.size()));
    unused_id.groups[index].raw.insert(unused_id.groups[index].raw.end(),out_of_range.begin(),out_of_range.end());
    ++unused_id.groups[index].items;unused_id.encode();reject("unrequested index record tile ID",std::move(unused_id.bytes));

    const auto tile_field=[&](const char*name,unsigned tile,unsigned at,unsigned value,unsigned width=1,unsigned image=1){
        Package q=p;put(q.groups[tile/p.per_group].raw,(tile%p.per_group)*TileBytes+at,value,width);
        q.encode();reject_image(name,std::move(q.bytes),image);
    };
    tile_field("zero tile width",f.ids[0],0,0);tile_field("oversized tile height",f.ids[0],1,17);
    tile_field("partial right edge width",f.ids[2],0,2);tile_field("partial bottom edge height",f.ids[3],1,2);
    tile_field("nonzero RGB right padding",f.ids[2],2+2,1);tile_field("nonzero alpha right padding",f.ids[2],514+1,1);
    tile_field("nonzero RGB bottom padding",f.ids[3],2+16*2,1);tile_field("nonzero alpha bottom padding",f.ids[3],514+16,1);
    tile_field("opaque span must have alpha 255 on first image",p.frames[2].ids[0],514,254,1,p.image_id(2));
    // Structural scanning must not mark an image as pixel-verified. Even an
    // empty canvas validates every referenced tile on its first real request.
    tile_field("inactive tile edge rejected on first image",p.frames[3].ids[0],1,3,1,p.image_id(3));
    const unsigned inactive_start=p.descriptors[3].offset;
    raw_field("inactive tile id range",index,inactive_start+12,unsigned(p.tiles.size()));
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
    // No descriptor addresses this tile. Whole-group validation must still
    // reject its nonzero padding when another tile loads the shared group.
    const unsigned tile=unsigned(unused.tiles.size())-1;
    put(unused.groups[tile/unused.per_group].raw,(tile%unused.per_group)*TileBytes+2+15*2,1,1);
    unused.encode();ImageRejector reject;reject("unreferenced tile padding in a loaded group",std::move(unused.bytes));
}
Package pressure(bool tiny){
    Package p;p.per_group=680;p.alignment=4096;
    const unsigned count=tiny?70:16;
    for(unsigned i=0;i<count;++i)p.frames.push_back(frame(tiny?1:80,tiny?1:200,tiny?Pattern::Alpha:Pattern::ManySpans,31+i));
    p.collect();p.scatter(10);std::vector<std::vector<unsigned>> plans;
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
    label="owners and cache lifetime";Package a=basic(85,1),b=basic(85,123);
    auto first=std::make_unique<Content>(),second=std::make_unique<Content>(),evictor=std::make_unique<Content>();
    Provider pa{&a.bytes},pb{&b.bytes},pe{&pressure_medium.bytes};
    CHECK(first->open(pa.source()));CHECK(second->open(pb.source()));
    std::vector<Picture> kept;
    for(unsigned i=0;i<a.frames.size();++i){picture(*first,a.image_id(i),a.frames[i]);picture(*second,b.image_id(i),b.frames[i]);kept.push_back(first->image(int(a.image_id(i))));}
    // Ten full tile groups exceed the 4 MiB pool. Index groups require 2--8
    // pages, deliberately mixing allocation lengths and forcing evictions.
    // Each medium frame references all ten tile groups while its index group
    // must remain pinned. Existing tiny image views must survive group reuse.
    CHECK(pressure_medium.groups[0].raw.size()==523600);
    CHECK(evictor->open(pe.source()));
    const uint64_t before_reconstruction=evictor->read_stats().callback_bytes;
    for(unsigned pass=0;pass<3;++pass)for(unsigned at=0;at<pressure_medium.frames.size();++at){
        const unsigned i=(at*7+pass*5)%unsigned(pressure_medium.frames.size());
        picture(*evictor,pressure_medium.image_id(i),pressure_medium.frames[i]);
    }
    CHECK(evictor->read_stats().callback_bytes-before_reconstruction>4u*1024u*1024u);CHECK(pe.max_read<=EncodedBound&&!pe.bad_range);
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
        // More than 64 tiny images and more than 4 MiB of groups evict both
        // caches, without resetting the target's ResourceReader page cache.
        fill_pressure(*other,evict,pressure_tiny);
        source.fail_first=p.group_offsets[0]+32+3+2;source.fail_last=source.fail_first+1;source.corrupt=corrupt;
        CHECK(!target->image(1).pixels);CHECK(source.failed>0);CHECK(!target->healthy());
        CHECK(!target->image(int(p.alias_id())).pixels&&!target->glyph('?',10).pixels);CHECK(!*target->string(0));
        CHECK(other->healthy());
        // A failed destination reuse must not leave another owner's old key
        // pointing at partially overwritten image or group bytes.
        // Seventy unique tiny frames plus the final frame-zero alias leave
        // frame seven as the next of the 64 tiny slots to be reused. Check it
        // before the longer pass can itself evict a stale destination key.
        picture(*other,pressure_tiny.image_id(7),pressure_tiny.frames[7]);
        for(unsigned i=0;i<pressure_tiny.frames.size();++i)picture(*other,pressure_tiny.image_id(i),pressure_tiny.frames[i]);
        source.fail_first=source.fail_last=0;CHECK(target->open(source.source()));picture(*target,1,p.frames[0]);
        CHECK(!source.bad_range&&!evict.bad_range);
    }
    label="inactive tiles are validated once and not cold-read for reconstruction";
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
void first_touch_and_validation_bits(const Package&pressure_tiny){
    Package empty=empty_canvas();Provider source{&empty.bytes},independent{&empty.bytes},evict{&pressure_tiny.bytes};
    auto target=std::make_unique<Content>(),other=std::make_unique<Content>(),evictor=std::make_unique<Content>();
    label="structural opening must not mark an inactive image verified";
    CHECK(target->open(source.source()));CHECK(target->healthy());block_tiles(source,empty);
    CHECK(!target->image(1).pixels&&source.failed>0);CHECK(!target->healthy());

    label="exact cache-hit alias inherits completed first-touch validation";
    source.fail_first=source.fail_last=0;CHECK(target->open(source.source()));
    picture(*target,1,empty.frames[0]);const Picture original=target->image(1);
    const auto reads=target->read_stats();const Picture alias=target->image(int(empty.alias_id()));
    CHECK(alias.pixels==original.pixels);CHECK(target->read_stats().callbacks==reads.callbacks);
    fill_pressure(*evictor,evict,pressure_tiny);block_tiles(source,empty);const unsigned failures=source.failed;
    picture(*target,empty.alias_id(),empty.frames[0]);CHECK(source.failed==failures);

    label="same descriptor in another owner has no inherited verification bit";
    CHECK(other->open(independent.source()));block_tiles(independent,empty);
    CHECK(!other->image(int(empty.alias_id())).pixels&&independent.failed>0);CHECK(!other->healthy());
    CHECK(target->healthy());picture(*target,empty.alias_id(),empty.frames[0]);

    for(bool failed_open:{false,true}){
        label=failed_open?"failed open then reopen clears validation bits":"successful reopen clears validation bits";
        source.fail_first=source.fail_last=0;CHECK(target->open(source.source()));
        picture(*target,1,empty.frames[0]);picture(*target,empty.alias_id(),empty.frames[0]);
        Bytes bad;
        if(failed_open){
            bad=empty.bytes;put(bad,empty.base,0);seal(bad);source.bytes=&bad;
            CHECK(!target->open(source.source())&&!target->healthy());source.bytes=&empty.bytes;
        }
        CHECK(target->open(source.source()));block_tiles(source,empty);const unsigned before=source.failed;
        CHECK(!target->image(int(empty.alias_id())).pixels&&source.failed>before);CHECK(!target->healthy());
    }
    CHECK(!source.bad_range&&!independent.bad_range&&!evict.bad_range);
}
struct FullDirectory {
    Bytes bytes;Frame expected;unsigned prefix,first_tile,last_tile;
};
FullDirectory no_bitmap_package(){
    // Maximal legal image/glyph directories plus a real, ordered lookup use
    // the fixed resident allocation to within seven bytes. Repeated stored
    // LZMA2 tile groups keep this fixture independent of any host compressor.
    // Only the first 300 tiles are referenced; the others still have checked
    // group headers. A single template avoids 54k duplicate raw/CRC buffers.
    Package p;p.images=65535;p.glyphs=32768;p.per_group=1;
    p.frames.push_back(frame(320,240,Pattern::Empty));p.collect();p.make_groups({{0}});p.encode();
    const unsigned count=(DirectoryBound-p.table)/8-1,prefix=p.table+(count+1)*8;
    CHECK(count>=300&&prefix<=DirectoryBound&&DirectoryBound-prefix<8);
    CHECK(prefix+(p.images+7)/8>DirectoryBound);
    Bytes bytes(prefix+16,0);bytes.reserve(prefix+16+9842+count*806+8192);
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
        put(b,20,crc32(raw));put(b,24,items);b.insert(b.end(),encoded.begin(),encoded.end());return b;
    };
    const Bytes tile=group_block(1,1,p.groups[0].raw);
    CHECK(tile.size()==806);bytes.resize((bytes.size()+4095)&~size_t(4095));
    const unsigned first=unsigned(bytes.size());
    const auto add_group=[&](unsigned id,const Bytes&block){
        const unsigned off=unsigned(bytes.size());bytes.insert(bytes.end(),block.begin(),block.end());
        put(bytes,off+8,id);put(bytes,p.table+id*8,off);put(bytes,p.table+id*8+4,unsigned(block.size()));
    };
    for(unsigned i=0;i<count;++i)add_group(i,tile);
    const unsigned last=unsigned(bytes.size());bytes.resize((bytes.size()+4095)&~size_t(4095));
    add_group(count,group_block(2,1,p.groups[p.tile_groups()].raw));seal(bytes);
    return {std::move(bytes),std::move(p.frames[0]),prefix,first,last};
}
void validation_workspace_fallbacks(const Package&pressure_tiny){
    auto direct=std::make_unique<Content>(),range=std::make_unique<Content>(),evictor=std::make_unique<Content>();
    Provider evict{&pressure_tiny.bytes};
    label="descriptor-chain workspace fallback preserves the image bitmap";
    Package chain=empty_canvas();chain.images=65535;chain.glyphs=32768;chain.reserved_descriptors=65530;chain.encode();
    const unsigned bitmap=(chain.images+7)/8,available=DirectoryBound-chain.prefix-bitmap;
    CHECK(chain.prefix+bitmap<=DirectoryBound);
    CHECK(4u*(2u*chain.reserved_descriptors+1u)>available);
    Provider source{&chain.bytes};CHECK(direct->open(chain.bytes.data(),chain.bytes.size()));CHECK(range->open(source.source()));
    picture(*direct,1,chain.frames[0]);picture(*range,1,chain.frames[0]);
    fill_pressure(*evictor,evict,pressure_tiny);block_tiles(source,chain);
    picture(*direct,1,chain.frames[0]);picture(*range,1,chain.frames[0]);CHECK(source.failed==0);
    Bytes bad=chain.bytes;put(bad,chain.image_entry(1)+6,239,2);
    Rejector reject;reject("workspace fallback still validates each image geometry",std::move(bad));

    label="no bitmap space falls back to full verification on each image miss";
    FullDirectory full=no_bitmap_package();Provider fallback{&full.bytes};
    CHECK(direct->open(full.bytes.data(),full.bytes.size()));CHECK(range->open(fallback.source()));
    picture(*direct,1,full.expected);picture(*range,1,full.expected);
    fill_pressure(*evictor,evict,pressure_tiny);
    const auto reads=direct->read_stats();picture(*direct,1,full.expected);
    CHECK(direct->read_stats().direct_reads-reads.direct_reads>=300);
    fallback.fail_first=full.first_tile;fallback.fail_last=full.last_tile;
    CHECK(!range->image(1).pixels&&fallback.failed>0);CHECK(!range->healthy());CHECK(direct->healthy());
    CHECK(!source.bad_range&&!fallback.bad_range&&!evict.bad_range);
}
} // namespace
int main(){
    valid_packages();malformed_container();malformed_geometry();maximum_frame_and_unused_tile();
    label="bounded group pressure fixtures";
    const Package medium=pressure(false),tiny=pressure(true);
    owners_lifetime_and_fragmentation(medium);failures_and_inactive_rebuild(tiny);
    first_touch_and_validation_bits(tiny);validation_workspace_fallbacks(tiny);
    std::printf("content tiles: %u checks passed; synthetic NSP2/15 structure, first-touch validation, aliases, owners, failures, workspace fallback and fragmented group reuse\n",checks);
}
