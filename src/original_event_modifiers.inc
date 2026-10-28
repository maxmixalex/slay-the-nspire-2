// Neow.GenerateInitialOptions yields one modifier option at a time. Only the
// five modifiers with GenerateNeowOption participate; an otherwise modified
// run receives no ordinary Neow relic choices. All paused work uses the
// existing persisted event chooser and its selected-card continuation.
constexpr uint32_t NeowDeckModifiers=RunExclusiveDeckModifiers|run_modifier_bit(RunModifier::Specialized)|run_modifier_bit(RunModifier::AllStar);
void modifier_neow_next(State&s){
    auto&o=s.run_options;auto&e=s.original_events;options(s);e.offer_count=0;
    o.setup_flags&=uint8_t(~1u);o.neow_current=o.draft_remaining=0;e.values[14]=0;
    for(unsigned i=0;i<run_modifier_count(o);++i){const RunModifier modifier=run_modifier_at(o,i);const uint32_t bit=run_modifier_bit(modifier);
        if(!(bit&NeowDeckModifiers)||(o.neow_completed&bit))continue;
        o.neow_current=uint8_t(unsigned(modifier)+1);page(s,"EVENT");option_key(s,0,"MODIFIER.",run_modifier_key(modifier));return;
    }
    finish_page(s,"DONE");
}
bool modifier_neow_begin(State&s){modifier_neow_next(s);return s.phase!=Phase::Error;}
bool modifier_neow_cards(State&s,RunModifier modifier,bool automatic){
    auto&e=s.original_events;WorldCardOptions options;
    options.flags=WorldNoUpgradeRoll;options.source=WorldCardSource::Other;
    unsigned requested=1;
    if(modifier==RunModifier::Draft){requested=3;options.flags|=WorldIsCardReward;}
    else if(modifier==RunModifier::SealedDeck){requested=30;options.flags|=WorldForceRarityOddsChange|WorldIsCardReward;}
    else{options.odds=WorldCardOdds::Uniform;options.flags|=WorldNoRarityModification;
        if(modifier==RunModifier::AllStar){options.pools[0]=WorldCardPool::Colorless;options.flags|=WorldNoCardPoolModifications;}}
    Card cards[MaxOriginalEventOffers]{};EventCardFacts facts[MaxOriginalEventOffers]{};unsigned made=0;
    if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),requested,options,cards,facts,MaxOriginalEventOffers,&made)||made!=requested)return false;
    const unsigned copies=modifier==RunModifier::Specialized?5:made;
    e.offer_count=uint16_t(copies);
    for(unsigned i=0;i<copies;++i){const unsigned source=modifier==RunModifier::Specialized?0:i;e.offers[i]=uint16_t(card_token(cards[source]));e.offer_facts[i]=facts[source];}
    const unsigned selected=automatic?copies:modifier==RunModifier::SealedDeck?10:1;
    if(!pause_selection(s,OriginalEventSelection::OfferCard,OriginalEventFilter::Any,selected,selected,false))return false;
    if(automatic){
        e.selection_committed=1;e.selected_count=uint16_t(selected);e.selected_cursor=0;
        for(unsigned i=0;i<selected;++i){e.selected[i]=uint16_t(i);e.selected_cards[i]={card_from_token(e.offers[i]),e.offer_facts[i],0,0};}
        s.phase=Phase::Event;apply_selection(s);
    }
    return s.phase!=Phase::Error;
}
void modifier_neow_complete(State&s,RunModifier modifier){
    if(run_modifier_bit(modifier)&RunExclusiveDeckModifiers)world_remove_relic_from_bags(s,WorldRelicPandorasBox);
    s.run_options.neow_completed|=run_modifier_bit(modifier);modifier_neow_next(s);
}
void modifier_neow_resume(State&s){
    auto&o=s.run_options;auto&e=s.original_events;
    // A committed acquisition can pause inside an independent relic chooser.
    // Do not advance the draft or generate its next cards until it has ended.
    while((o.setup_flags&1)&&e.selection==OriginalEventSelection::None&&!world_relic_pending(s)&&s.phase!=Phase::Dead&&s.phase!=Phase::Error){
        const RunModifier modifier=current_neow_modifier(s);
        if(modifier==RunModifier::Draft){
            if(o.draft_remaining)--o.draft_remaining;
            if(o.draft_remaining){if(!modifier_neow_cards(s,modifier,false))fail(s);return;}
        }else if(modifier==RunModifier::Insanity||modifier==RunModifier::AllStar){
            if(e.values[14]>0)--e.values[14];
            if(e.values[14]>0){if(!modifier_neow_cards(s,modifier,true)){fail(s);return;}continue;}
        }else if(modifier!=RunModifier::SealedDeck&&modifier!=RunModifier::Specialized){fail(s);return;}
        modifier_neow_complete(s,modifier);
    }
}
bool modifier_neow_choose(State&s){
    auto&o=s.run_options;const RunModifier modifier=current_neow_modifier(s);
    if(modifier==RunModifier::Count||!(run_modifier_bit(modifier)&NeowDeckModifiers)||(o.setup_flags&1))return false;
    options(s);o.setup_flags|=1;
    if(modifier==RunModifier::Draft)o.draft_remaining=10;
    s.original_events.values[14]=modifier==RunModifier::Insanity?30:modifier==RunModifier::AllStar?5:0;
    const bool automatic=modifier!=RunModifier::Draft&&modifier!=RunModifier::SealedDeck;
    return modifier_neow_cards(s,modifier,automatic);
}
