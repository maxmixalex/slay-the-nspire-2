// NRelicCollection / NPotionLab keep all categories in one scroll container.
#include "collection_metadata.inc"
static unsigned collection_character(unsigned mask){
    static constexpr uint8_t order[]={1,2,8,16,4};
    for(unsigned i=0;i<5;++i)if(mask&order[i])return i+1;return 0;
}
unsigned App::library_item_character(int tab,int index)const{
    if(tab==1&&index>=0&&index<WorldRelicKinds)return collection_character(world_relic_definitions[index].character_mask);
    if(tab==2&&index>=0&&index<PotionKinds)return collection_potion_character[index];return 0;
}
void App::prepare_collection(){
    if(!library||library_tab<1||library_tab>2)return;
    auto&v=collection;auto&grid=card_grids[unsigned(CardGridSurface::Library)];
    if(v.tab==library_tab&&v.generation==content.generation()&&grid.bound)return;
    v=CollectionPresentation{};v.tab=uint8_t(library_tab);v.generation=content.generation();v.started=map_clock_ms;
    const bool relic=library_tab==1;const auto&rank=collection_title_rank[content.english()?0:1];
    int y=150;const int item_size=relic?68:60;
    auto header=[&](unsigned kind){
        if(v.category_count<16){v.category_kind[v.category_count]=uint8_t(kind);v.category_y[v.category_count++]=int16_t(y*2/9);}y+=56;
    };
    auto rows=[&](const uint16_t*ids,unsigned count,unsigned category){
        for(unsigned start=0;start<count;start+=10){
            const unsigned n=count-start<10?count-start:10;
            if(v.row_count>=CollectionPresentation::MaxRows||v.count+n>WorldRelicKinds)return;
            auto&row=v.rows[v.row_count++];row={v.count,uint8_t(n),uint8_t(category),int16_t(y*2/9)};
            for(unsigned i=0;i<n;++i)v.ids[v.count++]=ids[start+i];y+=item_size+32;
        }
    };
    auto sorted_group=[&](int rarity,int ancient,int category){
        uint16_t ids[WorldRelicKinds];unsigned n=0;
        const unsigned total=relic?WorldRelicKinds:PotionKinds;
        for(unsigned id=0;id<total;++id){
            const unsigned r=relic?world_relic_definitions[id].rarity:potion_definitions[id].rarity;
            if(int(r)!=rarity&&!(rarity==4&&!relic&&r==5))continue;
            if(ancient&&collection_ancient[id]!=ancient)continue;ids[n++]=uint16_t(id);
        }
        auto compare=[&](unsigned l,unsigned r){
            const unsigned lc=relic?collection_character(world_relic_definitions[l].character_mask):collection_potion_character[l];
            const unsigned rc=relic?collection_character(world_relic_definitions[r].character_mask):collection_potion_character[r];
            if(!ancient&&lc!=rc)return int(lc)-int(rc);
            // Original character-specific groups retain ModelDb model order;
            // shared and Ancient groups use the active locale's title order.
            if(!ancient&&lc)return pile_compare_text(relic?world_relic_definitions[l].key:potion_definitions[l].key,relic?world_relic_definitions[r].key:potion_definitions[r].key);
            const unsigned offset=relic?0:WorldRelicKinds;return int(rank[offset+l])-int(rank[offset+r]);
        };
        for(unsigned i=1;i<n;++i){const uint16_t id=ids[i];unsigned j=i;while(j&&compare(ids[j-1],id)>0){ids[j]=ids[j-1];--j;}ids[j]=id;}
        rows(ids,n,unsigned(category));y+=38;
    };
    if(relic){
        header(0);
        static constexpr uint16_t starting[]={WorldRelicBurningBlood,WorldRelicRingOfTheSnake,WorldRelicDivineRight,WorldRelicBoundPhylactery,WorldRelicCrackedCore};
        static constexpr uint16_t upgraded[]={WorldRelicBlackBlood,WorldRelicRingOfTheDrake,WorldRelicDivineDestiny,WorldRelicPhylacteryUnbound,WorldRelicInfusedCore};
        rows(starting,5,0);y-=12;rows(upgraded,5,0);y+=38;
        for(unsigned category=1;category<5;++category){header(category);sorted_group(int(category)+1,0,int(category));}
        header(5);y-=40;
        for(unsigned ancient=1;ancient<=8;++ancient){header(7+ancient);sorted_group(7,int(ancient),7+int(ancient));}
        header(6);sorted_group(6,0,6);
    }else for(unsigned category=0;category<4;++category){header(category);sorted_group(int(category)+1,0,int(category));}
    grid.configure(unsigned(library_tab),pile_sort_epoch,v.count,0,240,0,10,57,false,(y+150)*2/9);
    library_selected=CardGridScroll::clamp(library_selected,0,v.count?int(v.count)-1:0);
}
void App::focus_collection(){
    prepare_collection();auto&grid=card_grids[unsigned(CardGridSurface::Library)];
    for(unsigned i=0;i<collection.row_count;++i){const auto&r=collection.rows[i];
        if(library_selected>=r.first&&library_selected<int(r.first+r.count)){
            grid.target=CardGridScroll::clamp((r.y+7-grid.height()/2)*CardGridScroll::Unit,grid.minimum(),grid.maximum());return;
        }
    }
}
bool App::collection_input(Key key){
    if(!library||!library_tab)return false;
    if(key!=Key::Left&&key!=Key::Right&&key!=Key::Up&&key!=Key::Down&&key!=Key::Confirm&&key!=Key::Detail)return false;
    prepare_collection();if(!collection.count)return true;
    unsigned row=0;for(;row+1<collection.row_count&&library_selected>=collection.rows[row].first+collection.rows[row].count;++row){}
    const auto&current=collection.rows[row];int column=library_selected-current.first;
    if(key==Key::Left)column=(column+current.count-1)%current.count;
    if(key==Key::Right)column=(column+1)%current.count;
    if(key==Key::Up&&row)--row;
    if(key==Key::Down&&row+1<collection.row_count)++row;
    const auto&next=collection.rows[row];library_selected=next.first+CardGridScroll::clamp(column,0,next.count-1);
    focus_collection();return true;
}
