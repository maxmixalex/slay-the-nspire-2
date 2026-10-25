// Original NCardLibrary predicates and presentation-only sorting. The retained
// index list is rebuilt on an actual filter/query change, never per hover.
#include "card_library_metadata.inc"
uint16_t App::localized_card_rank(CardKind kind)const{return kind<CardKinds?card_title_sort_rank[content.english()?0:1][kind]:0;}
static bool library_equal(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static void library_copy(char*out,unsigned capacity,const char*in){unsigned n=0;while(n+1<capacity&&in[n]){out[n]=in[n];++n;}out[n]=0;}
bool App::library_card_seen(CardKind kind)const{
    return kind<CardKinds&&(progress.discovered_cards[unsigned(kind)/8]&(1u<<(unsigned(kind)%8)));
}
bool App::library_item_seen(int tab,int index)const{
    if(tab==0)return index>=0&&index<CardKinds&&library_card_seen(CardKind(index));
    if(tab==1)return index>=0&&index<WorldRelicKinds&&(progress.discovered_relics[unsigned(index)/8]&(1u<<(unsigned(index)%8)));
    return tab==2&&index>=0&&index<PotionKinds&&(progress.discovered_potions[unsigned(index)/8]&(1u<<(unsigned(index)%8)));
}
void App::open_library(int tab){
    if(tab<0||tab>2)return;pointer_cancel();compendium_open=false;library=true;library_tab=tab;library_page=library_selected=0;
    library_upgraded=false;library_sort=0;library_search_editing=false;
    card_library=CardLibraryPresentation{};card_library.opened=map_clock_ms;collection=CollectionPresentation{};
    // NCardLibraryGrid.RefreshVisibility snapshots discovery when its
    // submenu opens. New canonical tooltip observations affect the next
    // opening, while search/detail predicates still use live progress.
    for(unsigned i=0;i<sizeof card_library.seen;++i)card_library.seen[i]=progress.discovered_cards[i];
    static constexpr uint8_t character_filter[5]={1,2,4,5,6};
    library_filter=!title&&unsigned(game.character)<5?character_filter[unsigned(game.character)]:1;
    static constexpr uint8_t filter_control[9]={0,0,1,5,4,2,3,6,7};
    card_library.control=int16_t(LibraryPoolFirst+filter_control[library_filter]);
    card_grids[unsigned(CardGridSurface::Library)].reset();
}
unsigned App::library_items(CardKind*out)const{
    auto&state=card_library;
    if(state.cached_epoch!=state.epoch||state.cached_generation!=content.generation()||state.cached_filter!=library_filter||state.cached_upgrades!=library_upgraded){
        state.cached_epoch=state.epoch;state.cached_generation=content.generation();state.cached_filter=library_filter;state.cached_upgrades=library_upgraded;state.count=0;
        static constexpr uint8_t pool_bits[9]={0,1,2,32,16,4,8,64,128};
        static constexpr const char*rarity_names[]={"none","basic","common","uncommon","rare","ancient","event","token","status","curse","quest"};
        const unsigned pool_mask=library_filter>=0&&library_filter<9?pool_bits[library_filter]:0;
        const bool searching=!library_query::empty(state.query);const library_query::Matcher query(state.query);const EventCardFacts canonical{};
        for(unsigned i=0;i<CardKinds;++i){
            if(!(library_card_metadata[i][0]&pool_mask))continue;
            const int rarity=card_rarity(i),type=int(definitions[i].type);
            const unsigned type_bit=1u<<(type<=int(CardType::Power)?type:3);
            const unsigned rarity_bit=1u<<(rarity>=2&&rarity<=4?rarity-2:3);
            if(state.types&&!(state.types&type_bit))continue;
            if(state.rarities&&library_filter>0&&library_filter<7&&!(state.rarities&rarity_bit))continue;
            const int energy=cost({CardKind(i),0});unsigned cost_bit=energy==-1?0:1u<<(energy<=0?0:energy>=3?3:energy);
            if(energy==-1||library_card_metadata[i][1])cost_bit|=16;
            if(state.costs&&!(state.costs&cost_bit))continue;
            if(searching){
                if(!library_card_seen(CardKind(i)))continue;
                bool match=rarity>=0&&rarity<11&&library_equal(state.query,rarity_names[rarity]);
                if(!match)match=query.matches(content.card_string(i),"");
                if(!match){char description[2048];world_card_description({CardKind(i),uint8_t(library_upgraded&&card_upgradeable({CardKind(i),0}))},-1,description,sizeof description,-1,-1,&canonical);
                    match=query.matches(content.card_string(i),description);}
                if(!match)continue;
            }
            state.ids[state.count++]=uint16_t(i);
        }
        auto compare=[&](unsigned l,unsigned r){
            for(const auto sorter:state.sort_priority){int d=0;
                if(sorter==0)d=int(definitions[l].type)-int(definitions[r].type);
                else if(sorter==1)d=card_rarity(l)-card_rarity(r);
                else if(sorter==2)d=cost({CardKind(l),0})-cost({CardKind(r),0});
                else{const auto&rank=card_title_sort_rank[content.english()?0:1];d=int(rank[l])-int(rank[r]);}
                if(d)return state.sort_reverse&(1u<<sorter)?-d:d;
            }return int(l)-int(r);
        };
        for(unsigned i=1;i<state.count;++i){const uint16_t id=state.ids[i];unsigned j=i;
            while(j&&compare(state.ids[j-1],id)>0){state.ids[j]=state.ids[j-1];--j;}state.ids[j]=id;}
    }
    if(out)for(unsigned i=0;i<state.count;++i)out[i]=CardKind(state.ids[i]);return state.count;
}
void App::commit_library_query(){
    char normalized[sizeof card_library.query];library_query::fold(normalized,sizeof normalized,card_library.draft);
    card_library.query_pending=false;
    if(library_equal(normalized,card_library.query))return;
    // A debounced filter can finish while the pointer is pressed. The old
    // card index must never activate a different card from the new result set.
    pointer_cancel();library_copy(card_library.query,sizeof card_library.query,normalized);card_library.invalidate();library_selected=0;
    card_grids[unsigned(CardGridSurface::Library)].reset();
}
void App::library_text(int code){
    if(auxiliary_text(code))return;
    if(!library||library_tab||!library_search_editing||detail_card>=0)return;
    if(code==27){library_search_editing=card_library.editing=false;return;}
    if(code==13){commit_library_query();library_search_editing=card_library.editing=false;return;}
    unsigned n=0;while(card_library.draft[n])++n;
    if(code==8){if(n){do{--n;}while(n&&(uint8_t(card_library.draft[n])&0xc0)==0x80);card_library.draft[n]=0;}}
    else if(code>=32&&code<=0x10ffff&&!(code>=0xd800&&code<=0xdfff)){
        char encoded[4];unsigned size=0;
        if(code<0x80)encoded[size++]=char(code);
        else if(code<0x800){encoded[size++]=char(0xc0|(code>>6));encoded[size++]=char(0x80|(code&63));}
        else if(code<0x10000){encoded[size++]=char(0xe0|(code>>12));encoded[size++]=char(0x80|((code>>6)&63));encoded[size++]=char(0x80|(code&63));}
        else{encoded[size++]=char(0xf0|(code>>18));encoded[size++]=char(0x80|((code>>12)&63));encoded[size++]=char(0x80|((code>>6)&63));encoded[size++]=char(0x80|(code&63));}
        if(n+size>=sizeof card_library.draft)return;
        for(unsigned i=0;i<size;++i)card_library.draft[n++]=encoded[i];card_library.draft[n]=0;
    }else return;
    card_library.query_pending=true;card_library.query_changed=map_clock_ms;
}
void App::library_control(int control){
    if(!library||library_tab||control<0||control>=LibraryControls)return;
    card_library.control=int16_t(control);library_search_editing=card_library.editing=false;
    if(control==LibrarySearch||control==LibraryClearSearch){
        library_search_editing=card_library.editing=true;
        if(control==LibraryClearSearch&&!library_query::empty(card_library.draft)){card_library.draft[0]=0;commit_library_query();}return;
    }
    if(control>=LibraryPoolFirst&&control<=LibraryPoolLast){
        static constexpr uint8_t filters[]={1,2,5,6,4,3,7,8};const int filter=filters[control-LibraryPoolFirst];
        library_filter=library_filter==filter?0:filter;
    }else if(control>=LibraryTypeFirst&&control<=LibraryTypeLast)card_library.types^=uint8_t(1u<<(control-LibraryTypeFirst));
    else if(control>=LibraryRarityFirst&&control<=LibraryRarityLast){if(library_filter<=0||library_filter>=7)return;card_library.rarities^=uint8_t(1u<<(control-LibraryRarityFirst));}
    else if(control>=LibraryCostFirst&&control<=LibraryCostLast)card_library.costs^=uint8_t(1u<<(control-LibraryCostFirst));
    else if(control==LibraryViewStats){card_library.show_stats=!card_library.show_stats;return;}
    else if(control==LibraryViewUpgrades){library_upgraded=!library_upgraded;if(library_query::empty(card_library.query))return;}
    else{const unsigned sorter=control==LibrarySortType?0:control==LibrarySortRarity?1:control==LibrarySortCost?2:3;
        card_library.sort_descending^=uint8_t(1u<<sorter);
        const bool reverse=bool(card_library.sort_descending&(1u<<sorter))!=(sorter==1);
        if(reverse)card_library.sort_reverse|=uint8_t(1u<<sorter);else card_library.sort_reverse&=uint8_t(~(1u<<sorter));
        unsigned at=0;while(at<4&&card_library.sort_priority[at]!=sorter)++at;
        for(unsigned i=at;i>0;--i)if(i<4)card_library.sort_priority[i]=card_library.sort_priority[i-1];card_library.sort_priority[0]=uint8_t(sorter);
    }
    card_library.invalidate();library_selected=0;card_grids[unsigned(CardGridSurface::Library)].reset();
}
static void library_control_cell(int control,int&row,int&column){
    column=0;row=0;
    if(control<=LibraryClearSearch){column=control;return;}
    if(control<=LibraryPoolLast){row=1+(control-LibraryPoolFirst)/4;column=(control-LibraryPoolFirst)%4;return;}
    if(control==LibrarySortType){row=3;return;}
    if(control<=LibraryTypeLast){row=4;column=control-LibraryTypeFirst;return;}
    if(control==LibrarySortRarity){row=5;return;}
    if(control<=LibraryRarityLast){row=6+control-LibraryRarityFirst;return;}
    if(control==LibrarySortCost){row=10;return;}
    if(control<=LibraryCostLast){row=11;column=control-LibraryCostFirst;return;}
    row=control==LibrarySortAlphabet?12:13;
    if(control==LibraryViewUpgrades)column=1;
}
bool App::library_input(Key key){
    if(!library||detail_card>=0||detail_item>=0)return false;
    if(library_tab)return collection_input(key);
    if(library_search_editing){
        if(key==Key::Back){library_text(27);return true;}if(key==Key::Confirm){library_text(13);return true;}
        if(key!=Key::Up&&key!=Key::Down)return true;library_search_editing=card_library.editing=false;
    }
    if(key!=Key::Left&&key!=Key::Right&&key!=Key::Up&&key!=Key::Down&&key!=Key::Confirm&&key!=Key::Detail&&key!=Key::EndTurn)return false;
    if(key==Key::EndTurn){library_control(LibraryViewUpgrades);return true;}
    const int count=int(library_items());auto&grid=configure_card_grid(CardGridSurface::Library,count);
    auto&control=card_library.control;
    if(control>=0){
        if(key==Key::Confirm){library_control(control);return true;}if(key==Key::Detail)return true;
        int row,column;library_control_cell(control,row,column);const int old_row=row,old_column=column;
        if(key==Key::Up)--row;if(key==Key::Down)++row;if(key==Key::Left)--column;if(key==Key::Right)++column;
        int closest=-1,distance=1000;
        for(int i=0;i<LibraryControls;++i){if(i>=LibraryRarityFirst&&i<=LibraryRarityLast&&(library_filter<=0||library_filter>=7))continue;
            int r,c;library_control_cell(i,r,c);if(r!=row)continue;const int d=c>column?c-column:column-c;
            if(d<distance){closest=i;distance=d;}}
        if(key==Key::Right&&(closest<0||closest==control||distance)){control=-1;library_selected=CardGridScroll::clamp(library_selected,0,count?count-1:0);grid.focus(library_selected);return true;}
        if(closest>=0)control=int16_t(closest);else if(key==Key::Up||key==Key::Down){
            const int step=key==Key::Up?-1:1;for(int r=old_row+step;r>=0&&r<15;r+=step){
                int next=-1;for(int i=0;i<LibraryControls;++i){int rr,cc;library_control_cell(i,rr,cc);if(rr==r&&!(i>=LibraryRarityFirst&&i<=LibraryRarityLast&&(library_filter<=0||library_filter>=7))){next=i;break;}}
                if(next>=0){control=int16_t(next);break;}}
        }(void)old_column;return true;
    }
    if(key==Key::Confirm||key==Key::Detail){select_library_item(library_selected);return true;}
    if(!count)return true;library_selected=CardGridScroll::clamp(library_selected,0,count-1);
    const int columns=grid.columns,column=library_selected%columns;
    if(key==Key::Left){if(!column)control=LibraryPoolFirst;else --library_selected;}
    if(key==Key::Right&&column+1<columns&&library_selected+1<count)++library_selected;
    if(key==Key::Up&&library_selected>=columns)library_selected-=columns;
    if(key==Key::Down&&library_selected+columns<count)library_selected+=columns;
    grid.focus(library_selected);return true;
}
