#pragma once
// Namespace-independent, allocation-free helper. The source includes this
// within spire; standalone tests may use it at global scope. Only constexpr
// output is used at runtime: no key pointers, dynamic initialization, extra
// decoder, or startup catalog scan survives. Existing sorted source rows stay
// authoritative; factor their first-dot domain once per contiguous group.
// Large pools may use within-group 16-bit offsets. The caller resolves one
// 32-bit group base before the existing key binary search; small tables keep
// their original absolute layout. No decoder or extra lookup is introduced.
namespace packed_content_index {
struct Entry {const char* key;unsigned short index;};
static_assert(sizeof(unsigned)==4&&sizeof(unsigned short)==2,"Content index word sizes");
constexpr unsigned prefix_length(const char*key){unsigned n=0;while(key[n]&&key[n]!='.')++n;return n;}
constexpr bool same_group(const char*a,const char*b){
 unsigned n=0;while(a[n]&&a[n]!='.'&&a[n]==b[n])++n;
 return a[n]=='.'&&b[n]=='.';
}
struct Layout {unsigned key_bytes=0,group_bytes=0,groups=0,max_group_bytes=0;bool all_grouped=true;};
template<unsigned N> constexpr Layout layout(const Entry(&rows)[N]){
 Layout result{};const char*previous=nullptr;unsigned group_start=0;
 for(unsigned i=0;i<N;++i){
  const char*key=rows[i].key;const unsigned prefix=prefix_length(key);
  if(key[prefix]=='.'){
   if(!previous||!same_group(previous,key)){++result.groups;result.group_bytes+=prefix+1;group_start=result.key_bytes;}
   previous=key;key+=prefix+1;
  }else{previous=nullptr;result.all_grouped=false;group_start=result.key_bytes;}
  do{++result.key_bytes;}while(*key++);
  const unsigned group_bytes=result.key_bytes-group_start;
  if(group_bytes>result.max_group_bytes)result.max_group_bytes=group_bytes;
 }
 return result;
}
template<bool Small> struct OffsetType {using type=unsigned;};
template<> struct OffsetType<true> {using type=unsigned short;};
template<bool Relative,unsigned Groups> struct KeyBases {};
template<unsigned Groups> struct KeyBases<true,Groups> {unsigned key_bases[Groups]{};};
template<unsigned N,unsigned Bytes,unsigned GroupBytes,unsigned Groups,bool Relative=false> struct Table:KeyBases<Relative,Groups> {
 static_assert(N<=65535&&Groups>0&&Groups<=N&&GroupBytes<=65536,"Bounded content index groups");
 using Offset=typename OffsetType<(Bytes<=65536||Relative)>::type;
 Offset offsets[N]{};
 unsigned short values[N]{},group_offsets[Groups]{},first[Groups]{},end[Groups]{};
 char keys[Bytes]{},group_keys[GroupBytes]{};
 constexpr unsigned group_base(unsigned group)const{
  if constexpr(Relative)return this->key_bases[group];else return 0;
 }
 constexpr const char*key(unsigned i,unsigned base)const{return keys+base+offsets[i];}
 constexpr const char*key(unsigned i)const{
  if constexpr(Relative){
   unsigned lo=0,hi=Groups;
   while(lo+1<hi){const unsigned mid=(lo+hi)/2;if(first[mid]<=i)lo=mid;else hi=mid;}
   return key(i,group_base(lo));
  }else return key(i,0);
 }
 constexpr const char*group(unsigned i)const{return group_keys+group_offsets[i];}
};
template<unsigned Bytes,unsigned GroupBytes,unsigned Groups,bool Relative=false,unsigned N>
constexpr Table<N,Bytes,GroupBytes,Groups,Relative> make(const Entry(&rows)[N]){
 Table<N,Bytes,GroupBytes,Groups,Relative> out{};unsigned at=0,group_at=0,count=0;const char*previous=nullptr;
 for(unsigned i=0;i<N;++i){
  const char*key=rows[i].key;const unsigned prefix=prefix_length(key);
  if(key[prefix]=='.'){
   if(!previous||!same_group(previous,key)){
    if constexpr(Relative)out.key_bases[count]=at;
    out.group_offsets[count]=static_cast<unsigned short>(group_at);out.first[count]=static_cast<unsigned short>(i);
    for(unsigned j=0;j<prefix;++j)out.group_keys[group_at++]=key[j];out.group_keys[group_at++]=0;++count;
   }
   out.end[count-1]=static_cast<unsigned short>(i+1);previous=key;key+=prefix+1;
  }else{if constexpr(Relative)__builtin_trap();previous=nullptr;}
  const unsigned offset=at-(Relative&&count?out.group_base(count-1):0);
  // Invalid relative layouts fail constant evaluation instead of truncating.
  if constexpr(Relative)if(offset>65535)__builtin_trap();
  out.offsets[i]=typename Table<N,Bytes,GroupBytes,Groups,Relative>::Offset(offset);out.values[i]=rows[i].index;
  do{out.keys[at++]=*key;}while(*key++);
 }
 return out;
}
}
