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
void collision_and_eviction(unsigned stride){
    for(auto& entry:image_cache)entry={};
    for(auto& bucket:image_buckets)bucket=0;
    for(auto& page:image_used)page=0;
    image_age=0;
    // Deliberately put every descriptor into the same bucket. The complete
    // descriptor and owner must still distinguish images, while a true alias
    // retains the very same stable decoded address.
    std::vector<uint8_t> metadata(40+(ImageEntries+2)*stride);
    const ImageDirectoryView directory{metadata.data()+40,stride};
    auto word=[&](unsigned at,uint32_t value){for(unsigned b=0;b<4;++b)metadata[at+b]=uint8_t(value>>(b*8));};
    for(unsigned id=0;id<ImageEntries+1;++id){
        word(40+id*stride,id*65536u);word(44+id*stride,0x00010001u);
        word(48+id*stride,stride==12?2:0);if(stride==16)word(52+id*stride,2);
    }
    std::copy_n(metadata.begin()+40,stride,metadata.begin()+40+(ImageEntries+1)*stride);
    const uint8_t* first=nullptr;
    for(unsigned id=0;id<ImageEntries;++id){
        CHECK(image_bucket(directory[id])==image_bucket(directory[0]));
        ImageReservation reservation(2);CHECK(reservation.bytes());
        reservation.bytes()[0]=uint8_t(id);if(!id)first=reservation.bytes();
        reservation.publish(id&1?&second_owner:&owner,directory,id,id%5,true);
    }
    CHECK(image_buffer(*find_image(&owner,directory,ImageEntries+1))==first);
    for(unsigned id=0;id<ImageEntries;++id){
        const Content* actual=id&1?&second_owner:&owner;
        auto* result=find_image(actual,directory,id);CHECK(result&&result->id()==id);
        CHECK(result->format()==id%5);
        CHECK(image_buffer(*result)[0]==uint8_t(id));
        CHECK(!find_image(id&1?&owner:&second_owner,directory,id));
    }
    // Overflow the key capacity, then remove every other chain link through
    // the real owner-clear operation. Neither operation may strand survivors.
    {ImageReservation reservation(2);CHECK(reservation.bytes());
     reservation.publish(&owner,directory,ImageEntries,0);}
    CHECK(!find_image(&owner,directory,0));
    CHECK(!find_image(&owner,directory,ImageEntries+1));
    CHECK(find_image(&owner,directory,ImageEntries));
    clear_caches(&owner);
    for(unsigned id=0;id<ImageEntries;++id)
        CHECK(bool(find_image(id&1?&second_owner:&owner,directory,id))==bool(id&1));
    clear_caches(&second_owner);
    for(auto bucket:image_buckets)CHECK(!bucket);
    for(auto page:image_used)CHECK(!page);
}
void frame_pool_profiles(){
    owner.set_image_cache_profile(false);
    CHECK(image_page_limit==1728&&tile_page_limit==48);
    CHECK(tile_pages==picture_pixels+(7u*1024u-256u)*1024u);
    CHECK(sizeof picture_pixels==(13u*1024u-256u)*1024u);
    owner.set_image_cache_profile(true);
    CHECK(image_page_limit==3008&&tile_page_limit==8);
    CHECK(tile_pages==picture_pixels+(12u*1024u-256u)*1024u);
    // Exhaust both banks with independent fragmentation patterns. Pin each
    // legal1..4-page index in turn, then require a full512 KiB tile to fit.
    // Fill both allocations and verify the image arena boundary is intact.
    for(unsigned initial=0;initial<8;++initial)for(unsigned index_pages=1;index_pages<=4;++index_pages){
        for(auto&group:tile_groups)tile_discard(group);
        for(unsigned i=0;i<initial;++i){auto*g=reserve_tile_group(TilePageBytes);CHECK(g);g->owner=&owner;g->age=next_tile_age();}
        TileGroup*index=reserve_tile_group(index_pages*TilePageBytes);CHECK(index);
        index->owner=&owner;index->age=next_tile_age();index->first|=128u;
        std::fill_n(tile_bytes(*index),index_pages*TilePageBytes,uint8_t(0x51));
        picture_pixels[(12u*1024u-256u)*1024u-1]=0x7b;
        TileGroup*tile=reserve_tile_group(4*TilePageBytes);CHECK(tile&&tile!=index);
        tile->owner=&owner;tile->age=next_tile_age();
        std::fill_n(tile_bytes(*tile),4*TilePageBytes,uint8_t(0xb2));
        CHECK(picture_pixels[(12u*1024u-256u)*1024u-1]==0x7b);
        CHECK((index->first&63u)+index->pages<=8&&(tile->first&63u)+tile->pages<=8);
        for(unsigned j=0;j<index_pages*TilePageBytes;++j)CHECK(tile_bytes(*index)[j]==0x51);
        owner.set_image_cache_profile(true);
        CHECK(index->owner==&owner&&tile->owner==&owner); // same profile preserves views
        index->first&=63u;
    }
    owner.set_image_cache_profile(false);
    CHECK(image_page_limit==1728&&tile_page_limit==48&&!tile_used);
    for(const auto&group:tile_groups)CHECK(!group.pages&&!group.owner);
    for(const auto&entry:image_cache)CHECK(!entry.pages&&!entry.owner);
    for(auto page:image_used)CHECK(!page);
}
void profile_surviving_images(unsigned stride){
    clear_caches(&owner);clear_caches(&second_owner);owner.set_image_cache_profile(false);
    std::vector<uint8_t> metadata(40+24*stride);
    const ImageDirectoryView directory{metadata.data()+40,stride};
    const auto word=[&](unsigned at,uint32_t value){for(unsigned b=0;b<4;++b)metadata[at+b]=uint8_t(value>>(b*8));};
    for(unsigned id=0;id<24;++id){word(40+id*stride,id*65536u);word(44+id*stride,0x00010001u);
        word(48+id*stride,stride==12?2:0);if(stride==16)word(52+id*stride,2);}
    ImageReservation first(ImagePageBytes);CHECK(first.bytes());
    std::fill_n(first.bytes(),ImagePageBytes,uint8_t(0x37));const auto*address=first.bytes();
    first.publish(&owner,directory,0,0);const unsigned age=image_age;
    owner.set_image_cache_profile(true);
    auto*kept=find_image(&owner,directory,0);CHECK(kept&&image_buffer(*kept)==address&&image_age==age);
    for(unsigned byte=0;byte<ImagePageBytes;++byte)CHECK(address[byte]==0x37);
    // Cross the old boundary with a real published allocation. The shrink
    // must release that entire entry before its bytes become tile storage.
    unsigned crossing=0;std::vector<unsigned> survivors;
    for(unsigned id=1;id<=18;++id){ImageReservation image(128*ImagePageBytes);CHECK(image.bytes());
        std::fill_n(image.bytes(),128*ImagePageBytes,uint8_t(id));image.publish(&owner,directory,id,id%5);
        auto*entry=find_image(&owner,directory,id);CHECK(entry);
        if(entry->first<ImagePages&&unsigned(entry->first)+entry->pages>ImagePages)crossing=id;
        if(unsigned(entry->first)+entry->pages<=ImagePages)survivors.push_back(id);
    }
    CHECK(crossing&&survivors.size()>1);owner.set_image_cache_profile(false);
    CHECK(!find_image(&owner,directory,crossing)&&find_image(&owner,directory,0));
    auto*group=reserve_tile_group(Lzma2BlockRawLimit,true);CHECK(group);group->owner=&second_owner;
    std::fill_n(tile_bytes(*group),Lzma2BlockRawLimit,uint8_t(0xd9));
    for(unsigned id:survivors){auto*entry=find_image(&owner,directory,id);CHECK(entry);
        for(unsigned byte=0;byte<128*ImagePageBytes;++byte)CHECK(image_buffer(*entry)[byte]==id);}
    for(unsigned byte=0;byte<ImagePageBytes;++byte)CHECK(address[byte]==0x37);
    for(unsigned page=ImagePages;page<RoomImagePages;++page)CHECK(!(image_used[page/32]&(uint32_t(1)<<(page%32))));
    clear_caches(&owner);clear_caches(&second_owner);
}
void large_room_allocations(){
    owner.set_image_cache_profile(false);clear_caches(&owner);clear_caches(&second_owner);
    image_page_limit=RoomImagePages-128;tile_pages=picture_pixels+image_page_limit*ImagePageBytes;tile_page_limit=12;
    for(unsigned initial=0;initial<20;++initial){
        for(auto&g:tile_groups)tile_discard(g);
        for(unsigned i=0;i<initial;++i){auto*g=reserve_tile_group((i%3+1)*RetainedPageBytes,true);CHECK(g);g->owner=&owner;g->age=next_tile_age();}
        auto*workspace=reserve_tile_group(2*RetainedPageBytes,true,true);CHECK(workspace);
        workspace->first|=128u;CHECK((workspace->first&63u)==0);
        std::fill_n(tile_bytes(*workspace),2*RetainedPageBytes,uint8_t(0x5a));
        auto*index=reserve_tile_group(4*TilePageBytes);CHECK(index);index->first|=128u;index->owner=&owner;index->age=next_tile_age();
        CHECK((index->first&63u)==8);index->first&=127u;
        auto*raw=reserve_tile_group(Lzma2LargeRawLimit);CHECK(raw);raw->first|=128u;
        CHECK((raw->first&63u)==4);std::fill_n(tile_bytes(*raw),Lzma2LargeRawLimit,uint8_t(0x36));
        auto*planes=reserve_tile_group(LargeTileItems*258u,true);CHECK(planes);planes->first|=128u;
        CHECK((planes->first&63u)==1);std::fill_n(tile_bytes(*planes),LargeTileItems*258u,uint8_t(0x91));
        for(unsigned i=0;i<2*RetainedPageBytes;++i)CHECK(tile_bytes(*workspace)[i]==0x5a);
        for(unsigned i=0;i<Lzma2LargeRawLimit;++i)CHECK(tile_bytes(*raw)[i]==0x36);
        tile_discard(*planes);tile_discard(*raw);tile_discard(*workspace);
        // Retained RGR3 drops its probabilities before reserving the source
        // slice; two maximum512KiB retained reservations must still coexist.
        auto*retained=reserve_tile_group(Lzma2BlockRawLimit,true);CHECK(retained);retained->first|=128u;
        auto*source=reserve_tile_group(Lzma2BlockRawLimit,true);CHECK(source&&source!=retained);source->first|=128u;
        tile_discard(*source);tile_discard(*retained);
    }
    owner.set_image_cache_profile(false);CHECK(tile_page_limit==48&&!tile_used);
}
void retained_quarters_share_and_evict(){
    owner.set_image_cache_profile(true);
    for(auto&group:tile_groups)tile_discard(group);
    tile_age=0;
    const auto add=[&](unsigned id,const Content*who,unsigned bytes=RetainedPageBytes){
        TileGroup*g=reserve_tile_group(bytes,true);CHECK(g);
        g->owner=who;g->id=id;g->size=bytes;g->age=next_tile_age();
        std::fill_n(tile_bytes(*g),bytes,uint8_t(id));return g;
    };
    // All32 distinct small groups must coexist in the unchanged1 MiB bank.
    // Writes span every quarter, so adjacent slices cannot alias or overrun.
    for(unsigned id=1;id<=32;++id)add(id,id&1?&owner:&second_owner);
    CHECK(tile_used==255u);
    for(unsigned page=0;page<8;++page)CHECK(retained_quarters(page)==15u);
    for(unsigned id=1;id<=32;++id){
        auto*g=find_tile_group(id&1?&owner:&second_owner,id);CHECK(g);
        CHECK(g->first&64u);CHECK(g->pages==1&&g->items<4);
        CHECK(tile_capacity(*g)==RetainedPageBytes);
        for(unsigned byte=0;byte<RetainedPageBytes;++byte)CHECK(tile_bytes(*g)[byte]==id);
    }
    // A refresh survives pressure, while the actual oldest entry is evicted.
    find_tile_group(&owner,1)->age=next_tile_age();
    add(33,&owner);
    CHECK(find_tile_group(&owner,1)&&find_tile_group(&owner,33));
    CHECK(!find_tile_group(&second_owner,2));
    // Clearing one owner must retain shared parent pages and the other owner.
    clear_tile_cache(&owner);
    for(unsigned id=4;id<=32;id+=2){
        auto*g=find_tile_group(&second_owner,id);CHECK(g);
        CHECK(tile_bytes(*g)[0]==id&&tile_bytes(*g)[RetainedPageBytes-1]==id);
        CHECK(tile_used&tile_mask(g->first&63u,1));
    }
    clear_tile_cache(&second_owner);CHECK(!tile_used);
    for(const auto&g:tile_groups)CHECK(!g.pages&&!g.owner);
    // A failed/unpublished decode releases its quarter without damaging the
    // already-published neighbor or leaving the parent falsely occupied.
    auto*neighbor=add(41,&owner);
    auto*failed=reserve_tile_group(1024,true);CHECK(failed&&failed!=neighbor);
    CHECK((failed->first&63u)==(neighbor->first&63u));
    tile_discard(*failed);CHECK(tile_used==1&&retained_quarters(0)==1);
    CHECK(tile_bytes(*neighbor)[0]==41&&tile_bytes(*neighbor)[RetainedPageBytes-1]==41);
    tile_discard(*neighbor);CHECK(!tile_used);
    // Merchant animation groups also fit in64 KiB. Share a page with one
    // of those and two32 KiB glyph groups, then reuse the middle hole for
    // a contiguous96 KiB group without touching its surviving neighbor.
    auto*half=add(50,&owner,2*RetainedPageBytes);
    auto*quarter_a=add(51,&second_owner);auto*quarter_b=add(52,&second_owner);
    CHECK((half->first&63u)==(quarter_a->first&63u)&&(half->first&63u)==(quarter_b->first&63u));
    CHECK(tile_capacity(*half)==2*RetainedPageBytes&&retained_quarters(0)==15u);
    CHECK(tile_bytes(*quarter_a)==tile_bytes(*half)+2*RetainedPageBytes);
    CHECK(tile_bytes(*quarter_b)==tile_bytes(*half)+3*RetainedPageBytes);
    tile_discard(*half);tile_discard(*quarter_a);CHECK(retained_quarters(0)==8u);
    auto*three=add(53,&owner,3*RetainedPageBytes);
    CHECK((three->first&63u)==0&&tile_capacity(*three)==3*RetainedPageBytes&&retained_quarters(0)==15u);
    for(unsigned byte=0;byte<RetainedPageBytes;++byte)CHECK(tile_bytes(*quarter_b)[byte]==52);
    tile_discard(*three);tile_discard(*quarter_b);CHECK(!tile_used);
    // Noncontiguous quarter holes cannot satisfy a64 KiB request. The
    // allocator uses another parent instead of crossing a live glyph group.
    auto*q0=add(54,&owner);auto*q1=add(55,&owner);auto*q2=add(56,&owner);auto*q3=add(57,&owner);
    tile_discard(*q0);tile_discard(*q2);auto*separate=add(58,&second_owner,2*RetainedPageBytes);
    CHECK((separate->first&63u)!=0&&retained_quarters(0)==10u);
    CHECK(tile_bytes(*q1)[0]==55&&tile_bytes(*q3)[RetainedPageBytes-1]==57);
    clear_tile_cache(&owner);clear_tile_cache(&second_owner);CHECK(!tile_used);
    // All large index sizes are pinned under mixed small/full-size pressure.
    // A maximum tile must still acquire a contiguous512 KiB bank, without
    // overwriting any index byte or the adjacent finished-picture arena.
    for(unsigned initial=0;initial<32;++initial)for(unsigned count=1;count<=4;++count){
        for(auto&g:tile_groups)tile_discard(g);
        for(unsigned id=0;id<initial;++id)add(id+1,&owner);
        auto*index=reserve_tile_group(count*TilePageBytes);CHECK(index);
        index->owner=&owner;index->id=100;index->age=next_tile_age();index->first|=128u;
        std::fill_n(tile_bytes(*index),count*TilePageBytes,uint8_t(0x51));
        for(unsigned id=0;id<12;++id)add(id+40,&second_owner,(1u+id%3u)*RetainedPageBytes);
        auto*full=reserve_tile_group(Lzma2BlockRawLimit,true);CHECK(full&&full!=index);
        full->owner=&second_owner;full->id=200;full->age=next_tile_age();
        CHECK(!(full->first&64u)&&tile_capacity(*full)==Lzma2BlockRawLimit);
        picture_pixels[(12u*1024u-256u)*1024u-1]=0x7b;
        std::fill_n(tile_bytes(*full),Lzma2BlockRawLimit,uint8_t(0xb2));
        CHECK(picture_pixels[(12u*1024u-256u)*1024u-1]==0x7b);
        for(unsigned byte=0;byte<count*TilePageBytes;++byte)CHECK(tile_bytes(*index)[byte]==0x51);
        index->first&=127u;
    }
    // Recency normalization retains both storage classes and their addresses.
    for(auto&g:tile_groups)tile_discard(g);
    auto*small=add(90,&owner);auto*large=add(91,&owner,TilePageBytes+1);
    auto*small_bytes=tile_bytes(*small);auto*large_bytes=tile_bytes(*large);
    tile_age=0xffffffffu;CHECK(next_tile_age()==3);
    CHECK(small->age==1&&large->age==2&&tile_bytes(*small)==small_bytes&&tile_bytes(*large)==large_bytes);
    owner.set_image_cache_profile(false);CHECK(!tile_used);
    for(const auto&g:tile_groups)CHECK(!g.pages&&!g.owner);
    CHECK(!reserve_tile_group(0,true)&&!reserve_tile_group(Lzma2LargeRawLimit+1,true));
}
void implicit_reservation_lifecycle(){
    clear_caches(&owner);clear_caches(&second_owner);owner.set_image_cache_profile(false);
    const unsigned pages_before=image_page_limit,groups_before=tile_page_limit;
    const unsigned chunk_pages=16,count=pages_before/chunk_pages;
    std::vector<uint8_t> metadata(40+(count*2+4)*16);
    const ImageDirectoryView directory{metadata.data()+40,16};
    const auto word=[&](unsigned at,uint32_t value){for(unsigned b=0;b<4;++b)metadata[at+b]=uint8_t(value>>(b*8));};
    for(unsigned id=0;id<count*2+4;++id){word(40+id*16,id*65536u);word(44+id*16,0x00010001u);word(52+id*16,2);}
    for(unsigned id=0;id<count;++id){
        ImageReservation image(chunk_pages*ImagePageBytes);CHECK(image.bytes());
        image.publish(&owner,directory,id,0,true);
    }
    // The actual production arena is full. Reconstruction may evict old
    // pictures, but subsequent image demand must never overwrite its lease.
    unsigned identity=0;CHECK(acquire_implicit_scratch(&identity,implicit_geometry::Pixels));
    auto*bytes=reinterpret_cast<uint8_t*>(implicit_dense.colors);
    std::fill_n(bytes,implicit_geometry::DenseBytes,uint8_t(0x5b));
    for(unsigned id=count;id<count*2;++id){
        ImageReservation image(chunk_pages*ImagePageBytes);CHECK(image.bytes());
        std::fill_n(image.bytes(),chunk_pages*ImagePageBytes,uint8_t(id));
        image.publish(&owner,directory,id,0,true);
    }
    for(unsigned i=0;i<implicit_geometry::DenseBytes;++i)CHECK(bytes[i]==0x5b);
    release_implicit_scratch(&identity);CHECK(!implicit_dense_owner&&!implicit_dense_reservation);
    CHECK(image_page_limit==pages_before&&tile_page_limit==groups_before);
    clear_caches(&owner);for(auto page:image_used)CHECK(!page);

    // An arena occupied only by unpublished results is not evictable. A
    // refused scratch lease must leave neither an owner nor a stranded page.
    std::vector<ImageCache*> held;
    for(unsigned i=0;i<count;++i){auto*entry=reserve_image(chunk_pages*ImagePageBytes);CHECK(entry);held.push_back(entry);}
    CHECK(!acquire_implicit_scratch(&identity,implicit_geometry::Pixels));
    CHECK(!implicit_dense_owner&&!implicit_dense_reservation);
    for(auto*entry:held)discard_image(*entry);
    for(auto page:image_used)CHECK(!page);

    // Shrinking the real room profile cancels a paused implicit preparation
    // before the former image pages become the relocated tile-group arena.
    owner.set_image_cache_profile(true);content_preload.owner=&owner;
    CHECK(acquire_implicit_scratch(&content_preload,implicit_geometry::Pixels));
    owner.set_image_cache_profile(false);
    CHECK(!content_preload.owner&&!implicit_dense_owner&&!implicit_dense_reservation);
    for(auto page:image_used)CHECK(!page);
    CHECK(acquire_implicit_scratch(&identity,33u*97u));
    CHECK(implicit_dense.bytes==33u*97u*3u);
    release_implicit_scratch(&identity);for(auto page:image_used)CHECK(!page);
}
}
int main(){
    collision_and_eviction(12);collision_and_eviction(16);
    // Include indexes on both sides of the former byte boundary. The count,
    // permutation and returned age must survive entries 256 through 319.
    CHECK(ImageEntries==Content::image_capacity_entries()&&ImageEntries==1151);
    std::vector<unsigned> all;
    for(unsigned i=0;i<ImageEntries;++i)all.push_back(i);
    image_wrap(all);
    image_wrap({0,7,63,127,128,191,254,255,256,287,319,1022,1023,1024,1150});
    image_wrap({ImageEntries-1});
    image_wrap({});
    profile_surviving_images(12);profile_surviving_images(16);
    frame_pool_profiles();
    owner.set_image_cache_profile(false,true);
    CHECK(image_page_limit==2176&&tile_page_limit==34);
    CHECK(image_page_limit*ImagePageBytes+tile_page_limit*TilePageBytes==PictureBufferBudget);
    owner.set_image_cache_profile(false);
    CHECK(image_page_limit==ImagePages&&tile_page_limit==48);
    large_room_allocations();
    retained_quarters_share_and_evict();
    implicit_reservation_lifecycle();
    std::printf("content cache ages: %u checks passed;1151 colliding keys with all five decoded formats, owner/alias eviction, age wrap, bounded frame-pool splits and compact retained quarters\n",checks);
}
