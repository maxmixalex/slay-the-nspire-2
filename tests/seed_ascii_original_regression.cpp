// Optional original-DLL differential test. Its private TSV is produced by
// tools/seed_reference_oracle ... ascii; no game assembly is distributed.
#include "core.hpp"
#include "world_rng.hpp"
#include "world_run.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace spire;
static unsigned checks=0,line_number=0;
static State state,loaded;static SaveData saved,again;
static reference::ReferenceMap map;
static void check(bool ok,const char*text){++checks;if(!ok){std::fprintf(stderr,"ASCII original line %u: %s\n",line_number,text);std::exit(1);}}
static std::vector<std::string> split(const std::string&s,char separator){std::vector<std::string> out;size_t first=0;for(;;){const size_t end=s.find(separator,first);out.push_back(s.substr(first,end-first));if(end==std::string::npos)return out;first=end+1;}}
static std::string unhex(const std::string&s){std::string out;if(s=="-")return out;check(s.size()%2==0,"hex size");for(size_t i=0;i<s.size();i+=2)out.push_back(char(std::strtoul(s.substr(i,2).c_str(),nullptr,16)));return out;}
static uint64_t number(const std::string&s){return std::strtoull(s.c_str(),nullptr,16);}
static void words(const reference::Random&r,const std::string&s){const auto expected=split(s,',');check(expected.size()==4,"four RNG words");for(unsigned i=0;i<4;++i)check(r.words[i]==number(expected[i]),"original RNG state word");}
static std::string graph(const reference::ReferenceMap&m){
 auto coord=[&](unsigned i){return std::to_string(m.points[i].col)+","+std::to_string(m.points[i].row);};
 std::vector<std::pair<std::string,unsigned>> points;for(unsigned i=0;i<m.count;++i)if(m.points[i].active)points.push_back({coord(i),i});
 std::sort(points.begin(),points.end());std::string result;
 for(const auto&entry:points){const auto&p=m.points[entry.second];if(!result.empty())result+=';';result+=entry.first+","+std::to_string(unsigned(p.type))+">";
  std::vector<std::string> children;for(unsigned i=0;i<p.children.used;++i)if(p.children.slots[i]!=reference::NoPoint)children.push_back(coord(p.children.slots[i]));
  std::sort(children.begin(),children.end());for(unsigned i=0;i<children.size();++i){if(i)result+='|';result+=children[i];}
 }return result;
}
int main(int argc,char**argv){
 if(argc!=2){std::fprintf(stderr,"Usage: seed_ascii_original_regression PRIVATE_ASCII_ORIGINAL_TSV\n");return 2;}
 std::ifstream file(argv[1]);check(file.good(),"open private original oracle");std::string line;uint64_t seed=0;unsigned seeds=0,streams=0,maps=0,saves=0;
 while(std::getline(file,line)){++line_number;const auto f=split(line,'\t');if(f.empty())continue;
  if(f[0]=="S"){
   check(f.size()==6,"seed fields");const auto input=unhex(f[2]),normal=unhex(f[3]);std::vector<char> canonical(input.size()*4+8);
   check(reference::canonicalize_seed(input.c_str(),canonical.data(),canonical.size())&&normal==canonical.data(),"original ASCII normalization");
   uint64_t raw=0;check(reference::seed_hash(input.c_str(),raw)&&raw==number(f[4]),"original raw ASCII hash");
   check(reference::seed_hash(canonical.data(),seed)&&seed==number(f[5]),"original normalized ASCII hash");
   new_run(state,73,false,Character::Ironclad);check(world_set_seed(state,input.c_str())==(normal.size()<=64),"explicit port UTF-8 capacity boundary");
   if(normal.size()<=64)check(state.world.run_seed==seed&&!std::strcmp(state.world.seed_text,normal.c_str()),"world stores exact canonical seed");
   else{char code[24]{};reference::format_seed_code(seed,code);check(world_import_seed(state,code)&&state.world.seed_imported&&state.world.run_seed==seed,"complete long seed import");}
   {
    check(save(state,saved)&&load(loaded,saved.bytes,saved.size)&&save(loaded,again),"seed save/load");
    check(saved.size==again.size&&!std::memcmp(saved.bytes,again.bytes,saved.size),"seed and all RNG states survive save byte-exactly");++saves;
   }++seeds;
  }else if(f[0]=="R"){
   check(f.size()==7,"RNG fields");reference::Random rng;rng.named(seed,f[2].c_str());words(rng,f[3]);
   for(const auto&draw:split(f[4],','))check(rng.next()==number(draw),"original RNG next draw");
   words(rng,f[5]);check(rng.calls==std::strtoul(f[6].c_str(),nullptr,10),"original RNG call counter");++streams;
  }else if(f[0]=="M"){
   check(f.size()==11,"map fields");reference::ActSettings settings;
   settings.act=reference::ActKind(std::strtoul(f[2].c_str(),nullptr,10));settings.increased_elites=f[3]=="1";settings.replace_treasure=f[4]=="1";settings.second_boss=f[5]=="1";
   reference::QuotaTrace trace;check(reference::generate_act_map(map,seed,settings,trace)&&map.valid(),"generate valid standard map");
   const auto quota=split(f[6],',');const unsigned actual[]={map.config.rests,map.config.shops,map.config.elites,map.config.unknowns};
   for(unsigned i=0;i<4;++i)check(actual[i]==std::strtoul(quota[i].c_str(),nullptr,10),"original room quota");
   check(trace.rest_calls+trace.unknown_calls==std::strtoul(f[7].c_str(),nullptr,10),"original quota draw count");
   check(map.final_rng.calls==std::strtoul(f[8].c_str(),nullptr,10),"original complete map draw count");words(map.final_rng,f[9]);
   const auto actual_graph=graph(map);if(actual_graph!=f[10]){std::fprintf(stderr,"actual: %s\noriginal: %s\n",actual_graph.c_str(),f[10].c_str());check(false,"original standard-map coordinates, room types and connections");}++checks;++maps;
  }else check(false,"known record kind");
 }
 check(seeds>800&&streams==seeds*15&&maps>6000,"substantial ASCII/stream/map set");
 std::printf("ASCII original: %u checks, %u seeds, %u RNG streams, %u maps, %u save roundtrips.\n",checks,seeds,streams,maps,saves);
}
