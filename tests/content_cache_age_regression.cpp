// Compile the production cache in this test TU, as the Canvas test does for
// the private renderer. The archive's content.cpp object is not needed, and
// no public API or runtime hook is added just to reach the 32-bit age boundary.
#include "../src/content.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

using namespace spire;
namespace {
unsigned checks=0;
static Content owner;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"cache-age failure line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

void image_wrap(const std::vector<unsigned>& indexes){
    for(auto& entry:image_cache)entry={};
    std::vector<std::pair<uint32_t,unsigned>> expected;
    for(unsigned index:indexes){
        CHECK(index<ImageEntries);
        auto& entry=image_cache[index];entry.owner=&owner;entry.image=index;
        entry.age=1000000u+((index*73u)%256u)*37u;
        expected.push_back({entry.age,index});
    }
    // Independent numeric sorting supplies the expected old-to-new order.
    std::sort(expected.begin(),expected.end());
    image_age=0xffffffffu;
    CHECK(next_image_age()==indexes.size()+1);
    for(unsigned rank=0;rank<expected.size();++rank){
        const unsigned index=expected[rank].second;
        CHECK(image_cache[index].age==rank+1);
        CHECK(image_cache[index].owner==&owner&&image_cache[index].image==index);
    }
    for(unsigned i=0;i<ImageEntries;++i)if(!image_cache[i].owner)CHECK(image_cache[i].age==0);
    CHECK(next_image_age()==indexes.size()+2);
}
}
int main(){
    // All byte indexes, including 255, participate in one wrap. The count
    // and returned age must represent 256/257 without truncation.
    CHECK(ImageEntries==256);
    std::vector<unsigned> all;
    for(unsigned i=0;i<256;++i)all.push_back(i);
    image_wrap(all);
    image_wrap({0,7,63,127,128,191,254,255});
    image_wrap({255});
    image_wrap({});
    std::printf("content cache ages: %u checks passed; 256-entry wrap, sparse entries, index 255 and empty cache\n",checks);
}
