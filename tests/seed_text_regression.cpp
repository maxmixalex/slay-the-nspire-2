#include "reference_rng.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace spire::reference;
static unsigned checks=0;
static void require(bool ok,const char*message){
    ++checks;if(!ok){std::fprintf(stderr,"seed text: %s\n",message);std::exit(1);}
}
static std::string utf8(uint32_t cp){
    char text[5]{};require(seed_append(text,sizeof text,cp),"encode valid scalar");return text;
}
static std::string unhex(const std::string&text){
    if(text=="-")return {};
    require(!(text.size()&1),"even hexadecimal fixture length");
    std::string out;
    for(size_t i=0;i<text.size();i+=2){
        char*end=nullptr;const auto pair=text.substr(i,2);const auto b=std::strtoul(pair.c_str(),&end,16);
        require(end==pair.c_str()+2&&b<=255,"hexadecimal fixture byte");out.push_back(char(b));
    }
    return out;
}
static void canonical(const char*input,const char*expected){
    char out[512]{};require(canonicalize_seed(input,out,sizeof out),"valid canonical input");
    require(!std::strcmp(out,expected),"original invariant uppercase / replacements / Trim");
    char again[512]{};require(canonicalize_seed(out,again,sizeof again)&&!std::strcmp(out,again),"canonicalization is idempotent");
}
static void ordinary_text(){
    canonical("  nspire-i-o\t","NSP1RE-1-0");canonical("","");canonical(" \t\r\n","");
    canonical(u8"é öłđ",u8"É ÖŁĐ");canonical(u8"ıiİIſßﬃᾀ",u8"ı1İ1Sßﬃᾈ");
    canonical(u8"𐐨𞤢",u8"𐐀𞤀");canonical(u8"ＡｏＩ",u8"ＡＯＩ");
    canonical(u8"e\u0301",u8"E\u0301");canonical(u8"\u200btest\ufeff",u8"\u200bTEST\ufeff");
    canonical("oldA","0LDA");canonical(u8"oldé",u8"0LDÉ");
    const uint32_t spaces[]={9,10,11,12,13,32,0x85,0xa0,0x1680,0x2000,0x2001,0x2002,0x2003,
        0x2004,0x2005,0x2006,0x2007,0x2008,0x2009,0x200a,0x2028,0x2029,0x202f,0x205f,0x3000};
    for(uint32_t cp:spaces){const auto space=utf8(cp);canonical((space+"ai o"+space).c_str(),"A1 0");
        const std::string internal="A"+space+"B";canonical(internal.c_str(),internal.c_str());}
    uint64_t a=0,b=0;
    require(seed_hash(u8"É",a)&&seed_hash(u8"E\u0301",b)&&a!=b,"do not add NFC/NFKC normalization");
    require(seed_hash("oldA",a)&&a==UINT64_C(0xcdcab7dd),"signed legacy result is zero-extended by original conv.u8");
    require(seed_hash("olda",b)&&b==UINT64_C(0xcdcab7bd)&&a!=b,"raw legacy suffix retains its case");
    require(seed_hash("old",a)&&a==UINT64_C(0x2d2816fe),"empty legacy suffix uses the original two-lane hash");
    require(seed_hash("",a)&&a==UINT64_C(0xef46db3751d8e999),"explicit empty identity uses xxHash64 of empty UTF-8");
}
static void bounded_text(){
    struct Guard {unsigned before=0x719ec35au;char text[65]{};unsigned after=0x718ec35au;} guard;
    const auto intact=[&](){require(guard.before==0x719ec35au&&guard.after==0x718ec35au,"output guard bytes unchanged");};
    std::string exact(64,'a');require(canonicalize_seed(exact.c_str(),guard.text,sizeof guard.text),"exact 64-byte seed fits");
    require(std::strlen(guard.text)==64,"terminator fits after exact capacity");intact();
    require(!canonicalize_seed((exact+"a").c_str(),guard.text,sizeof guard.text)&&!guard.text[0],"over-capacity clears output without truncation");intact();
    const std::string expansion=std::string(62,'a')+u8"ȿ";
    require(expansion.size()==64&&!canonicalize_seed(expansion.c_str(),guard.text,sizeof guard.text),"uppercase UTF-8 expansion is capacity checked");intact();
    const std::string fits=std::string(61,'a')+u8"ȿ";
    require(canonicalize_seed(fits.c_str(),guard.text,sizeof guard.text)&&std::strlen(guard.text)==64,"uppercase exact expanded capacity fits");intact();
    require(canonicalize_seed(("\t "+exact+"\t ").c_str(),guard.text,sizeof guard.text),"discarded outer whitespace does not consume output capacity");intact();
    char one='z';require(canonicalize_seed("",&one,1)&&!one,"empty canonical result needs only a terminator");
    one='z';require(!canonicalize_seed("a",&one,1)&&!one,"single-byte output cannot fit nonempty text");
    require(!canonicalize_seed("a",nullptr,1),"null output is rejected");
    one='z';require(!canonicalize_seed("a",&one,0)&&one=='z',"zero capacity writes nothing");
    require(!canonicalize_seed(nullptr,guard.text,sizeof guard.text)&&!guard.text[0],"null input is rejected");
    uint64_t hash=UINT64_C(0x719ec35ad25a3);require(!seed_hash(nullptr,hash)&&hash==UINT64_C(0x719ec35ad25a3),"failed raw hash is atomic");
    const char*bad[]={"\x80","\xbf","\xc0\x80","\xc1\xbf","\xc2","\xe0\x80\x80","\xe0\xa0",
        "\xed\xa0\x80","\xed\xbf\xbf","\xf0\x80\x80\x80","\xf0\x90\x80","\xf4\x90\x80\x80","\xf5\x80\x80\x80","\xff","\xc2" "A"};
    for(const char*text:bad){
        require(!canonicalize_seed(text,guard.text,sizeof guard.text)&&!guard.text[0],"malformed / truncated UTF-8 rejected");intact();
        for(const char*prefix:{"","old"}){const std::string input=std::string(prefix)+text;const uint64_t previous=hash;
            require(!seed_hash(input.c_str(),hash)&&hash==previous,"malformed modern or legacy input preserves hash");}
    }
    for(uint32_t cp:{0x80u,0x800u,0x10000u,0x10ffffu}){
        const auto full=utf8(cp);for(size_t n=1;n<full.size();++n){const std::string partial=full.substr(0,n);
            require(!canonicalize_seed(partial.c_str(),guard.text,sizeof guard.text),"each truncation of valid multibyte input is rejected");intact();}
    }
}
static void editing(){
    char text[65]{};
    for(uint32_t cp:{uint32_t('a'),0xe9u,0x4e2du,0x1f600u})require(seed_append(text,sizeof text,cp),"append full Unicode scalar");
    require(!std::strcmp(text,u8"aé中😀"),"mixed UTF-8 edit string");
    seed_backspace(text);require(!std::strcmp(text,u8"aé中"),"remove four-byte scalar");
    seed_backspace(text);require(!std::strcmp(text,u8"aé"),"remove three-byte scalar");
    seed_backspace(text);require(!std::strcmp(text,"a"),"remove two-byte scalar");
    seed_backspace(text);seed_backspace(text);require(!*text,"backspace empty is stable");
    for(unsigned i=0;i<60;++i)require(seed_append(text,sizeof text,'a'),"fill editor");
    require(seed_append(text,sizeof text,0x1f600)&&std::strlen(text)==64,"astral scalar fills exact byte capacity");
    const std::string exact=text;require(!seed_append(text,sizeof text,'b')&&text==exact,"full append refuses without partial write");
    seed_backspace(text);require(std::strlen(text)==60,"full-capacity backspace retains valid prefix");
    for(uint32_t cp:{0u,0xd800u,0xdbffu,0xdc00u,0xdfffu,0x110000u,0xffffffffu}){
        const std::string old=text;require(!seed_append(text,sizeof text,cp)&&text==old,"invalid scalar cannot enter editor");}
    require(!seed_append(nullptr,65,'a')&&!seed_append(text,0,'a'),"invalid edit buffers rejected");seed_backspace(nullptr);
}
static void vectors(const char*path){
    std::ifstream in(path);require(in.good(),"open original method vectors");
    std::string line;unsigned count=0;
    while(std::getline(in,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream fields(line);std::string input_hex,canonical_hex,raw_hex,normalized_hex;
        require(bool(fields>>input_hex>>canonical_hex>>raw_hex>>normalized_hex),"four original vector fields");
        const std::string input=unhex(input_hex),expected=unhex(canonical_hex);char actual[512]{};
        if(!canonicalize_seed(input.c_str(),actual,sizeof actual)||actual!=expected){std::fprintf(stderr,"original canonical vector %u input=%s\n",count,input_hex.c_str());require(false,"original canonicalizer mismatch");}
        ++checks;uint64_t actual_raw=0,actual_normal=0;
        const uint64_t raw=std::strtoull(raw_hex.c_str(),nullptr,16),normal=std::strtoull(normalized_hex.c_str(),nullptr,16);
        if(!seed_hash(input.c_str(),actual_raw)||actual_raw!=raw){std::fprintf(stderr,"original raw vector %u input=%s expected=%016llx actual=%016llx\n",count,input_hex.c_str(),(unsigned long long)raw,(unsigned long long)actual_raw);require(false,"original RunRngSet mismatch");}
        ++checks;require(seed_hash(actual,actual_normal)&&actual_normal==normal,"original canonical RunRngSet hash");++count;
    }
    require(count>=30,"substantial original method vector set");std::printf("Original method vectors: %u\n",count);
}
static void scalars(const char*path){
    std::ifstream in(path);require(in.good(),"open complete original scalar oracle");
    std::vector<std::pair<uint32_t,std::string>> changed;std::string line;
    while(std::getline(in,line)){
        const auto tab=line.find('\t');require(tab!=std::string::npos,"scalar oracle separator");
        const uint32_t cp=uint32_t(std::strtoul(line.substr(0,tab).c_str(),nullptr,16));
        changed.emplace_back(cp,line.size()==tab+1?std::string():unhex(line.substr(tab+1)));
    }
    size_t entry=0;unsigned count=0;
    for(uint32_t cp=1;cp<=0x10ffff;++cp){
        if(cp>=0xd800&&cp<=0xdfff)continue;
        const std::string text=utf8(cp);const bool mapped=entry<changed.size()&&changed[entry].first==cp;
        const std::string&expected=mapped?changed[entry].second:text;char actual[8]{};
        if(!canonicalize_seed(text.c_str(),actual,sizeof actual)||actual!=expected){std::fprintf(stderr,"original scalar U+%X mismatch\n",cp);require(false,"complete original scalar canonicalization");}
        ++checks;++count;if(mapped)++entry;
    }
    require(entry==changed.size(),"consume every changed original scalar");
    std::printf("Original scalar canonicalization: %u\n",count);
}
int main(int argc,char**argv){
    ordinary_text();bounded_text();editing();if(argc>1)vectors(argv[1]);if(argc>2)scalars(argv[2]);
    std::printf("Seed text: %u checks\n",checks);return 0;
}
