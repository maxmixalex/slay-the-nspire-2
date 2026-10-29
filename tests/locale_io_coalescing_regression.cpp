// Private-resource oracle: replay ordinary Continue/End Turn with the same
// virtual resource, recording every complete RGB565 frame, canonical State,
// logical request and physical read. No fixture resources are embedded here.
#include "app.hpp"
#include "resource_file_cache.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SplitResourceSource split;
static MultipartResourceSource multipart;
using ArtFiles=ResourceFileCache<8,4>;
static ArtFiles artwork;
static ResourceSource part_sources[8],virtual_source;
static FILE*part_handles[8]{};
static std::string parts_folder;
static std::vector<uint8_t> save_bytes,progress_bytes;
static const char*stage="open";
static unsigned frame=0,sequence=0,calls=0,bytes=0,seeks=0,opens=0;
static FILE*transcript=nullptr,*pixels_file=nullptr,*states_file=nullptr;
static SaveData canonical;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"trace:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void row(const char*kind,const char*name,uint32_t at,uint32_t n,uint32_t aux){
 std::printf("%u,%s,%u,%s,%s,%u,%u,%u\n",++sequence,stage,frame,kind,name,at,n,aux);
}
static int seek_file(void*f,long at,int origin){return std::fseek(static_cast<FILE*>(f),at,origin);}
static unsigned get_file(void*out,unsigned size,unsigned n,void*f){return unsigned(std::fread(out,size,n,static_cast<FILE*>(f)));}
static long tell_file(void*f){return std::ftell(static_cast<FILE*>(f));}
static void*open_part(void*,unsigned i){auto path=parts_folder+"/"+ArtworkPartNames[i];auto*f=std::fopen(path.c_str(),"rb");part_handles[i]=f;row("open",ArtworkPartNames[i],0,0,f!=nullptr);if(!std::strcmp(stage,"action"))++opens;return f;}
static int close_part(void*f){for(unsigned i=0;i<8;++i)if(part_handles[i]==f){part_handles[i]=nullptr;row("close",ArtworkPartNames[i],0,0,0);break;}return std::fclose(static_cast<FILE*>(f));}
static void count_read(uint32_t n,bool sought){if(!std::strcmp(stage,"action")){++calls;bytes+=n;seeks+=sought;}}
static bool read_part(void*context,uint32_t at,void*out,uint32_t n){
 unsigned i=0;while(i<8&&part_sources[i].context!=context)++i;CHECK(i<8);bool sought=false;
 const bool good=ArtFiles::read_source(context,at,out,n,sought);row("physical",ArtworkPartNames[i],at,n,sought);count_read(n,sought);return good;
}
struct File {FILE*file=nullptr;ResourceFileCursor cursor;uint32_t length=0;const char*name=nullptr;
 void open(const std::string&path,const char*label){file=std::fopen(path.c_str(),"rb");CHECK(file&&!std::fseek(file,0,SEEK_END));const long n=std::ftell(file);CHECK(n>0);length=uint32_t(n);name=label;cursor={};}
 ResourceSource source();
 void close(){if(file){CHECK(!std::fclose(file));file=nullptr;}cursor={};}
};
static bool read_file(void*context,uint32_t at,void*out,uint32_t n){auto&f=*static_cast<File*>(context);bool sought=false;const bool good=f.cursor.read(f.file,at,out,n,seek_file,get_file,sought);row("physical",f.name,at,n,sought);count_read(n,sought);return good;}
ResourceSource File::source(){return {this,length,read_file};}
static bool read_virtual(void*,uint32_t at,void*out,uint32_t n){row("virtual","NSP2",at,n,0);return virtual_source.read_at(virtual_source.context,at,out,n);}
static std::vector<uint8_t> read_all(const char*path){FILE*f=std::fopen(path,"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0&&!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>v(size_t(n),0);CHECK(std::fread(v.data(),1,v.size(),f)==v.size()&&!std::fclose(f));return v;}
static bool load_run(State&s){return load(s,save_bytes.data(),save_bytes.size());}
static bool load_progress(UiProgress&p){return decode_ui_progress(progress_bytes.data(),progress_bytes.size(),p);}
static unsigned profile(){return 1;}
static void paint(){app.render();CHECK(app.content.healthy());if(transcript&&!std::strcmp(stage,"action")){uint64_t hash=1469598103934665603ULL;for(uint16_t p:app.pixels){hash=(hash^uint8_t(p))*1099511628211ULL;hash=(hash^uint8_t(p>>8))*1099511628211ULL;}std::fprintf(transcript,"%u %016llx\n",frame,(unsigned long long)hash);CHECK(std::fwrite(app.pixels,1,sizeof app.pixels,pixels_file)==sizeof app.pixels);CHECK(save(app.game,canonical));const uint32_t size=canonical.size;CHECK(std::fwrite(&size,1,sizeof size,states_file)==sizeof size);CHECK(std::fwrite(canonical.bytes,1,size,states_file)==size);}}
int main(int argc,char**argv){
 CHECK(argc==7);const unsigned mode=unsigned(std::strtoul(argv[1],nullptr,10));CHECK(mode<=2);
 const std::string old_folder=argv[2];parts_folder=argv[3];save_bytes=read_all(argv[4]);progress_bytes=read_all(argv[5]);const std::string destination=argv[6];transcript=std::fopen((destination+".hashes").c_str(),"wb");pixels_file=std::fopen((destination+".rgb565").c_str(),"wb");states_file=std::fopen((destination+".states").c_str(),"wb");CHECK(transcript&&pixels_file&&states_file);
 File common,locale;common.open((mode?parts_folder:old_folder)+"/nSpire2.dat.tns","common");ResourceSource art=common.source();
 if(mode){CHECK(artwork.open({nullptr,open_part,close_part,seek_file,tell_file,get_file,nullptr},MaxContentFile));for(unsigned i=0;i<8;++i)part_sources[i]=artwork.source(i,read_part);CHECK(multipart.open(art,part_sources,8));art=multipart.source();common.close();}
 locale.open((mode==2?parts_folder:old_folder)+"/nSpire2.eng.dat.tns","locale");CHECK(split.open(art,locale.source(),ResourceLanguage::English));virtual_source=split.source();
 Callbacks cb{};cb.load=load_run;cb.load_progress=load_progress;cb.current_profile=profile;app.calculator_keyboard=true;
 CHECK(app.init({nullptr,virtual_source.length,read_virtual,nullptr,nullptr,virtual_source.required_cached_prefix},cb));
 stage="title";for(frame=0;frame<900;++frame){app.advance(33);paint();}
 stage="continue";app.input(Key::Confirm);
 stage="warm";for(frame=0;frame<240;++frame){app.advance(33);paint();}
 CHECK(!app.title&&app.game.phase==Phase::Combat&&!app.combat_preparation.active);const unsigned cards=app.game.card_count;
 stage="action";frame=0;app.input(Key::EndTurn);
 for(frame=0;frame<182;++frame){app.advance(33);paint();}
 CHECK(app.game.card_count==cards+5);unsigned dazed=0;for(unsigned i=0;i<app.game.card_count;++i)dazed+=app.game.cards[i].kind==Dazed;CHECK(dazed==5);
 std::fprintf(stderr,"mode=%u calls=%u bytes=%u seeks=%u opens=%u\n",mode,calls,bytes,seeks,opens);
 stage="shutdown";locale.close();common.close();artwork.close();CHECK(!std::fclose(transcript)&&!std::fclose(pixels_file)&&!std::fclose(states_file));return 0;
}
