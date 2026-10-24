// NDeckViewScreen keeps a priority stack of the four independent sorters.
// NCardPileScreen preserves discard/exhaust order and hides draw order by
// sorting the original rarity then model ID. None of this mutates a pile.
static int pile_compare_text(const char*a,const char*b){
    while(*a&&*a==*b){++a;++b;}return int(uint8_t(*a))-int(uint8_t(*b));
}
unsigned App::pile_items(int*out)const{
    int ids[MaxCards];unsigned count=0;
    if(pile_tab==0){count=game.deck_count;for(unsigned i=0;i<count;++i)ids[i]=int(i);}
    else{const auto&source=pile(game,pile_tab==1?Zone::Draw:pile_tab==2?Zone::Discard:pile_tab==3?Zone::Exhaust:Zone::Power);
        count=source.count;for(unsigned i=0;i<count;++i)ids[i]=source.ids[i];}
    if(!out)return count;
    auto compare=[&](int left,int right){
        if(pile_tab==1){const auto l=game.cards[left],r=game.cards[right];const int rarity=card_rarity(l.kind)-card_rarity(r.kind);
            return rarity?rarity:pile_compare_text(definitions[l.kind].key,definitions[r.kind].key);}
        if(pile_tab)return 0;
        const auto l=game.deck[left],r=game.deck[right];
        for(auto order:pile_sort_priority){int result=0;
            if(order==0)result=left-right;
            else if(order==1)result=int(card_face_type(l,-1,left))-int(card_face_type(r,-1,right));
            else if(order==2)result=card_offer_cost(l,event_card_facts(game,l,-1,left))-card_offer_cost(r,event_card_facts(game,r,-1,right));
            else if(order==3)result=pile_compare_text(content.card_string(l.kind),content.card_string(r.kind));
            if(result)return (pile_sort_descending&(1u<<order))?-result:result;
        }return left-right;
    };
    if(pile_tab<2)for(unsigned i=1;i<count;++i){const int id=ids[i];unsigned j=i;
        while(j&&compare(ids[j-1],id)>0){ids[j]=ids[j-1];--j;}ids[j]=id;}
    for(unsigned i=0;i<count;++i)out[i]=ids[i];return count;
}
void App::sort_pile(unsigned column){
    if(panel!=1||pile_tab||column>=4)return;
    pile_sort_descending^=uint8_t(1u<<column);unsigned at=0;
    while(at<4&&pile_sort_priority[at]!=column)++at;
    for(unsigned i=at;i>0;--i)if(i<4)pile_sort_priority[i]=pile_sort_priority[i-1];
    pile_sort_priority[0]=uint8_t(column);scroll=inspect_cursor=0;
}
