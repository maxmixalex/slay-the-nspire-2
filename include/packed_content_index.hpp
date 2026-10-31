#pragma once
// Namespace-independent, allocation-free helper. content.cpp includes this
// inside its private source namespace; tests may include it at global scope.
// Only the constexpr output is used at runtime: no absolute key pointers,
// dynamic initialization, extra decoder, or startup catalog scan survives.
namespace packed_content_index {
struct Entry {const char* key;unsigned short index;};
static_assert(sizeof(unsigned)==4&&sizeof(unsigned short)==2,"Content index word sizes");
template<unsigned N> constexpr unsigned byte_count(const Entry(&rows)[N]){
 unsigned size=0;
 for(unsigned i=0;i<N;++i)for(unsigned j=0;;++j){++size;if(!rows[i].key[j])break;}
 return size;
}
template<bool Small> struct OffsetType {using type=unsigned;};
template<> struct OffsetType<true> {using type=unsigned short;};
template<unsigned N,unsigned Bytes> struct Table {
 using Offset=typename OffsetType<(Bytes<=65536)>::type;
 Offset offsets[N]{};
 unsigned short values[N]{};
 char keys[Bytes]{};
 constexpr const char* key(unsigned i)const{return keys+offsets[i];}
};
template<unsigned Bytes,unsigned N> constexpr Table<N,Bytes> make(const Entry(&rows)[N]){
 Table<N,Bytes> out{};unsigned at=0;
 for(unsigned i=0;i<N;++i){
  out.offsets[i]=typename Table<N,Bytes>::Offset(at);out.values[i]=rows[i].index;
  unsigned j=0;do{out.keys[at++]=rows[i].key[j];}while(rows[i].key[j++]);
 }
 return out;
}
}
