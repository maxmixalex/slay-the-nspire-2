// Independent explicit sparse records remain the byte oracle. The source
// writer does not call either production geometry regeneration routine.
#define main existing_content_tiles_suite
#include "content_tiles_regression.cpp"
#undef main
#include "resource_transfer.hpp"
#include <new>

namespace {
Frame canonical(unsigned w,unsigned h,unsigned salt,bool bitmap=false){
    Frame f{w,h,{},{},{},{},{}};std::vector<Pixel> pixels(w*h);
    for(unsigned y=0;y<h;++y){
        std::vector<Span> row;
        if(bitmap){for(unsigned x=(y%2)*3;x+8<=w;x+=20)row.push_back({x,8,0});}
        else switch(y%5){
        case 0:break;
        case 1:row.push_back({0,w,0});break;
        case 2:row.push_back({1,w-2,1});break;
        case 3:row={{0,8,0},{8,12,1},{20,w-20,0}};break;
        default:row.push_back({2,w-4,1});break;
        }
        append(f.expected,unsigned(row.size()),2);append(f.geometry,unsigned(row.size()),2);
        for(const auto&s:row){
            append(f.geometry,s.x,2);append(f.geometry,s.count,2);append(f.geometry,s.mode,1);
            append(f.expected,s.x,2);append(f.expected,s.count,2);append(f.expected,s.mode,1);
            for(unsigned x=s.x;x<s.x+s.count;++x){
                Pixel p{uint16_t(0x1234u+salt*911u+x*317u+y*191u),s.mode?uint8_t(91):uint8_t(255)};
                if(bitmap&&x==s.x)p.color=0; // opaque black still copies
                if(!bitmap&&y%5==4&&((x>=12&&x<22)||x<7||x>=w-6))p.alpha=255;
                pixels[y*w+x]=p;append(f.expected,p.color,2);if(s.mode)append(f.expected,p.alpha,1);
            }
        }
    }
    for(unsigned ty=0;ty<h;ty+=16)for(unsigned tx=0;tx<w;tx+=16){
        Bytes tile(TileBytes);tile[0]=uint8_t(std::min(16u,w-tx));tile[1]=uint8_t(std::min(16u,h-ty));
        for(unsigned y=0;y<tile[1];++y)for(unsigned x=0;x<tile[0];++x){
            const auto p=pixels[(ty+y)*w+tx+x];const unsigned at=y*16+x;
            put(tile,2+at*2,p.color,2);tile[514+at]=p.alpha;
        }
        f.tiles.push_back(std::move(tile));
    }
    return f;
}
Bytes marker_geometry(const Frame&f,bool bitmap){
    const unsigned columns=(f.w+15)/16,cells=unsigned(f.ids.size()),maskbytes=(cells+7)/8;
    Bytes geometry(f.geometry.size());CHECK(geometry.size()>=2+maskbytes+(bitmap?9600:0));
    put(geometry,0,bitmap?0xfffc:0xfffd,2);
    unsigned at=0;
    for(unsigned y=0;y<f.h;++y){
        const unsigned spans=f.geometry[at]|unsigned(f.geometry[at+1])<<8;at+=2;
        for(unsigned j=0;j<spans;++j){
            const unsigned x=f.geometry[at]|unsigned(f.geometry[at+1])<<8;
            const unsigned n=f.geometry[at+2]|unsigned(f.geometry[at+3])<<8;at+=5;
            for(unsigned tx=x/16;tx<=(x+n-1)/16;++tx){const unsigned cell=(y/16)*columns+tx;geometry[2+cell/8]|=uint8_t(1u<<(cell&7));}
            if(bitmap)for(unsigned px=x;px<x+n;++px)geometry[40+y*40+px/8]|=uint8_t(1u<<(px&7));
        }
    }
    CHECK(at==f.geometry.size());return geometry;
}
void encode46(Package&p){
    p.encode(false,InspectionAuxiliaryCount,46,Ui7StringCount,BestiaryKaiserAnimationKinds);
    const unsigned strings=40+p.images*16+p.glyphs*16;
    put(p.bytes,strings+LocaleMarkerString*8,unsigned(p.bytes.size()));put(p.bytes,strings+LocaleMarkerString*8+4,3);
    p.bytes.insert(p.bytes.end(),{'e','n','g',0});seal(p.bytes);
}
Package implicit_fixture(bool marked=true){
    Package p;p.per_group=680;
    p.frames={canonical(320,240,1),canonical(33,97,7),canonical(320,240,23,true),frame(33,17,Pattern::Mixed,9),frame(48,32,Pattern::Empty)};
    p.collect();p.scatter(3);p.make_groups({{0},{1},{2},{3},{4}});
    if(marked)for(unsigned i:{0u,1u,2u,4u}){
        auto&group=p.groups[p.tile_groups()+p.descriptors[i].group];const auto&f=p.frames[i];
        const auto geometry=marker_geometry(f,i==2);const unsigned at=p.descriptors[i].offset+12+unsigned(f.ids.size()*4);
        std::copy(geometry.begin(),geometry.end(),group.raw.begin()+at);
    }
    encode46(p);return p;
}
unsigned finish_implicit(Content&c,unsigned id){
    unsigned steps=0;PreparationResult result=PreparationResult::Pending;
    while(result==PreparationResult::Pending){const auto before=c.read_stats();result=c.preload_image_step(int(id));
        const auto after=c.read_stats();CHECK(++steps<20000);CHECK(c.healthy()&&!resource_transfer_uses);
        CHECK(after.callbacks-before.callbacks<=2&&after.callback_bytes-before.callback_bytes<=32768);
    }
    CHECK(result==PreparationResult::Ready&&c.image_cached(int(id)));return steps;
}
void exact_images(){
    label="NSP46 implicit/bitmap/explicit memory, ranged and incremental exact bytes";
    const Package p=implicit_fixture();Provider provider{&p.bytes};auto c=std::make_unique<Content>();
    for(bool direct:{false,true})for(bool incremental:{false,true}){
        CHECK(c->open(direct?ResourceSource::memory(p.bytes.data(),p.bytes.size()):provider.source()));
        for(unsigned i=0;i<p.frames.size();++i){if(incremental)finish_implicit(*c,p.image_id(i));picture(*c,p.image_id(i),p.frames[i]);}
        const auto before=c->read_stats();for(unsigned i=0;i<p.frames.size();++i)picture(*c,p.image_id(i),p.frames[i]);
        CHECK(same_reads(before,c->read_stats()));CHECK(!provider.bad_range);
    }
    // Index bytes differ, but pixel streams and their cold read order do not.
    const Package explicit_pack=implicit_fixture(false);std::vector<unsigned> reads[2];
    struct Trace {
        const Package*p;std::vector<unsigned>*groups;
        static bool read(void*context,uint32_t at,void*out,uint32_t count){
            auto&t=*static_cast<Trace*>(context);if(at>t.p->bytes.size()||count>t.p->bytes.size()-at)return false;
            for(unsigned g=0;g<t.p->tile_groups();++g)if(at<=t.p->group_offsets[g]&&at+count>t.p->group_offsets[g])t.groups->push_back(g);
            std::memcpy(out,t.p->bytes.data()+at,count);return true;
        }
    };
    for(unsigned n=0;n<2;++n){const auto&pack=n?p:explicit_pack;Trace trace{&pack,&reads[n]};
        CHECK(c->open({&trace,uint32_t(pack.bytes.size()),Trace::read}));picture(*c,1,pack.frames[0]);}
    CHECK(!reads[0].empty()&&reads[0]==reads[1]);
}
void interruptions(){
    label="implicit scratch paused across synchronous image, bitmap, cancel and same-address reopen";
    const Package p=implicit_fixture();Provider provider{&p.bytes};auto c=std::make_unique<Content>();
    CHECK(c->open(provider.source()));const unsigned total=finish_implicit(*c,1);CHECK(total>100);
    for(unsigned cutoff=1;cutoff<total;cutoff+=29){
        for(unsigned foreground:{2u,3u,4u}){
            CHECK(c->open(provider.source()));for(unsigned n=0;n<cutoff;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
            picture(*c,foreground,p.frames[foreground-1]);finish_implicit(*c,1);picture(*c,1,p.frames[0]);
        }
        CHECK(c->open(provider.source()));for(unsigned n=0;n<cutoff;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
        c->cancel_preload();finish_implicit(*c,2);picture(*c,2,p.frames[1]);finish_implicit(*c,1);picture(*c,1,p.frames[0]);
        CHECK(c->open(provider.source()));for(unsigned n=0;n<cutoff;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
        // Content::open is the actual cache reset used by both App::init entries.
        auto*address=c.release();address->~Content();c.reset(::new(static_cast<void*>(address)) Content);
        CHECK(c->open(provider.source()));finish_implicit(*c,2);picture(*c,2,p.frames[1]);
    }
    // A different Content can request the same global scratch while the
    // first owner is paused. It must cancel that task, not inherit its rows.
    auto other=std::make_unique<Content>();CHECK(other->open(provider.source()));
    CHECK(c->open(provider.source()));for(unsigned n=0;n<total/2;++n)CHECK(c->preload_image_step(1)==PreparationResult::Pending);
    picture(*other,3,p.frames[2]);finish_implicit(*c,1);picture(*c,1,p.frames[0]);
    CHECK(!provider.bad_range);
}
void faults(){
    label="implicit marker, mask, regenerated length, IO failure reject then retry cleanly";
    const Package original=implicit_fixture();Provider provider{&original.bytes};auto c=std::make_unique<Content>();
    for(unsigned mutation=0;mutation<9;++mutation)for(bool incremental:{false,true}){
        Package p=original;unsigned image=1;
        const unsigned index=p.tile_groups();auto&raw=p.groups[index].raw;
        const unsigned geo=12+unsigned(p.frames[0].ids.size()*4);
        if(mutation==0)raw[geo+39]|=0x10; // unused mask bit
        if(mutation==1)raw.back()=1;      // reserved padding
        if(mutation==2)raw[geo+2]=0;      // removes populated cells
        if(mutation==3)put(raw,0,unsigned(p.frames[0].expected.size())-1);
        if(mutation==4){image=3;auto&g=p.groups[index+2].raw;const unsigned at=12+unsigned(p.frames[2].ids.size()*4);g[at+2]^=1;}
        if(mutation==8){image=5;auto&g=p.groups[index+4].raw;const unsigned at=12+unsigned(p.frames[4].ids.size()*4);g[at+2]|=1;}
        encode46(p);if(mutation==5)put(p.bytes,4,45);
        provider.bytes=&p.bytes;CHECK(c->open(provider.source()));
        if(mutation==6){provider.fail_first=p.group_offsets[0]+32;provider.fail_last=provider.fail_first+1;}
        if(mutation==7){provider.fail_first=p.group_offsets[index]+32;provider.fail_last=provider.fail_first+1;}
        if(incremental){PreparationResult result=PreparationResult::Pending;unsigned steps=0;
            while(result==PreparationResult::Pending){result=c->preload_image_step(int(image));CHECK(++steps<20000);}
            CHECK(result==PreparationResult::Failed);
        }else CHECK(!c->image(int(image)).pixels);
        CHECK(!c->healthy()&&!c->image_cached(int(image))&&!resource_transfer_uses);
        provider.bytes=&original.bytes;provider.fail_first=provider.fail_last=0;
        CHECK(c->open(provider.source()));finish_implicit(*c,2);picture(*c,2,original.frames[1]);
    }
    CHECK(!provider.bad_range);
}
}
int main(){exact_images();interruptions();faults();
    std::printf("NSP46 implicit geometry: %u checks; original bytes, group order, scratch interruption, cancellation, same-address reopen, faults\n",checks);
}
