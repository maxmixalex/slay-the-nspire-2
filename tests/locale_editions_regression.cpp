// Actual optional language packs exercise the production formatting, loading
// and sprite-font routes. The public no-asset mode checks only the registry.
#include "../src/app.cpp"
#include "locale_boot.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace spire;
static App app;
static unsigned checks,failures,descriptions;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"locale editions: line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<uint8_t> file(const std::string&path){
    FILE*f=std::fopen(path.c_str(),"rb");if(!f)return {};
    CHECK(!std::fseek(f,0,SEEK_END));const long length=std::ftell(f);CHECK(length>0);CHECK(!std::fseek(f,0,SEEK_SET));
    std::vector<uint8_t>data(size_t(length),0);CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));return data;
}
static uint32_t word(const std::vector<uint8_t>&data,unsigned at){CHECK(at+4<=data.size());return uint32_t(data[at])|uint32_t(data[at+1])<<8|uint32_t(data[at+2])<<16|uint32_t(data[at+3])<<24;}
static void put(std::vector<uint8_t>&data,unsigned at,uint32_t value){CHECK(at+4<=data.size());for(unsigned i=0;i<4;++i)data[at+i]=uint8_t(value>>(i*8));}
static void startup_fallback(ResourceSource common,const std::vector<uint8_t>&english,
                             const std::vector<uint8_t>&language,unsigned id){
    if(id==1)return;
    // Exercise real container parsing and App::init after a failed selected
    // locale. The last case injects a provider read failure after open(),
    // when the UI directory is first requested; it cannot be caught by the
    // calculator's cheap installed-filename probe.
    const bool blocked=word(language,0)==0x334c534eu;
    for(unsigned damage=0;damage<(blocked?6u:4u);++damage){
        app.title=true;app.library=app.character_select=app.run_settings=false;
        std::vector<uint8_t> broken;
        if(damage==1)broken.assign(language.begin(),language.begin()+32);
        if(damage==2){broken=language;broken[0]^=0xff;}
        if(damage==4){broken=language;put(broken,32,0);} // Invalid NSL3 address table.
        if(damage==5){
            // Valid table/extent around an invalid one-byte compressed page.
            // Opening must fail at functional LZ4 decoding, then try English.
            constexpr uint32_t mask=0x7fffffffu;const unsigned blocks=word(language,24),first=word(language,32)&mask,last=word(language,36)&mask;
            CHECK(first<last&&last<=language.size());const unsigned removed=last-first-1;
            broken.assign(language.begin(),language.begin()+first);broken.push_back(0xf0);broken.insert(broken.end(),language.begin()+last,language.end());
            put(broken,12,unsigned(broken.size()));put(broken,32,first);
            for(unsigned i=1;i<=blocks;++i){const unsigned value=word(language,32+i*4);put(broken,32+i*4,((value&mask)-removed)|(value&~mask));}
        }
        const auto damaged=damage==3?ResourceSource::memory(language.data(),language.size()):ResourceSource::memory(broken.data(),broken.size());
        SplitResourceSource split;uint32_t available=locale_language_mask(1)|locale_language_mask(id);unsigned calls=0;
        const auto load=[&](unsigned candidate){
            CHECK(candidate==(calls?1u:id));++calls;
            const auto bytes=candidate==1?ResourceSource::memory(english.data(),english.size()):damaged;
            if(!split.open(common,bytes,ResourceLanguage(candidate)))return false;
            auto source=split.source();
            if(candidate==id&&damage==3){
                source.direct=nullptr;source.read_at=[](void*,uint32_t,void*,uint32_t){return false;};
            }
            return app.init(source,{});
        };
        // Saved or OS-selected pack failures follow the same production path.
        CHECK(locale_load_initial(available,damage&1?id:0,id,load)==1);
        CHECK(calls==2&&available==locale_language_mask(1));
        CHECK(app.content.language()==1&&app.content.healthy()&&app.title);
        app.render();CHECK(app.content.healthy());
    }
}
static void reset(unsigned character=0){
    new_run(app.game,0x4c434431,false,Character(character));init_world_relics(app.game);app.game.phase=Phase::Map;
    app.title=app.character_select=app.run_settings=false;app.library=true;CHECK(validate(app.game));
}
static void normal_label_pixels(){
    // FontManager's pinned substitution list, independently of the production
    // selector: these seven locales use Regular for MegaLabel.normal_font.
    const unsigned id=app.content.language();
    const bool substitute=id==2||id==3||id==9||id==10||id==11||id==13||id==15;
    const unsigned face=substitute?0:5;Canvas canvas{app};
    const char*label=app.content.world_string("card_selection","VIEW_UPGRADES");CHECK(*label);
    std::fill(std::begin(app.pixels),std::end(app.pixels),0);
    Canvas expected=canvas;expected.font_face=uint8_t(face);Canvas shadow=expected;shadow.opacity=128;
    shadow.text(label,19,198,8,0,140,1);expected.text(label,18,197,8,rgb(239,200,81),140,1);
    std::vector<uint16_t> pixels(std::begin(app.pixels),std::end(app.pixels));
    std::fill(std::begin(app.pixels),std::end(app.pixels),0);app.hit_count=0;
    card_selection_upgrades_control(canvas);
    for(unsigned y=197;y<213;++y)for(unsigned x=18;x<159;++x)CHECK(app.pixels[y*ScreenW+x]==pixels[y*ScreenW+x]);
    // The title and energy labels share the same locale role. Rich emphasis
    // remains a separate Bold/Italic role in body text.
    CHECK(app.content.original_label_fonts()==!substitute);
    CHECK(source_label_face(!substitute,5)==face&&source_label_face(!substitute,6)==(substitute?0u:6u));
    Canvas rich=canvas;rich.bold_face=5;RichReader reader("A[b]A[/b][i]A[/i]");RichChar ch{};
    const unsigned faces[]={0,5,2},ancient_faces[]={0,5,6};
    for(unsigned i=0;i<3;++i){CHECK(reader.next(ch));CHECK(rich.font_style(ch)==faces[i]);CHECK(ancient_font_face(ch)==ancient_faces[i]);}
}
static void resolved(const char*context,const char*key,const char*text){
    ++descriptions;
    if(std::strchr(text,'{')||std::strchr(text,'}')){
        ++failures;std::fprintf(stderr,"unexpanded language=%s route=%s model=%s\n",locale_language_info(app.content.language()).code,context,key);
    }
}
static void model_text(){
    reset();char text[8192]{};
    for(unsigned kind=0;kind<CardKinds;++kind)for(unsigned upgraded=0;upgraded<2;++upgraded){
        app.card_description({CardKind(kind),uint8_t(upgraded)},-1,text,sizeof text);resolved("card",definitions[kind].key,text);
    }
    for(unsigned relic=0;relic<WorldRelicKinds;++relic){app.relic_description(RelicId(relic),text,sizeof text);resolved("relic",world_relic_key(RelicId(relic)),text);}
    for(unsigned potion=1;potion<=PotionKinds;++potion){
        const auto*d=potion_definition(Potion(potion));CHECK(d);ui_potion_description(app,int(potion),text,sizeof text);resolved("potion",d->key,text);
    }
    for(unsigned character=0;character<CharacterKinds;++character){
        for(unsigned event=0;event<unsigned(OriginalEventId::Count);++event){
            const auto id=OriginalEventId(event);if(!original_event_implemented(id))continue;reset(character);app.library=false;
            if(!original_event_begin(app.game,id,0x4c434431,0))continue;
            app.original_event_text(-1,true,text,sizeof text);resolved("event",original_event_key(id),text);
            const unsigned count=original_event_option_count(app.game);
            for(unsigned option=0;option<count;++option)for(unsigned description=0;description<2;++description){
                app.original_event_text(int(option),description!=0,text,sizeof text);resolved("event-option",original_event_key(id),text);
            }
        }
    }
}
static unsigned glyphs(){
    std::set<uint32_t>seen;
    // Full native text inventory, including pack-local composed Thai glyphs.
    for(unsigned slot=0;slot<8704;++slot){RichReader reader(app.content.string(slot));RichChar ch{};while(reader.next(ch))
        if(ch.codepoint>=32&&!text_format_control(ch.codepoint)&&!inline_icon(ch.codepoint))seen.insert(ch.codepoint);}
    for(unsigned id=1;id<=LocaleLanguageCount;++id){RichReader reader(locale_language_info(id).name);RichChar ch{};while(reader.next(ch))seen.insert(ch.codepoint);}
    for(uint32_t codepoint:seen)for(unsigned size:{8u,10u,12u}){
        const auto g=app.content.glyph(codepoint,int(size),0);
        if(!(g.advance>0&&g.w>0&&g.h>0&&g.pixels))std::fprintf(stderr,"missing visible glyph language=%s cp=U+%04X size=%u\n",locale_language_info(app.content.language()).code,codepoint,size);
        CHECK(g.advance>0&&g.w>0&&g.h>0&&g.pixels);
    }
    CHECK(app.content.healthy());return unsigned(seen.size());
}
static void energy_sprites(){
    static constexpr int assets[]={-1,U_SilentEnergy,U_DefectEnergy,U_RegentEnergy,U_NecrobinderEnergy};
    Canvas canvas{app};
    for(unsigned character=0;character<CharacterKinds;++character){reset(character);enter_combat(app.game);app.library=false;CHECK(validate(app.game));
        const int asset=character?app.content.ui_image(assets[character]):A_Energy;
        for(int size:{8,10,12}){
            canvas.rect(0,0,320,240,Panel);canvas.picture(asset,20,20,size,size);
            const std::vector<uint16_t>expected(app.pixels,app.pixels+ScreenW*ScreenH);
            canvas.rect(0,0,320,240,Panel);canvas.text(InlineEnergyText,20,20,size,White);
            CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
            canvas.rect(0,0,320,240,Panel);const std::string bold=std::string("[b][gold]")+InlineEnergyText+"[/gold][/b]";
            canvas.text(bold.c_str(),20,20,size,White);CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
            canvas.rect(0,0,320,240,Panel);
            profile_text_box(canvas,InlineEnergyText,20,20,size,size+3,{size,0,White,255,1,Ink,1},false,false,{},true,true);
            CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
            CHECK(canvas.width(InlineEnergyText,size)==size);
        }
    }
    // A scaled text box may crop on all four sides. Sprite glyphs must follow
    // that clip and must not be repeated by the shadow/outline draw passes.
    for(const char*icon:{InlineEnergyText,InlineStarText}){
        const ProfileTransform transform{24,24,5120};const int left=transform.horizontal(20),top=transform.vertical(20);
        const int right=transform.horizontal(26),bottom=transform.vertical(27),size=transform.horizontal(32)-left;
        const uint32_t cp=icon==InlineEnergyText?InlineEnergyCodepoint:InlineStarCodepoint;
        canvas.rect(0,0,320,240,Panel);canvas.picture(canvas.inline_picture(cp),left,top,size,size);
        std::vector<uint16_t>expected(app.pixels,app.pixels+ScreenW*ScreenH);
        for(int y=0;y<ScreenH;++y)for(int x=0;x<ScreenW;++x)if(x<left||x>=right||y<top||y>=bottom)expected[y*ScreenW+x]=Panel;
        canvas.rect(0,0,320,240,Panel);profile_text_box(canvas,icon,20,20,6,7,{12,0,White,255,1,Ink,1},false,false,transform,true,true);
        CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));
    }
}
int main(int argc,char**argv){
    // The original Turkish undiscovered-relic sentence begins with U+00AD.
    // It has no ink or advance and must not break a real adjacent letter pair.
    RichReader controls("A\xc2\xad" "V");RichChar visible{};
    CHECK(controls.next(visible)&&visible.codepoint=='A');
    CHECK(controls.next(visible)&&visible.codepoint=='V'&&visible.previous=='A');CHECK(!controls.next(visible));
    uint32_t mask=0;for(unsigned id=1;id<=LocaleLanguageCount;++id){const auto&info=locale_language_info(id);
        CHECK(locale_language_id(info.code)==id);mask|=locale_language_mask(id);CHECK(boot_preference(reinterpret_cast<const uint8_t*>(info.preference),8)==BootLanguage(id));}
    CHECK(locale_language_count(mask)==16);for(unsigned i=0;i<16;++i)CHECK(locale_language_at(mask,i)==i+1);
    CHECK(!locale_language_id("")&&!locale_language_id("e")&&!locale_language_id("en")&&!locale_language_id("english"));
    if(argc==1){std::printf("locale registry: %u checks passed\n",checks);return 0;}
    CHECK(argc==2||argc==3);const std::string directory=argv[1],selected=argc==3?std::string(",")+argv[2]+",":"";const auto common_bytes=file(directory+"/nSpire2.dat.tns");CHECK(!common_bytes.empty());
    ResourceSource common=ResourceSource::memory(common_bytes.data(),common_bytes.size());
    static MultipartResourceSource multipart;std::vector<uint8_t> parts[8];ResourceSource part_sources[8];
    if(word(common_bytes,0)==0x314d534eu){for(unsigned i=0;i<8;++i){parts[i]=file(directory+"/"+ArtworkPartNames[i]);CHECK(!parts[i].empty());part_sources[i]=ResourceSource::memory(parts[i].data(),parts[i].size());}
        CHECK(multipart.open(common,part_sources,8));common=multipart.source();}
    const auto english=file(directory+"/nSpire2.eng.dat.tns");CHECK(!english.empty());
    for(unsigned id=1;id<=LocaleLanguageCount;++id){const auto&info=locale_language_info(id);if(!selected.empty()&&selected.find(std::string(",")+info.code+",")==std::string::npos)continue;
        const auto language=file(directory+"/"+info.file);CHECK(!language.empty());
        startup_fallback(common,english,language,id);
        SplitResourceSource split;CHECK(split.open(common,ResourceSource::memory(language.data(),language.size()),ResourceLanguage(id)));
        CHECK(app.init(split.source(),{}));CHECK(app.content.language()==id);const unsigned begin=descriptions,failed=failures;
        model_text();const auto codepoints=glyphs();normal_label_pixels();energy_sprites();CHECK(app.content.healthy());
        std::printf("{\"language\":\"%s\",\"descriptions\":%u,\"visible_codepoints\":%u,\"unexpanded\":%u,\"energy_sprite_sizes\":3}\n",info.code,descriptions-begin,codepoints,failures-failed);std::fflush(stdout);
    }
    std::printf("locale editions: %u checks, %u descriptions, %u failures\n",checks,descriptions,failures);return failures?1:0;
}
