#pragma once
// Namespace-independent, allocation-free helper. The source includes this
// within spire; standalone tests may use it at global scope. Only constexpr
// output is used at runtime: no key pointers, dynamic initialization, extra
// decoder, or startup catalog scan survives. Existing sorted source rows stay
// authoritative; factor their first-dot domain once per contiguous group.
namespace packed_content_index {
struct Entry {const char* key;unsigned short index;};
static_assert(sizeof(unsigned)==4&&sizeof(unsigned short)==2,"Content index word sizes");
constexpr unsigned prefix_length(const char*key){unsigned n=0;while(key[n]&&key[n]!='.')++n;return n;}
constexpr bool same_group(const char*a,const char*b){
 unsigned n=0;while(a[n]&&a[n]!='.'&&a[n]==b[n])++n;
 return a[n]=='.'&&b[n]=='.';
}
struct Layout {unsigned key_bytes=0,group_bytes=0,groups=0;};
template<unsigned N> constexpr Layout layout(const Entry(&rows)[N]){
 Layout result{};const char*previous=nullptr;
 for(unsigned i=0;i<N;++i){
  const char*key=rows[i].key;const unsigned prefix=prefix_length(key);
  if(key[prefix]=='.'){
   if(!previous||!same_group(previous,key)){++result.groups;result.group_bytes+=prefix+1;}
   previous=key;key+=prefix+1;
  }else previous=nullptr;
  do{++result.key_bytes;}while(*key++);
 }
 return result;
}
template<bool Small> struct OffsetType {using type=unsigned;};
template<> struct OffsetType<true> {using type=unsigned short;};
template<unsigned N,unsigned Bytes,unsigned GroupBytes,unsigned Groups> struct Table {
 static_assert(N<=65535&&Groups>0&&Groups<=N&&GroupBytes<=65536,"Bounded content index groups");
 using Offset=typename OffsetType<(Bytes<=65536)>::type;
 Offset offsets[N]{};
 unsigned short values[N]{},group_offsets[Groups]{},first[Groups]{},end[Groups]{};
 char keys[Bytes]{},group_keys[GroupBytes]{};
 constexpr const char*key(unsigned i)const{return keys+offsets[i];}
 constexpr const char*group(unsigned i)const{return group_keys+group_offsets[i];}
};
template<unsigned Bytes,unsigned GroupBytes,unsigned Groups,unsigned N>
constexpr Table<N,Bytes,GroupBytes,Groups> make(const Entry(&rows)[N]){
 Table<N,Bytes,GroupBytes,Groups> out{};unsigned at=0,group_at=0,count=0;const char*previous=nullptr;
 for(unsigned i=0;i<N;++i){
  const char*key=rows[i].key;const unsigned prefix=prefix_length(key);
  if(key[prefix]=='.'){
   if(!previous||!same_group(previous,key)){
    out.group_offsets[count]=static_cast<unsigned short>(group_at);out.first[count]=static_cast<unsigned short>(i);
    for(unsigned j=0;j<prefix;++j)out.group_keys[group_at++]=key[j];out.group_keys[group_at++]=0;++count;
   }
   out.end[count-1]=static_cast<unsigned short>(i+1);previous=key;key+=prefix+1;
  }else previous=nullptr;
  out.offsets[i]=typename Table<N,Bytes,GroupBytes,Groups>::Offset(at);out.values[i]=rows[i].index;
  do{out.keys[at++]=*key;}while(*key++);
 }
 return out;
}
}
