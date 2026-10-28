// NDeckViewScreen keeps a priority stack of the four independent sorters.
// NCardPileScreen preserves discard/exhaust order and hides draw order by
// sorting the original rarity then model ID. None of this mutates a pile.
#include "card_title_sort_rank.inc"
static int pile_compare_text(const char*a,const char*b){
    while(*a&&*a==*b){++a;++b;}return int(uint8_t(*a))-int(uint8_t(*b));
}
namespace {
struct PileListCache {
    const App*owner=nullptr;uint32_t generation=0,revision=0,epoch=0,seed=0;
    uint16_t count=0,order=0;int tab=-1;uint16_t ids[MaxCards]{};
};
static PileListCache pile_list_cache;
}
unsigned App::pile_items(int*out)const{
    const Pile*source=pile_tab?&pile(game,pile_tab==1?Zone::Draw:pile_tab==2?Zone::Discard:pile_tab==3?Zone::Exhaust:Zone::Power):nullptr;
    const unsigned count=source?source->count:game.deck_count;if(!out)return count;
    // Discard/exhaust already have the authored display order. The two sorted
    // screens retain one bounded index list until a canonical command, screen
    // reset, locale reload or sorter change invalidates it.
    if(pile_tab>=2){for(unsigned i=0;i<count;++i)out[i]=source->ids[i];return count;}
    uint16_t order=pile_sort_descending;for(unsigned i=0;i<4;++i)order|=uint16_t(pile_sort_priority[i]<<(4+2*i));
    auto&cache=pile_list_cache;
    if(cache.owner!=this||cache.generation!=content.generation()||cache.revision!=game.revision||cache.epoch!=pile_sort_epoch||
       cache.seed!=game.seed||cache.tab!=pile_tab||cache.count!=count||cache.order!=order){
        cache.owner=this;cache.generation=content.generation();cache.revision=game.revision;cache.epoch=pile_sort_epoch;
        cache.seed=game.seed;cache.tab=pile_tab;cache.count=uint16_t(count);cache.order=order;
        for(unsigned i=0;i<count;++i)cache.ids[i]=source?source->ids[i]:uint16_t(i);
        bool use_type=false,use_cost=false;
        if(!pile_tab)for(auto sort:pile_sort_priority){if(!sort)break;use_type|=sort==1;use_cost|=sort==2;}
        uint8_t types[MaxDeck];int costs[MaxDeck];
        if(use_type||use_cost)for(unsigned i=0;i<count;++i){const auto card=game.deck[i];
            if(use_type)types[i]=uint8_t(card_face_type(card,-1,int(i)));
            if(use_cost)costs[i]=card_offer_cost(card,event_card_facts(game,card,-1,int(i)));
        }
        auto compare=[&](int left,int right){
            if(pile_tab==1){const auto l=game.cards[left],r=game.cards[right];const int rarity=card_rarity(l.kind)-card_rarity(r.kind);
                return rarity?rarity:pile_compare_text(definitions[l.kind].key,definitions[r.kind].key);}
            const auto l=game.deck[left],r=game.deck[right];
            for(auto sort:pile_sort_priority){int result=0;
                if(sort==0)result=left-right;
                else if(sort==1)result=int(types[left])-int(types[right]);
                else if(sort==2)result=costs[left]-costs[right];
                else if(sort==3){const auto&rank=card_title_sort_rank[content.english()?0:1];
                    result=int(rank[l.kind+(l.upgraded?CardKinds:0)])-int(rank[r.kind+(r.upgraded?CardKinds:0)]);}
                if(result)return (pile_sort_descending&(1u<<sort))?-result:result;
            }return left-right;
        };
        for(unsigned i=1;i<count;++i){const auto id=cache.ids[i];unsigned j=i;
            while(j&&compare(cache.ids[j-1],id)>0){cache.ids[j]=cache.ids[j-1];--j;}cache.ids[j]=id;}
    }
    for(unsigned i=0;i<count;++i)out[i]=cache.ids[i];return count;
}
void App::sort_pile(unsigned column){
    if(panel!=1||pile_tab||column>=4)return;
    pile_sort_descending^=uint8_t(1u<<column);unsigned at=0;
    while(at<4&&pile_sort_priority[at]!=column)++at;
    for(unsigned i=at;i>0;--i)if(i<4)pile_sort_priority[i]=pile_sort_priority[i-1];
    pile_sort_priority[0]=uint8_t(column);scroll=inspect_cursor=0;card_grids[unsigned(CardGridSurface::Pile)].reset();
}
