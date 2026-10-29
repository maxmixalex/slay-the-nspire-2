#include "loading_screen.hpp"
#include "calculator_system_language.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&bytes,unsigned at,uint32_t n){for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(n>>(i*8));}
struct ProbeSource {const std::vector<uint8_t>&data;unsigned calls=0,bytes=0;};
static bool probe_read(void*context,uint32_t offset,void*out,uint32_t count){
    auto&s=*static_cast<ProbeSource*>(context);if(offset>s.data.size()||count>s.data.size()-offset)return false;
    ++s.calls;s.bytes+=count;std::memcpy(out,s.data.data()+offset,count);return true;
}
static uint16_t locale_result[64];
static int locale_expression,locale_error,locale_format_error;
static unsigned locale_evaluations,locale_formats,locale_releases;
static int locale_evaluate(void*first,void*second,const uint16_t*query,void*out,void*offset){
    CHECK(!first&&!second);const char*expected="getLangInfo()";
    for(unsigned i=0;i<=std::strlen(expected);++i)CHECK(query[i]==uint16_t(expected[i]));
    CHECK(*static_cast<int*>(offset)==0);*static_cast<void**>(out)=&locale_expression;++locale_evaluations;return locale_error;
}
static int locale_format(void*expression,void*options,uint16_t**out){
    CHECK(expression==&locale_expression&&!options);*out=locale_result;++locale_formats;return locale_format_error;
}
static void locale_release(void*pointer){CHECK(pointer==&locale_expression||pointer==locale_result);++locale_releases;}
static void settings_archive(){
    const char*xml="<settings><lang>zh_CN</lang><other>1</other></settings>";
    const unsigned length=unsigned(std::strlen(xml));
    std::vector<uint8_t>zip(42+length);
    put(zip,0,0x04034b50);zip[4]=20;put(zip,18,length);put(zip,22,length);zip[26]=12;
    std::memcpy(zip.data()+30,"settings.xml",12);std::memcpy(zip.data()+42,xml,length);
    CalculatorSettingsEntry entry;
    CHECK(calculator_settings_entry(zip.data(),zip.size(),entry));
    CHECK(!entry.deflated&&entry.data==zip.data()+42&&entry.compressed==length&&entry.plain==length);
    CHECK(calculator_language_xml(entry.data,entry.plain)==2);
    for(unsigned n=0;n<zip.size();++n)CHECK(!calculator_settings_entry(zip.data(),n,entry));
    auto damaged=zip;damaged[6]=1;CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    damaged=zip;damaged[6]=8;CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    damaged=zip;damaged[8]=7;CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    damaged=zip;put(damaged,22,8193);CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    damaged=zip;put(damaged,18,0xffffffff);CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    damaged=zip;damaged[28]=255;CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    damaged=zip;damaged[30]='x';CHECK(!calculator_settings_entry(damaged.data(),damaged.size(),entry));
    // ZIP metadata identifies raw Deflate; the actual OS inflater is exercised
    // separately in ordinary Firebird OS, not replaced with a mock decoder.
    damaged=zip;damaged[8]=8;CHECK(calculator_settings_entry(damaged.data(),damaged.size(),entry)&&entry.deflated);
    CHECK(calculator_language_xml(nullptr,20)==0);
    for(const char*bad:{"<lang>zh_CN", "<lang>zh_CN</other>", "<lang>不存在</lang>", "<lang>unknown</lang>"})
        CHECK(calculator_language_xml(reinterpret_cast<const uint8_t*>(bad),std::strlen(bad))==0);
    const char*english="<settings><lang>en_US</lang></settings>";
    CHECK(calculator_language_xml(reinterpret_cast<const uint8_t*>(english),std::strlen(english))==1);
}
int main(){
    settings_archive();
    const uint16_t quoted_zh[]={'"','z','h','_','C','N','"',0};
    std::memcpy(locale_result,quoted_zh,sizeof quoted_zh);
    CHECK(calculator_system_language(locale_evaluate,locale_format,locale_release)==2);
    CHECK(locale_evaluations==1&&locale_formats==1&&locale_releases==2);
    locale_error=1;
    CHECK(calculator_system_language(locale_evaluate,locale_format,locale_release)==0);
    CHECK(locale_evaluations==2&&locale_formats==1&&locale_releases==3);
    locale_error=0;locale_format_error=1;
    CHECK(calculator_system_language(locale_evaluate,locale_format,locale_release)==0);
    CHECK(locale_evaluations==3&&locale_formats==2&&locale_releases==5);
    CHECK(calculator_system_language(nullptr,locale_format,locale_release)==0);
    CHECK(locale_evaluations==3&&locale_releases==5);
    CHECK(calculator_language_result(nullptr)==0);
    locale_result[6]=0;CHECK(calculator_language_result(locale_result)==0);
    std::memcpy(locale_result,quoted_zh,sizeof quoted_zh);locale_result[7]='x';locale_result[8]=0;
    CHECK(calculator_language_result(locale_result)==0);
    locale_result[7]=0;locale_result[1]=0x4e2d;CHECK(calculator_language_result(locale_result)==0);
    for(auto&word:locale_result)word='e';CHECK(calculator_language_result(locale_result)==0);
    const struct {const char*tag;unsigned id;} system_languages[]={
        {nullptr,0},{"",0},{"C",0},{"C.UTF-8",0},{"POSIX",0},{"sv_SE",0},{"da_DK",0},{"nl_NL",0},
        {"fi_FI",0},{"no_NO",0},{"nb_NO",0},{"nn_NO",0},{"ar_SA",0},{"cs_CZ",0},
        {"en",1},{"EN_gb.UTF-8",1},{"eng",1},{"zh",2},{"zh_CN.UTF-8",2},{"zh-Hans-SG",2},
        {"zh_TW",3},{"zh-hant",3},{"zh-HK",3},{"zh_MO",3},{"de_DE@euro",4},
        {"es_MX",5},{"es-419",5},{"fr_FR.UTF-8",6},{"id-ID",7},{"it_IT",8},
        {"ja-JP",9},{"ko_KR",10},{"pl-PL",11},{"pt_PT",12},{"pt-BR",12},
        {"ru_RU",13},{"es",14},{"es_ES.UTF-8",14},{"es-Latn-ES",14},{"es-Latn-MX",5},
        {"es-Latn",14},{"th-TH",15},{"tr_TR",16}
    };
    for(const auto&language:system_languages){
        CHECK(locale_system_language_id(language.tag)==language.id);
        if(language.tag){
            const std::string xml=std::string("<settings><lang>")+language.tag+"</lang></settings>";
            CHECK(calculator_language_xml(reinterpret_cast<const uint8_t*>(xml.data()),xml.size())==language.id);
            std::fill(std::begin(locale_result),std::end(locale_result),0);
            locale_result[0]='"';const unsigned n=unsigned(std::strlen(language.tag));
            for(unsigned i=0;i<n;++i)locale_result[i+1]=uint8_t(language.tag[i]);locale_result[n+1]='"';
            CHECK(calculator_language_result(locale_result)==language.id);
        }
        // Unsupported calculator languages always choose English when installed,
        // including when the only other file is a supported unrelated locale.
        for(unsigned other=2;other<=LocaleLanguageCount;++other){
            const auto installed=locale_language_mask(1)|locale_language_mask(other);
            CHECK(locale_default_language(installed,0,language.id)==(language.id==other?other:1u));
        }
    }
    for(unsigned id=1;id<=LocaleLanguageCount;++id){
        CHECK(locale_default_language(0xffffu,id,id%LocaleLanguageCount+1)==id);
        CHECK(locale_default_language(0xffffu,0,id)==id);
        CHECK(locale_default_language(locale_language_mask(id),0,0)==id);
        unsigned calls=0;uint32_t installed=0xffffu;
        CHECK(locale_load_initial(installed,id,1,[&](unsigned selected){++calls;CHECK(selected==id);return true;})==id);
        CHECK(calls==1&&installed==0xffffu); // No extra pack/locale checks on success.
    }
    CHECK(locale_default_language(0,1,2)==0);
    CHECK(locale_default_language(locale_language_mask(1)|locale_language_mask(2),6,4)==1);
    CHECK(locale_default_language(locale_language_mask(4)|locale_language_mask(6),3,2)==4);
    CHECK(locale_default_language(locale_language_mask(4)|locale_language_mask(6),1,6)==6);
    // Explicit saved French -> system German -> English -> first remaining
    // Japanese. Each failed candidate is tried once, never stored as available.
    for(unsigned success:{6u,4u,1u,9u,0u}){
        uint32_t installed=locale_language_mask(1)|locale_language_mask(4)|locale_language_mask(6)|locale_language_mask(9);
        const unsigned expected[]={6,4,1,9};unsigned calls=0;
        const unsigned chosen=locale_load_initial(installed,6,4,[&](unsigned candidate){CHECK(calls<4&&candidate==expected[calls]);++calls;return candidate==success;});
        CHECK(chosen==success);CHECK(calls==(success==6?1:success==4?2:success==1?3:4));
        for(unsigned i=0;i<calls-(success?1:0);++i)CHECK(!(installed&locale_language_mask(expected[i])));
        if(success)CHECK(installed&locale_language_mask(success));else CHECK(!installed);
    }
    // Every installed subset verifies the final English/first-found contract.
    for(uint32_t installed=0;installed<0x10000u;++installed){
        unsigned first=0;for(unsigned id=1;id<=LocaleLanguageCount&&!first;++id)if(installed&locale_language_mask(id))first=id;
        CHECK(locale_default_language(installed,0,0)==first);
        CHECK(locale_default_language(installed,99,99)==first);
    }
    for(unsigned id=0;id<unsigned(BootText::Count);++id)for(unsigned language=0;language<=LocaleLanguageCount;++language){
        const char*label=boot_text(static_cast<BootText>(id),BootLanguage(language));CHECK(*label);CHECK(BootPaint::width(label)<=304);
        CHECK(!std::strcmp(label,boot_text(static_cast<BootText>(id),BootLanguage::English)));
        for(const char*p=label;*p;++p)CHECK(*p>=32&&*p<127);
    }
    std::vector<uint16_t> pixels(320*240+2,0xface);auto*screen=pixels.data()+1;
    for(auto language:{BootLanguage::Unknown,BootLanguage::English,BootLanguage::Chinese})for(unsigned stage=0;stage<unsigned(ResourceStage::Count);++stage){
        loading_screen(screen,static_cast<ResourceStage>(stage),50,100,0,nullptr,false,language);
        unsigned gold=0;for(unsigned x=40;x<280;++x)gold+=screen[169*320+x]==0xc568;CHECK(gold==120);
        CHECK(pixels.front()==0xface&&pixels.back()==0xface);
    }
    for(auto error:{BootText::MissingResources,BootText::MissingLanguage,BootText::InvalidResources}){
        boot_error_screen(screen,error,BootLanguage::English);
        const std::vector<uint16_t> english(pixels);
        for(unsigned language=0;language<=LocaleLanguageCount;++language){
            boot_error_screen(screen,error,static_cast<BootLanguage>(language));CHECK(pixels==english);
            CHECK(pixels.front()==0xface&&pixels.back()==0xface);
        }
    }
    loading_screen(nullptr,ResourceStage::Header,0,1,0,nullptr,false,BootLanguage::Chinese);
    for(unsigned id=1;id<=LocaleLanguageCount;++id){
        const auto language=static_cast<BootLanguage>(id);
        const auto*bytes=reinterpret_cast<const uint8_t*>(boot_preference_bytes(language));CHECK(boot_preference(bytes,8)==language);
        CHECK(boot_preference(bytes,7)==BootLanguage::Unknown);CHECK(boot_preference(bytes,9)==BootLanguage::Unknown);
    }
    CHECK(boot_preference(nullptr,8)==BootLanguage::Unknown);CHECK(boot_preference(reinterpret_cast<const uint8_t*>("NSL1zzz\n"),8)==BootLanguage::Unknown);
    std::vector<uint8_t> pack(40+16+16+6111*8+4);std::memcpy(pack.data(),"NSP2",4);
    put(pack,4,18);put(pack,8,0x41cef1ea);put(pack,12,pack.size());put(pack,16,1);put(pack,20,1);put(pack,24,6111);
    const unsigned row=40+16+16+6110*8;put(pack,row,pack.size()-4);put(pack,row+4,3);
    ProbeSource probe{pack};ResourceSource ranged{&probe,uint32_t(pack.size()),probe_read,nullptr};
    for(unsigned version=17;version<=Ui12ContentVersion;++version){put(pack,4,version);
        std::memcpy(pack.data()+pack.size()-4,"eng",4);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::English);
        probe.calls=probe.bytes=0;CHECK(boot_probe_language(ranged)==BootLanguage::English);CHECK(probe.calls==3&&probe.bytes==52);
        std::memcpy(pack.data()+pack.size()-4,"zhs",4);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Chinese);
        probe.calls=probe.bytes=0;CHECK(boot_probe_language(ranged)==BootLanguage::Chinese);CHECK(probe.calls==3&&probe.bytes==52);
    }
    put(pack,4,Ui12ContentVersion+1);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Unknown);
    probe.calls=probe.bytes=0;CHECK(boot_probe_language(ranged)==BootLanguage::Unknown);CHECK(probe.calls==1&&probe.bytes==40);put(pack,4,19);
    put(pack,row,0xffffffffu);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Unknown);
    put(pack,16,0xffffffffu);CHECK(boot_probe_language(ResourceSource::memory(pack.data(),pack.size()))==BootLanguage::Unknown);
    CHECK(boot_probe_language({})==BootLanguage::Unknown);
    std::printf("%u boot system-language/fallback/coverage/bounds checks passed\n",checks);
}
