#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
using namespace spire;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&b,size_t p,uint32_t n,unsigned width=4){for(unsigned i=0;i<width;i++)b[p+i]=(uint8_t)(n>>(i*8));}
static void hash(std::vector<uint8_t>&b){uint32_t h=2166136261u;for(size_t i=40;i<b.size();i++){h^=b[i];h*=16777619u;}put(b,36,h);}
// Generated synthetic parser fixtures; historical save fixture bytes are
// unrelated and are never rewritten when a content profile changes.
constexpr unsigned minimum_images=CardKinds>A_Count+160?CardKinds:A_Count+160;
constexpr unsigned fixture_images=minimum_images>U_Count?minimum_images:U_Count;
static std::vector<uint8_t> pack(unsigned version=ContentVersion){
    const unsigned images=fixture_images,strings=version==14?ContentStrings:version==13?1556:version==12?749:version==11?407:EventTextEnd;
    const unsigned clips=version==14?AnimationKinds:version==13?38:17,cards=version==14?CardKinds:version==13?497:version==12?228:version==11?114:68;
    const unsigned ui_count=version==14?U_Count:version==13?339:version==12?233:version==11?185:version==10?105:90;
    const unsigned dir=40+images*16+strings*8+clips*16+cards*16+ui_count*4;
    std::vector<uint8_t>b(dir+images*82*60*2+strings*2);
    put(b,0,0x3250534e);put(b,4,version);put(b,8,0x41cef1ea);put(b,12,(uint32_t)b.size());
    put(b,16,images);put(b,24,strings);put(b,28,clips);put(b,32,cards);
    for(unsigned i=0;i<images;i++){
        put(b,40+i*16,dir+i*9840);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,9840);
    }
    unsigned off=40+images*16;
    for(unsigned i=0;i<strings;i++){
        unsigned p=dir+images*9840+i*2;put(b,off+i*8,p);put(b,off+i*8+4,1);b[p]='X';
    }
    off+=strings*8;
    for(unsigned i=0;i<clips;i++){put(b,off+i*16,0);put(b,off+i*16+4,1,2);put(b,off+i*16+6,33,2);}
    off+=clips*16;
    for(unsigned i=0;i<cards;i++)for(unsigned j=0;j<4;j++)put(b,off+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<ui_count;i++)put(b,off+cards*16+i*4,i==U_CampFlames?A_Count:i);
    hash(b);return b;
}
struct AliasImage {unsigned w,h,format,length;};
struct AliasGlyph {unsigned cp,size,w,h,advance,encoded;};
// A minimal valid v5 profile: one shared card image, synthetic aliased images,
// empty strings and single-frame clips. No original payload is embedded.
static std::vector<uint8_t> alias_pack(AliasImage first,AliasImage second,const std::vector<uint8_t>&pixels,
                                     const std::vector<AliasGlyph>&glyphs={},const std::vector<uint8_t>&glyph_bytes={},
                                     unsigned images=160){
    constexpr unsigned strings=66,clips=11,cards=26,ui=56;
    const unsigned dir=40+images*16+(unsigned)glyphs.size()*16+strings*8+clips*16+cards*16+ui*4,at=dir+9840;
    std::vector<uint8_t>b(at+pixels.size()+glyph_bytes.size()+1);
    put(b,0,0x3250534e);put(b,4,5);put(b,8,0x41cef1ea);put(b,12,(uint32_t)b.size());
    put(b,16,images);put(b,20,(unsigned)glyphs.size());put(b,24,strings);put(b,28,clips);put(b,32,cards);
    for(unsigned i=0;i<images;++i){
        const auto image=i==0?AliasImage{82,60,0,9840}:i==2?second:first;
        put(b,40+i*16,i==0?dir:at);put(b,44+i*16,image.w,2);put(b,46+i*16,image.h,2);
        put(b,48+i*16,image.format);put(b,52+i*16,image.length);
    }
    unsigned off=40+images*16;
    for(const auto&g:glyphs){
        put(b,off,g.cp);put(b,off+4,at+(unsigned)pixels.size());put(b,off+8,g.size,2);
        put(b,off+10,g.w,1);put(b,off+11,g.h,1);put(b,off+12,g.advance,2);put(b,off+14,g.encoded,2);off+=16;
    }
    for(unsigned i=0;i<strings;++i)put(b,off+i*8,(uint32_t)b.size()-1);
    off+=strings*8;
    for(unsigned i=0;i<clips;++i){put(b,off+i*16+4,1,2);put(b,off+i*16+6,33,2);}
    std::memcpy(b.data()+at,pixels.data(),pixels.size());
    if(!glyph_bytes.empty())std::memcpy(b.data()+at+pixels.size(),glyph_bytes.data(),glyph_bytes.size());
    hash(b);return b;
}
struct AliasProvider {
    const std::vector<uint8_t>* bytes;bool fail=false;
    static bool read(void*context,uint32_t offset,void*out,uint32_t count){
        const auto&p=*(AliasProvider*)context;
        if(p.fail||offset>p.bytes->size()||count>p.bytes->size()-offset)return false;
        std::memcpy(out,p.bytes->data()+offset,count);return true;
    }
    ResourceSource source(){return {this,(uint32_t)bytes->size(),read,nullptr};}
};
static std::vector<uint8_t> image_pressure_pack(){
    const unsigned images=Content::image_capacity_entries()+32;
    const AliasImage tiny{1,1,0,2};auto bytes=alias_pack(tiny,tiny,{0x34,0x12},{},{},images);
    // Distinct payload offsets exceed the current public cache capacity.
    // Reopening one two-key owner is not cache pressure.
    if(bytes.size()%2)bytes.push_back(0);
    for(unsigned i=1;i<images;++i){
        put(bytes,40+i*16,(unsigned)bytes.size());bytes.push_back(uint8_t(i));bytes.push_back(uint8_t(i*31));
    }
    put(bytes,12,(unsigned)bytes.size());hash(bytes);return bytes;
}
static void image_cache_aliases(){
    auto memory=std::make_unique<Content>(),ranged=std::make_unique<Content>();
    AliasProvider source{nullptr};
    const auto valid=[&](AliasImage first,AliasImage second,const std::vector<uint8_t>&payload,const std::vector<uint8_t>&decoded){
        auto b=alias_pack(first,second,payload);source.bytes=&b;
        CHECK(memory->open(b.data(),b.size()));CHECK(ranged->open(source.source()));
        for(unsigned id=1;id<=2;++id){
            const auto spec=id==1?first:second;const unsigned fmt=spec.format==5?payload[0]:spec.format;
            const auto&bytes=spec.format==5?decoded:payload;const unsigned count=spec.format==5?(unsigned)decoded.size():spec.length;
            const Picture m=memory->image(id),r=ranged->image(id);
            CHECK(m.pixels&&r.pixels&&m.w==spec.w&&r.w==spec.w&&m.h==spec.h&&r.h==spec.h);
            CHECK(m.format==fmt&&r.format==fmt);
            const uint8_t*mp=fmt==2||fmt==3?m.palette:m.pixels;const uint8_t*rp=fmt==2||fmt==3?r.palette:r.pixels;
            CHECK(mp&&rp&&!std::memcmp(mp,bytes.data(),count)&&!std::memcmp(rp,bytes.data(),count));
            if(fmt==1||fmt==3){
                CHECK(m.alpha==m.pixels+spec.w*spec.h*(fmt==1?2:1));
                CHECK(r.alpha==r.pixels+spec.w*spec.h*(fmt==1?2:1));
            }else CHECK(!m.alpha&&!r.alpha);
        }
        // Exact aliases still share the bounded decoded view. Comparing the
        // resident directory must not produce another provider range read.
        const Picture m1=memory->image(1),r1=ranged->image(1);
        const auto before_memory=memory->read_stats(),before_range=ranged->read_stats();
        const Picture m3=memory->image(3),r3=ranged->image(3);
        CHECK(m1.pixels==m3.pixels&&r1.pixels==r3.pixels);
        CHECK(memory->read_stats().direct_reads==before_memory.direct_reads);
        CHECK(ranged->read_stats().callbacks==before_range.callbacks&&ranged->read_stats().cache_hits==before_range.cache_hits);
        CHECK(memory->healthy()&&ranged->healthy());
    };
    const AliasImage raw{1,1,0,2},compressed{1,1,5,8},spans{2,1,4,11};
    const std::vector<uint8_t> pixel={0x34,0x12},rgba={0x34,0x12,0xab};
    const std::vector<uint8_t> packed={0,2,0,0,0,0x20,0x34,0x12};
    const std::vector<uint8_t> span_bytes={1,0,0,0,2,0,0,0x34,0x12,0x78,0x56};
    valid(raw,raw,pixel,{});
    valid(raw,{1,1,1,3},rgba,{}); // Both prefix interpretations are legal.
    valid({2,1,0,4},{1,2,0,4},{0x34,0x12,0x78,0x56},{});
    valid(compressed,compressed,packed,pixel);
    valid({2,1,5,10},{1,2,5,10},{0,4,0,0,0,0x40,0x34,0x12,0x78,0x56},{0x34,0x12,0x78,0x56});
    valid(spans,spans,span_bytes,{});
    std::vector<uint8_t> indexed(514);indexed[14]=0x34;indexed[15]=0x12;indexed[512]=7;indexed[513]=0xab;
    valid({1,1,2,513},{1,1,3,514},indexed,{});

    const auto recovery=alias_pack(raw,raw,pixel);
    const auto rejected=[&](AliasImage good,AliasImage bad,const std::vector<uint8_t>&payload){
        for(bool reverse:{false,true}){
            auto b=alias_pack(reverse?bad:good,reverse?good:bad,payload);source.bytes=&b;
            CHECK(memory->open(b.data(),b.size()));CHECK(ranged->open(source.source()));
            const unsigned good_id=reverse?2:1,bad_id=reverse?1:2;
            CHECK(memory->image(good_id).pixels&&ranged->image(good_id).pixels);
            CHECK(!memory->image(bad_id).pixels&&!ranged->image(bad_id).pixels);
            CHECK(!memory->healthy()&&!ranged->healthy());
            CHECK(!memory->image(good_id).pixels&&!ranged->image(good_id).pixels);
            // Reopening the same Content owners discards both a failed key
            // and any previously cached good aliases from that source.
            source.bytes=&recovery;CHECK(memory->open(recovery.data(),recovery.size()));CHECK(ranged->open(source.source()));
            CHECK(memory->healthy()&&ranged->healthy());
            CHECK(!std::memcmp(memory->image(1).pixels,pixel.data(),2)&&!std::memcmp(ranged->image(1).pixels,pixel.data(),2));
        }
    };
    rejected(raw,{1,1,0,1},pixel);
    rejected(raw,{2,1,0,2},pixel);
    rejected(raw,{1,1,1,2},pixel);
    rejected(compressed,{1,1,5,7},packed);
    rejected(compressed,{2,1,5,8},packed);
    rejected(compressed,{1,1,0,8},packed);
    rejected(spans,{1,2,4,11},span_bytes); // Equal area, invalid row geometry.
    std::vector<uint8_t> packed_spans={4,11,0,0,0,0xb0};packed_spans.insert(packed_spans.end(),span_bytes.begin(),span_bytes.end());
    rejected({2,1,5,17},{1,2,5,17},packed_spans);

    // Populate another owner past the entry limit, then explicitly retain
    // the tiny alias again before failing the evicted card's real read.
    const auto pressure=image_pressure_pack();auto evictor=std::make_unique<Content>();AliasProvider other{&pressure};
    CHECK(evictor->open(other.source()));
    for(unsigned i=0;i<Content::image_capacity_entries()+32;++i)CHECK(evictor->image(i).pixels);
    CHECK(ranged->image(1).pixels);
    CHECK(!ranged->image_cached(0)&&ranged->image_cached(1));
    source.fail=true;CHECK(ranged->image(1).pixels);CHECK(!ranged->image(0).pixels);
    CHECK(!ranged->healthy()&&!ranged->image(1).pixels&&!ranged->image(3).pixels);
    CHECK(memory->healthy()&&memory->image(1).pixels); // Other owners stay usable.
    const std::vector<uint8_t> different_pixel={0xef,0xbe};const auto replaced=alias_pack(raw,raw,different_pixel);
    source.fail=false;source.bytes=&replaced;CHECK(ranged->open(source.source()));
    CHECK(ranged->healthy()&&!std::memcmp(ranged->image(1).pixels,different_pixel.data(),2));
    CHECK(!std::memcmp(memory->image(1).pixels,pixel.data(),2));
}
static void glyph_cache_aliases(){
    auto memory=std::make_unique<Content>(),ranged=std::make_unique<Content>();AliasProvider source{nullptr};
    const AliasImage raw{1,1,0,2};const std::vector<uint8_t>pixel={0x34,0x12};
    const auto valid=[&](AliasGlyph first,AliasGlyph second,const std::vector<uint8_t>&payload){
        const auto b=alias_pack(raw,raw,pixel,{first,second},payload);source.bytes=&b;
        for(bool reverse:{false,true}){
            CHECK(memory->open(b.data(),b.size()));CHECK(ranged->open(source.source()));
            const AliasGlyph order[]={reverse?second:first,reverse?first:second};
            Glyph previous{};ResourceReadStats before{};
            for(unsigned i=0;i<2;++i){
                const auto spec=order[i];const unsigned length=spec.encoded?spec.encoded:spec.w*spec.h;
                const Glyph m=memory->glyph(spec.cp,spec.size),r=ranged->glyph(spec.cp,spec.size);
                CHECK(m.pixels&&r.pixels&&m.w==spec.w&&r.w==spec.w&&m.h==spec.h&&r.h==spec.h);
                CHECK(m.advance==spec.advance&&r.advance==spec.advance&&m.encoded_size==spec.encoded&&r.encoded_size==spec.encoded);
                CHECK(!std::memcmp(m.pixels,payload.data(),length)&&!std::memcmp(r.pixels,payload.data(),length));
                const unsigned previous_length=order[0].encoded?order[0].encoded:order[0].w*order[0].h;
                if(i&&length==previous_length){
                    // Glyphs store the original bytes: identical byte ranges
                    // remain shareable across code points, layout and encoding.
                    CHECK(previous.pixels==r.pixels);
                    CHECK(ranged->read_stats().callbacks==before.callbacks&&ranged->read_stats().cache_hits==before.cache_hits);
                }
                previous=r;before=ranged->read_stats();
            }
            CHECK(memory->healthy()&&ranged->healthy());
        }
    };
    valid({'A',10,1,1,2,0},{'B',10,2,2,3,0},{0x21,0x35,0x67,0x91}); // Short raw prefix, then the full glyph.
    valid({'A',10,1,1,2,2},{'B',10,3,1,4,4},{1,0x21,2,0x91}); // Two valid RLE prefixes.
    valid({'A',10,2,1,3,0},{'B',12,1,2,7,0},{0x21,0x91}); // Same bytes, different font/layout.
    valid({'A',10,2,1,3,0},{'B',10,1,1,7,2},{1,0x7f}); // Raw and RLE interpretations share two bytes.

    const std::vector<AliasGlyph>specs={{'A',10,1,1,2,0},{'B',10,2,2,3,0}};
    const std::vector<uint8_t>old_bytes={0x21,0x35,0x67,0x91},new_bytes={0x12,0x53,0x76,0x19};
    const auto original=alias_pack(raw,raw,pixel,specs,old_bytes),replacement=alias_pack(raw,raw,pixel,specs,new_bytes);
    source.bytes=&original;CHECK(memory->open(original.data(),original.size()));CHECK(ranged->open(source.source()));
    CHECK(ranged->glyph('A',10).pixels&&ranged->glyph('B',10).pixels);
    // A later failed image read invalidates cached glyph views as well.
    const auto pressure=image_pressure_pack();auto evictor=std::make_unique<Content>();AliasProvider other{&pressure};
    CHECK(evictor->open(other.source()));
    for(unsigned i=0;i<Content::image_capacity_entries()+32;++i)CHECK(evictor->image(i).pixels);
    source.fail=true;CHECK(!ranged->image(0).pixels);CHECK(!ranged->healthy());
    CHECK(!ranged->glyph('A',10).pixels&&!ranged->glyph('B',10).pixels);
    CHECK(memory->healthy()&&!std::memcmp(memory->glyph('B',10).pixels,old_bytes.data(),old_bytes.size()));
    source.fail=false;source.bytes=&replacement;CHECK(ranged->open(source.source()));
    CHECK(!std::memcmp(ranged->glyph('A',10).pixels,new_bytes.data(),1));
    CHECK(!std::memcmp(ranged->glyph('B',10).pixels,new_bytes.data(),new_bytes.size()));
    CHECK(!std::memcmp(memory->glyph('B',10).pixels,old_bytes.data(),old_bytes.size()));
}
int main(int argc,char**argv){
    image_cache_aliases();
    glyph_cache_aliases();
    auto original=pack();Content c;CHECK(c.open(original.data(),original.size()));CHECK(c.card_count()==CardKinds);CHECK(c.current_profile());
    for(int i=0;i<CardKinds;i++){CHECK(c.card_image(i)==i);CHECK(!std::strcmp(c.card_string(i),"X"));}
    CHECK(c.card_image(-1)==-1&&c.card_image(CardKinds)==-1);CHECK(!*c.card_string(CardKinds));
    constexpr unsigned carddir=40+fixture_images*16+ContentStrings*8+AnimationKinds*16;
    auto b=original;put(b,carddir+23*16,9999);hash(b);CHECK(c.open(b.data(),b.size()));CHECK(c.card_image(23)==-1);
    b=original;put(b,carddir+3*4,ContentStrings);hash(b);CHECK(c.open(b.data(),b.size()));CHECK(!*c.card_string(0,2));
    b=original;put(b,32,25);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;b.back()^=1;CHECK(c.open(b.data(),b.size()));CHECK(c.image(0).pixels);CHECK(!*c.string(ContentStrings-1)&&!c.healthy());
    CHECK(!c.open(nullptr,0));CHECK(c.card_count()==0);
    for(unsigned n=0;n<40;n++)CHECK(!c.open(original.data(),n));
    // Card count alone cannot identify the new relic-art directory. A valid
    // older profile stays readable by Content but must not boot this App.
    static App legacy;
    for(unsigned version:{8u,10u,11u,12u,13u}){
        b=pack(version);CHECK(c.open(b.data(),b.size()));CHECK(c.card_count()==(version==13?497u:version==12?228u:version==11?114u:68u));
        CHECK(!c.current_profile());CHECK(!legacy.init(b.data(),b.size(),{}));
    }
    // The immutable v11/v12 directories must not expand when the current registry
    // does. Merely relabeling either version leaves an incompatible pack.
    b=original;put(b,4,11);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=pack(11);put(b,4,12);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,4,12);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=pack(12);put(b,4,13);hash(b);CHECK(!c.open(b.data(),b.size()));
    b=original;put(b,28,17);hash(b);CHECK(!c.open(b.data(),b.size()));
    if(argc>1){
        FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));long length=std::ftell(f);CHECK(length>0&&length<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t> data((size_t)length);size_t n=std::fread(data.data(),1,data.size(),f);std::fclose(f);
        CHECK(c.open(data.data(),n));
        for(int i=0;i<CardKinds;i++){
            CHECK(c.image(c.card_image(i)).pixels);CHECK(c.image(c.card_image(i)).w==82);
            for(int v=0;v<3;v++)CHECK(*c.card_string(i,v));
        }
        CHECK(c.current_profile());
        for(int i=233;i<U_Count;i++){auto picture=c.image(c.ui_image(i));CHECK(picture.pixels&&picture.w&&picture.h);}
        for(int i=17;i<AnimationKinds;i++){
            Clip clip=c.clip(i);CHECK(clip.count&&clip.frame_ms);CHECK(c.image(clip.first).pixels);CHECK(c.image(clip.first+clip.count-1).pixels);
        }
        CHECK(c.frame_image(Card{DefectZap,0})==c.ui_image(U_BlueSkill));
        CHECK(c.frame_image(Card{RegentAlignment,0})==c.ui_image(U_OrangeSkill));
        CHECK(c.frame_image(Card{NecroAfterlife,0})==c.ui_image(U_PinkSkill));
        CHECK(c.energy_image(Card{DefectZap,0})==c.ui_image(U_DefectEnergy));
        CHECK(c.energy_image(Card{RegentAlignment,0})==c.ui_image(U_RegentEnergy));
        CHECK(c.energy_image(Card{NecroAfterlife,0})==c.ui_image(U_NecrobinderEnergy));
        for(int i=0;i<WorldRelicKinds;i++)CHECK(c.relic_image(i)==c.world_image("relics",world_relic_key(RelicId(i))));
        static App a;CHECK(a.init(data.data(),n,{}));new_run(a.game,7,true);a.title=false;
        const Card offering{Offering,1};char desc[512];a.card_description(offering,0,desc,sizeof desc);
        CHECK(std::strstr(desc,"6"));CHECK(std::strstr(desc,"5"));CHECK(!std::strchr(desc,'{'));
        a.render();
    }
    std::printf("content: %u checks passed\n",checks);
}
