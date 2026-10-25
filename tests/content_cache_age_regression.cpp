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
static Content second_owner;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"cache-age failure line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

void image_wrap(const std::vector<unsigned>& indexes){
    for(auto& entry:image_cache)entry={};
    for(auto& bucket:image_buckets)bucket=0;
    for(auto& page:image_used)page=0;
    std::vector<std::pair<uint32_t,unsigned>> expected;
    for(unsigned index:indexes){
        CHECK(index<ImageEntries);
        auto& entry=image_cache[index];entry.owner=&owner;entry.image=index;
        entry.age=1000000u+((index*73u)%ImageEntries)*37u;
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
void collision_and_eviction(){
    for(auto& entry:image_cache)entry={};
    for(auto& bucket:image_buckets)bucket=0;
    for(auto& page:image_used)page=0;
    image_age=0;
    // Deliberately put every descriptor into the same bucket. The complete
    // descriptor and owner must still distinguish images, while a true alias
    // retains the very same stable decoded address.
    std::vector<uint8_t> metadata(40+(ImageEntries+2)*16);
    auto word=[&](unsigned at,uint32_t value){for(unsigned b=0;b<4;++b)metadata[at+b]=uint8_t(value>>(b*8));};
    for(unsigned id=0;id<ImageEntries+1;++id){
        word(40+id*16,id*65536u);word(44+id*16,0x00010001u);word(52+id*16,2);
    }
    std::copy_n(metadata.begin()+40,16,metadata.begin()+40+(ImageEntries+1)*16);
    const uint8_t* first=nullptr;
    for(unsigned id=0;id<ImageEntries;++id){
        CHECK(image_bucket(metadata.data()+40+id*16)==image_bucket(metadata.data()+40));
        ImageReservation reservation(2);CHECK(reservation.bytes());
        reservation.bytes()[0]=uint8_t(id);if(!id)first=reservation.bytes();
        reservation.publish(id&1?&second_owner:&owner,metadata.data(),id,0);
    }
    CHECK(image_buffer(*find_image(&owner,metadata.data(),ImageEntries+1))==first);
    for(unsigned id=0;id<ImageEntries;++id){
        const Content* actual=id&1?&second_owner:&owner;
        auto* result=find_image(actual,metadata.data(),id);CHECK(result&&result->id()==id);
        CHECK(image_buffer(*result)[0]==uint8_t(id));
        CHECK(!find_image(id&1?&owner:&second_owner,metadata.data(),id));
    }
    // Overflow the key capacity, then remove every other chain link through
    // the real owner-clear operation. Neither operation may strand survivors.
    {ImageReservation reservation(2);CHECK(reservation.bytes());
     reservation.publish(&owner,metadata.data(),ImageEntries,0);}
    CHECK(!find_image(&owner,metadata.data(),0));
    CHECK(!find_image(&owner,metadata.data(),ImageEntries+1));
    CHECK(find_image(&owner,metadata.data(),ImageEntries));
    clear_caches(&owner);
    for(unsigned id=0;id<ImageEntries;++id)
        CHECK(bool(find_image(id&1?&second_owner:&owner,metadata.data(),id))==bool(id&1));
    clear_caches(&second_owner);
    for(auto bucket:image_buckets)CHECK(!bucket);
    for(auto page:image_used)CHECK(!page);
}
}
int main(){
    collision_and_eviction();
    // Include indexes on both sides of the former byte boundary. The count,
    // permutation and returned age must survive entries 256 through 319.
    CHECK(ImageEntries==Content::image_capacity_entries()&&ImageEntries==480);
    std::vector<unsigned> all;
    for(unsigned i=0;i<ImageEntries;++i)all.push_back(i);
    image_wrap(all);
    image_wrap({0,7,63,127,128,191,254,255,256,287,319});
    image_wrap({ImageEntries-1});
    image_wrap({});
    std::printf("content cache ages: %u checks passed; 480 colliding keys, owner/alias eviction, chain clearing and age wrap\n",checks);
}
