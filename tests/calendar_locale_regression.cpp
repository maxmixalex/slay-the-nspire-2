#include "ui_calendar.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"calendar:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::string unhex(const std::string& hex){
    CHECK(hex.size()%2==0);std::string result;
    for(size_t i=0;i<hex.size();i+=2){char*end=nullptr;const auto pair=hex.substr(i,2);const auto value=std::strtoul(pair.c_str(),&end,16);CHECK(end&&!*end&&value<256);result+=char(value);}
    return result;
}
static void expected(const UiCalendar& value,unsigned language,const char* pattern,const std::string& text,bool boundaries){
    char output[160]{};CHECK(ui_calendar_format(value,language,pattern,output,sizeof output));
    if(output!=text)std::fprintf(stderr,"calendar language=%u pattern=%s expected=%s actual=%s\n",language,pattern,text.c_str(),output);
    CHECK(output==text);
    if(!boundaries)return;
    // Every capacity around UTF-8 month/day-period boundaries: an exhausted
    // buffer is empty, never a partial scalar, and adjacent bytes stay intact.
    for(size_t capacity=0;capacity<=text.size()+2;++capacity){
        unsigned char guarded[192];std::memset(guarded,0xa5,sizeof guarded);
        char* target=reinterpret_cast<char*>(guarded+8);
        const bool good=ui_calendar_format(value,language,pattern,target,capacity);
        CHECK(good==(capacity>text.size()));
        for(unsigned i=0;i<8;++i)CHECK(guarded[i]==0xa5);
        for(size_t i=8+capacity;i<sizeof guarded;++i)CHECK(guarded[i]==0xa5);
        if(good)CHECK(text==target);else if(capacity)CHECK(!target[0]);
    }
}
static void grammar(){
    const auto value=ui_calendar(1792850820u); // 2026-10-24 14:07 UTC.
    CHECK(value.year==2026&&value.month==10&&value.day==24&&value.hour==14&&value.minute==7);
    expected(value,11,"MMMM","październik",true);
    expected(value,11,"d MMMM","24 października",true);
    expected(value,11,"'d' MMMM","d październik",true);
    expected(value,11,"\\d MMMM","d październik",false);
    expected(value,13,"d MMMM","24 октября",true);
    expected(value,7,"h:mm tt","2.07 PM",true);
    expected(value,7,"h':'mm tt","2:07 PM",true);
    expected(value,10,"yyyy/MM/dd","2026. 10. 24",true);
    expected(value,15,"yyyy-M-d","2569-10-24",true);
    expected(value,0,"yyyy-M-d","2026-10-24",true);
    expected(value,99,"yyyy-M-d","2026-10-24",false);
    expected(value,9,"t","午",true);
    expected(value,16,"t","Ö",true);
    expected(value,14,"h:mm tt","2:07 p.\xc2\xa0m.",true);
    expected(value,1,"yy yyyyy 'year'","26 02026 year",false);
    char out[64];
    for(const char* pattern:{"MMM","dddd","HHH","ttt","%","s","yyyy 'unfinished","yyyy\\","yyyyyyyyyyy"}){
        std::strcpy(out,"stale");CHECK(!ui_calendar_format(value,1,pattern,out,sizeof out));CHECK(!out[0]);
    }
    CHECK(!ui_calendar_format(value,1,nullptr,out,sizeof out));CHECK(!out[0]);
    CHECK(!ui_calendar_format(value,1,"yyyy",nullptr,0));
    auto invalid=value;invalid.month=13;CHECK(!ui_calendar_format(invalid,1,"yyyy",out,sizeof out));CHECK(!out[0]);
}
int main(int argc,char** argv){
    CHECK(argc==2);std::ifstream source(argv[1]);CHECK(source.good());std::string line;unsigned rows=0;
    bool languages[17]{};
    while(std::getline(source,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream fields(line);std::vector<std::string> row;std::string field;
        while(std::getline(fields,field,'\t'))row.push_back(field);
        CHECK(row.size()==8);const unsigned language=unsigned(std::stoul(row[0]));CHECK(language>=1&&language<=16);languages[language]=true;
        const auto value=ui_calendar(uint32_t(std::stoull(row[1])));
        for(unsigned i=2;i<8;++i)row[i]=unhex(row[i]);
        expected(value,language,row[2].c_str(),row[5],rows<64);
        expected(value,language,row[3].c_str(),row[6],rows<64);
        expected(value,0,row[4].c_str(),row[7],rows<64);
        ++rows;
    }
    for(unsigned i=1;i<=16;++i)CHECK(languages[i]);CHECK(rows>=64);grammar();
    std::printf("calendar locales: %u checks, %u original-provider vectors, 16 languages passed\n",checks,rows);
}
