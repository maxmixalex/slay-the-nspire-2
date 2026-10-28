// Actual optional language packs exercise the production formatting, loading
// and sprite-font routes. The public no-asset mode checks only the registry.
#include "../src/app.cpp"
#include "locale_boot.hpp"
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
static void reset(unsigned character=0){
    new_run(app.game,0x4c434431,false,Character(character));init_world_relics(app.game);app.game.phase=Phase::Map;
    app.title=app.character_select=app.run_settings=false;app.library=true;CHECK(validate(app.game));
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
    CHECK(argc==2||argc==3);const std::string directory=argv[1],selected=argc==3?std::string(",")+argv[2]+",":"";const auto common=file(directory+"/nSpire2.dat.tns");CHECK(!common.empty());
    for(unsigned id=1;id<=LocaleLanguageCount;++id){const auto&info=locale_language_info(id);if(!selected.empty()&&selected.find(std::string(",")+info.code+",")==std::string::npos)continue;
        const auto language=file(directory+"/"+info.file);CHECK(!language.empty());
        SplitResourceSource split;CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage(id)));
        CHECK(app.init(split.source(),{}));CHECK(app.content.language()==id);const unsigned begin=descriptions,failed=failures;
        model_text();const auto codepoints=glyphs();energy_sprites();CHECK(app.content.healthy());
        std::printf("{\"language\":\"%s\",\"descriptions\":%u,\"visible_codepoints\":%u,\"unexpanded\":%u,\"energy_sprite_sizes\":3}\n",info.code,descriptions-begin,codepoints,failures-failed);std::fflush(stdout);
    }
    std::printf("locale editions: %u checks, %u descriptions, %u failures\n",checks,descriptions,failures);return failures?1:0;
}
