// Exercise the production lookup overloads with every existing registry key.
// No original artwork is needed; the index strings and numeric IDs are source.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../src/content.cpp"
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"packed index:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static_assert(sizeof(packed_content_index::Table<1,65536,1,1>::Offset)==2,"Last 16-bit offset remains valid");
static_assert(sizeof(packed_content_index::Table<1,65537,1,1>::Offset)==4,"Large key pool needs full offsets");
static constexpr WorldContentIndex boundary[]={{".empty_group",65535},{"alpha.",0},{"alpha.extended.key",42},{"zeta.final",1}};
static constexpr auto bounds=packed_content_index::layout(boundary);
static constexpr auto packed=packed_content_index::make<bounds.key_bytes,bounds.group_bytes,bounds.groups>(boundary);
static_assert(packed.offsets[0]==0&&packed.values[0]==65535&&packed.values[1]==0,"Exact static fields");
static_assert(packed.keys[sizeof packed.keys-1]==0,"Final key is terminated");
template<unsigned N,unsigned Bytes,unsigned GroupBytes,unsigned Groups>
static void compare(const WorldContentIndex(&source)[N],const packed_content_index::Table<N,Bytes,GroupBytes,Groups>&table){
 unsigned end=0;
 for(unsigned i=0;i<N;++i){
  CHECK(table.offsets[i]==end&&table.values[i]==source[i].index);
  const char*dot=std::strchr(source[i].key,'.');const char*tail=dot?dot+1:source[i].key;
  CHECK(!std::strcmp(table.key(i),tail));end+=unsigned(std::strlen(tail))+1;if(!dot)continue;
  const std::string group(source[i].key,dot-source[i].key);const char*key=dot+1;
  CHECK(wc_find(table,group.c_str(),key)==wc_find(source,group.c_str(),key));
  CHECK(wc_find(table,group.c_str(),key)==source[i].index);
  const char*last=std::strrchr(source[i].key,'.');const std::string nested(source[i].key,last-source[i].key);
  CHECK(wc_find(table,nested.c_str(),last+1)==wc_find(source,nested.c_str(),last+1));
  const auto absent=std::string(key)+"~missing";
  CHECK(wc_find(table,group.c_str(),absent.c_str())==wc_find(source,group.c_str(),absent.c_str()));
 }
 CHECK(end==Bytes);unsigned group_end=0;
 for(unsigned g=0;g<Groups;++g){
  CHECK(table.group_offsets[g]==group_end&&table.first[g]<table.end[g]&&table.end[g]<=N);
  const unsigned length=unsigned(std::strlen(table.group(g)));group_end+=length+1;
  if(g)CHECK(std::strcmp(table.group(g-1),table.group(g))<0&&table.end[g-1]<=table.first[g]);
  for(unsigned i=table.first[g];i<table.end[g];++i)CHECK(!std::strncmp(table.group(g),source[i].key,length)&&source[i].key[length]=='.');
 }CHECK(group_end==GroupBytes);
 for(const char*group:{static_cast<const char*>(nullptr),"","missing","world","~"})
  for(const char*key:{static_cast<const char*>(nullptr),"","missing","nested.key"})
   CHECK(wc_find(table,group,key)==wc_find(source,group,key));
 const std::string long_text(256,'x');
 CHECK(wc_find(table,long_text.c_str(),"x")==-1&&wc_find(table,"x",long_text.c_str())==-1);
}
int main(){
 compare(world_text_source,world_text_index);compare(world_image_source,world_image_index);
 compare(world_clip_source,world_clip_index);compare(boundary,packed);
 Content content;
 for(const auto&row:world_text_source){
  const char*dot=std::strchr(row.key,'.');CHECK(dot);
  const std::string group(row.key,dot-row.key);const char*key=dot+1;
  const int native=wc_find(native_ui_text_index,group.c_str(),key);
  CHECK(content.world_string_id(group.c_str(),key)==(native>=0?native:row.index));
 }
 CHECK(content.world_string_id(nullptr,"x")==-1&&content.world_string_id("x",nullptr)==-1);
 const unsigned entries=sizeof(world_text_source)/sizeof(world_text_source[0])+sizeof(world_image_source)/sizeof(world_image_source[0])+sizeof(world_clip_source)/sizeof(world_clip_source[0]);
 std::printf("packed content index: %u checks; %u source entries; exact production lookup and IDs; no resource input\n",checks,entries);
}
